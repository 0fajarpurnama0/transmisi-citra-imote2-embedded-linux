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
  printf("Usage: ./recv_file [file], example: ./recv_file image.ppm");
  return 1;
 }
 
 // Declaration
 int tosmac_dev, file, i;
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
 file = open(argv[1], O_RDWR);
 // Check Error
 if(file<0){
  creat(argv[1], O_RDWR); // create empty file, argv[1] if no file exist: user input (./recv_file argv[1]) 
  file = open(argv[1], O_RDWR); // Open created file
 }

 // receving file
 printf("User read from driver:\n");
 
 // receive 28 bytes of file for infinity    
 while(1){
  i = lseek(file,0,SEEK_END); // Seek to end of file to continue receive (this feature allows continueable download)
  send_pkt.data[1] = (i/TOSH_DATA_LENGTH)-3000; // Since the max value of data type is 3000 we start from -3000, so we could put a number up to 6000, This feature request tells the transmitter how much bytes already received so the transmitter will sinchronize.
  write(tosmac_dev, (TOS_Msg*)&send_pkt, sizeof(TOS_Msg)); // sending i in send_pkt.data[1]
  alarm(2); // 2  seconds timeout 
  read(tosmac_dev, (TOS_Msg*)&recv_pkt, sizeof(TOS_Msg)); // Read receive file from Tosmac Device, Pause if device == NULL, !=sizeof(TOS_Msg)
  
  // Stop code, break infinite while loop if this code is received, send application should send this string to tell if transmission finish 
  if(strncmp(recv_pkt.data,"0S0T1O1P0",9)==0){ 
   break;
  }
  // Use group as identifier, so it will not process packet if it is not on this group
  if(recv_pkt.group!=7){
   continue; // it will ignore next code and restart while loop
  }
  
  // Verbose, shows accumulative received number of bytes
  printf("Receiving %d bytes of data\n", recv_pkt.length);
  i += recv_pkt.length; // Equal to i = i + recv_pkt.length
  printf("Received %d bytes\n", i);

  // Writing received 28 bytes to file that had just been created
  write(file, recv_pkt.data, recv_pkt.length); // write will automatically go to last byte order of file
 }
  
  printf("FINISH!!");
  // closing device and file
  close (tosmac_dev);
  close(file);

  return 0;
}
