#include "msg_queue_mod.h"
#include <sys/msg.h>
#include <sys/ipc.h>

int msg_queue_init(int *msg_fd)
{
    int ret = 0;
    key_t key = ftok(".", 'a'); //获取键值
    if(-1 == key) {
        printf("[error] ftok failed\n");
        return -1;
    }
    ret = msgget(key, IPC_CREAT | 0777);    ///创建消息队列
    if(ret == -1) {
        printf("[error] msg_queue_init msgget failed\n");
        return ret;
    }
    *msg_fd = ret;
    return 0;
}

int msg_queue_deinit(int msg_fd)
{
    int ret = 0;
    ret = msgctl(msg_fd, IPC_RMID, NULL);   //删除消息队列
    if(ret < 0) {
        printf("[error] msg_queue_deinit msgctrl failed\n");
        return -1;
    }
    return 0;
}


int msg_queue_send(int msg_fd, uint8_t cmd, uint8_t *data_addr, uint16_t data_len)
{
    int ret = 0, count;
    msg_queue_mod_t send_msg;
    send_msg.mtype = MTYPE_VAL;
    send_msg.cmd = cmd;
    send_msg.length = data_len;
    for(count = 0 ; count < data_len ; count++) {
        send_msg.data[count] = data_addr[count];
    }
    printf("[start] msg send, msg fd:%d\n", msg_fd);
    ret = msgsnd(msg_fd, &send_msg, sizeof(send_msg), 0);
    if(ret < 0) {
        printf("[error] msg_queue_send\n");
        return -1;
    }
    printf("[ok] msg send\n");
    return 0;
}

int msg_queue_recv(int msg_fd, msg_queue_mod_t *msg_queue_ptr)
{
    int ret = 0;
    if(NULL == msg_queue_ptr) {
        printf("[error] msg_queue_recv ptr is null\n");
        return -1;
    }
    memset(msg_queue_ptr->data, 0, sizeof(msg_queue_ptr->data));
    ret = msgrcv(msg_fd, msg_queue_ptr, sizeof(msg_queue_mod_t), 1, 0);
    if(ret < 0) {
        printf("[error] msg_queue_recv\n");
        return -1;
    }
    return 0;
}