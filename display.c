// display.c
#include "display.h"
#include <stdio.h>

static unsigned long frame_counter = 0;

void display_clear_screen(void) {
    printf("\033[2J\033[H");
}

void display_joystick_info(Joystick *left, Joystick *right) {
    printf("%-40s %s\n", "Left Stick Axes:", "Right Stick Axes:");
    printf("%-40s %s\n", left->name, right->name);
    
    int max_axes = (left->axis_count > right->axis_count) ? 
                   left->axis_count : right->axis_count;
    
    for (int i = 0; i < max_axes; i++) {
        // Left stick axis
        if (i < left->axis_count) {
            printf("Axis %d: %6d ", i, left->axes[i]);
            printf("[");
            int center = 10;
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
            printf("%40s", "");
        }

        printf("    ");  // Spacing between sticks
        
        // Right stick axis
        if (i < right->axis_count) {
            printf("Axis %d: %6d ", i, right->axes[i]);
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

    // Display buttons
    printf("\nLeft Stick Buttons (%d):\n", left->button_count);
    for (int i = 0; i < left->button_count && left->buttons != NULL; i++) {
        if (i > 0 && i % 16 == 0) printf("\n");
        left->buttons[i] ? printf("🟩 ") : printf("⬛ ");
    }
    
    printf("\n\nRight Stick Buttons (%d):\n", right->button_count);
    for (int i = 0; i < right->button_count && right->buttons != NULL; i++) {
        if (i > 0 && i % 16 == 0) printf("\n");
        right->buttons[i] ? printf("🟩 ") : printf("⬛ ");
    }
    printf("\n");
}


void display_crsf_info(uint16_t *channels, CRSF_Frame *frame) {
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

void display_full_debug(Joystick *left, Joystick *right, uint16_t *channels, CRSF_Frame *frame) {
    frame_counter++;
    display_clear_screen();
    printf("Frame: %lu\n\n", frame_counter);
    display_joystick_info(left, right);
    display_crsf_info(channels, frame);
}
