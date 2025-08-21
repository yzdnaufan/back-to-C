#include <stdio.h>

int swap(int* a, int* b){
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
    return 0;
}

int main() {
    int a_1, b_1;

    a_1 = 10;
    b_1 = 20;

    swap(&a_1, &b_1);

    printf("a_1 = %d, b_1 = %d\n", a_1, b_1);
}
