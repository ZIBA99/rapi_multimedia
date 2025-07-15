#include <wiringPi.h>
#include <stdio.h>

#define SW 5
#define LED 1

int switchControl()
{
    pinMode(SW, INPUT);
    pinMode(LED, OUTPUT);

    for(;;){
        if(digitalRead(SW) == LOW){
            digitalWrite(LED, HIGH);
            delay(1000);
            digitalWrite(LED, LOW);
        }
        delay(10);
    }

    return 0;
}

int main(int argc, char **argv)
{
    wiringPiSetup();       // GPIO 번호: wiringPi 번호 기준
    switchControl();
    return 0;
}

