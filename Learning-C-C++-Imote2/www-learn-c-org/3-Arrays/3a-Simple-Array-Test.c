#include <stdio.h>

int main() {
 int numbers[10], i;

 /* populate the array */
 numbers[0] = 10;
 numbers[1] = 20;
 numbers[2] = 30;
 numbers[3] = 40;
 numbers[4] = 50;
 numbers[5] = 60;
 numbers[6] = 70;
 numbers[7] = 80;
 numbers[8] = 90;
 numbers[9] = 100;

 for (i=0;i<10;i++) {
  printf("number %d = %d \n", i, numbers[i]);
 }
 return 0;
}
 
