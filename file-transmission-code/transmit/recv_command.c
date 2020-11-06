//************************************************************
//
// blocking.c
//
// Gefan Zhang
//
//*************************************************************
// Modified by Fajar Purnama

/* The only difference is system() function is added from stdlib.h so the message received will be proccess as a command
   with this code still limited to 28 bytes and cannot proccess spaces example "cd ..". Example of message receive is
   recv_pkt.data = "reboot", it will be system("reboot"), and it will reboot the system */

#include <stdio.h>
#include <stdlib.h>
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
  read(tosmac_dev, &recv_pkt, sizeof(TOS_Msg));// != sizeof(TOS_Msg);
  printf("data is %s\n", recv_pkt.data);
  system(recv_pkt.data);// Process recv_pkt.data as a command in terminal
  // close device
  close (tosmac_dev);

  return 0;
}
