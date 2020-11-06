/*
 * driver/tosmac/ostimer.c
 * Author: Gefan Zhang
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/hardware.h>
#include <asm/arch/pxa-regs.h>
#include <asm/arch/stargate2.h>

#include "ostimer.h"
#include "SG2_CC2420.h"

uint8_t timer_running;

void timer_init()
{
    OMCR5 = 0x0UL;
    OMCR5 = (OMCR5 | OMCR_C | OMCR_R | OMCR_CRES(0x4));
    timer_running = FALSE;
//    printk(KERN_INFO "OMCR5 is %x\n", OMCR5);
}


void timer_start(uint32_t interval)
{   
//    printk(KERN_INFO "timer started\n");
//    printk(KERN_INFO "interval is %d\n", interval);
    timer_running = TRUE;
    OSMR5 = (interval << 5);
    OIER |= OIER_E5;
    OSCR5 = 0x0UL;
}

void timer_stop()
{
    timer_running = FALSE;
    OIER &= ~(OIER_E5);
}

uint8_t timer_is_running()
{
    return timer_running;
}

uint8_t timer_channel_check()
{
    if (OSSR & OIER_E5) {
//	printk("OS Timer IRQ\n");
	OSSR = OIER_E5;
	OIER &= ~(OIER_E5);
	timer_running = FALSE;
	return SUCCESS;
    }
    else
	return FAIL;
}


