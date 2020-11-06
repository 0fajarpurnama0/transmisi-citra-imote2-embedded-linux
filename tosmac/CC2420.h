#ifndef __CC2420_H
#define __CC2420_H

#ifdef PSI
#include "CC2420_psi.h"
#define NOT_KERNEL
#endif

#ifndef NOT_KERNEL
#include "TOSMac.h"
#include <linux/poll.h>
#endif

#include "AM.h"
//#include "byteorder.h"
//#include "HPLCC2420.h"

//#define TRUE  1
//#define FALSE 0

//uint16_t g_current_parameters[14];
//uint8_t state = 0;
/*
enum {
    IDLE_STATE = 0,
    INIT_STATE,
    INIT_STATE_DONE,
    START_STATE,
    START_STATE_DONE,
    STOP_STATE,
};
*/
/*
#define TOSH_DATA_LENGTH	28
#define TOS_BCAST_ADDR		0xffff
 
struct __TOS_Msg
{
        __u8 length;
        __u8 fcfhi;
        __u8 fcflo;
        __u8 dsn;
        __u16 destpan;  // destPAN
        __u16 addr;     // destAddr
        __u8 type;
        __u8 group;
        __s8 data[TOSH_DATA_LENGTH]; // default size of data[] is 28
	__u8 strength;
	__u8 lqi;
	bool crc;
	bool ack;
	__u16 time;
};

typedef struct __TOS_Msg  TOS_Msg;
typedef struct TOS_Msg*   TOS_MsgPtr;

enum {
	// size of the header not including the length byte
	MSG_HEADER_SIZE = 9, //offsetof(struct TOS_Msg, data) - 1,
	// size of the footer
	MSG_FOOTER_SIZE = 2,
	// size of the full packet
	MSG_DATA_SIZE = 10 + TOSH_DATA_LENGTH + sizeof(uint16_t),
	// size of the data length
	DATA_LENGTH = TOSH_DATA_LENGTH,
	// position of the length byte
	LENGTH_BYTE_NUMBER = 1,
};
*/

void     CC2420_set_parameters(void);
uint8_t  CC2420_set_regs(void);
uint8_t  CC2420_oscillator_on(void);
uint8_t  CC2420_oscillator_off(void);
void     CC2420_setup_irq(void);
uint8_t  CC2420_tune_preset(uint8_t chnl);
uint8_t  CC2420_tune_manual(uint16_t DesiredFreq);
uint16_t CC2420_get_frequency(void);
uint8_t  CC2420_get_preset(void);
uint8_t  CC2420_tx_mode(void);
uint8_t  CC2420_tx_onCCA(void);
uint8_t  CC2420_rx_mode(void);
uint8_t  CC2420_set_rf_power(uint8_t power);
uint8_t  CC2420_get_rf_power(void);
uint8_t  CC2420_enable_AutoAck(void);
uint8_t  CC2420_disable_AutoAck(void);
uint8_t  CC2420_enable_AddrDecode(void);
uint8_t  CC2420_disable_AddrDecode(void);
void     CC2420_enable_ack(void);
void     CC2420_disable_ack(void);
uint8_t  CC2420_set_short_addr(uint16_t addr);
int	 CC2420_read(void);
int	 CC2420_read_poll(void);
unsigned int CC2420_poll(struct file *filp, struct poll_table_struct *wait);
void     CC2420_read_packet(unsigned long unused);
void     CC2420_send_failed(void);
void     CC2420_flush_RXFIFO(void);
void     CC2420_flush_TXFIFO(void);
void     CC2420_start_send(void); 
void     CC2420_try_to_send(void);
void     CC2420_send_test(void);
void     CC2420_send_packet(void);
void	 CC2420_packet_sent(void);
void	 CC2420_packet_rcvd(void);
int      CC2420_send(TOS_MsgPtr pMsg);
uint8_t  CC2420_RXFIFO_done(uint8_t length);
uint8_t  CC2420_init(struct tosmac_device *dev);
//uint8_t  CC2420_start(struct tosmac_device * dev);
uint8_t  CC2420_start(uint8_t is_non_blocking);
void	 CC2420_stop(struct tosmac_device * dev);
uint8_t  CC2420_exit(struct tosmac_device *dev);
void     CC2420_set_initial_timer(uint16_t jiffy);
void	 CC2420_set_backoff_timer(uint16_t jiffy);
void     CC2420_set_ack_timer(uint16_t jiffy);
void     CC2420_set_restart_timer(uint16_t jiffy);
int      CC2420_check_send_done(void);
int8_t   Mac_initial_backoff(void);
int8_t   Mac_congestion_backoff(void);

irqreturn_t CC2420_irq_handler(int irq, void* dev, struct pt_regs *regs);
irqreturn_t CC2420_timer_irq_handler(int irq, void* dev, struct pt_regs *regs);

#endif
