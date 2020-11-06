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
   pMsg->crc = 0;
   pMsg->lqi = 0;
   pMsg->ack = 0;
   pMsg->time = 0;
}
//--------------------- main -------------------------------

int main(int argc, char* argv[])
{
  int tosmac_dev;
  TOS_Msg recv_pkt;
  TOS_Msg send_pkt;

  // open as blocking mode
  tosmac_dev = open(TOSMAC_DEVICE, O_RDWR);
  if (tosmac_dev < 0)
  {
    fprintf(stderr, "Open error: %s\n", TOSMAC_DEVICE);
    return 1;
  }

  printf("User read from driver:\n");
  msg_init(&recv_pkt);
  read(tosmac_dev, (TOS_Msg*)&recv_pkt, sizeof(recv_pkt));// != sizeof(TOS_Msg);
 
  printf("size of TOS_Msg %d\n", sizeof(TOS_Msg)); 
  printf("size of packet %d\n", sizeof(recv_pkt));
  printf("length is %d\n", recv_pkt.length);
  printf("dsn %d\n", recv_pkt.dsn);
  printf("fcfhi %d\n", recv_pkt.fcfhi);
  printf("fcflo %d\n", recv_pkt.fcflo);
  printf("crc %d\n", recv_pkt.crc);
  printf("lqi %d\n", recv_pkt.lqi);
  printf("ack %d\n", recv_pkt.ack);
  printf("time %d\n", recv_pkt.time);
  printf("strength %d\n", recv_pkt.strength);
  printf("group %d\n", recv_pkt.group);
  printf("type %d\n", recv_pkt.type);
  printf("addr %d\n", recv_pkt.addr);
  printf("destpan %d\n", recv_pkt.destpan);
  printf("data is %s\n", recv_pkt.data);
  printf("code %d\n", recv_pkt.data[15]);
  // close device
  close (tosmac_dev);

  return 0;
}
