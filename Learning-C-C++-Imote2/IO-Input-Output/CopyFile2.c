#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, const char *argv[]) {
 int file1, file2;
 char *pkt;

 file1 = open(argv[1], O_RDWR);
 creat(argv[2], O_RDWR);
 file2 = open(argv[2], O_RDWR);
 
 int file_size = lseek(file1,0,SEEK_END);
 lseek(file1,0,SEEK_SET);
 pkt = malloc(file_size * sizeof(char));
 read(file1, pkt, file_size);
 printf("Writing %d bytes\n", file_size);
 write(file2, pkt, file_size);
 free(pkt);
 printf("Successful\n");
 return 0;
}
