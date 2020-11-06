#include <stdio.h>

int main() {
 int average, grades[3], i;
 int sum = 0;
 grades[0] = 80;
 grades[1] = 85;
 grades[2] = 90;
 
 for (i=0;i<3;i++) {
  sum += grades[i];
  printf("grades %d = %d \n", i, grades[i]);
 }
 
 average = sum/3;
 printf("average = %d \n", average);
 return 0;
}
