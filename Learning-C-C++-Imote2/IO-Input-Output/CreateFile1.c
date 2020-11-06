#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, const char *argv[]) {
 int file1;
 creat(argv[1], O_RDWR);
 file1 = open(argv[1], O_RDWR);
 write(file1, "Hello", 5); 
 close(file1);
return 0;
}

