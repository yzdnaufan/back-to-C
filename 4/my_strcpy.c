#include <stdio.h>

char* my_strcpy(const char* src, char* dest){
    // Copy src string to dest
    char* init = dest;
    while(*src!='\0'){
        *dest= *src;
        // printf("%c", *dest);
        dest++;
        src++;
        // printf("%c", *src);
    }
    *dest = *src;
    // printf("%c", *dest);
    return init;
}

char* my_strncpy(const char* src, char* dest, int size){
    // Copy n size char from src string to dest.
    char* init = dest;
    int counter=0;
    while(*src!='\0'){
        *dest = *src;
        dest++;
        src++;
        counter++;
        if(counter>=size){
            break;
        }
    }
    *dest = '\0'; // null terminator
    return init;
}


int main()
{
    char stri[] = "Dunia";
    char dest[20] = "Halo ";
    int c =0;
    
    my_strcpy(stri, dest);
    my_strncpy(stri,dest, 9);
    
    printf("%s", dest);
}