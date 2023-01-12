#ifndef __UART_DRV_H__
#define __UART_DRV_H__

#include "common.h"


#define UART_DEV    "/dev/ttyUSB0"


int uart_drv_init(int *fd, int baud, int data_bit);
int uart_read(int fd, uint8_t *data_buf, int len);
int uart_write(int fd, uint8_t *data_buf, int len);
void uart_drv_uninit(int fd);

#endif
