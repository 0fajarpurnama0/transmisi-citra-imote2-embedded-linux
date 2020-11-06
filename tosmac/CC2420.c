/*
 *   driver/tosmac/CC2420.c
 *
 *   Author: Gefan Zhang
 *	     Trevor Pering
 *   Last modified by Y.Cheng:11/April/2008
 *
 */

#ifdef PSI
#include "CC2420_psi.h"
#define NOT_KERNEL
#endif

#ifndef NOT_KERNEL

#include <linux/config.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/wait.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/mach-types.h>
#include <asm/hardware.h>
#include <asm/system.h>
#include <asm/atomic.h>
#include <asm/arch/pxa-regs.h>
#include <asm/arch/stargate2.h>
#include "../platx/pmic.h"

#endif

#ifndef NOT_KERNEL
#include "SG2_CC2420.h"
#include "TOSMac.h"
#include "ostimer.h"
#endif

#include "byteorder.h"
#include "CC2420.h"
#include "CC2420Const.h"
#include "AM.h"

uint8_t CC2420_CHANNEL =            CC2420_DEF_CHANNEL;
uint8_t CC2420_RFPOWER =            CC2420_DEF_RFPOWER;

#define MAX_SEND_TRIES   8
#define MAX_RESTART_TRIES 10


static DECLARE_WAIT_QUEUE_HEAD(wq);
static DECLARE_WAIT_QUEUE_HEAD(rq);
static DECLARE_WAIT_QUEUE_HEAD(inq);

// don't make these static so our HPL hacks can get to them
static __u8 send_done = 0;  
static __u8 recv_done = 0;  

static DECLARE_MUTEX(CC2420_sem);     //declare the singnal variable
DECLARE_TASKLET(read_packet_tasklet, CC2420_read_packet, 0);


enum {
    DISABLED_STATE = 0,
    DISABLED_STATE_STARTTASK,
    IDLE_STATE,//YC: ready to send
    TX_STATE,//YC: CCA passed , get data ready, send data
    TX_WAIT, //YC: wait for read message to receive ack message
    PRE_TX_STATE,//YC: check CCA in this state, after CCA available, to TX_STATE; if fail, backoff to retry CCA
    POST_TX_STATE,//YC: check ack message in this state
    POST_TX_ACK_STATE,//YC: this state indicate ack check has been done, either passed or failed from POST_TX_STATE
    RX_STATE, //YC: not used
    POWER_DOWN_STATE,
    WARMUP_STATE,

    TIMER_INITIAL = 0,
    TIMER_BACKOFF,
    TIMER_ACK,
    TIMER_RESTART,
    TIMER_TRANSMIT
	
};

uint8_t count_retry;
uint8_t count_send_restart;
uint8_t CC2420_state;
uint8_t CC2420_timer_state;
uint8_t non_blocking_mode;
uint8_t ok_to_read;
uint8_t state_timer;
uint8_t current_DSN; // data sequence number
uint8_t is_ack_enable;
uint8_t is_packet_receiving;
TOS_MsgPtr tx_buf_ptr;  // pointer to transmit buffer
TOS_MsgPtr rx_buf_ptr;  // pointer to receive buffer
TOS_Msg    rx_buf;      // temp save received messages
TOS_Msg    rx_packet;   // save received packet
uint8_t	   rx_packet_length; // length of recevied packet
uint16_t   g_current_parameters[14];


//CC2420 Params setting

void CC2420_set_parameters( )
{
    // Set default parameters from CC2420 chipset specs, p.62
    g_current_parameters[CP_MAIN] = 0xf800;
	
    g_current_parameters[CP_MDMCTRL0] = ((0 << CC2420_MDMCTRL0_ADRDECODE) |
       (2 << CC2420_MDMCTRL0_CCAHIST) | (3 << CC2420_MDMCTRL0_CCAMODE)  |
       (1 << CC2420_MDMCTRL0_AUTOCRC) | (2 << CC2420_MDMCTRL0_PREAMBL));

    g_current_parameters[CP_MDMCTRL1] = 20 << CC2420_MDMCTRL1_CORRTHRESH;

    g_current_parameters[CP_RSSI] =     0xE080;
    g_current_parameters[CP_SYNCWORD] = 0xA70F;
    g_current_parameters[CP_TXCTRL] = ((1 << CC2420_TXCTRL_BUFCUR) |
       (1 << CC2420_TXCTRL_TURNARND) | (3 << CC2420_TXCTRL_PACUR) |
       (1 << CC2420_TXCTRL_PADIFF) | (CC2420_RFPOWER << CC2420_TXCTRL_PAPWR));

    g_current_parameters[CP_RXCTRL0] = ((1 << CC2420_RXCTRL0_BUFCUR) |
       (2 << CC2420_RXCTRL0_MLNAG) | (3 << CC2420_RXCTRL0_LOLNAG) |
       (2 << CC2420_RXCTRL0_HICUR) | (1 << CC2420_RXCTRL0_MCUR) |
       (1 << CC2420_RXCTRL0_LOCUR));

    g_current_parameters[CP_RXCTRL1] = ((1 << CC2420_RXCTRL1_LOLOGAIN) |
       (1 << CC2420_RXCTRL1_HIHGM) | (1 << CC2420_RXCTRL1_LNACAP) |
       (1 << CC2420_RXCTRL1_RMIXT) | (1 << CC2420_RXCTRL1_RMIXV)  |
       (2 << CC2420_RXCTRL1_RMIXCUR));

    g_current_parameters[CP_FSCTRL] = ((1 << CC2420_FSCTRL_LOCK) |
       ((357+5*(CC2420_CHANNEL-11)) << CC2420_FSCTRL_FREQ));

    g_current_parameters[CP_SECCTRL0] = ((1 << CC2420_SECCTRL0_CBCHEAD) |
       (1 << CC2420_SECCTRL0_SAKEYSEL) | (1 << CC2420_SECCTRL0_TXKEYSEL) |
       (1 << CC2420_SECCTRL0_SECM));

    g_current_parameters[CP_SECCTRL1] = 0;
    g_current_parameters[CP_BATTMON]  = 0;

    // set fifop threshold to greater than size of tos msg,
    // fifop goes active at end of msg
    g_current_parameters[CP_IOCFG0] = (((127) << CC2420_IOCFG0_FIFOTHR) |
        (1 <<CC2420_IOCFG0_FIFOPPOL));

    g_current_parameters[CP_IOCFG1] = 0;

}
   /************************************************************************
   * CC2420_set_regs
   *  - Configure CC2420 registers with current values
   *  - Readback 1st register written to make sure electrical connection OK
   *************************************************************************/
uint8_t CC2420_set_regs( )
{
    uint16_t data;
    
    CC2420_write_reg(CC2420_MAIN,g_current_parameters[CP_MAIN]); 
    CC2420_write_reg(CC2420_MDMCTRL0, g_current_parameters[CP_MDMCTRL0]);
    data = CC2420_read_reg(CC2420_MDMCTRL0);
//    printk(KERN_INFO "MDMCTRL0 is %x\n", data);
//    printk(KERN_INFO "gcurrent is  is %x\n", g_current_parameters[CP_MDMCTRL0]);
    
    CC2420_write_reg(CC2420_MDMCTRL1, g_current_parameters[CP_MDMCTRL1]);
    CC2420_write_reg(CC2420_RSSI, g_current_parameters[CP_RSSI]);
    CC2420_write_reg(CC2420_SYNCWORD, g_current_parameters[CP_SYNCWORD]);
    CC2420_write_reg(CC2420_TXCTRL, g_current_parameters[CP_TXCTRL]);
    CC2420_write_reg(CC2420_RXCTRL0, g_current_parameters[CP_RXCTRL0]);
    CC2420_write_reg(CC2420_RXCTRL1, g_current_parameters[CP_RXCTRL1]);
    CC2420_write_reg(CC2420_FSCTRL, g_current_parameters[CP_FSCTRL]);

    CC2420_write_reg(CC2420_SECCTRL0, g_current_parameters[CP_SECCTRL0]);
    CC2420_write_reg(CC2420_SECCTRL1, g_current_parameters[CP_SECCTRL1]);
//    printk(KERN_INFO "before set:IOCFG0 is %x\n", CC2420_read_reg(CC2420_IOCFG0));
    CC2420_write_reg(CC2420_IOCFG0, g_current_parameters[CP_IOCFG0]);
    CC2420_write_reg(CC2420_IOCFG1, g_current_parameters[CP_IOCFG1]);

//    printk(KERN_INFO "after set:IOCFG0 is %x\n", CC2420_read_reg(CC2420_IOCFG0));
    CC2420_cmd_strobe(CC2420_SFLUSHTX);    //flush Tx fifo
    CC2420_cmd_strobe(CC2420_SFLUSHRX);
    
    return TRUE;

  }


uint8_t CC2420_oscillator_on( )
{
    uint8_t status;
    int timeout = 500; // 500us delay
    
    CC2420_cmd_strobe(CC2420_SXOSCON);
    do {
	status = CC2420_cmd_strobe(CC2420_SNOP);
	if (timeout-- <= 0)
	  return FAIL;
	udelay(1);
    } while (!(status & (1 << CC2420_XOSC16M_STABLE)));
//    printk(KERN_INFO "After Oscillator on, status is %d\n", status);
    return SUCCESS;
}

uint8_t CC2420_oscillator_off( )
{
    uint8_t status;
    status = CC2420_cmd_strobe(CC2420_SNOP);
    status = CC2420_cmd_strobe(CC2420_SXOSCOFF);
    return SUCCESS;
}

void CC2420_send_failed() 
{
    CC2420_state = IDLE_STATE;
//    printk("in send failed, CC2420_state changed to: %d", CC2420_state);
//    tx_buf_ptr->length = tx_buf_ptr->length - MSG_HEADER_SIZE - MSG_FOOTER_SIZE;
    tx_buf_ptr->length = 255;
    send_done = 1;
    wake_up_interruptible(&wq);
}

void CC2420_recv_failed(void)
{
    
    CC2420_state = IDLE_STATE;
    send_done=1;//added by YC
    wake_up_interruptible(&wq);//added by YC
//    printk("in recv failed, CC2420_state changed to: %d", CC2420_state);
    rx_packet.length = 255; // length=255 means rx_packet is not a valid packet
    rx_buf_ptr->length = 0;
    if(!non_blocking_mode) {
    	recv_done = 1;
    	wake_up_interruptible(&rq);
    }
}

void CC2420_flush_RXFIFO() 
{
    set_irq_type(CC2420_FIFOP_IRQ, IRQT_NOEDGE);
    CC2420_read_reg(CC2420_RXFIFO);
    CC2420_cmd_strobe(CC2420_SFLUSHRX);
    CC2420_cmd_strobe(CC2420_SFLUSHRX); // do it twice
    is_packet_receiving = FALSE;
    set_irq_type(CC2420_FIFOP_IRQ, IRQT_FALLING);
}
void CC2420_flush_TXFIFO() //added by YC, not used
{

    CC2420_cmd_strobe(CC2420_SFLUSHTX);
    CC2420_cmd_strobe(CC2420_SFLUSHTX); // do it twice

}

void CC2420_set_initial_timer(uint16_t jiffy) 
{
    CC2420_timer_state = TIMER_INITIAL;
    

    if (jiffy == 0)
	timer_start(2);
    else
	timer_start(jiffy);
}

void CC2420_set_transmit_timer(uint16_t jiffy) 
{
    CC2420_timer_state = TIMER_TRANSMIT;

    if (jiffy < 1)
      jiffy = 1;
    timer_start(jiffy);
}


void CC2420_set_backoff_timer(uint16_t jiffy)
{
    CC2420_timer_state = TIMER_BACKOFF;

    if (jiffy == 0)
	timer_start(2);
    else
	timer_start(jiffy);

}
 
void CC2420_set_ack_timer(uint16_t jiffy) 
{
    CC2420_timer_state = TIMER_ACK;

    timer_start(jiffy);
}

void CC2420_set_restart_timer(uint16_t jiffy) 
{
    CC2420_timer_state = TIMER_RESTART;

    if (jiffy == 0)
	timer_start(2);
    else
	timer_start(jiffy);
}

int8_t Mac_initial_backoff()
{
    int8_t random_byte;
    get_random_bytes(&random_byte, 1);
    return ((random_byte & 0xF) + 1);
}

int8_t Mac_congestion_backoff()
{
    int8_t random_byte;
    get_random_bytes(&random_byte, 1);
    return ((random_byte & 0x3F) + 1);
}

void CC2420_packet_rcvd() 
{
    int i;

    // FIXME: This function is horribly inefficient and should be rewritten to use
    // macros and functions like memcpy and memset, and also to copy into
    // a buffer specificed by the upper layers instead of into an internal
    // holding buffer -- right now there is a lot of unncessary copying that 
    // goes on.



/*
    rx_packet.length = rx_buf_ptr->length;
    rx_packet.fcfhi  = rx_buf_ptr->fcfhi;
    rx_packet.fcflo  = rx_buf_ptr->fcflo;
    rx_packet.dsn    = rx_buf_ptr->dsn;
    rx_packet.destpan = rx_buf_ptr->destpan;
    rx_packet.addr = rx_buf_ptr->addr;
    rx_packet.type = rx_buf_ptr->type;
    rx_packet.group = rx_buf_ptr->group;
    for(i = 0; i < (rx_packet_length - MSG_HEADER_SIZE - 1 - MSG_FOOTER_SIZE); i++) {	
	rx_packet.data[i] = rx_buf_ptr->data[i];
    }
    
    for(; i < TOSH_DATA_LENGTH; i++)   
 	rx_packet.data[i] = '\0';

   i = TOSH_DATA_LENGTH;
    rx_packet.data[i++] = rx_buf_ptr->strength;
    rx_packet.data[i++] = rx_buf_ptr->lqi;
    rx_packet.data[i++] = rx_buf_ptr->crc;
    rx_packet.data[i++] = rx_buf_ptr->ack;
    rx_packet.data[i++] = (rx_buf_ptr->time>>8) & 0xFF;
    rx_packet.data[i++] = rx_buf_ptr->time & 0xFF;
    

    rx_buf_ptr->length = 0;

    for (i = 0; i < TOSH_DATA_LENGTH; i++)
   	rx_buf_ptr->data[i] = '\0';
 */   

   //YC: below three statements replace the copying/set zero operations above
    memcpy(&rx_packet, rx_buf_ptr,rx_buf_ptr->length+MSG_HEADER_SIZE  +1 + TOSMSG_FOOTER_SIZE);
    rx_buf_ptr->length = 0;
    memset(rx_buf_ptr->data,'\0',TOSH_DATA_LENGTH);



    is_packet_receiving = FALSE;
 //   CC2420_state = IDLE_STATE; //commemted out by YC, this will break the CC2420 tx loop, and hang writing operation
 //   wake_up_interruptible(&wq);//added by YC, if comment out the state change, no need for this
 //   printk("in packet rcvd, CC2420_state changed to: %d\n", CC2420_state);

    // FIXME: not sure why there are two receive queues -- really only needs
    // to be one, and it's just used differently at the higher layer
    if(!non_blocking_mode){
        
    	recv_done = 1;
    	wake_up_interruptible(&rq);
    } else {
	ok_to_read = TRUE;
	wake_up_interruptible(&inq);
    }

    return;
}

void CC2420_packet_sent() 
{
    CC2420_state = IDLE_STATE;
//    printk("in packet sent, CC2420_state changed to: %d\n", CC2420_state);
    tx_buf_ptr->length = tx_buf_ptr->length - MSG_HEADER_SIZE - MSG_FOOTER_SIZE;
    // send is done, end waiting
    send_done = 1;
    wake_up_interruptible(&wq);
    return;
}


void CC2420_start_send() 
{
//YC: actually no need to flushtx, it is flushed when write to txfifo
//keep this here to ensure the unusal underflow error
    if(!CC2420_cmd_strobe(CC2420_SFLUSHTX)){
	CC2420_send_failed();
	return;
    }
    //write txbuf data to TXFIFO
    // length does not include the length itself, so add that in here
    if(!CC2420_write_TXFIFO(tx_buf_ptr->length+1, (uint8_t*)tx_buf_ptr)){
	CC2420_send_failed();
	return;
    }
}

/*
 * CCA detection. 
 * If failed, backoff
 */
void CC2420_try_to_send() 
{
    uint8_t current_state = CC2420_state;

//    printk("into try to send, state=%d\n",current_state);

    if(current_state == PRE_TX_STATE){
        //Comment out by YC, no relation between FIFOP/FIFO with TXFIFO
        //if FIFO overflow occurs, or if data length is invalid,
        // flush RXFIFO to get back to a normal state
	//if(!TEST_FIFO_PIN() && !TEST_FIFOP_PIN()) {
        //    CC2420_flush_RXFIFO();
	//    printk("flushed \n");
	//}
	if(TEST_CCA_PIN()) {
	    CC2420_state = TX_STATE;
//	    printk("in try to send, CC2420_state changed to: %d\n", CC2420_state);
//            printk(KERN_INFO "CCA is good\n");
	    CC2420_send_packet();
	}	   
        else {
//            printk(KERN_INFO "CCA is bad\n");
	    if(count_retry-- <= 0) {
		//CC2420_flush_TXFIFO();
		count_retry = MAX_SEND_TRIES;
		if(count_send_restart-- > 0)
	            CC2420_set_restart_timer(Mac_congestion_backoff() * CC2420_SYMBOL_UNIT);
		else
		    CC2420_send_failed();
		return;
	    }
	    CC2420_set_backoff_timer(Mac_congestion_backoff() * CC2420_SYMBOL_UNIT);
	}
    }
	
     else if(current_state == IDLE_STATE){ //added by YC, flush TXFIFO when idle
//	printk("before flush------ \n");
	CC2420_flush_TXFIFO();
//	printk("after flush \n");
	}
    return;
}

/*
 * Try to send a packet after CCA is good.  If unsuccessful, backoff again
 */
void CC2420_send_packet() 
{
   uint8_t status;

   CC2420_cmd_strobe(CC2420_STXONCCA);
   status = CC2420_cmd_strobe(CC2420_SNOP);
   if ((status >> CC2420_TX_ACTIVE) & 0x01) {
      CC2420_set_transmit_timer(CC2420_TMIT_DELAY);
   }
   else {
      // try again to send the packet
//      printk("set backoff status \n");
      CC2420_state = PRE_TX_STATE;
//      printk("in send packet, CC2420_state changed to: %d\n", CC2420_state);
      CC2420_set_backoff_timer(Mac_congestion_backoff() * CC2420_SYMBOL_UNIT);
   }
}

int CC2420_send(TOS_MsgPtr pMsg) 
{

    uint8_t current_state; 
    current_state = CC2420_state;
//    printk(KERN_INFO "send: current state: %d\n", current_state);

    if (current_state == IDLE_STATE) {
	// put default FCF values in to get address checking to pass
#ifndef PSI
      // not sure why the code is filling this in.  Seems like the
      // higher layer(s) should be responsible for this
	pMsg->fcflo = CC2420_DEF_FCF_LO;
	if (is_ack_enable)
	  pMsg->fcfhi = CC2420_DEF_FCF_HI_ACK;
	else
	  pMsg->fcfhi = CC2420_DEF_FCF_HI;
	// destination PAN is broadcast
	pMsg->destpan = TOS_BCAST_ADDR;
	// adjust the destination address to be in the right byte order
//	printk(KERN_INFO "DestAddr is %d\n", (int)(pMsg->addr));
	pMsg->addr = toLSB16(pMsg->addr);
//	printk(KERN_INFO "DestAddr is %d\n", (int)(pMsg->addr));
#endif
#ifndef PSI
	// keep the DSN increasing for ACK recognition
	pMsg->dsn = ++current_DSN;
#endif
	// reset the time field
	pMsg->time = 0;
	// FCS bytes generated by CC2420	
	// adjust the data length to now include the full packet, sans length
	pMsg->length = pMsg->length+MSG_HEADER_SIZE+MSG_FOOTER_SIZE+1;//YC: should include length byte here
	if (pMsg->length >= MSG_DATA_SIZE)
	  pMsg->length = MSG_DATA_SIZE; //length should include the whole message packet

	pMsg->length--; //length doesn't include length field itself, so one less here 
	tx_buf_ptr = pMsg;
	count_retry = MAX_SEND_TRIES;
	count_send_restart = MAX_RESTART_TRIES;
	CC2420_state = PRE_TX_STATE;
//	printk("in send, CC2420_state changed to: %d\n", CC2420_state);
	CC2420_set_initial_timer(Mac_initial_backoff() * CC2420_SYMBOL_UNIT);

#ifndef PSI
	
		
	if(wait_event_interruptible(wq, send_done == 1)) {
	  CC2420_state = IDLE_STATE;
//	  printk("in send wait event failed, CC2420_state changed to: %d\n", CC2420_state);
	  send_done = 1;
	  wake_up_interruptible(&wq);//added by YC, wait up event set before
//	  printk(KERN_INFO "wait failed after\n");
	  return FAIL;
	}
	send_done = 0;
#endif
        return SUCCESS;
    }
    printk(KERN_INFO "System not idle\n");
    return FAIL;

}

int CC2420_check_send_done(void)
{
  if (send_done) {
    send_done = 0;
    return 1;
  }

  if ((CC2420_state < TX_STATE)||
      (CC2420_state >= POST_TX_ACK_STATE))
    return -1;

  return 0;
}

int CC2420_read()
{
#ifndef PSI
    // FIXME: the use of RX_STATE is broken -- not sure why it's here at all
    // basically the system should receive packets all the time, and then
    // they just get lost of nobody is trying to read them
//    CC2420_state = RX_STATE; //comment out by YC, this is the main reason casuing write failed after read.
//    printk("in read, CC2420_state changed to: %d\n", CC2420_state);
    
     if(!non_blocking_mode) {
//      printk("into read waiting\n");
      if(wait_event_interruptible(rq, recv_done == 1)) {
//	CC2420_state = IDLE_STATE;
//	printk("in read wait failed, CC2420_state changed to: %d\n", CC2420_state);
	
      }
//      printk("out read waiting\n");
      recv_done = 0;
    } else
#endif
     {
	if (ok_to_read){
	  ok_to_read = 0;
	  return SUCCESS;
	} else {
	  return FAIL;
	}
    }
    return SUCCESS;
}

// to check if data in rxfifo 
int CC2420_read_poll()
{
    if(ok_to_read)
	return TRUE;
    else
	return FALSE;
}

/*
 * Tasklet function
 * Since FIFOP interrupt happened, read buffer from RXFIFO
 */

void CC2420_read_packet(unsigned long unused) 
{
   uint8_t len = MSG_DATA_SIZE;
   uint8_t _is_packet_receiving;

//	printk("read read read\n");
  
   if(!TEST_FIFO_PIN() && !TEST_FIFOP_PIN())
   {
        CC2420_flush_RXFIFO();
	CC2420_recv_failed();
        return;
   }
    _is_packet_receiving = is_packet_receiving;

    if (_is_packet_receiving) {
        tasklet_schedule(&read_packet_tasklet);
    } else {
        is_packet_receiving = TRUE;
    }

    if (!_is_packet_receiving) {
//	printk(KERN_INFO "packet is being read\n");
      if (!CC2420_read_RXFIFO (&len, (uint8_t*)rx_buf_ptr)) {
	  is_packet_receiving = FALSE;
//	  printk(KERN_INFO "read_RXFIFO failed\n");
    	  CC2420_flush_RXFIFO();
	  CC2420_recv_failed();
         // tasklet_schedule(&read_packet_tasklet);
	  return;
      }
    }

    CC2420_flush_RXFIFO();
    rx_packet_length = len;
    CC2420_RXFIFO_done(len);
    
}

/*
 * After the buffer is received from the RXFIFO,
 * process it
 */
uint8_t CC2420_RXFIFO_done(uint8_t length) 
{
     uint8_t current_state = CC2420_state;
     uint8_t * data = (uint8_t *)rx_buf_ptr;

     //printk(KERN_INFO "rxfifo_done: length is %d\n", length);

    // if a FIFO overflow occurs or if the data length is invalid, flush
    // the RXFIFO to get back to a normal state.
    if((!TEST_FIFO_PIN() && !TEST_FIFOP_PIN())
        || (length == 0) || (length > MSG_DATA_SIZE)) {
        CC2420_flush_RXFIFO();
        is_packet_receiving = FALSE;
	CC2420_recv_failed();
//	printk(KERN_INFO "fifo overflow\n");
        return SUCCESS;
    }

    // check for an acknowledgement that passes the CRC check
    if (is_ack_enable && (current_state == POST_TX_STATE) &&
        ((rx_buf_ptr->fcfhi & 0x07) == CC2420_DEF_FCF_TYPE_ACK) &&
        (rx_buf_ptr->dsn == current_DSN) && ((data[length-1] >> 7) == 1)) {
           CC2420_state = POST_TX_ACK_STATE;
//           printk("in RXFIFO_done, CC2420_state changed to: %d\n", CC2420_state);
	   tx_buf_ptr->ack = 1;
           tx_buf_ptr->strength = data[length-2];
           tx_buf_ptr->lqi = data[length-1] & 0x7F;
           is_packet_receiving = FALSE;
           CC2420_packet_sent();
	  // printk(KERN_INFO "recv ACK!\n");
//	   printk(KERN_INFO "valid ack\n");
           return SUCCESS;
    }

#if 0
    // All packets should be passed to the higher
    // layers and then sorted out there.  We shouldn't really
    // be filtering on the fcf at this layer.

    // check for invalid packets
    // an invalid packet is a non-data packet with the wrong
    // addressing mode (FCFLO byte)
    if (((rx_buf_ptr->fcfhi & 0x07) != CC2420_DEF_FCF_TYPE_DATA) ||
         (rx_buf_ptr->fcflo != CC2420_DEF_FCF_LO)) {
	 CC2420_flush_RXFIFO();
	 is_packet_receiving = FALSE;
	 CC2420_recv_failed();
	 printk(KERN_INFO "invalid packet\n");
	 return SUCCESS;
    }
#endif

    {
      int len = rx_buf_ptr->length;
      len -= MSG_HEADER_SIZE + MSG_FOOTER_SIZE + 1;
      if (len < 0)
	len = 0;
//      printk("Length convert from %d to %d\n",rx_buf_ptr->length,len);
      rx_buf_ptr->length = len;
    }

    if (rx_buf_ptr->length > TOSH_DATA_LENGTH) {
	printk(KERN_INFO "bad length: %d > %d\n",rx_buf_ptr->length,TOSH_DATA_LENGTH);
	CC2420_flush_RXFIFO();
	is_packet_receiving = FALSE;
	CC2420_recv_failed();
	return SUCCESS;
    }

    // adjust destination to the right byte order
    rx_buf_ptr->addr = fromLSB16(rx_buf_ptr->addr);

    // if the length is shorter, we have to move the CRC bytes
    rx_buf_ptr->crc = data[length-1] >> 7;
    // put in RSSI
    rx_buf_ptr->strength = data[length-2];
    // put in LQI
    rx_buf_ptr->lqi = data[length-1] & 0x7F;

    CC2420_packet_rcvd();

    if(!TEST_FIFO_PIN() && !TEST_FIFOP_PIN()){
        CC2420_flush_RXFIFO();
        return SUCCESS;
    }

    if(!TEST_FIFOP_PIN()){
       tasklet_schedule(&read_packet_tasklet);
       return SUCCESS;
    }
    CC2420_flush_RXFIFO();
   // printk(KERN_INFO "end of RXFIFO\n");
    return SUCCESS;
}

irqreturn_t CC2420_timer_irq_handler(int irq, void *dev, struct pt_regs *regs)
{
   uint8_t current_state = CC2420_state;

//   printk(KERN_INFO "Timer IRQ %d happened \n",CC2420_timer_state);
 
   if(timer_channel_check() == SUCCESS) {
	switch (CC2420_timer_state) {

	case TIMER_INITIAL:
//	printk("timer is INITIAL\n");
	    CC2420_start_send();
//	printk("start send\n");
	    CC2420_try_to_send();            
//	printk("try to send\n");
            break;
	case TIMER_BACKOFF:
//	printk("backoff timer\n");
            CC2420_try_to_send();
            break;
	case TIMER_ACK:
            if (current_state == POST_TX_STATE) {
//		printk(KERN_INFO "No ACK received\n");
            	tx_buf_ptr->ack = 0;
            	CC2420_state = POST_TX_ACK_STATE;
//		printk("in timer irq, CC2420_state changed to: %d\n", CC2420_state);
            	CC2420_packet_sent();
            }
//		printk("ack timer\n");
            break;
	case TIMER_RESTART:
	    CC2420_start_send();
	    CC2420_try_to_send();            
	    break;
	case TIMER_TRANSMIT:
//	while(TEST_SFD_PIN()){
//	printk("wait for transmit\n");
//	udelay(2);	
//	}

#ifdef PSI
	    P1IFG |= CC2420_SFD; // trigger SW interrupt to simulate SFD

#endif
	    break;
	}
	
   }
   return IRQ_HANDLED;
}


irqreturn_t CC2420_irq_handler(int irq, void *dev, struct pt_regs *regs)
{
   int gpio_pin = IRQ_TO_GPIO(irq);
//printk(KERN_INFO "IRQ %d happened \n",CC2420_state);
   switch (gpio_pin) {
   case SG2_GPIO16_CC_SFD:
	switch (CC2420_state) {
	       case TX_STATE:
//                printk(KERN_INFO "IRQ type is SFD!\n");
		// if SFD already fell, then disable interrupt
		if(!TEST_SFD_PIN()){
		    set_irq_type(CC2420_SFD_IRQ, IRQT_NOEDGE);
//		    printk(KERN_INFO "SFD already fallen\n");
		   }
		else{
#ifdef PSI
		  set_irq_type(CC2420_SFD_IRQ, IRQT_FALLING);
#endif
		    CC2420_state = TX_WAIT;
//		    printk("in irq 1, CC2420_state changed to: %d\n", CC2420_state);
//		    printk(KERN_INFO "CC2420 state is in TX_WAIT\n");
		    }
		tx_buf_ptr->time = 0;
//              printk(KERN_INFO "SFD:FSMSTATE is %d\n", CC2420_read_reg(CC2420_FSMSTATE));
		if(CC2420_state == TX_WAIT){
//		    printk(KERN_INFO "cc2420_state: Tx_Wait\n");
		    CC2420_set_transmit_timer(CC2420_TMIT_DELAY);
		    break;
		}
	       case TX_WAIT:
		CC2420_state = POST_TX_STATE;
//		printk("in irq 2, CC2420_state changed to: %d\n", CC2420_state);
//		printk(KERN_INFO "SFD: TX_WAIT\n");
		//disable SFD interrupt
		set_irq_type(CC2420_SFD_IRQ, IRQT_NOEDGE);
		//enable SFD interrupt as rising edge interrupt
#ifdef PSI
		set_irq_type(CC2420_SFD_IRQ, IRQT_FALLING);
#else
		set_irq_type(CC2420_SFD_IRQ, IRQT_BOTHEDGE);
#endif
		//if acks are enabled and it is a unicast packet, wait for ack
		if((is_ack_enable) && (tx_buf_ptr->addr != TOS_BCAST_ADDR)) {
		    CC2420_set_ack_timer(CC2420_ACK_DELAY);
		}
		else{	
		    CC2420_packet_sent();
		}
	        break;
	        default:
		rx_buf_ptr->time = 0;
	       }
	break;
   case SG2_GPIO114_CC_FIFO:
      break;

   case SG2_GPIO0_CC_FIFOP:
//	printk(KERN_INFO "IRQ type is FIFOP\n"); 
 //      if(CC2420_state != IDLE_STATE || non_blocking_mode) {//comment out by YC, cc2420 should always read when data available
	 // if we're trying to send a message and a FIFOP interrupt occurs
	 // and acks are enabled, we need to backoff longer so that we don't
	 // interfere with the ACK
	 if (is_ack_enable && (CC2420_state == PRE_TX_STATE)) {
	   if (timer_is_running()) {
	     timer_stop();
	     timer_start((Mac_congestion_backoff( ) * CC2420_SYMBOL_UNIT) 
			 + CC2420_ACK_DELAY);
	   }
	 }

	 /** Check for RXFIFO overflow **/
	 if(!TEST_FIFO_PIN()){
	   CC2420_flush_RXFIFO();
	   // printk(KERN_INFO "RXFIFO overflow\n"); 
	   CC2420_recv_failed();
	   return SUCCESS;
	 }
	 //bottom half
	 tasklet_schedule(&read_packet_tasklet);
	 set_irq_type(CC2420_FIFOP_IRQ, IRQT_NOEDGE);
//       } else
//	 CC2420_flush_RXFIFO();
       break;
//   case SG2_GPIO116_CC_CCA:
//	break;
   default:
	printk(KERN_INFO "Unknown GPIO %d interrupt happened\n", gpio_pin);
	break; 
   }
  return IRQ_HANDLED;
}

unsigned int CC2420_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    poll_wait(filp, &inq, wait);
    if(ok_to_read) {
        mask |= POLLIN | POLLRDNORM;
//        printk(KERN_INFO "poll: data ready\n");
    }// else
//        printk(KERN_INFO "poll: data not ready\n");
   return mask;
}

/*
 * -Set CC2420 Channel
 * Valid channel values are 11 through 26
 * Freq = 2045 + 5(k-11) Mhz for k = 11, 12, ..., 26
 * chnl: requested 802.15.4 channel
 * return Status of the tune operation 
*/

uint8_t CC2420_tune_preset(uint8_t chnl)
{
    int fsctrl;
    uint8_t status;

    fsctrl = 357 + 5*(chnl-11);
    g_current_parameters[CP_FSCTRL] = (g_current_parameters[CP_FSCTRL] & 0xfc00)\
				    | (fsctrl << CC2420_FSCTRL_FREQ);
    status = CC2420_write_reg(CC2420_FSCTRL, g_current_parameters[CP_FSCTRL]);
    // if the oscillator is started, recalibrate for the new frequency
    // if the oscillator is NOT on, we should not transition to RX mode
    if (status & (1 << CC2420_XOSC16M_STABLE))
       CC2420_cmd_strobe(CC2420_SRXON);
    return SUCCESS;
}

  /*
   * TuneManual
   * Tune the radio to a given frequency. Frequencies may be set in
   * 1 MHz steps between 2400 MHz and 2483 MHz
   *
   * Desiredfreq: The desired frequency, in MHz.
   * Return Status of the tune operation
  */
uint8_t CC2420_tune_manual(uint16_t DesiredFreq) 
{
    int fsctrl;
    uint8_t status;

    fsctrl = DesiredFreq - 2048;
    g_current_parameters[CP_FSCTRL] = (g_current_parameters[CP_FSCTRL] & 0xfc00) \
				    | (fsctrl << CC2420_FSCTRL_FREQ);
    status = CC2420_write_reg(CC2420_FSCTRL, g_current_parameters[CP_FSCTRL]);
    // if the oscillator is started, recalibrate for the new frequency
    // if the oscillator is NOT on, we should not transition to RX mode
    if (status & (1 << CC2420_XOSC16M_STABLE))
      CC2420_cmd_strobe(CC2420_SRXON);
    return SUCCESS;
}
/*
 * Get the current frequency of the radio
 */
uint16_t CC2420_get_frequency( ) 
{
    return ((g_current_parameters[CP_FSCTRL] & (0x1FF << CC2420_FSCTRL_FREQ))+2048);
}

  /*
   * Get the current channel of the radio
   */
uint8_t CC2420_get_preset( ) 
{
    uint16_t _freq = (g_current_parameters[CP_FSCTRL] & (0x1FF << CC2420_FSCTRL_FREQ));
    _freq = (_freq - 357)/5;
    _freq = _freq + 11;
    return _freq;
}

/*************************************************************************
* TxMode
* Shift the CC2420 Radio into transmit mode.
* return SUCCESS if the radio was successfully switched to TX mode.
*************************************************************************/
uint8_t CC2420_tx_mode( ) 
{
    CC2420_cmd_strobe(CC2420_STXON);
    return SUCCESS;
}

/*************************************************************************
* TxModeOnCCA
* Shift the CC2420 Radio into transmit mode when the next clear channel
* is detected.
*
* return SUCCESS if the transmit request has been accepted
*************************************************************************/
uint8_t CC2420_tx_onCCA( ) 
{
   CC2420_cmd_strobe(CC2420_STXONCCA);
   return SUCCESS;
}

/*************************************************************************
 * RxMode
 * Shift the CC2420 Radio into receive mode
*************************************************************************/
uint8_t CC2420_rx_mode( ) 
{
    CC2420_cmd_strobe(CC2420_SRXON);
    return SUCCESS;
}

/*************************************************************************
* SetRFPower
* power = 31 => full power    (0dbm)
*          3 => lowest power  (-25dbm)
* return SUCCESS if the radio power was successfully set
*************************************************************************/
uint8_t CC2420_set_rf_power(uint8_t power) 
{
   g_current_parameters[CP_TXCTRL] = (g_current_parameters[CP_TXCTRL] &
				    (~CC2420_TXCTRL_PAPWR_MASK)) |
				    (power << CC2420_TXCTRL_PAPWR);
   CC2420_write_reg(CC2420_TXCTRL, g_current_parameters[CP_TXCTRL]);
   return SUCCESS;
}
  
/*************************************************************************
 * GetRFPower
 * return power seeting
*************************************************************************/
uint8_t CC2420_get_rf_power( ) 
{
   return (g_current_parameters[CP_TXCTRL] & CC2420_TXCTRL_PAPWR_MASK); //rfpower;
}


uint8_t CC2420_enable_AutoAck( ) 
{
    g_current_parameters[CP_MDMCTRL0] |= (1 << CC2420_MDMCTRL0_AUTOACK);
    return CC2420_write_reg(CC2420_MDMCTRL0,g_current_parameters[CP_MDMCTRL0]);
}

uint8_t CC2420_disable_AutoAck( ) 
{
    g_current_parameters[CP_MDMCTRL0] &= ~(1 << CC2420_MDMCTRL0_AUTOACK);
    return CC2420_write_reg(CC2420_MDMCTRL0, g_current_parameters[CP_MDMCTRL0]);
}

uint8_t CC2420_enable_AddrDecode( ) 
{
    g_current_parameters[CP_MDMCTRL0] |= (1 << CC2420_MDMCTRL0_ADRDECODE);
    return CC2420_write_reg(CC2420_MDMCTRL0, g_current_parameters[CP_MDMCTRL0]);
}

uint8_t CC2420_disable_AddrDecode( ) 
{
    g_current_parameters[CP_MDMCTRL0] &= ~(1 << CC2420_MDMCTRL0_ADRDECODE);
    return CC2420_write_reg(CC2420_MDMCTRL0, g_current_parameters[CP_MDMCTRL0]);
}

/** Enable link layer hardware acknowledgements **/   
void CC2420_enable_ack() 
{
    is_ack_enable = TRUE;
    CC2420_enable_AddrDecode();
    CC2420_enable_AutoAck();
}

/** Disable link layer hardware acknowledgements **/
void CC2420_disable_ack() 
{
    is_ack_enable = FALSE;
    CC2420_disable_AddrDecode();
    CC2420_disable_AutoAck();
}

uint8_t CC2420_set_short_addr(uint16_t addr) 
{
    addr = toLSB16(addr);
    return CC2420_write_RAM(CC2420_RAM_SHORTADR, 2, (uint8_t*)&addr);
}

/* 
 * CC2420_init( ) is called when driver is installed.
 * It will turn CC2420 oscillator on, setup SPI port.
 */
uint8_t CC2420_init(struct tosmac_device *dev)
{
    int ret;
    down(&CC2420_sem);

    CC2420_state = DISABLED_STATE;
//    printk("in init 1, CC2420_state changed to: %d\n", CC2420_state);
    TOS_LOCAL_ADDRESS = local_addr;
    HPLCC2420_init();
    CC2420_set_parameters();
    CC2420_state = WARMUP_STATE;
//    printk("in init 2, CC2420_state changed to: %d\n", CC2420_state);
    HPLCC2420_start();
    if (CC2420_oscillator_on() != SUCCESS)
      goto fail;
//    printk(KERN_INFO "CC2420 Oscillator is on.\n"); 
    CC2420_set_regs();
    CC2420_set_short_addr(TOS_LOCAL_ADDRESS);
    CC2420_tune_manual(((g_current_parameters[CP_FSCTRL] <<
			     CC2420_FSCTRL_FREQ) & 0x1FF) + 2048);
    
    CC2420_rx_mode();
    CC2420_state = IDLE_STATE;
//    printk("in init 3, CC2420_state changed to: %d\n", CC2420_state);
    non_blocking_mode = FALSE;
    current_DSN = 0;
    is_ack_enable = FALSE;
    is_packet_receiving = FALSE;
    rx_buf_ptr = &rx_buf;
//$$    rx_buf_ptr->length = 0;      
    count_retry = 0;
    count_send_restart = 0;
    rx_buf_ptr->length = 0;
#ifndef PSI
//    CC2420_enable_ack();
#endif
//    printk(KERN_INFO "IOCFG0 is %x\n", CC2420_read_reg(CC2420_IOCFG0));
//    printk(KERN_INFO "cc2420_start_finished\n");
	
        //enable interrupts
        ret = request_irq(CC2420_FIFOP_IRQ, CC2420_irq_handler,
		      SA_INTERRUPT, "cc2420", dev);
	if(ret) {
//	    printk(KERN_INFO "Failed to request FIFOP interrupt.\n");
	    goto fail;
	}
	set_irq_type(CC2420_FIFOP_IRQ, IRQT_FALLING);

	ret = request_irq(CC2420_SFD_IRQ, CC2420_irq_handler,
		      SA_INTERRUPT, "cc2420", dev);
	if(ret) {
//	    printk(KERN_INFO "Failed to request SFD interrupt.\n");
	    goto fail;
	}
#ifdef PSI
	set_irq_type(CC2420_SFD_IRQ, IRQT_FALLING);
#else
	set_irq_type(CC2420_SFD_IRQ, IRQT_BOTHEDGE);
#endif

	ret = request_irq(CC2420_OST_IRQ, CC2420_timer_irq_handler,
		      SA_INTERRUPT, "cc2420", dev);
	if(ret) {
//	    printk(KERN_INFO "Failed to request OS Timer interrupt.\n");
	    goto fail;
	}
	// init timer
	timer_init();
        up(&CC2420_sem);
        return SUCCESS;
fail:
    up(&CC2420_sem);
    return FAIL;
}
/*
 * CC2420_start( ) is called when CC2420 device is opened.
 * It will request all interrupts and do init related with CC2420
 */
uint8_t CC2420_start(uint8_t is_non_blocking) //(struct tosmac_device *dev)
{ 
    if(is_non_blocking) {
	non_blocking_mode = TRUE;
	ok_to_read = FALSE;
    }
    else
	non_blocking_mode = FALSE;

    return SUCCESS;
}

void CC2420_stop(struct tosmac_device * dev)
{
//    printk(KERN_INFO "Close device\n");
}

uint8_t CC2420_exit(struct tosmac_device *dev)
{
    free_irq(CC2420_FIFOP_IRQ, dev);
    free_irq(CC2420_SFD_IRQ, dev);
    free_irq(CC2420_OST_IRQ, dev);
    CC2420_state = DISABLED_STATE;
//    printk("in exit, CC2420_state changed to: %d\n", CC2420_state);
    CC2420_oscillator_off();
    HPLCC2420_exit();

    return SUCCESS;
}

