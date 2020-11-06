#include<stdio.h>

int foo(int bar);

int main() {
 /* calling foo from main */
 printf("The value of foo is %d", foo(1));
return 0;
}

int foo(int bar) {
 return bar + 1;
}
