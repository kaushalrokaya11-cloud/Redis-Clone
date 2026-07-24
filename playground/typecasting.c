#include<stdio.h>

int main (){
    int value = 52;
    int * val = &value;
    printf("%d", *val);

    char *ch = (char*) val;
    printf("%c", *ch);

}