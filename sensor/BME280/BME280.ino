
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)
float THI;
float temp;
float hum;
float test1;

Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime;



void setup() {
  Serial.begin(9600);
  Serial.println(F("BME280 test"));

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  bool status = bme.begin(0x76);  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  Serial.println("-- Default Test --");
  delayTime = 1000;

  Serial.println();
}


void loop() { 
  printTemp();
  test1=printHum();
  printTHI();
  delay(delayTime);
}

float printTemp(){
    
  temp = bme.readTemperature();
  Serial.print("Temperature = ");
  Serial.print(temp);
  Serial.println(" *C");
  return temp;
}

float printHum(){
  
  hum = bme.readHumidity();
     Serial.print("Humidity = ");
     Serial.print(hum);
  Serial.println("%");
  return hum;
}

float printTHI(){
   Serial.print ("THI = ");
  //Calculation of Temeratur Humidity Index (THI)
  THI = 0.8 * bme.readTemperature() + bme.readHumidity()*0.01*(bme.readTemperature()-14.4) + 46,4;
  Serial.println(THI);
  return THI;
}
