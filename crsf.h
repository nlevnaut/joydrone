// crsf.h
#ifndef CRSF_H
#define CRSF_H

#include <stdint.h>

// CRSF Protocol Constants
#define CRSF_SYNC_BYTE 0xC8
#define CRSF_CHANNELS_FRAME_SIZE 24
#define CRSF_NUM_CHANNELS 16
#define CRSF_CHANNEL_VALUE_MIN 172
#define CRSF_CHANNEL_VALUE_MAX 1811

typedef struct {
    uint8_t device_addr;
    uint8_t frame_size;
    uint8_t type;
    uint8_t payload[CRSF_CHANNELS_FRAME_SIZE];
    uint8_t crc;
} CRSF_Frame;

// Function declarations
void crsf_pack_channels(uint16_t *channels, uint8_t *payload);
uint8_t crsf_calc_crc(uint8_t *data, int length);
void crsf_prepare_frame(CRSF_Frame *frame, uint16_t *channels);

#endif // CRSF_H
