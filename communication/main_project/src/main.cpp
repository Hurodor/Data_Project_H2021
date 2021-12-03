#include <Arduino.h>
#include <WiFi.h>
#include <UbidotsEsp32Mqtt.h>
#include <esp_wifi.h>
#include <esp_bt.h>



// ubidots parameters BEGIN
const char *UBIDOTS_TOKEN = "BBFF-O7IdMgAv4p0AzptrTKIQGkQr1jTCZZ"; // TOKEN
const char *WIFI_SSID = "Hurodor";								   // SSID
const char *WIFI_PASS = "123456789";							   //  password
char *DEVICE_LABEL = "Test";								   //  Device label 
char *VARIABLE_LABEL = "random_value";					   // Variable label

// context variabel  "key":"value"
char my_dict[] = "\"test\":\"this_is_new\"";

Ubidots ubidots(UBIDOTS_TOKEN);



// Clock variables
#define NTP_SRV "time-a-b.nist.gov"
#define NTP_TIMEOUT 30  // seconds
#define TIME_TO_SLEEP 5  //seconds to deepsleep

unsigned int wifi_timeout = 30; // seconds
RTC_DATA_ATTR unsigned long lastUpdate = 0;


// current loop counter
RTC_DATA_ATTR int timer = 0;


// storing data between deepsleep:
const int RTC_LIST_SIZE = 20; //how many elements can be saved in rtc memory
RTC_DATA_ATTR int rtc_list_current_index = 0;

RTC_DATA_ATTR float saved_activity[RTC_LIST_SIZE];
RTC_DATA_ATTR unsigned long saved_timestamp[RTC_LIST_SIZE];


int value = 156;

float avgOfArray(float array[], int array_size) {
    double sum = 0;
    for (int i = 0; i < array_size; i++)
    {
        sum += array[i];
    }
    return sum /(double)array_size;
  }

bool rtcArraysFull(int current_index, int max_size){
    if (current_index < max_size){
        return false;
    }
    return true;
}

void connectToWifi(const char* WIFI_SSID, const char* WIFI_PASS){
    
    // local timeout variables
    int timer = 0;
    int timeout = wifi_timeout * 2;

    // try connect
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");

        timer += 1;
        if (timer >= timeout){
            break;
        };
        
    }

    if (WiFi.status() != WL_CONNECTED){
        Serial.println();
        Serial.println("Could not connect to wifi [wifi timed out]");
        return;
    }

    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());
}

void sync_clock(){

    struct tm tm_now;
    configTime(0, 0, NTP_SRV); //UTC time

    if (!getLocalTime(&tm_now, NTP_TIMEOUT * 1000ULL)) {
      Serial.println("Unable to sync with NTP server");
      return;
    }

    lastUpdate = time(NULL);
    Serial.println("NTP time updated");
}

void disconctUnwantedServices(){
    // disconect bluethoot and wifi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    btStop();
    esp_wifi_stop();
    esp_bt_controller_disable();

}

void goToDeepSleep()
{
    Serial.println("Going to sleep...");

    // turn off bluethoot and wifi
    disconctUnwantedServices();

    // Configure the timer to wake us up!
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000ULL);

    // Go to sleep! Zzzz
    esp_deep_sleep_start();
}

void connectToUbidots(){//const char WIFI_SSID[], const char WIFI_PASS[]){
    // this function connect to wifi and ubidots MQTT broker

    

    ubidots.setup();
    ubidots.reconnect();
}

void pushToUbidots(char varable_label[], char device_label[], int value, char context[], unsigned long timestamp){

    if (!ubidots.connected())
	    {
		    ubidots.reconnect();
	    }

    // making payload
    ubidots.add(varable_label, value, context, timestamp);

    // sending payload
    ubidots.publish(device_label);
    
    // print what is sendt
    Serial.println();
    Serial.print("sendt value: ");
	Serial.print(value);
    Serial.print(" | to device: ");
    Serial.print(device_label);
    Serial.print(" | to lablel: ");
    Serial.print(varable_label);
    Serial.print(" | time: ");
    Serial.print(timestamp);
    Serial.print(" | context: ");
    Serial.println(context);
    Serial.println();
    
    // time to make sure the pacage is sendt
    delay(500);

    // this has to be to send (dont understand why)
    ubidots.loop();
}

void setup(){
    Serial.begin(115200);

    // Disconect wifi and bluethoot for powersaving
    disconctUnwantedServices();
}

void loop(){

    if (timer >= 5){

        timer = 0;

        connectToWifi(WIFI_SSID, WIFI_PASS);

        if (WiFi.status() != WL_CONNECTED){
            goToDeepSleep();
        }

        // testing clock
        sync_clock();

        connectToUbidots();


        pushToUbidots(VARIABLE_LABEL, DEVICE_LABEL, value, my_dict, time(NULL));

    
    }else {
        Serial.println("i woke up and dident send data");
    }

    Serial.println(time(NULL));


    timer += 1;

    goToDeepSleep();


}

