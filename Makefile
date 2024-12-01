CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_DEFAULT_SOURCE
TARGET = joydrone
SRCS = joystick.c
OBJS = $(SRCS:.c=.o)

# Debug build flags
DEBUG_CFLAGS = -g -DDEBUG_OUTPUT=1 -DSIMULATE_SERIAL=1
RELEASE_CFLAGS = -DDEBUG_OUTPUT=0 -DSIMULATE_SERIAL=0

.PHONY: all clean debug release

# default target
all: release

# debug build
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: $(TARGET)

# release build
release: CFLAGS += $(RELEASE_CFLAGS)
release: $(TARGET)

# link
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# compile
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean build files
clean:
	rm -f $(OBJS) $(TARGET)
