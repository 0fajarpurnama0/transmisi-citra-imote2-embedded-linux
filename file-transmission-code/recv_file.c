//************************************************************
//
// blocking.c
//
// Gefan Zhang
//
//*************************************************************
// Modified by : Fajar Purnama

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
   memset(pMsg->data, 0, TOSH_DATA_LENGTH); //28 byte usually
   pMsg->strength = 0;
   pMsg->crc = 0;
   pMsg->lqi = 0;
   pMsg->ack = 0;
   pMsg->time = 0;
}

//--------------------- main -------------------------------

int main(int argc, const char *argv[]) {
 
 // Check Error
 if(argv[1]==NULL){
  printf("Usage: ./recv_file [file], example: ./recv_file image.ppm\n");
 return 1;
 }
 
 // Declaration
 int tosmac_dev, file, i, j;
 TOS_Msg recv_pkt;
 TOS_Msg send_pkt;
 
 // open as blocking mode
 tosmac_dev = open(TOSMAC_DEVICE, O_RDWR); // TOSMAC_DEVICE = /dev/tosmac, O_RDWR = Open as Read & Write
 // Check Error
 if (tosmac_dev < 0)
 {
   fprintf(stderr, "Open error: %s\n", TOSMAC_DEVICE);
   return 1;
 }
  
 // file
 // creat(argv[1], O_RDWR); // create empty file, argv[1]: user input (./recv_file argv[1]) 
 file = open(argv[1], O_RDWR); // Open created file

 // Check whether file exist, if not create a new file
 if(file<0){
  creat(argv[1], O_RDWR); // create empty file, argv[1]: user input (./recv_file argv[1])
  file = open(argv[1], O_RDWR);
 }

 // receving file
 printf("User read from driver:\n");
 // receive 28 bytes of file for infinity    
 while(1){ 
  alarm(1);
  i = lseek(file,0,SEEK_CUR); // if previous file exist and transmission stops in the middle, it will seek end of file, can be use to continue transmission, RUN THIS APP AGAIN!!
  j = i/28;
  printf("Received %d bytes\n", i);
  send_pkt.data[1] = j;
  write(tosmac_dev, (TOS_Msg*)&send_pkt, sizeof(TOS_Msg)); // sending empty message as ready to reaceive packet (ACK)
  read(tosmac_dev, &recv_pkt, sizeof(TOS_Msg)); // Read receive file from Tosmac Device, Pause if device == NULL, !=sizeof(TOS_Msg)

  // Stop code, break infinite while loop if this code is received, send application should send this string to tell if transmission finish
  if(strncmp(recv_pkt.data,"0S0T1O1P0",9)==0){
   break;
  }
  
  // Verbose, shows accumulative received number of bytes
  printf("Receiving %d bytes of data\n", recv_pkt.length);
  //i += recv_pkt.length; // Equal to i = i + recv_pkt.length
  // Writing received 28 bytes to file
  write(file, recv_pkt.data, recv_pkt.length); // write will automatically go to last byte order of file
  }
  
 printf("FINISH!!");
 // closing device and file
 close (tosmac_dev);
 close(file);

return 0;
}
