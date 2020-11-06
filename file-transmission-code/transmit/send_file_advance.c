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
   memset(pMsg->data, 0, TOSH_DATA_LENGTH); // 28 bytes usually
   pMsg->strength = 0;
   pMsg->lqi = 0;
   pMsg->crc = 0;
   pMsg->ack = 0;
   pMsg->time = 0;
}
//--------------------- main -------------------------------

int main(int argc, const char *argv[]){
 
 // Check Error
 if(argv[1]==NULL){
  printf("Usage: ./send_file [file], example: ./send_file image.ppm");
  return 1;
 }

 //sleep(10); //pause for 10 sec (give time for receiver to prepare) cross this out if not needed
 
 // Declaration
 int tosmac_dev, file, file_size, h, i, j, k;
 //char *packet;
 TOS_Msg recv_pkt;
 TOS_Msg send_pkt;

 // open as blocking mode
 tosmac_dev = open(TOSMAC_DEVICE, O_RDWR);
 if (tosmac_dev < 0)
 {
   fprintf(stderr, "Open error: %s\n", TOSMAC_DEVICE);
   return 1;
 }

 // open file to be send
 file = open(argv[1], O_RDWR); // open file
 file_size = lseek(file,0,SEEK_END); // calculate filesize by going to final byte of file using lseek function from stdlib.h
 lseek(file,0,SEEK_SET); // return to start of file

 msg_init(&send_pkt);
 send_pkt.addr = 99;  // Address

 h = file_size/TOSH_DATA_LENGTH; // How much packet or times of transmission should be prepared (using div)
 i = file_size%TOSH_DATA_LENGTH; // Remainder of "h" (using mod)
 j = 0;
 k = 0;

 while(j+1<h){ // Because j (requested file order) starts at zero so it's j+1

  // Use this if need resting time for when k reach certain accumulative bytes //
  /*if(k%1400==0){
  sleep(1);
  }*/
  read(tosmac_dev, (TOS_Msg*)&recv_pkt, sizeof(TOS_Msg));  // Waiting for receiver to send a packet (for ACK), it will wait until receiver is ready
  j = recv_pkt.data[1]+3000; // Requested order of file in packet, 3000 because it starts at -3000, the data type of data doesn't allow over 3000 while 3300 packet is needed, So we start from below 0.
  printf("Packet %d/%d\n",j,h); // Verbose
  k = j*TOSH_DATA_LENGTH; // Packet converted into byte
  lseek(file,k,SEEK_SET); // Seek to the requested byte
   read(file,send_pkt.data,TOSH_DATA_LENGTH); // reading 28 byte from file, then read next 28 due to while() loop, file >>>>> send_pkt.data
   send_pkt.group = 7; // Group identifier, modify this as wished
   // writing packet to device
   printf("User writing %d bytes to driver\n", TOSH_DATA_LENGTH); // verbose 
   write(tosmac_dev, (TOS_Msg*)&send_pkt, sizeof(TOS_Msg)); // writing to device, (TOS_Msg*)&send_pkt >>>>> tosmac_dev, & it's a pointer
   k += TOSH_DATA_LENGTH; // accumulative payload sent
   printf("%d bytes written\n", k); // verbose

   // use this function if for some reason need to slow down
   // usleep(30000); // in micro seconds
 }
 
 // Sending the last bytes
 read(tosmac_dev, (TOS_Msg*)&recv_pkt, sizeof(TOS_Msg));  // Waiting for receiver to send a packet (for ACK), it will wait until receiver is ready
  read(file,send_pkt.data,i); // i = remainder 
  printf("User writing %d bytes to driver\n", i); // verbose
  write(tosmac_dev, (TOS_Msg*)&send_pkt, i); // final sending
  k += i; // accumulative payload sent
  memcpy(send_pkt.data,"0S0T1O1P0",9);
  write(tosmac_dev, (TOS_Msg*)&send_pkt, 9);
  printf("%d bytes written, FINISH!!\n", k); // verbose

 //close device  
  close(tosmac_dev);
  close(file); 
return 0;
}
