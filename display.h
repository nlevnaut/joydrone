// display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include "joystick.h"
#include "crsf.h"

void display_clear_screen(void);
void display_joystick_info(Joystick *left, Joystick *right);
void display_channel_info(uint16_t *channels);
void display_frame_info(CRSF_Frame *frame);
void display_full_debug(Joystick *left, Joystick *right, uint16_t *channels, CRSF_Frame *frame);

#endif // DISPLAY_H
