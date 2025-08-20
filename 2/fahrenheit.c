#include <stdio.h>

// Fahrenheit= 0, 20,... 300

int main() {
    int fahrenheit, celcius;
    int upper,lower,step;

    lower = 0;
    upper = 300;
    step = 20;

    fahrenheit = lower;
    printf("Fahrenheit to Celcius Conversion Table\n");
    printf("Fahrenheit\tCelcius\n");

    while (fahrenheit <= upper){
        celcius = (fahrenheit-32) * 5 / 9;
        printf("%d\t\t%d\n", fahrenheit, celcius);
        fahrenheit = fahrenheit + step;
    }
}
