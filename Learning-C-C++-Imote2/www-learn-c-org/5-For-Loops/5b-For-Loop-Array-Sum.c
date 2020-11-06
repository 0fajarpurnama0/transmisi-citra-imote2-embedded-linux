#include<stdio.h>

int main() {
 int array[10] = {1,2,3,4,5,6,7,8,9,10};
 int sum = 0;
 int i;
 
 for (i=0;i<10;i++) {
  sum += array[i];
 }

 /* sum now contains a[0] + a[1] + ... + a[9] */
 printf("Sum of the array is %d\n", sum);
 return 0;
}
