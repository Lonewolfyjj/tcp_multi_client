#include "fifo.h"

/******************************************************************************************
 * Function Name :  fifo_init
 * Description   :  fifo结构体初始化
 * Parameters    :  myfifo：fifo结构体指针
                    fifo_size: fifo结构体中存放空间的大小
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int fifo_init(fifo_buf_p fifo_ptr, int fifo_size)
{
    if (NULL == fifo_ptr || fifo_size <= 0) {
        printf("fifo_init failed!\n");
        return -1;
    }
    fifo_ptr->buffer_addr = (uint8_t *)malloc(sizeof(uint8_t) * fifo_size);
    if (NULL == fifo_ptr->buffer_addr) {
        printf("malloc fifo_ptr failed!\n");
        return -1;
    }
    fifo_ptr->rd_place = 0;
    fifo_ptr->wr_place = 0; 
    fifo_ptr->size = fifo_size;
    return 0;
}

int fifo_get_readable_num(fifo_buf_p fifo_ptr)
{
    int ret = 0;
    if (NULL == fifo_ptr) {
        printf("fifo_get_readable_num failed!\n");
        return -1;
    }
    if ( fifo_ptr->rd_place == fifo_ptr->wr_place) {
        return 0;
    }
    ret = (fifo_ptr->size + (fifo_ptr->wr_place - fifo_ptr->rd_place)) % fifo_ptr->size;
    return ret;
}

int fifo_get_writeable_num(fifo_buf_p fifo_ptr)
{
    int ret = 0;
    if (NULL == fifo_ptr) {
        printf("fifo_get_writeable_num failed 1!\n");
        return -1;
    }
    ret = fifo_get_readable_num(fifo_ptr);
    ret = fifo_ptr->size - ret;
    return ret - 1; 
}



/******************************************************************************************
 * Function Name :  fifo_read
 * Description   :  读取fifo中的内容，并返回读取到的字符数
 * Parameters    :  myfifo：fifo结构体指针
                    read_save_buf: 存放读取到的内容的空间
                    read_size：要读取字符的大小
 * Returns       :  成功返回 读取到的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_read(fifo_buf_p fifo_ptr, uint8_t *read_save_buf, int read_size)
{
    int count;
    if (NULL == fifo_ptr || NULL == read_save_buf || read_size <= 0) {
        printf("fifo_read failed!\r\n");
        return -1;
    } 
    count = fifo_get_readable_num(fifo_ptr);
    if (count == 0) {
        printf("fifo readable_num = 0 !\r\n");
        return -1;
    } else if (count < read_size) {
        read_size = count;
    } 
    for (count = 0; count < read_size; count++) {
        read_save_buf[count] = fifo_ptr->buffer_addr[fifo_ptr->rd_place];
        fifo_ptr->rd_place = (fifo_ptr->rd_place + 1) % fifo_ptr->size;
    }
    return read_size;
}

/******************************************************************************************
 * Function Name :  fifo_write
 * Description   :  往fifo中写入字符内容，并返回写入的字符数
 * Parameters    :  fifo_ptr：fifo结构体指针
                    write_cbuf: 要写入的字符
                    write_size：要写入字符的大小
 * Returns       :  成功返回 写入成功的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_write(fifo_buf_p fifo_ptr, uint8_t *write_cbuf, int write_size)
{
    int count;
    if (NULL == fifo_ptr || NULL == write_cbuf || write_size <= 0) {
        printf("fifo_read failed!\r\n");
        return -1;
    }
    count = fifo_get_writeable_num(fifo_ptr);
    if (count == 0) {
        printf("fifo write_num = 0 !\r\n");
        return -1;
    } else if (count < write_size) {
        printf("fifo write_num = 0 !\r\n");
        return -1;
    }
    for (count = 0; count < write_size; count++) {
        fifo_ptr->buffer_addr[fifo_ptr->wr_place] = write_cbuf[count];
        fifo_ptr->wr_place = (fifo_ptr->wr_place + 1) % fifo_ptr->size;
    }
    return write_size;
}

/******************************************************************************************
 * Function Name :  fifo_uninit
 * Description   :  释放fifo中的堆空间
 * Parameters    :  myfifo：fifo结构体指针
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int fifo_uninit(fifo_buf_p fifo_ptr)
{   
    if (NULL == fifo_ptr) {
        dbg_perror("fifo_uninit failed!\r\n");
        return -1;
    }
    free(fifo_ptr->buffer_addr);
    return 0;
}


