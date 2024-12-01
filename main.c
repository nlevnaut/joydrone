#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include "joystick.h"
#include "crsf.h"
#include "display.h"

// Function prototypes
int init_serial(const char *serial_port);
void map_joysticks_to_channels(Joystick *left, Joystick *right, uint16_t *channels);

int main() {
    Joystick left_stick, right_stick;
    CRSF_Frame frame;
    uint16_t channels[CRSF_NUM_CHANNELS] = {0};
    
    // Initialize joysticks
    if (joystick_init(&left_stick, "/dev/input/js0") < 0 ||
        joystick_init(&right_stick, "/dev/input/js1") < 0) {
        fprintf(stderr, "Failed to initialize joysticks\n");
        return 1;
    }

    // Initialize serial port if not simulating
    int serial_fd = -1;
    if (!SIMULATE_SERIAL) {
        serial_fd = init_serial("/dev/ttyAMA0");
        if (serial_fd < 0) {
            fprintf(stderr, "Failed to initialize serial port - continuing in simulation mode\n");
        }
    }

    #if DEBUG_OUTPUT
    printf("Initialization complete!\n");
    printf("Left stick: %s - %d axes, %d buttons\n", 
           joystick_get_name(&left_stick), left_stick.axis_count, left_stick.button_count);
    printf("Right stick: %s - %d axes, %d buttons\n", 
           joystick_get_name(&right_stick), right_stick.axis_count, right_stick.button_count);
    printf("Serial mode: %s\n", SIMULATE_SERIAL ? "SIMULATION" : "REAL");
    sleep(2);  // Give time to read the initialization info
    #endif

    // Main loop
    while (1) {
        // Update joystick states
        joystick_update(&left_stick);
        joystick_update(&right_stick);

        // Map joysticks to channels
        map_joysticks_to_channels(&left_stick, &right_stick, channels);

        // Prepare and send CRSF frame
        crsf_prepare_frame(&frame, channels);
        
        if (!SIMULATE_SERIAL && serial_fd >= 0) {
            write(serial_fd, &frame, frame.frame_size + 2);
        }

        #if DEBUG_OUTPUT
        display_full_debug(&left_stick, &right_stick, channels, &frame);
        #endif

        // Control update rate (100Hz)
        usleep(10000);
    }

    // Cleanup
    joystick_close(&left_stick);
    joystick_close(&right_stick);
    if (serial_fd >= 0) {
        close(serial_fd);
    }
    
    return 0;
}

int init_serial(const char *serial_port) {
    int fd = open(serial_port, O_RDWR | O_NOCTTY);
    if (fd < 0) return -1;

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tcsetattr(fd, TCSANOW, &tty);
    return fd;
}

void map_joysticks_to_channels(Joystick *left, Joystick *right, uint16_t *channels) {
    // Map main axes (X/Y for each stick)
    for (int i = 0; i < 4; i++) {
        float normalized;
        if (i < 2) {
            normalized = joystick_get_axis_normalized(left, i);
        } else {
            normalized = joystick_get_axis_normalized(right, i-2);
        }
        // Convert from -1.0 to 1.0 range to CRSF range
        channels[i] = (uint16_t)(((normalized + 1.0f) * 0.5f) * 
            (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN) + CRSF_CHANNEL_VALUE_MIN);
    }

    // Map extra axes (thumbsticks, etc.)
    channels[4] = (uint16_t)(((joystick_get_axis_normalized(right, 3) + 1.0f) * 0.5f) * 
        (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN) + CRSF_CHANNEL_VALUE_MIN);
    channels[5] = (uint16_t)(((joystick_get_axis_normalized(right, 4) + 1.0f) * 0.5f) * 
        (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN) + CRSF_CHANNEL_VALUE_MIN);

    // Pack buttons into remaining channels (7-16)
    int buttons_per_channel = 11;  // Since CRSF channels are 11-bit
    int channel_index = 6;  // Start at channel 7
    int button_index = 0;

    // Pack left stick buttons
    while (button_index < left->button_count && channel_index < CRSF_NUM_CHANNELS) {
        uint16_t packed_value = CRSF_CHANNEL_VALUE_MIN;  // Start with minimum value
        
        // Pack up to 11 buttons into this channel
        for (int i = 0; i < buttons_per_channel && button_index < left->button_count; i++) {
            if (joystick_get_button(left, button_index)) {
                packed_value |= (1 << i);
            }
            button_index++;
        }
        
        // Scale the packed value to CRSF range if any buttons are pressed
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
            if (joystick_get_button(right, button_index)) {
                packed_value |= (1 << i);
            }
            button_index++;
        }
        
        if (packed_value > CRSF_CHANNEL_VALUE_MIN) {
            packed_value = CRSF_CHANNEL_VALUE_MIN + packed_value;
        }
        channels[channel_index++] = packed_value;
    }

    // Fill any remaining channels with center value
    while (channel_index < CRSF_NUM_CHANNELS) {
        channels[channel_index++] = (CRSF_CHANNEL_VALUE_MIN + CRSF_CHANNEL_VALUE_MAX) / 2;
    }
}
