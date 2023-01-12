#ifndef __UART_MOD_H__
#define __UART_MOD_H__

#include "common.h"
#include "uart_drv.h"
#include "fifo.h"
#include "hup.h"

#define UART_BAUD       115200
#define UART_DATA_BIT   8



typedef struct uart_module_struct {
    int uart_fd;
    int msg_queue_fd;
    pth_stru_t read_pth;
    pth_stru_t hand_pth;
    pth_stru_t send_pth;
    fifo_buf_t *recv_fifo;
    fifo_buf_t *send_fifo;
    pthread_mutex_t mutex;
}uart_mod_t;

int uart_mod_init(uart_mod_t *uart_ptr, int q_msg_fd);
int uart_mod_deinit(uart_mod_t *uart_ptr);
int uart_mod_start(uart_mod_t *uart_ptr);
int uart_mod_stop(uart_mod_t *uart_ptr);
int uart_mod_send(uart_mod_t *uart_ptr, char *data, int data_size, uint8_t cmd);

#endif
