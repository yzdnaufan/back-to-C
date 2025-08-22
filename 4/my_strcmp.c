
#include <stdio.h>

int my_strcmp(const char* a, const char* b){
    while(*a!='\0'|*b!='\0'){
        if(*b - *a !=0){
            return *b - *a;
        }
        a++;
        b++;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    /* code */
    return 0;
}
