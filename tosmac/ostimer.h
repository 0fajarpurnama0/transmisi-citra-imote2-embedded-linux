/*
 * driver/tosmac/ostimer.h
 * Author: Gefan Zhang
 */

#ifndef __TIMER_H
#define __TIMER_H

void timer_init(void);
void timer_start(uint32_t interval);
void timer_stop(void);
uint8_t  timer_channel_check(void);
uint8_t  timer_is_running(void);
#endif 
