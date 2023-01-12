#ifndef __HIP_MOD_H__
#define __HIP_MOD_H__

#include "common.h"
#include "fifo.h"

#define DEVICE_ID_SIZE      8
#define SEQ_SIZE            4

#define BYTE_PACK_HEAD_SIZE   17   

typedef struct byte_msg_struct {
    uint8_t version;
    uint8_t commond;
    uint8_t length_h;
    uint8_t length_l;
    uint8_t dev_type;
    uint8_t dev_id[DEVICE_ID_SIZE];
    uint8_t seq[SEQ_SIZE];
}byte_msg_t;

typedef struct hip_pack_struct {
    uint16_t data_len;
    uint8_t data[255];
    byte_msg_t msg_head;
}hip_msg_pack_t;


enum pack_cmd {
    HIP_USER_LOGIN_PACK         = 0x02,               //用户登录
    HIP_KEEP_ALIVE_PACK         = 0x03,                //心跳保持
    HIP_UART_PASS_THROUGH_PACK  = 0x07,              //串口透传
};


int hip_mod_unpack(uint8_t *data_in, int data_in_len, hip_msg_pack_t *msg_data_out);
int hip_mod_pack(uint8_t *data_in, uint16_t data_length, uint8_t cmd, uint8_t *data_out);   


#endif