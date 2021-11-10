<<<<<<< HEAD
#include <Arduino.h>

// Deep sleep with periodic NTP update
// This example stores a variable in the RTC memory
// to keep track of the last time NTP was checked
// Larry Bernstone <lbernstone@gmail.com>

 
#include <WiFi.h>
#include <esp_sleep.h>

#define NTP_CYCLE 3600  // one hour
#define NTP_TIMEOUT 30  // seconds
#define TIME_TO_SLEEP 30  // seconds
#define NTP_SRV "time-a-b.nist.gov"

RTC_DATA_ATTR unsigned long lastUpdate = 0;
const char* ssid = "Kugalskap";
const char* passwd = "datakom696969";

void setup() {
  Serial.begin(115200);
  unsigned long now = time(NULL);
  Serial.printf("Now: %lu\nLast Update: %lu\n", now, lastUpdate);
  if (lastUpdate > now) lastUpdate = 0;

  if ((lastUpdate == 0) || ((now - lastUpdate) > NTP_CYCLE)) { 
    WiFi.begin(ssid, passwd);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Unable to connect to WiFi");
      return;
    }

    struct tm tm_now;
    configTime(0, 0, NTP_SRV); //UTC time
    if (!getLocalTime(&tm_now, NTP_TIMEOUT * 1000ULL)) {
      Serial.println("Unable to sync with NTP server");
      return;
    }
    lastUpdate = time(NULL);
    Serial.println("NTP time updated");
  }
}

void loop() {  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000ULL);
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("********")
  Serial.println(time(NULL))
  Serial.println("********")
=======

#include <UbidotsEsp32Mqtt.h>
#include "Wire.h"
#include "math.h"

/****************************************
 * Define Constants
 ****************************************/
const char *UBIDOTS_TOKEN = "BBFF-O7IdMgAv4p0AzptrTKIQGkQr1jTCZZ"; // Put here your Ubidots TOKEN
const char *WIFI_SSID = "Hurodor";								   // Put here your Wi-Fi SSID
const char *WIFI_PASS = "123456789";							   // Put here your Wi-Fi password
const char *DEVICE_LABEL = "Test";								   // Put here your Device label to which data  will be published
const char *VARIABLE_LABEL = "random_value";					   // Put here your Variable label to which data  will be published

const char *DEVICE_LABEL2 = "Test";			   // Put here your Device label to which data  will be published
const char *VARIABLE_LABEL2 = "random_value2"; // Put here your Variable label to which data  will be published

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


// need to find out what this does:
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
uint16_t getActivity(int16_t x, int16_t y, int16_t z)
{
	double act = sqrt(pow((double)x, 2) + pow((double)y, 2) + pow((double)z, 2));
	return (uint16_t)act;
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

	// vegar
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
	Wire.requestFrom(MPU_ADDR, 4 * 2, true);

	accl_x = Wire.read() << 8 | Wire.read();
	accl_y = Wire.read() << 8 | Wire.read();
	accl_z = Wire.read() << 8 | Wire.read();
	activity = getActivity(accl_x, accl_y, accl_z);
	//Serial.print("Activity: ");
	//Serial.print(activity);

	temp = Wire.read() << 8 | Wire.read();
	temp = temp / 340 + 36.53;
	//Serial.print(" | Temperature: ");
	//Serial.println(temp);
	delay(100);
>>>>>>> b6918ac632e7dabb48c08439f38cfdb374da08fa
}