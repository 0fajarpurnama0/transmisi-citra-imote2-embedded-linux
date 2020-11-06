#include<stdio.h>

int main() {
 //char * name = "Joh";
 char name[] = "";
 printf("Login: ");
 scanf("%s", name);

 if (strncmp(name,"John",4) == 0) {
  printf("Hello, John!\n");
 } else {
  printf("You are not John. Go away.\n");
 }
return 0;
}
