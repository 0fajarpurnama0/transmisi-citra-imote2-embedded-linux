#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, const char *argv[]) {
 int file1;
 char *pkt;
 pkt = malloc(10 * sizeof(char));
 file1 = open(argv[1], O_RDWR);
 lseek(file1,1,SEEK_SET);
 read(file1, pkt, 5);
 printf("%s\n", pkt);  
 close(file1);
return 0;
}

