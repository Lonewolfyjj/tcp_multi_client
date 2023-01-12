#include "led_drv.h"



/******************************************************************************
  Function:       led_single_ctrl
  Description:    initialize led device and set it on or off
  Input:          led_dev_name  --  led name, such as 'myc\:blue\:cpu0'
                  status		--  led status. 1: on; 0: off. 
  Output:          
  Return:         int		-- return the led set status
  Others:         NONE
*******************************************************************************/

int led_single_ctrl(const char *led_dev_name, int status)
{
    int ret = 0;
    char cmdline[128];
    sprintf(cmdline, "echo %d > /sys/class/leds/%s/brightness", status, led_dev_name);
    ret =  system(cmdline);
    return  WEXITSTATUS(ret);    
}





