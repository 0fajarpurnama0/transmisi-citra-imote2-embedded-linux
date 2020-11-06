#include <stdio.h>

int main(int argc,int argv[]) {
 
 printf("This program is %s\n", argv[0]);

 if(argc==2) {
  printf("The argument supply is %s\n", argv[1]);
 }
 else if(argc>2) {
  printf("Too many argument");
 }
 else {
  printf("Argument expected");
 }
} 
