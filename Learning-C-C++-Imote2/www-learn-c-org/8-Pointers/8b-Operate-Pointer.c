#include<stdio.h>

int main() {
 int a = 1;
 int * pointer_to_a = &a;
 
 /* let's change variable a */
 a += 1;
 
 /* let's change variable a again */
 *pointer_to_a += 1;
 
 /* will print out 3 */
 printf("The value of a is %d", a);
return 0;
}
