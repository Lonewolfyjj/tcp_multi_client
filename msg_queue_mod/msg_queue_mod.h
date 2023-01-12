#ifndef __MSG_QUEUE_MOD_H__
#define __MSG_QUEUE_MOD_H__

#include "common.h"


#define MSG_QUEUE_DATA_SIZE     256
#define MTYPE_VAL   1


typedef struct q_msg_param_struct {
    long mtype;
    uint8_t cmd;
    uint16_t length;
    uint8_t data[MSG_QUEUE_DATA_SIZE];
}msg_queue_mod_t, *msg_queue_mod_p;





int msg_queue_init(int *msg_fd);
int msg_queue_deinit(int msg_fd);
int msg_queue_send(int msg_fd, uint8_t cmd, uint8_t *data_addr, uint16_t data_len);
int msg_queue_recv(int msg_fd, msg_queue_mod_t *mag_queue_ptr);



#endif