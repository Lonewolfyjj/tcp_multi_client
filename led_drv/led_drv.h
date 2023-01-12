#ifndef __LED_DRV_H__
#define __LED_DRV_H__

#include "common.h"


#define LED_DEV_NAME1   "myc:green:user1"


#define LED_ON      1
#define LED_OFF     0


int led_single_ctrl(const char * led, int on);

#endif	 // __LED_DRV_H__	


