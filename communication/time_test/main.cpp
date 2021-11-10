#include <Arduino.h>

unsigned long esp_clock;
unsigned long timer;

void setup(){
Serial.begin(115200);
}

unsigned long getLocalTimeS(unsigned long internal_time){
    int divider = 1000000;
    internal_time = esp_timer_get_time();
    timer = esp_clock / divider;
    return timer;
}


void loop(){
    esp_clock = esp_timer_get_time();
    timer = getLocalTimeS(esp_clock);
    Serial.println(timer);
    
}
//hello dakness my old friend

<<<<<<< HEAD
void funk(){
    int hello =12;
}
=======
int test(){
    int i = 2;
    int b = 3;

    return i + b; 
}

>>>>>>> b6918ac632e7dabb48c08439f38cfdb374da08fa
