#ifndef __HUP_H__
#define __HUP_H__

#include "common.h"

typedef struct pack_msg {
    uint8_t cmd_id;
    uint16_t length;
    uint16_t size;
    uint8_t data_buf[255];
    bool status;
}pack_msg_t, *pack_msg_p;


int hup_unpack(uint8_t pack_data, pack_msg_t *msg);
int hup_pack(uint8_t *data_in, uint16_t data_length, uint8_t cmd, uint8_t *data_out);    



#endif

