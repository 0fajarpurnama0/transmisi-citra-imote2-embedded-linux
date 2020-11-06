#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, const char *argv[]) {
 int i = 0;
 int seek;
 int file = open(argv[1], O_RDWR);
 seek=lseek(file,0,SEEK_END);
 //while(seek!=lseek(file,0,SEEK_SET)){
 // seek=lseek(file,-1,SEEK_CUR);
 // i++;
 //}
 printf("filesize is %d bytes \n", seek);
}

