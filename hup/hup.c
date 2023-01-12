#include "hup.h"

#define HUP_TAG_HR  0xAA
#define HUP_TAG_L   0xDD
#define HUP_TAG_HS  0xBB

enum hup_state {
    HUP_TAG_ONE = 1,
    HUP_TAG_TWO,
    HUP_CMD_ID,
    HUP_LENGTH_H,
    HUP_LENGTH_L,
    HUP_DATA,
    HUP_CHECK_SUM
};

/******************************************************************************************
 * Function Name :  hup_unpack
 * Description   :  根据传入的pack_data，进行hup协议解析包数据
 * Parameters    :  pack_data：待解析的包数据
                    msg: 存放解析成功后的有用数据
 * Returns       :  成功返回        0
                    失败返回        -1
*******************************************************************************************/
// 0xAA 0xDD 01 00 01 01 77 
int hup_unpack(uint8_t pack_data, pack_msg_t *msg)
{
    static int s_data_num = 0;
    static uint8_t s_check_num = 0;
    static int s_state = HUP_TAG_ONE;
    if (NULL == msg) {
        dbg_perror("hup_unpack failed!\r\n");
        return -1;
    }
    msg->status = false;
    switch (s_state) {
        case HUP_TAG_ONE:
            if (HUP_TAG_HR == pack_data) { 
                s_data_num = 0;
                s_check_num = 0;
                s_state = HUP_TAG_TWO;                
            } else {
                s_state = HUP_TAG_ONE;
            }
            break;
        case HUP_TAG_TWO:
            if (HUP_TAG_L == pack_data) { 
                s_state = HUP_CMD_ID;
                s_check_num = (HUP_TAG_HR ^ HUP_TAG_L);
            } else {
                s_state = HUP_TAG_ONE;
            }
            break;
        case HUP_CMD_ID:
            msg->cmd_id = pack_data;
            s_state = HUP_LENGTH_H;
            s_check_num ^= pack_data;
            break;
        case HUP_LENGTH_H:
            msg->length = pack_data;
            s_state = HUP_LENGTH_L;
            break;
        case HUP_LENGTH_L:
            msg->length = (msg->length << 8) | pack_data;
            s_check_num ^= msg->length;
            if (0 == msg->length) {
                s_state = HUP_CHECK_SUM;
            } else {
                s_state = HUP_DATA;
            }
            break;
        case HUP_DATA:
            s_check_num ^= pack_data;
            msg->data_buf[s_data_num++] = pack_data;
            if (s_data_num == msg->length) {
                s_state = HUP_CHECK_SUM;
            }
            break;
        case HUP_CHECK_SUM:
            dbg_printf("[hup s_check_num:--%x--]\n", s_check_num);
            if (s_check_num == pack_data) {
                msg->status = true;
                printf("[true hup pack]\n");
            }
            s_state = HUP_TAG_ONE;
            s_check_num = 0;
            break;
        default :
            dbg_perror("hup_unpack switch default!\r\n");
            break;
    }
    return 0;
}



 /******************************************************************************************
 * Function Name :  hup_pack
 * Description   :  根据传入的msg，进行hup协议打包数据
 * Parameters    :  msg: 存放解析成功后的有用数据
                    data_buf：存放打包好的数据
 * Returns       :  成功返回        0
                    失败返回        -1
*******************************************************************************************/

int hup_pack(uint8_t *data_in, uint16_t data_length, uint8_t cmd, uint8_t *data_out)
{
    int count;
    uint8_t check_num = 0;
    if (NULL == data_in) {
        dbg_perror("hup_pack failed!\r\n");
        return -1;
    }
    data_out[0] = HUP_TAG_HS;
    data_out[1] = HUP_TAG_L;
    data_out[2] = cmd;
    data_out[3] = data_length >> 8;
    data_out[4] = data_length & 0xff;

    check_num = data_out[0]^data_out[1]^data_out[2]^data_length;
    for (count = 0; count < data_length; count++) {
        check_num ^= data_in[count];
        data_out[count + 5] = data_in[count];
    }
    data_out[data_length + 5] = check_num;
    return data_length + 6;
}

