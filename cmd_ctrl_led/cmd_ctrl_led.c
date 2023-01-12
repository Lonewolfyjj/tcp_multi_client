#include "cmd_ctrl_led.h"
#include "led_drv.h"
#include "timer_drv.h"


#define LED_OFF_ONE         0x00
#define LED_ON_TWO          0x01
#define LED_LOW_FLASH       0x02
#define LED_QUICK_FLASH     0x05
#define END_SEND            0x06 

/******************************************************************************************
 * Function Name :  led_display
 * Description   :  灯的亮灭
 * Parameters    :  无
 * Returns       :  无
*******************************************************************************************/
static void led_display(void)
{
    static int s_rev_fg = 1;
    if (s_rev_fg == 1) {
        led_single_ctrl(LED_DEV_NAME1, LED_ON);
        s_rev_fg = 2;
    } else {
        led_single_ctrl(LED_DEV_NAME1, LED_OFF);
        s_rev_fg = 1;
    }
}

/******************************************************************************************
 * Function Name :  cmd_deal
 * Description   :  处理fifo中解析成功的得到的命令，比如灯的亮灭，快慢闪烁
 * Parameters    :  msg：解析成功的数据结构体
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int cmd_deal(pack_msg_p msg)
{
    static int s_led_fg = 0;
    if (NULL == msg) {
        dbg_perror("cmd_deal failed!\r\n");
        return -1;
    }
    if (1 == s_led_fg) {
        s_led_fg = 0;
        start_or_stop_time(0, 0);
    }
    switch(msg->cmd_id) {
        case LED_OFF_ONE:
            led_single_ctrl(LED_DEV_NAME1, LED_OFF);
            break;
        case LED_ON_TWO:
            led_single_ctrl(LED_DEV_NAME1, LED_ON);
            break;
        case LED_LOW_FLASH:
            s_led_fg = 1;
            timer_signal_init(led_display);
            start_or_stop_time(msg->data_buf[0], 0);
            break;
        case LED_QUICK_FLASH: 
            s_led_fg = 1;
            timer_signal_init(led_display);
            start_or_stop_time(0, 100000 * msg->data_buf[0]);
            break;
        case END_SEND:
            //uart_stop(manage_pth);
            break;
        default:
            dbg_perror("cmd_deal arg error!\r\n");
            break;
    }
    return 0;
}




