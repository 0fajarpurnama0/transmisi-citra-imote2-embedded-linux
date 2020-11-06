//************************************************************
//
// blocking.c
//
// Gefan Zhang
//
//*************************************************************
// Modified by : Fajar Purnama 
// size of TOSH_DATA_LENGTH only usually 28 bytes

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

int main(int argc, const char *argv[]){
  if(argv[1]==NULL){
   printf("Usage: ./send_small_file [file]\n");
  return 1;
  }
  int tosmac_dev, file, file_size;
  TOS_Msg recv_pkt;
  TOS_Msg send_pkt;

  // open as blocking mode
  tosmac_dev = open(TOSMAC_DEVICE, O_RDWR);
  file = open(argv[1], O_RDWR);
  file_size = lseek(file,0,SEEK_END);
  lseek(file,0,SEEK_SET);

  if (tosmac_dev < 0)
  {
    fprintf(stderr, "Open error: %s\n", TOSMAC_DEVICE);
    return 1;
  }
  msg_init(&send_pkt);
  send_pkt.addr = 99;

  read(file,send_pkt.data,TOSH_DATA_LENGTH);
  send_pkt.length = TOSH_DATA_LENGTH;
  printf("User writing %d bytes to driver\n", send_pkt.length); 
  write(tosmac_dev, (TOS_Msg*)&send_pkt, sizeof(TOS_Msg));
  printf("%d bytes written", send_pkt.length);
  // close device
  close (tosmac_dev);
return 0;
}
