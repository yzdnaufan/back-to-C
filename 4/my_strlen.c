
#include <stdio.h>


int my_strlen(const char* s){
    int counter=0;
    while(*s!='\0'){
        counter++;
        s++;
    }
    return counter;
}

int main()
{
    char stri[] = "Dunia";
    char dest[20] = "Halo ";
    int c =0;
    
    
    printf("%d", my_strlen(stri));
}