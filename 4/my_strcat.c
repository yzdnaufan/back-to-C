
#include <stdio.h>

char* my_strcat(const char* src, char* dest){
    char* init = dest;
    while(*dest!='\0'){
        dest++;
    }
    while(*src!='\0'){
        *dest=*src;
        dest++;
        src++;
    }
    *dest = '\0';
    return init;
}

char* my_strncat(const char* src, char* dest, int size){
    //append src in size to dest string
    char* init = dest;
    int counter = 0;
    while(*dest!='\0'){
        dest++;
    }
    while(*src!='\0'){
        if(counter>=size){
            break;
        }
        *dest=*src;
        dest++;
        src++;
        counter++;
    }
    *dest = '\0';
    return init;
}

int main()
{
    char stri[] = "Dunia";
    char dest[20] = "Halo ";
    int c =0;
    
    my_strcat(stri, dest);
    my_strncat(stri, dest, 9);
    
    printf("%s", dest);
}