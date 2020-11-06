#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, const char *argv[]) {
 int file1;
 int file2;
 char *pkt1;
 char *pkt2;
 pkt1 = malloc(460808 * sizeof(char));
 pkt2 = malloc(460807 * sizeof(char));
 file1 = open("test", O_RDWR);
 read(file1, pkt1, 460808);
 creat("testcopy", O_RDWR);
 file2 = open("testcopy", O_RDWR);
 write(file2, pkt1, 460808);
 lseek(file1,460808,SEEK_SET);
 lseek(file2,460808,SEEK_SET);
 read(file1, pkt2, 460807);
 write(file2, pkt2, 460807);
 free(pkt1);
 free(pkt2);
return 0;
}
