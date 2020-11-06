//************************************************************
//
// RfmToLed.c
// Receive tosmsg from CntToLedsAndRfm
// Use the last 3 bits of first payload byte to set leds.
//
// Author:Gefan Zhang
//
//*************************************************************

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../../include/linux/tosmac.h"
#include "../platx/led.h"

#define LED_TEST
#define RAW_TEST
//#define ACCEL_TEST

void msg_init(TOS_Msg* pMsg)
{
   pMsg->length = 0;
   pMsg->fcfhi = 0;
   pMsg->fcflo = 0;
   pMsg->dsn = 0;
   pMsg->destpan = 0;
   pMsg->addr = 0;
   pMsg->type = 0;
   pMsg->group = 0;
   memset(pMsg->data, 0, TOSH_DATA_LENGTH);
#if 0
   pMsg->strength = 0;
   pMsg->crc = 0;
   pMsg->lqi = 0;
   pMsg->ack = 0;
   pMsg->time = 0;
#endif
}

int main(int argc, char* argv[])
{
    int tosmac_dev;
    int leds;
    TOS_Msg recv_pkt;
    TOS_Msg send_pkt;
    char *data = recv_pkt.data;
    int i;
    // open as blocking mode
    tosmac_dev = open(TOSMAC_DEVICE, O_RDWR);
    if (tosmac_dev < 0)
    {
      fprintf(stderr, "Open %s error.\n", TOSMAC_DEVICE);
      return 1;
    }
    leds = open(LED_DEV, O_RDWR);
    if (leds < 0)
    {
      fprintf(stderr, "Open %s error.\n", LED_DEV);
      return 1;
    }

    for ( ; ; ) {
	read(tosmac_dev, &recv_pkt, sizeof(TOS_Msg));

#ifdef RAW_TEST
        printf("Length:%02d ", recv_pkt.length);
	printf("Fcf: 0x%02x%02x ",recv_pkt.fcfhi,recv_pkt.fcflo);
        printf("Seq#:%02x ", recv_pkt.dsn);
        printf("DestPAN:%04x ", recv_pkt.destpan);
        printf("DestAddr:%04x ", recv_pkt.addr);
        printf("TypeID:%02x ", (unsigned char) recv_pkt.type);
        printf("GroupID:%02x\n", (unsigned char) recv_pkt.group);

        printf("Data: ");
        for(i = 0; i < recv_pkt.length; i++)
	  printf("%02x ", (unsigned char) data[i]);
        printf("\n");
#endif

#ifdef LED_TEST
        if(data[0] & 0x01)
           ioctl (leds, CLED_IOSET, RED);
        else
           ioctl (leds, CLED_IOSET, RED_OFF);

        if(data[0] & 0x02)
           ioctl (leds, CLED_IOSET, GREEN);
        else
            ioctl (leds, CLED_IOSET, GREEN_OFF);

        if(data[0] & 0x04)
            ioctl (leds, CLED_IOSET, BLUE);
        else
            ioctl (leds, CLED_IOSET, BLUE_OFF);
#endif

#ifdef ACCEL_TEST
 {
   int x,y,z;
   x = data[5]*256 + data[4];
   y = data[7]*256 + data[6];
   z = data[9]*256 + data[8];
   printf("%10d %10d %10d\n",x,y,z);
 }

#endif
#if 0
	printf("Strength:%d ", recv_pkt.strength);
        printf("LQI:%d ", recv_pkt.lqi);
        printf("CRC:%d\n", recv_pkt.crc);
#endif
   }
   // close device
   close (tosmac_dev);
   close (leds);
   return 0;
}
