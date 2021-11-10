/******************************************
 *
 * This example works for both Industrial and STEM users.
 *
 * Developed by Jose Garcia, https://github.com/jotathebest/
 *
 * ****************************************/

/****************************************
 * Include Libraries
 ****************************************/
#include <UbidotsEsp32Mqtt.h>
#include "Wire.h"
#include "math.h"

/****************************************
 * Define Constants
 ****************************************/
const char *UBIDOTS_TOKEN = "BBFF-O7IdMgAv4p0AzptrTKIQGkQr1jTCZZ";  // Put here your Ubidots TOKEN
const char *WIFI_SSID = "Hurodor";      // Put here your Wi-Fi SSID
const char *WIFI_PASS = "123456789";      // Put here your Wi-Fi password
const char *DEVICE_LABEL = "Test";   // Put here your Device label to which data  will be published
const char *VARIABLE_LABEL = "random_value"; // Put here your Variable label to which data  will be published

const char *DEVICE_LABEL2 = "Test";   // Put here your Device label to which data  will be published
const char *VARIABLE_LABEL2 = "random_value2"; // Put here your Variable label to which data  will be published
const int value2 = 10;


/*
unsigned long timestamp = 1635931003;
char* context = "not nulll";
*/

const int PUBLISH_FREQUENCY = 5000; // Update rate in milliseconds

unsigned long timer;
uint8_t analogPin = 34; // Pin used to read data from GPIO34 ADC_CH6.

Ubidots ubidots(UBIDOTS_TOKEN);

//glenns varables:

const int MPU_ADDR = 0x68;
int16_t temp;
int16_t accl_x, accl_y, accl_z;

uint16_t activity;



/****************************************
 * Auxiliar Functions
 ****************************************/

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// glenn
uint16_t getActivity(int16_t x, int16_t y, int16_t z) {
  double act = sqrt(pow((double) x, 2) + pow((double) y, 2) + pow((double) z, 2));
  return (uint16_t) act;
}



/****************************************
 * Main Functions
 ****************************************/

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  // ubidots.setDebug(true);  // uncomment this to make debug messages available

  // glenn
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  
  // vegar
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();

  activity = 0;

  timer = millis();
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (!ubidots.connected())
  {
    ubidots.reconnect();
  }
  
   // ubidots.add(VARIABLE_LABEL, value); // Insert your variable Labels and the value to be sent
    ubidots.add(VARIABLE_LABEL2, activity);
    
    ubidots.publish(DEVICE_LABEL);
    
    
    Serial.print("sendt value:");
    Serial.println(activity);
    //timer = millis();
  
  ubidots.loop();

  //glenn
    Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 4*2, true);

  accl_x = Wire.read()<<8 | Wire.read();
  accl_y = Wire.read()<<8 | Wire.read();
  accl_z = Wire.read()<<8 | Wire.read();
  activity = getActivity(accl_x, accl_y, accl_z);
  //Serial.print("Activity: ");
  //Serial.print(activity);

  temp = Wire.read()<<8 | Wire.read();
  temp = temp/340 + 36.53;
  //Serial.print(" | Temperature: ");
  //Serial.println(temp);
  delay(100);
}