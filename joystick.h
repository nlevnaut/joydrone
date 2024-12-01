// joystick.h
#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>
#include <linux/joystick.h>

// Joystick structure definition
typedef struct {
    int js_fd;
    int axis_count;
    int button_count;
    struct js_event event;
    int16_t *axes;
    int8_t *buttons;
    char name[128];
} Joystick;

// Function declarations
int joystick_init(Joystick *js, const char *device_path);
void joystick_update(Joystick *js);
void joystick_close(Joystick *js);

// Utility functions
float joystick_get_axis_normalized(Joystick *js, int axis_index);  // Returns -1.0 to 1.0
int joystick_get_button(Joystick *js, int button_index);          // Returns 0 or 1
const char* joystick_get_name(Joystick *js);

#endif // JOYSTICK_H
