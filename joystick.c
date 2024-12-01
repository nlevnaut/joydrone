// joystick.c
#include "joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int joystick_init(Joystick *js, const char *device_path) {
    js->js_fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (js->js_fd < 0) {
        return -1;
    }

    // Get joystick name
    if (ioctl(js->js_fd, JSIOCGNAME(sizeof(js->name)), js->name) < 0) {
        strcpy(js->name, "Unknown");
    }

    unsigned char num_axes;
    unsigned char num_buttons;
    
    if (ioctl(js->js_fd, JSIOCGAXES, &num_axes) < 0 ||
        ioctl(js->js_fd, JSIOCGBUTTONS, &num_buttons) < 0) {
        close(js->js_fd);
        return -1;
    }

    js->axis_count = num_axes;
    js->button_count = num_buttons;

    js->axes = calloc(js->axis_count, sizeof(int16_t));
    js->buttons = calloc(js->button_count, sizeof(int8_t));

    if (!js->axes || !js->buttons) {
        close(js->js_fd);
        free(js->axes);
        free(js->buttons);
        return -1;
    }

    return 0;
}

void joystick_update(Joystick *js) {
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

void joystick_close(Joystick *js) {
    if (js->js_fd >= 0) {
        close(js->js_fd);
    }
    free(js->axes);
    free(js->buttons);
    js->axes = NULL;
    js->buttons = NULL;
    js->js_fd = -1;
}

float joystick_get_axis_normalized(Joystick *js, int axis_index) {
    if (axis_index < 0 || axis_index >= js->axis_count) {
        return 0.0f;
    }
    return js->axes[axis_index] / 32767.0f;
}

int joystick_get_button(Joystick *js, int button_index) {
    if (button_index < 0 || button_index >= js->button_count) {
        return 0;
    }
    return js->buttons[button_index];
}

const char* joystick_get_name(Joystick *js) {
    return js->name;
}
