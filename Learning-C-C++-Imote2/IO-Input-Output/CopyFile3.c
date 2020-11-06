#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, const char *argv[]){
 int file1 = open(argv[1], O_RDWR);
 creat(argv[2], O_RDWR);
 int file2 = open(argv[2], O_RDWR);
 int file_size = lseek(file1,0,SEEK_END);
 lseek(file1,0,SEEK_SET);
 char *pkt;
 int h = 0;
 int i = 100000;
 int j = file_size/i;
 int k = 0;
 int a, b;
  while(k<j){
  pkt = malloc(i * sizeof(char));
  read(file1,pkt,i);
  write(file2,pkt,i);
  h += i;
  printf("writing %d bytes %d bytes written\n", i, h);
  a = lseek(file1,0,SEEK_CUR);
  b = lseek(file2,0,SEEK_CUR);
  printf("file1 on byte %d, file2 on byte %d\n", a, b);
  free(pkt);
  k++;
 }
 int l = file_size % i;
 read(file1,pkt,l);
 write(file2,pkt,l);
 h += l;
 printf("writing %d bytes %d bytes written\n Successful\n", l, h);
 return 0;
}
