//Fajar Purnama//

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, const char *argv[]){
 int file = open(argv[1], O_RDWR);
 printf("%d bytes\n", lseek(file,0,SEEK_END));
return 0;
}
