//************************************************************
//
// CntToLedsAndRfm.c
// Send tosmsg out every 1 second
// Last 3 bits in first byte of payload are used to set leds state
//
// Author: Gefan Zhang
//
//*************************************************************

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../../include/linux/tosmac.h"
#include "../platx/led.h"

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
   pMsg->lqi = 0;
   pMsg->crc = 0;
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
  __u8 i = 1;
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

  msg_init(&send_pkt);
  send_pkt.addr = 0xFFFF;
  send_pkt.type = 0x04;
  send_pkt.group = 0x7d;
  send_pkt.length = 4;
  send_pkt.data[1] = 0x0;
  send_pkt.data[2] = 0x85;
  send_pkt.data[3]  = 0x0;

  for(i = 1; ;i++) {
      send_pkt.data[0] = i;
      if(i & 0x01)
        ioctl (leds, CLED_IOSET, RED);
      else
        ioctl (leds, CLED_IOSET, RED_OFF);

      if(i & 0x02)
        ioctl (leds, CLED_IOSET, GREEN);
      else
        ioctl (leds, CLED_IOSET, GREEN_OFF);

      if(i & 0x04)
        ioctl (leds, CLED_IOSET, BLUE);
      else
        ioctl (leds, CLED_IOSET, BLUE_OFF);

      write(tosmac_dev, (TOS_Msg*)&send_pkt, 4);
      sleep(1);
  }


  // close device
  close (tosmac_dev);
  close (leds);
  return 0;
}
