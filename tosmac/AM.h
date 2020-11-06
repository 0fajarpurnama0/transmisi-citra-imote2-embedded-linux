/*
 * driver/tosmac/AM.h
 * Author: Gefan Zhang
 */

#ifndef __AM_H
#define __AM_H

//extern __u8 flag;

#define MAX_TOSH_DATA_LENGTH        116

#define TOS_BCAST_ADDR          0xffff

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
        __s8 data[MAX_TOSH_DATA_LENGTH + 6];
        __u8 strength;
	__u8 lqi;
        __u8 crc;
        __u8 ack;
        __u16 time;
};

typedef struct __TOS_Msg  TOS_Msg;
typedef TOS_Msg*   TOS_MsgPtr;

enum {
        // size of the header not including the length byte
        MSG_HEADER_SIZE = 9, //offsetof(struct TOS_Msg, data) - 1,
        // size of the footer
        MSG_FOOTER_SIZE = 2,
	//size of TOSMSG FOOTER
	TOSMSG_FOOTER_SIZE = 6,
        // position of the length byte
        LENGTH_BYTE_NUMBER = 1,
};
//size of payload data
extern int TOSH_DATA_LENGTH;
// size of the full packet
extern int MSG_DATA_SIZE;
// size of header(including length byte) and payload data
extern int MSG_HEADER_DATA_SIZE;
// size of the whole TOSMSG
extern int TOSMSG_SIZE;
// size of the data length
//DATA_LENGTH = TOSH_DATA_LENGTH,

extern __u16 TOS_LOCAL_ADDRESS;
extern __u16 local_addr;
extern uint8_t tx_length;
extern TOS_MsgPtr tx_buf_ptr;  // pointer to transmit buffer
extern TOS_MsgPtr rx_buf_ptr;  // pointer to receive buffer
extern TOS_Msg rx_buf;         // save received messages
extern TOS_Msg rx_packet;
extern uint8_t rx_packet_length;

#endif
