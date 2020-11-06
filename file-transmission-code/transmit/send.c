//************************************************************
//
// blocking.c
//
// Gefan Zhang
//
//*************************************************************

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <tosmac.h>

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
   pMsg->strength = 0;
   pMsg->lqi = 0;
   pMsg->crc = 0;
   pMsg->ack = 0;
   pMsg->time = 0;
}
//--------------------- main -------------------------------

int main(int argc, char* argv[])
{
  int tosmac_dev,i;
  TOS_Msg recv_pkt;
  TOS_Msg send_pkt;

  // open as blocking mode
  tosmac_dev = open(TOSMAC_DEVICE, O_RDWR);
  if (tosmac_dev < 0)
  {
    fprintf(stderr, "Open error: %s\n", TOSMAC_DEVICE);
    return 1;
  }
  msg_init(&send_pkt);
  send_pkt.addr = 90;
  memcpy(send_pkt.data, "DATA for test", 14);
  send_pkt.data[15]=1;
  //i = 912615;
  //send_pkt.data[1] = 15;

  printf("User write to driver\n"); 
  printf("dev %d\n", sizeof(tosmac_dev));
  write(tosmac_dev, (TOS_Msg*)&send_pkt, sizeof(send_pkt));
  printf("dev %d\n", sizeof(tosmac_dev));
  // close device
  close (tosmac_dev);

  return 0;
}
