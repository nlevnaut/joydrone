#include "display.h"
#include <stdio.h>
#include <string.h>

static unsigned long frame_counter = 0;
static char display_buffer[4096];  // Adjust size as needed

// Helper function to write to our buffer instead of directly to stdout
static void buffer_printf(char *buffer, size_t *offset, const char *format, ...) {
    va_list args;
    va_start(args, format);
    *offset += vsprintf(buffer + *offset, format, args);
    va_end(args);
}

void display_joystick_info(Joystick *left, Joystick *right) {
    size_t offset = 0;
    
    // Clear buffer
    memset(display_buffer, 0, sizeof(display_buffer));
    
    buffer_printf(display_buffer, &offset, "%-40s %s\n", "Left Stick Axes:", "Right Stick Axes:");
    buffer_printf(display_buffer, &offset, "%-40s %s\n", left->name, right->name);
    
    int max_axes = (left->axis_count > right->axis_count) ? 
                   left->axis_count : right->axis_count;
    
    for (int i = 0; i < max_axes; i++) {
        // Left stick axis
        if (i < left->axis_count) {
            buffer_printf(display_buffer, &offset, "Axis %d: %6d ", i, left->axes[i]);
            buffer_printf(display_buffer, &offset, "[");
            int center = 10;
            int pos = (left->axes[i] + 32768) * 20 / 65535;
            for (int j = 0; j < 20; j++) {
                if (pos > center && j >= center && j <= pos) {
                    buffer_printf(display_buffer, &offset, "█");
                } else if (pos < center && j <= center && j >= pos) {
                    buffer_printf(display_buffer, &offset, "█");
                } else if (j == center) {
                    buffer_printf(display_buffer, &offset, "|");
                } else {
                    buffer_printf(display_buffer, &offset, " ");
                }
            }
            buffer_printf(display_buffer, &offset, "]");
        } else {
            buffer_printf(display_buffer, &offset, "%40s", "");
        }

        buffer_printf(display_buffer, &offset, "    ");
        
        // Right stick axis
        if (i < right->axis_count) {
            buffer_printf(display_buffer, &offset, "Axis %d: %6d ", i, right->axes[i]);
            buffer_printf(display_buffer, &offset, "[");
            int center = 10;
            int pos = (right->axes[i] + 32768) * 20 / 65535;
            for (int j = 0; j < 20; j++) {
                if (pos > center && j >= center && j <= pos) {
                    buffer_printf(display_buffer, &offset, "█");
                } else if (pos < center && j <= center && j >= pos) {
                    buffer_printf(display_buffer, &offset, "█");
                } else if (j == center) {
                    buffer_printf(display_buffer, &offset, "|");
                } else {
                    buffer_printf(display_buffer, &offset, " ");
                }
            }
            buffer_printf(display_buffer, &offset, "]");
        }
        buffer_printf(display_buffer, &offset, "\n");
    }

    // Display buttons
    buffer_printf(display_buffer, &offset, "\nLeft Stick Buttons (%d):\n", left->button_count);
    for (int i = 0; i < left->button_count && left->buttons != NULL; i++) {
        if (i > 0 && i % 16 == 0) buffer_printf(display_buffer, &offset, "\n");
        left->buttons[i] ? buffer_printf(display_buffer, &offset, "🟩 ") : 
                          buffer_printf(display_buffer, &offset, "⬛ ");
    }
    
    buffer_printf(display_buffer, &offset, "\n\nRight Stick Buttons (%d):\n", right->button_count);
    for (int i = 0; i < right->button_count && right->buttons != NULL; i++) {
        if (i > 0 && i % 16 == 0) buffer_printf(display_buffer, &offset, "\n");
        right->buttons[i] ? buffer_printf(display_buffer, &offset, "🟩 ") : 
                           buffer_printf(display_buffer, &offset, "⬛ ");
    }
    buffer_printf(display_buffer, &offset, "\n");
    
    // Write the entire buffer at once
    printf("%s", display_buffer);
}

void display_crsf_info(uint16_t *channels, CRSF_Frame *frame) {
    size_t offset = 0;
    memset(display_buffer, 0, sizeof(display_buffer));
    
    buffer_printf(display_buffer, &offset, "\n\nCRSF Channels:");
    buffer_printf(display_buffer, &offset, "                                                            ╔════════════ CRSF Frame ════════════╗\n");
    
    for (int i = 0; i < CRSF_NUM_CHANNELS/2; i++) {
        // Left column (channels 1-8)
        buffer_printf(display_buffer, &offset, "CH%2d: %4d ", i + 1, channels[i]);
        int pos = (channels[i] - CRSF_CHANNEL_VALUE_MIN) * 20 / 
                 (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN);
        buffer_printf(display_buffer, &offset, "[");
        for (int j = 0; j < 20; j++) {
            if (j <= pos) buffer_printf(display_buffer, &offset, "█");
            else buffer_printf(display_buffer, &offset, " ");
        }
        buffer_printf(display_buffer, &offset, "]    ");

        // Right column (channels 9-16)
        int right_ch = i + CRSF_NUM_CHANNELS/2;
        buffer_printf(display_buffer, &offset, "CH%2d: %4d ", right_ch + 1, channels[right_ch]);
        pos = (channels[right_ch] - CRSF_CHANNEL_VALUE_MIN) * 20 / 
              (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN);
        buffer_printf(display_buffer, &offset, "[");
        for (int j = 0; j < 20; j++) {
            if (j <= pos) buffer_printf(display_buffer, &offset, "█");
            else buffer_printf(display_buffer, &offset, " ");
        }
        buffer_printf(display_buffer, &offset, "]    ");

        switch(i) {
            case 0:
                buffer_printf(display_buffer, &offset, "║  Addr: C8  Size: 1A  Type: 16      ║");
                break;
            case 1:
                buffer_printf(display_buffer, &offset, "║  Payload:                          ║");
                break;
            case 2:
                buffer_printf(display_buffer, &offset, "║  %02X %02X %02X %02X %02X %02X %02X %02X           ║",
                    frame->payload[0], frame->payload[1], frame->payload[2], 
                    frame->payload[3], frame->payload[4], frame->payload[5],
                    frame->payload[6], frame->payload[7]);
                break;
            case 3:
                buffer_printf(display_buffer, &offset, "║  %02X %02X %02X %02X %02X %02X %02X %02X           ║",
                    frame->payload[8], frame->payload[9], frame->payload[10],
                    frame->payload[11], frame->payload[12], frame->payload[13],
                    frame->payload[14], frame->payload[15]);
                break;
            case 4:
                buffer_printf(display_buffer, &offset, "║  %02X %02X %02X %02X %02X %02X %02X %02X           ║",
                    frame->payload[16], frame->payload[17], frame->payload[18],
                    frame->payload[19], frame->payload[20], frame->payload[21],
                    frame->payload[22], frame->payload[23]);
                break;
            case 5:
                buffer_printf(display_buffer, &offset, "║  CRC: %02X                           ║", frame->crc);
                break;
            case 6:
                buffer_printf(display_buffer, &offset, "║                                    ║");
                break;
            case 7:
                buffer_printf(display_buffer, &offset, "╚════════════════════════════════════╝");
                break;
        }
        buffer_printf(display_buffer, &offset, "\n");
    }
    
    printf("%s", display_buffer);
}

void display_full_debug(Joystick *left, Joystick *right, uint16_t *channels, CRSF_Frame *frame) {
    frame_counter++;
    // Save cursor and move to home
    printf("\033[H\033[J");
    // Draw output
    printf("Frame: %lu\n\n", frame_counter);
    display_joystick_info(left, right);
    display_crsf_info(channels, frame);
    // make sure output is displayed
    fflush(stdout);
}
