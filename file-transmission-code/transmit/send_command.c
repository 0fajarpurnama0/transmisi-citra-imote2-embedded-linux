//************************************************************
//
// blocking.c
//
// Gefan Zhang
//
//*************************************************************
// Modified by Fajar Purnama

/* Only Difference from original send.c is an addition of arguement (argc, argv)
   we could send any message we want without changing its source code for example
   sending hello message just type ./send_command hello */

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

int main(int argc, const char *argv[]) {

  if(argv[1]==NULL){
   printf("Usage: ./send_command [message], example ./send_command reboot, ./send_command hello\n");
  return 1;
  }

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
  msg_init(&send_pkt);
  send_pkt.addr = 99;
  memcpy(send_pkt.data, argv[1], TOSH_DATA_LENGTH);
  send_pkt.length = TOSH_DATA_LENGTH;

  printf("User write to driver\n"); 
  write(tosmac_dev, (TOS_Msg*)&send_pkt, sizeof(TOS_Msg));
  // close device
  close (tosmac_dev);

  return 0;
}
