#include<stdio.h>

int main() {
 /* define a local variable */
 int a = 1;

 /* define a pointer variable and point using & operator */
 int * pointer_to_a = &a;

 printf("The value of a is %d\n", a);
 printf("The value of a is also %d\n", *pointer_to_a);
return 0;
}
