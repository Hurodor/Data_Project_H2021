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

