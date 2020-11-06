#include<stdio.h>

int factorial(int number);

int main() {
 /* testing code */
 printf("1! = %i\n", factorial(1));
 printf("3! = %i\n", factorial(3));
 printf("5! = %i\n", factorial(5));
return 0;
}

int factorial(int number) {
 int f = number;
 if (number > 1) {
  f *= factorial(number-1);
 }
return f;
}
