#include "uart_drv.h"





/******************************************************************************************
 * Function Name :  uart_drv_init
 * Description   :  串口初始化，配置波特率，数据位
 * Parameters    :  fd：串口操作符地址
                    baud：波特率数值
                    data_bit：数据位数值
 * Returns       :  成功返回 状态值
                    失败返回 错误原因
*******************************************************************************************/
int uart_drv_init(int *fd, int baud, int data_bit)
{
    int ret = 0;
    char cmdline[128];
    *fd = open(UART_DEV, O_RDWR);
    if (*fd < 0) {
        dbg_perror("open uart device failed!\r\n");
        exit(EXIT_FAILURE);
    }
    sprintf(cmdline, "stty -F /dev/ttyUSB0 ispeed %d ospeed %d cs%d", baud, baud, data_bit);
    ret =  system(cmdline);
    return  WEXITSTATUS(ret);
}

int uart_read(int fd, uint8_t *data_buf, int len)
{
    int ret = 0;
    if (NULL == data_buf) {
        perror("uart_read arg error!");
        return -1;
    }
    ret = read(fd, data_buf, len);
    if (ret < 0) {
        perror("uart_read failed, uart_fd close!");
        return -1;
    }
    return ret;
}

int uart_write(int fd, uint8_t *data_buf, int len)
{
    int ret = 0;
    if (NULL == data_buf) {
        perror("uart_write arg error!");
        return -1;
    }
    ret = write(fd, data_buf, len);
    if (ret < 0) {
        perror("uart_write failed, uart_fd close!");
        return -1;
    }
    return ret;
}

/******************************************************************************************
 * Function Name :  uart_uninit
 * Description   :  串口初始化，关闭串口设备文件
 * Parameters    :  fd：串口操作符地址
 * Returns       :  无
*******************************************************************************************/
void uart_drv_uninit(int fd)
{
    close(fd);
}


