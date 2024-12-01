#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <termios.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

// Debug configuration
#define DEBUG_OUTPUT 1  // Set to 0 to disable debug printing
#define SIMULATE_SERIAL 1  // Set to 0 to use real serial port

// CRSF Protocol Constants
#define CRSF_SYNC_BYTE 0xC8
#define CRSF_CHANNELS_FRAME_SIZE 24
#define CRSF_NUM_CHANNELS 16
#define CRSF_CHANNEL_VALUE_MIN 172
#define CRSF_CHANNEL_VALUE_MAX 1811

// Joystick mapping structure
typedef struct {
    int js_fd;
    int axis_count;
    int button_count;
    struct js_event event;
    int16_t *axes;
    int8_t *buttons;
    char name[128];  // Added to store joystick name
} Joystick;

// CRSF frame structure
typedef struct {
    uint8_t device_addr;
    uint8_t frame_size;
    uint8_t type;
    uint8_t payload[CRSF_CHANNELS_FRAME_SIZE];
    uint8_t crc;
} CRSF_Frame;

// Function prototypes
int init_joystick(Joystick *js, const char *device_path);
int init_serial(const char *serial_port);
void update_joystick(Joystick *js);
void pack_channels_into_crsf(uint16_t *channels, uint8_t *payload);
uint8_t calc_crsf_crc(uint8_t *data, int length);
void map_joysticks_to_channels(Joystick *left, Joystick *right, uint16_t *channels);
void print_debug_info(Joystick *left, Joystick *right, uint16_t *channels, CRSF_Frame *frame);

int main() {
    Joystick left_stick, right_stick;
    CRSF_Frame frame;
    uint16_t channels[CRSF_NUM_CHANNELS] = {0};
    
    // Initialize joysticks
    if (init_joystick(&left_stick, "/dev/input/js0") < 0 ||
        init_joystick(&right_stick, "/dev/input/js1") < 0) {
        fprintf(stderr, "Failed to initialize joysticks\n");
        return 1;
    }

    // Initialize serial port for CRSF output
    int serial_fd = -1;
    if (!SIMULATE_SERIAL) {
        serial_fd = init_serial("/dev/ttyAMA0");
        if (serial_fd < 0) {
            fprintf(stderr, "Failed to initialize serial port\n");
            return 1;
        }
    }

    #if DEBUG_OUTPUT
    printf("Initialization complete!\n");
    printf("Left stick: %s - %d axes, %d buttons\n", left_stick.name, left_stick.axis_count, left_stick.button_count);
    printf("Right stick: %s - %d axes, %d buttons\n", right_stick.name, right_stick.axis_count, right_stick.button_count);
    #endif

    // Main loop
    while (1) {
        // Update joystick states
        update_joystick(&left_stick);
        update_joystick(&right_stick);

        // Map joystick inputs to CRSF channels
        map_joysticks_to_channels(&left_stick, &right_stick, channels);

        // Prepare CRSF frame
        frame.device_addr = 0xC8;
        frame.frame_size = CRSF_CHANNELS_FRAME_SIZE + 2;
        frame.type = 0x16;  // RC channels
        
        // Pack channels into CRSF format
        pack_channels_into_crsf(channels, frame.payload);
        
        // Calculate CRC
        frame.crc = calc_crsf_crc((uint8_t*)&frame, frame.frame_size - 1);

        #if DEBUG_OUTPUT
        print_debug_info(&left_stick, &right_stick, channels, &frame);
        #endif

        if (!SIMULATE_SERIAL) {
            write(serial_fd, &frame, frame.frame_size + 2);
        }

        // Control update rate (100Hz)
        usleep(10000);
    }

    // Cleanup
    close(left_stick.js_fd);
    close(right_stick.js_fd);
    if (!SIMULATE_SERIAL) {
        close(serial_fd);
    }
    free(left_stick.axes);
    free(left_stick.buttons);
    free(right_stick.axes);
    free(right_stick.buttons);
    
    return 0;
}

void print_debug_info(Joystick *left, Joystick *right, uint16_t *channels, CRSF_Frame *frame) {
    static unsigned long frame_counter = 0;
    frame_counter++;
    
    // Clear screen and move cursor to top
    printf("\033[2J\033[H");
    
    // Print frame counter
    printf("Frame: %lu\n\n", frame_counter);
    
    // Print joystick axes side by side
    printf("%-40s %s\n", "Left Stick Axes:", "Right Stick Axes:");
    printf("%-40s %s\n", left->name, right->name);
    for (int i = 0; i < (left->axis_count > right->axis_count ? left->axis_count : right->axis_count); i++) {
        // Left stick axis
        if (i < left->axis_count) {
            printf("Axis %d: %6d ", i, left->axes[i]);
            // Visual indicator with filled area
            printf("[");
            int center = 10;  // Center position
            int pos = (left->axes[i] + 32768) * 20 / 65535;
            for (int j = 0; j < 20; j++) {
                if (pos > center && j >= center && j <= pos) {
                    printf("█");
                } else if (pos < center && j <= center && j >= pos) {
                    printf("█");
                } else if (j == center) {
                    printf("|");
                } else {
                    printf(" ");
                }
            }
            printf("]");
        } else {
            printf("%40s", "");  // Padding if no left axis
        }

        // Right stick axis
        printf("    ");  // Spacing between sticks
        if (i < right->axis_count) {
            printf("Axis %d: %6d ", i, right->axes[i]);
            // Visual indicator with filled area
            printf("[");
            int center = 10;
            int pos = (right->axes[i] + 32768) * 20 / 65535;
            for (int j = 0; j < 20; j++) {
                if (pos > center && j >= center && j <= pos) {
                    printf("█");
                } else if (pos < center && j <= center && j >= pos) {
                    printf("█");
                } else if (j == center) {
                    printf("|");
                } else {
                    printf(" ");
                }
            }
            printf("]");
        }
        printf("\n");
    }
    
    // Print buttons in groups of 16 for better readability
    printf("\nLeft Stick Buttons (%d):\n", left->button_count);
    for (int i = 0; i < left->button_count && left->buttons != NULL; i++) {
        if (i > 0 && i % 16 == 0) printf("\n");
	left->buttons[i] ? printf("🟩 ") : printf("⬛ ");
        //printf("%d ", left->buttons[i]);
    }
    
    printf("\n\nRight Stick Buttons (%d):\n", right->button_count);
    for (int i = 0; i < right->button_count && right->buttons != NULL; i++) {
        if (i > 0 && i % 16 == 0) printf("\n");
	right->buttons[i] ? printf("🟩 ") : printf("⬛ ");
        //printf("%d ", right->buttons[i]);
    }
    
    // Calculate width needed for CRSF channels display
    // Each channel takes: "CHxx: xxxx [xxxxxxxxxxxxxxxxxxxx]    " = 37 chars
    // Two columns = 74 chars + 4 spacing = 78 total
    
    // Print CRSF channels
    printf("\n\nCRSF Channels:");
    // Add proper spacing before frame box
    printf("                                                            ╔════════════ CRSF Frame ════════════╗\n");
    
    for (int i = 0; i < CRSF_NUM_CHANNELS/2; i++) {
        // Left column (channels 1-8)
        printf("CH%2d: %4d ", i + 1, channels[i]);
        int pos = (channels[i] - CRSF_CHANNEL_VALUE_MIN) * 20 / 
                 (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN);
        printf("[");
        for (int j = 0; j < 20; j++) {
            if (j <= pos) printf("█");
            else printf(" ");
        }
        printf("]    ");

        // Right column (channels 9-16)
        int right_ch = i + CRSF_NUM_CHANNELS/2;
        printf("CH%2d: %4d ", right_ch + 1, channels[right_ch]);
        pos = (channels[right_ch] - CRSF_CHANNEL_VALUE_MIN) * 20 / 
              (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN);
        printf("[");
        for (int j = 0; j < 20; j++) {
            if (j <= pos) printf("█");
            else printf(" ");
        }
        printf("]    "); // Consistent 4-space gap before frame box

        // Print CRSF frame info in the box with consistent width
        switch(i) {
            case 0:
                printf("║  Addr: C8  Size: 1A  Type: 16      ║");
                break;
            case 1:
                printf("║  Payload:                          ║");
                break;
            case 2:
                printf("║  %02X %02X %02X %02X %02X %02X %02X %02X           ║",
                    frame->payload[0], frame->payload[1], frame->payload[2], 
                    frame->payload[3], frame->payload[4], frame->payload[5],
                    frame->payload[6], frame->payload[7]);
                break;
            case 3:
                printf("║  %02X %02X %02X %02X %02X %02X %02X %02X           ║",
                    frame->payload[8], frame->payload[9], frame->payload[10],
                    frame->payload[11], frame->payload[12], frame->payload[13],
                    frame->payload[14], frame->payload[15]);
                break;
            case 4:
                printf("║  %02X %02X %02X %02X %02X %02X %02X %02X           ║",
                    frame->payload[16], frame->payload[17], frame->payload[18],
                    frame->payload[19], frame->payload[20], frame->payload[21],
                    frame->payload[22], frame->payload[23]);
                break;
            case 5:
                printf("║  CRC: %02X                           ║", frame->crc);
                break;
            case 6:
                printf("║                                    ║");
                break;
            case 7:
                printf("╚════════════════════════════════════╝");
                break;
        }
        printf("\n");
    }
    printf("\n");
}

int init_joystick(Joystick *js, const char *device_path) {
    js->js_fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (js->js_fd < 0) {
        fprintf(stderr, "Failed to open %s\n", device_path);
        return -1;
    }

    // Get joystick name
    if (ioctl(js->js_fd, JSIOCGNAME(sizeof(js->name)), js->name) < 0) {
        strcpy(js->name, "Unknown");
    }

    // Use unsigned char for button count to match kernel interface
    unsigned char num_axes;
    unsigned char num_buttons;
    
    if (ioctl(js->js_fd, JSIOCGAXES, &num_axes) < 0) {
        fprintf(stderr, "Failed to get axis count for %s\n", device_path);
        close(js->js_fd);
        return -1;
    }

    if (ioctl(js->js_fd, JSIOCGBUTTONS, &num_buttons) < 0) {
        fprintf(stderr, "Failed to get button count for %s\n", device_path);
        close(js->js_fd);
        return -1;
    }

    // Sanity check the counts
    if (num_axes > 32 || num_buttons > 128) {  // reasonable limits
        fprintf(stderr, "Unreasonable axis/button count for %s: %d axes, %d buttons\n",
                device_path, num_axes, num_buttons);
        close(js->js_fd);
        return -1;
    }

    js->axis_count = num_axes;
    js->button_count = num_buttons;

    #if DEBUG_OUTPUT
    printf("Initializing %s: %d axes, %d buttons\n", device_path, js->axis_count, js->button_count);
    #endif

    js->axes = calloc(js->axis_count, sizeof(int16_t));
    js->buttons = calloc(js->button_count, sizeof(int8_t));

    if (!js->axes || !js->buttons) {
        fprintf(stderr, "Failed to allocate memory for %s\n", device_path);
        close(js->js_fd);
        free(js->axes);  // free is safe on NULL
        free(js->buttons);
        return -1;
    }

    return 0;
}

int init_serial(const char *serial_port) {
    int fd = open(serial_port, O_RDWR | O_NOCTTY);
    if (fd < 0) return -1;

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    // CRSF runs at 420000 baud
    cfsetospeed(&tty, B115200);  // You'll need to modify termios.h for 420000
    cfsetispeed(&tty, B115200);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tcsetattr(fd, TCSANOW, &tty);
    return fd;
}

void update_joystick(Joystick *js) {
    while (read(js->js_fd, &js->event, sizeof(struct js_event)) > 0) {
        switch (js->event.type & ~JS_EVENT_INIT) {
            case JS_EVENT_AXIS:
                if (js->event.number < js->axis_count) {
                    js->axes[js->event.number] = js->event.value;
                }
                break;
            case JS_EVENT_BUTTON:
                if (js->event.number < js->button_count) {
                    js->buttons[js->event.number] = js->event.value;
                }
                break;
        }
    }
}

void pack_channels_into_crsf(uint16_t *channels, uint8_t *payload) {
    // CRSF packs 16 11-bit channels into 24 bytes
    memset(payload, 0, CRSF_CHANNELS_FRAME_SIZE);
    
    int bit_index = 0;
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++) {
        uint16_t value = channels[i];
        int byte_index = bit_index / 8;
        int bits_remaining = 11;
        int bits_in_first_byte = 8 - (bit_index % 8);
        
        payload[byte_index] |= (value & ((1 << bits_in_first_byte) - 1)) << (bit_index % 8);
        
        if (bits_in_first_byte < 11) {
            value >>= bits_in_first_byte;
            payload[byte_index + 1] = value & 0xFF;
            
            if (bits_in_first_byte < 3) {
                payload[byte_index + 2] = (value >> 8) & 0xFF;
            }
        }
        
        bit_index += 11;
    }
}

uint8_t calc_crsf_crc(uint8_t *data, int length) {
    uint8_t crc = 0;
    for (int i = 0; i < length; i++) {
        crc = crc ^ data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0xD5;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

void map_joysticks_to_channels(Joystick *left, Joystick *right, uint16_t *channels) {
    // Map main axes (X/Y for each stick)
    for (int i = 0; i < 4; i++) {
        if (i < 2) {
            channels[i] = ((left->axes[i] + 32768) * 
                (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN) / 65535) 
                + CRSF_CHANNEL_VALUE_MIN;
        } else {
            channels[i] = ((right->axes[i-2] + 32768) * 
                (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN) / 65535) 
                + CRSF_CHANNEL_VALUE_MIN;
        }
    }

    // Map thumbstick axes for right joystick. 
    channels[4] = ((right->axes[3] + 32768) * 
        (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN) / 65535) 
        + CRSF_CHANNEL_VALUE_MIN;
    channels[5] = ((right->axes[4] + 32768) * 
        (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN) / 65535) 
        + CRSF_CHANNEL_VALUE_MIN;

    // Pack buttons into remaining channels (7-16)
    // Each channel can hold 11 buttons (since CRSF channels are 11-bit)
    int buttons_per_channel = 11;
    int channel_index = 6;  // Start at channel 7
    int button_index = 0;

    // Pack left stick buttons
    while (button_index < left->button_count && channel_index < CRSF_NUM_CHANNELS) {
        uint16_t packed_value = CRSF_CHANNEL_VALUE_MIN;  // Start with minimum value
        
        // Pack up to 11 buttons into this channel
        for (int i = 0; i < buttons_per_channel && button_index < left->button_count; i++) {
            if (left->buttons[button_index]) {
                packed_value |= (1 << i);
            }
            button_index++;
        }
        
        // Scale the packed value to CRSF range
        if (packed_value > CRSF_CHANNEL_VALUE_MIN) {
            packed_value = CRSF_CHANNEL_VALUE_MIN + packed_value;
        }
        channels[channel_index++] = packed_value;
    }

    // Pack right stick buttons
    button_index = 0;
    while (button_index < right->button_count && channel_index < CRSF_NUM_CHANNELS) {
        uint16_t packed_value = CRSF_CHANNEL_VALUE_MIN;
        
        for (int i = 0; i < buttons_per_channel && button_index < right->button_count; i++) {
            if (right->buttons[button_index]) {
                packed_value |= (1 << i);
            }
            button_index++;
        }
        
        if (packed_value > CRSF_CHANNEL_VALUE_MIN) {
            packed_value = CRSF_CHANNEL_VALUE_MIN + packed_value;
        }
        channels[channel_index++] = packed_value;
    }
}
