#include <Arduino.h>
#include <WiFi.h>
#include <UbidotsEsp32Mqtt.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <Wire.h>
#include <ICM20948_WE.h>


// ubidots parameters BEGIN
const char *UBIDOTS_TOKEN = "BBFF-O7IdMgAv4p0AzptrTKIQGkQr1jTCZZ"; // TOKEN
const char *WIFI_SSID = "Hurodor";								   // SSID
const char *WIFI_PASS = "123456789";							   //  password
char *DEVICE_LABEL = "cow_necklaces";								   //  Device label 
char *VARIABLE_LABEL = "activity";					   // Variable label

// context variabel  "key":"value" (need context to send timestamp to ubidots)
char context[] = "\"Critical\":\"0\"";

Ubidots ubidots(UBIDOTS_TOKEN);


// Clock variables
#define NTP_SRV "time-a-b.nist.gov"
#define NTP_TIMEOUT 30  // seconds
#define TIME_TO_SLEEP 60  //seconds to deepsleep

unsigned int wifi_timeout = 30; // seconds
RTC_DATA_ATTR unsigned long lastUpdate = 0;


// current loop counter
RTC_DATA_ATTR int timer = 0;


// storing data between deepsleep:
const int RTC_LIST_SIZE = 60; //how many elements can be saved in rtc memory
RTC_DATA_ATTR int rtc_list_current_index = 0;

RTC_DATA_ATTR float rtc_saved_activity[RTC_LIST_SIZE];
RTC_DATA_ATTR unsigned long rtc_saved_timestamp[RTC_LIST_SIZE];


// messurment variables
const int delay_time = 100;  // time between messurments
const int sample_size = 10;  // how many messurments too take
float activity_array[sample_size];

// sensor variables
#define ICM20948_ADDR 0x68
ICM20948_WE myIMU = ICM20948_WE(ICM20948_ADDR);
float activity;

// sensor testing/analyzing variables
float minimum_value = 10000.0; // Set high to ensure updating right away
float maximum_value = -1.0;    // Set low to ensure updating right away
bool start_average_min_max = false;  // Variable for enabling minimum and maximum average values
int averaging_index = 0;       // Keeps track of current index for keeping a running average

//case variables
int critical = 1337;
bool send = false;
int elements_to_send = RTC_LIST_SIZE;

float avgOfArray(float array[], int array_size) {
    // calculate avg of list and returns value
    double sum = 0;
    for (int i = 0; i < array_size; i++)
    {
        sum += array[i];
    }
    return sum /(double)array_size;
  }

void printList(float array[], const int size){
    // print function to debug
    
    Serial.println("printing list");
    for (int i = 0; i < size; i++){
        Serial.print("index: ");
        Serial.print(i);
        Serial.print(" | ");
        Serial.println(array[i]);
    }
}

void getMinAndMaxValue(float data_value, float &min_val, float &max_val){
    // update and print the min and max values (for activity testing) 
    if (data_value < min_val){
        min_val = data_value;
    }
    if (data_value > max_val){
        max_val = data_value;
    }
    Serial.print("Min: ");
    Serial.print(min_val);
    Serial.print(" - Max: ");
    Serial.println(max_val);
}

void runningAverage(float data_value, int &index, float &min_val, float &max_val){
    /* Print the running average at each measurement and keep track of the maximum and
    minimum average values. */
    float average = 0;                  // reset average value
    activity_array[index] = data_value; // set activity_array index value to lates value 
    index += 1;
    if (index >= sample_size){          // Once all indexes of array are filled
        index = 0;                      //  the index is set back to 0 to repeat cycle.
        start_average_min_max = true;   // Set boolean to true to enable to getMinAndMaxValue
    }                                   //  function. If the function is enabled from start, the
                                        //  min and max values may be biased.
    for (int i = 0; i < 10; ++i){
        average += activity_array[i];   // Average the content of the full array for each new
    }                                   //  data_value. This means that the first 10 average
    average = average/sample_size;      //  values are invalid, as not all indexes are filled.

    if(start_average_min_max == true){
        getMinAndMaxValue(average, min_val, max_val);
    }
    
    Serial.print("Average: ");
    Serial.println(average);
}

void findGyroOffsets(ICM20948_WE sensor){
    /*When using this function it is very important that the sensor is sitting flat on a lever surface.
    Avoid any disturbance of the sensor while the function is running.
    The values printed out at the end should be entered into the 'setGyrOffsets' function within the 'setupActivitySensor' function
    This function has to be used for each individual ICM20948 that is to be used, as the offsets can vary.*/
    float x_offset;
    float y_offset;
    float z_offset;

    long timer = 300000; // 5 minutes in milliseconds
    unsigned long start = millis();

    Serial.print("Starting 5 minute timer to stabilize sensor temperature");
    while ((millis()-start)<timer){
      sensor.readSensor(); // The sensor is reading measurements but not using them, this is only to get it to it's operating temperature
      delay(100);
      Serial.print('.'); // Serial output to show that function is running.
    }
    Serial.println();
    
    Serial.print("Calculating offsets");
    // Take 100 measurements over a time of ~1 second
    for (int i = 0; i < 100; ++i){
      sensor.readSensor();
      xyzFloat gyrRaw = sensor.getGyrRawValues();
      x_offset += gyrRaw.x; // Sum up each measurement for later averaging.
      y_offset += gyrRaw.y;
      z_offset += gyrRaw.z;
      delay(10);
      Serial.print('.'); // Serial output to show that function is running.
    }
    Serial.println();
    Serial.println("Done.");
    
    // Average the sum of 100 measurements to get the average offset.
    x_offset = x_offset/100.0;
    y_offset = y_offset/100.0;
    z_offset = z_offset/100.0;

    Serial.print("X offset = ");
    Serial.print(x_offset);
    Serial.print(", Y offset = ");
    Serial.print(y_offset);
    Serial.print(", Z offset = ");
    Serial.print(z_offset);
}

void setupActivitySensor(ICM20948_WE sensor){
    // configure sensor
    if (!myIMU.init())
    {
        Serial.println("ICM20948 does not respond");
    }
    else
    {
        Serial.println("ICM20948 is connected");
    }
    myIMU.setGyrOffsets(73.11, 49.81, 22.18); // After running findGyroOffsets(), replace these values with produced values from function
    //myIMU.setGyrOffsets(X offset, Y offset, Z offset);

    myIMU.enableAcc(false);

    myIMU.setGyrRange(ICM20948_GYRO_RANGE_250);
    myIMU.setGyrDLPF(ICM20948_DLPF_6);
}

float getActivity(ICM20948_WE sensor){
    // take one measurement of activity
    sensor.readSensor();
    xyzFloat gyr = sensor.getGyrValues();
    return pow((pow(gyr.x, 2) + pow(gyr.y, 2) + pow(gyr.z, 2)), 0.5);
}


void connectToWifi(const char* WIFI_SSID, const char* WIFI_PASS){
    // connect to wifi
    
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
    //sync rtc clock with real time 

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

void connectToUbidots(){
    // this function connect to ubidots MQTT broker

    ubidots.setup();
    ubidots.reconnect();
}

void pushToUbidots(char varable_label[], char device_label[], float value, char context[], unsigned long timestamp){
    // send one value to ubidots

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
    delay(1000);

    // this has to be to send (dont understand why)
    ubidots.loop();
}

float takeMessurment()
// take "sample_size" number of messurments and save avg in rtc "rtc_saved_activity"
{
    for (int i = 0; i < sample_size; i++){
        activity = getActivity(myIMU);

        // store messurments in list
        activity_array[i] = activity;
        delay(delay_time);
    }

    // save avg of activity in rtc
    float avg_activity = avgOfArray(activity_array, sample_size);

    rtc_saved_activity[rtc_list_current_index] = avg_activity;

    // save coresponding timestamp
    rtc_saved_timestamp[rtc_list_current_index] = time(NULL);

    Serial.println();
    Serial.print("activity: ");
    Serial.println(rtc_saved_activity[rtc_list_current_index]);

    return avg_activity;
}

void sendSavedActivityToUbidots(char context[],int list_size){
// send only when full list
// make sure connection to ubidots is estahblished before sending!
    for (int i = 0; i < list_size; i++){
        pushToUbidots(
            VARIABLE_LABEL, 
            DEVICE_LABEL, 
            rtc_saved_activity[i], 
            context, 
            rtc_saved_timestamp[i]
            );
        //delay(delay_time);  delay in push-function
    }
}


void setup(){
    Wire.begin();
    Serial.begin(115200);
    
    // if clock is never synced sunc clock before starting messurments
    if (time(NULL) < 100){
        connectToWifi(WIFI_SSID, WIFI_PASS);
        if (WiFi.status() != WL_CONNECTED)
        {
            goToDeepSleep();
        }
        sync_clock();
    }

    setupActivitySensor(myIMU);
    //findGyroOffsets(myIMU);

    // Disconect wifi and bluethooth for powersaving
    disconctUnwantedServices();
    delay(100);
}


void loop(){

       
    float current_messurment = takeMessurment();
    
    Serial.println();
    Serial.print("current index: ");
    Serial.println(rtc_list_current_index);
    Serial.println();

    // update index counter
    rtc_list_current_index += 1;
    
    // if list is full send
    if (rtc_list_current_index >= RTC_LIST_SIZE){
        critical = 0;
    }
    // threshold value send
    else if (current_messurment < 1){
        critical = 1;
    }

    //normal send
    else { critical = 1337; }

    switch (critical)
    {
        
    case 0:  // normal send
        send = true;
        elements_to_send = RTC_LIST_SIZE;
        break;
        
    case 1:  // critcal send
        send = true;
        elements_to_send = rtc_list_current_index;
        break;
        
    default: // wake up and take messurment
        send = false;
        break;
    }


    delay(200);
    // if list is full
    if (send){
        
        // connect to wifi
        connectToWifi(WIFI_SSID, WIFI_PASS);

        // if wifi timed_out
        if (WiFi.status() != WL_CONNECTED){
            // throw package and start new sekvens
            rtc_list_current_index = 0;  // discus if it is better to not thorw package
            goToDeepSleep();
        }

        sync_clock();
        connectToUbidots();

        sendSavedActivityToUbidots(context, elements_to_send);

        // reset index counter
        rtc_list_current_index = 0;
    }

    
    // go sleep
    printList(rtc_saved_activity, RTC_LIST_SIZE);
    goToDeepSleep();
}

