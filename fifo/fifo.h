#ifndef __FIFO_H__
#define __FIFO_H__

#include "common.h"
#include <stdio.h>
#include <stdlib.h>



typedef struct{
    uint8_t *buffer_addr;       //缓存空间地址
    int size;                   //缓存总空间大小
    int rd_place;               //当前读取字符位置的下一个位置
    int wr_place;              //当前写入字符位置的下一个位置
}fifo_buf_t, *fifo_buf_p;


int fifo_init(fifo_buf_p fifo_ptr, int fifo_size);

int fifo_get_readable_num(fifo_buf_p fifo_ptr);

int fifo_get_writeable_num(fifo_buf_p fifo_ptr);

int fifo_read(fifo_buf_p fifo_ptr, uint8_t *read_save_buf, int read_size);

int fifo_write(fifo_buf_p fifo_ptr, uint8_t *write_cbuf, int write_size);

int fifo_uninit(fifo_buf_p fifo_ptr);



#endif
