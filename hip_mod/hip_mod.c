#include "hip_mod.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>


int hip_mod_unpack(uint8_t *data_in, int data_in_len, hip_msg_pack_t *msg_data_out)
{
    int num = 0, length = 0, count, hip_msg_len;
    while (length < data_in_len) {
		byte_msg_t *hip_msg = (byte_msg_t *)(data_in + length);
        hip_msg_len = ((hip_msg->length_h << 8) | hip_msg->length_l);
		if (data_in_len - length < hip_msg_len + BYTE_PACK_HEAD_SIZE) {
			dbg_printf("[error, recv data error left_len: %d length: %d]\r\n", (data_in_len - length), (hip_msg_len + BYTE_PACK_HEAD_SIZE));
			break;
		}  
        //有一个完整包数据
        msg_data_out[num].msg_head.version = hip_msg->version;
        msg_data_out[num].msg_head.commond = hip_msg->commond;
        msg_data_out[num].msg_head.length_h = hip_msg->length_h;
        msg_data_out[num].msg_head.length_l = hip_msg->length_l;
        msg_data_out[num].msg_head.dev_type = hip_msg->dev_type;
        msg_data_out[num].data_len = hip_msg_len;
        memcpy(msg_data_out[num].msg_head.dev_id, hip_msg->dev_id, 8);
        memcpy(msg_data_out[num].msg_head.seq, hip_msg->seq, 4);
        for (count = 0; count < hip_msg_len; count++) {
            msg_data_out[num].data[count] = data_in[BYTE_PACK_HEAD_SIZE + count];
        }
        length += (hip_msg_len + BYTE_PACK_HEAD_SIZE);
        num++;
    }
    return num;//返回包数量
}



static int get_local_ip(char *ip) 
{
    struct ifaddrs *if_addr_struct;
    void *tmpAddrPtr = NULL;
    getifaddrs(&if_addr_struct);
    while (if_addr_struct != NULL) {
        if (if_addr_struct->ifa_addr->sa_family == AF_INET) {
            tmpAddrPtr=&((struct sockaddr_in *)if_addr_struct->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, ip, INET_ADDRSTRLEN);
        }
        if_addr_struct = if_addr_struct->ifa_next;
    }
    freeifaddrs(if_addr_struct);
    return 0;
}


//包头，数据  00:0c:29:e6:fa:63 
void hip_pack_tcp_data(uint8_t cmd, uint8_t *data_out)
{
    int count, ip_len;
    uint8_t ip_addr[16];
    get_local_ip(ip_addr);
    ip_len = strlen(ip_addr);
    data_out[0] = 0x01;
    data_out[1] = cmd;
    data_out[2] = 0;
    data_out[3] = 2 + ip_len;
    data_out[4] = 0x01;
    data_out[5] = 0;
    data_out[6] = 0;
    data_out[7] = 0;
    data_out[8] = 0x0c;
    data_out[9] = 0x29;
    data_out[10] = 0xe6;
    data_out[11] = 0xfa;
    data_out[12] = 0x63;
    for (count = 0; count < 4; count++) {
        data_out[count + 13] = cmd;
    }
    data_out[17] = 0xfa;
    data_out[18] = 0x63;    //mac
    memcpy(data_out + 19, ip_addr, ip_len);    //ip
}



void hip_pack_uart_data(uint8_t *data_in, uint16_t data_length, uint8_t cmd, uint8_t *data_out)
{
    int count;
    data_out[0] = 0x01;
    data_out[1] = cmd;
    data_out[2] = (data_length >> 8);
    data_out[3] = (data_length & 0xff);
    data_out[4] = 0x01;
    data_out[5] = 0;
    data_out[6] = 0;
    data_out[7] = 0;
    data_out[8] = 0x0c;
    data_out[9] = 0x29;
    data_out[10] = 0xe6;
    data_out[11] = 0xfa;
    data_out[12] = 0x63;
    for (count = 0; count < 4; count++) {
        data_out[count + 13] = cmd;         //seq
    }
    for (count = 0; count < data_length; count++) {
        data_out[count + 17] = data_in[count];
    }
}

int hip_mod_pack(uint8_t *data_in, uint16_t data_length, uint8_t cmd, uint8_t *data_out)
{
    int count;
    if (data_out == NULL) {
       dbg_perror("[error] hip_mod_pack failed!\r\n");
       return -1;
    }    
    switch (cmd) {
        case HIP_USER_LOGIN_PACK:
            hip_pack_tcp_data(cmd, data_out);
            break;
        case HIP_KEEP_ALIVE_PACK:
            hip_pack_tcp_data(cmd, data_out);
            break;
        case HIP_UART_PASS_THROUGH_PACK:
            hip_pack_uart_data(data_in, data_length, cmd, data_out);
            break;
        default:
            dbg_perror("[error] hip_mod_pack default!\r\n");
            break;   
    }
    return (BYTE_PACK_HEAD_SIZE + (data_out[2] | data_out[3]));
}

