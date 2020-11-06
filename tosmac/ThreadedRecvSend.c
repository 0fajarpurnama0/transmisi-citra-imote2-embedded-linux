/*
 * 
 *   Threaded block read. 
 *   A seperate thread is started doing block reading, so that main program is not block waiting for data.
 *   
 *   Author: Y Cheng 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include "tosmac.h"

//Structure of arguments to pass to client thread 
struct ThreadArgs
{
    int dev;                     
};
void tosmsg_init(TOS_Msg* pMsg)
{
   pMsg->length = 0;
   pMsg->fcfhi = 0;
   pMsg->fcflo = 0;
   pMsg->dsn = 0;
   pMsg->destpan = 0;
   pMsg->addr = 0;
   pMsg->type = 0;
   pMsg->group = 0;
   memset(pMsg->data, '\0', TOSH_DATA_LENGTH);
   pMsg->strength = 0;
   pMsg->lqi = 0;
   pMsg->crc = 0;
   pMsg->ack = 0;
   pMsg->time = 0;
}

int cc2420_init()
{
	int ret,tosmac_dev;

	// open as blocking mode
	tosmac_dev = open(TOSMAC_DEVICE, O_RDWR);
	if (tosmac_dev < 0)
	{
		fprintf(stderr, "Open error: %s\n", TOSMAC_DEVICE);
		return 1;
	}

	//set freq   2405 to 2480
	if(ioctl(tosmac_dev, TOSMAC_IOSETFREQ, 2405) < 0)
	{
		fprintf (stderr, "IOCTL set TOSMAC freq error: %s\n", TOSMAC_DEVICE);
		close (tosmac_dev);
		return 1;
	}
	//get frequency
	ret = ioctl(tosmac_dev, TOSMAC_IOGETFREQ);
	printf("freq is %d\n",ret);
	//set channel  11 to 26
	if(ioctl(tosmac_dev, TOSMAC_IOSETCHAN, 11) < 0)
	{
		fprintf (stderr, "IOCTL set TOSMAC channel error: %s\n", TOSMAC_DEVICE);
		close (tosmac_dev);
		return 1;
	}
	//set local address
	if(ioctl(tosmac_dev, TOSMAC_IOSETADDR, 40) < 0)
	{
		fprintf (stderr, "IOCTL set TOSMAC address error: %s\n", TOSMAC_DEVICE);
		close (tosmac_dev);
		return 1;
	}
	// enable auto ack
	ret = ioctl(tosmac_dev, TOSMAC_IODISAUTOACK);
	if(ret < 0)
	{
		fprintf (stderr, "IOCTL enable TOSMAC AutoAck error: %s\n", TOSMAC_DEVICE);
		close (tosmac_dev);
		return 1;
	}
	//change max payload size (max value can be set is 116 bytes, change in "tosmacapi.h")
	if(ioctl(tosmac_dev, TOSMAC_IOSETMAXDATASIZE, 100) < 0)
	{
		fprintf (stderr, "IOCTL set TOSMAC max payload size error: %s\n", TOSMAC_DEVICE);
		close (tosmac_dev);
		return 1;
	}
	// close device
	close (tosmac_dev);
	return 0;
}

void *thread_tosreceive (void * threadArgs){
	int nresult,i;	
	int tos;
	TOS_Msg recv_pkt;

        pthread_detach(pthread_self()); 
        // Extract file descriptor from argument 
    	tos = ((struct ThreadArgs *) threadArgs) -> dev;
	// Deallocate memory for argument    	
	free(threadArgs); 
	
    	while(1){				
		nresult = read(tos, &recv_pkt, TOSH_DATA_LENGTH);
		if(nresult<0)printf("receive error!\n");
		else{			
			printf("Data:--------------------------------------: \n");
			for(i = 0; i < recv_pkt.length; i++)
			printf("%x ", (unsigned char) recv_pkt.data[i]);
			printf("\n");
		}
	}
}

int main()
{
	int nresult;	
	int tosmac_dev;
	pthread_t pt_tosreceive;
	TOS_Msg send_pkt;
	struct ThreadArgs *threadArgs;   // Pointer to argument structure for thread 
	int i = 0,j=0;
        unsigned char buf[TOSH_DATA_LENGTH];
	
	cc2420_init();
	
        tosmac_dev = open(TOSMAC_DEVICE, O_RDWR);
	if (tosmac_dev < 0)
	{
		printf("Open error: \n");
		return 1;
	}
	
	threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));
	if ( threadArgs== NULL)
	printf("malloc() failed\n");
	threadArgs -> dev = tosmac_dev;
	if(pthread_create(&pt_tosreceive,NULL,thread_tosreceive,(void *) threadArgs)!=0) //create read thread with default parameters	
        printf("pthread_create() failed\n");


        for(i=0;i<5;i++)buf[i] = i;
	tosmsg_init(&send_pkt);
	send_pkt.addr = 40;
	memcpy(send_pkt.data,buf,5);		
	send_pkt.length = 5;

	while(1)
	{
		nresult = write(tosmac_dev, (TOS_Msg*)&send_pkt, send_pkt.length);
		printf("write out: %d\n",nresult);			
		usleep(100000);
  	}	 
	close( tosmac_dev);
	return 0;

}   // end of main





