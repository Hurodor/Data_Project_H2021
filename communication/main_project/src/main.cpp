//inluded liberaries:
#include <Arduino.h>
#include <UbidotsEsp32Mqtt.h>

// included liberaries end:


//ubidots variables:
const char *UBIDOTS_TOKEN = "BBFF-O7IdMgAv4p0AzptrTKIQGkQr1jTCZZ";  // Put here your Ubidots TOKEN
const char *WIFI_SSID = "Hurodor";      // Put here your Wi-Fi SSID
const char *WIFI_PASS = "123456789";      // Put here your Wi-Fi password

// define device and variabelname made in ubidots here:
const char *DEVICE_LABEL = "Test";   // Put here your Device label to which data  will be published
const char *VARIABLE_LABEL = "random_value2"; // Put here your Variable label to which data  will be published

const int value = 10;

unsigned long timer;

//testing


char context[25];
const int timestamp = 1635944256;

// initialize ubidots constraint
Ubidots ubidots(UBIDOTS_TOKEN);

void setup(){
    Serial.begin(115200);
    
    ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
    //ubidots.setCallback(callback);
    ubidots.setup();
    ubidots.reconnect();

    timer = millis();
}

void loop(){
    if (!ubidots.connected()){
        ubidots.reconnect();
    }

    
    // add value to the spesified label:

    if (millis() >= timer + 5000){

        // making context key value pair
        ubidots.addContext("test", "10");
        ubidots.getContext(context);
        // making payload
        ubidots.add(VARIABLE_LABEL, value, context, timestamp);

        // sending payload
        ubidots.publish(DEVICE_LABEL);

        // print statments
        Serial.print("value: ");
        Serial.print(value);
        Serial.println(" should be sent ");
        timer = millis();
    }
    

    // needs to do this for some reason
    ubidots.loop();


}