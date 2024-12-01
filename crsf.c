// crsf.c
#include "crsf.h"
#include <string.h>

void crsf_pack_channels(uint16_t *channels, uint8_t *payload) {
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

uint8_t crsf_calc_crc(uint8_t *data, int length) {
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

void crsf_prepare_frame(CRSF_Frame *frame, uint16_t *channels) {
    frame->device_addr = CRSF_SYNC_BYTE;
    frame->frame_size = CRSF_CHANNELS_FRAME_SIZE + 2;
    frame->type = 0x16;  // RC channels
    
    crsf_pack_channels(channels, frame->payload);
    frame->crc = crsf_calc_crc((uint8_t*)frame, frame->frame_size - 1);
}
