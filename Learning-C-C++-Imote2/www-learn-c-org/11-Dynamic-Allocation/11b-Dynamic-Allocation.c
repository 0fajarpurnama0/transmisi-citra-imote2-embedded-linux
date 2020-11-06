#include<stdio.h>

typedef struct {
 char * name;
 int age;
} person;

int main() {
 person * myperson = malloc(sizeof(person));
 myperson->name = "John";
 myperson->age = 27;
 printf("%s is %d years old.\n", myperson->name, myperson->age);
 free(myperson);
return 0;
}
