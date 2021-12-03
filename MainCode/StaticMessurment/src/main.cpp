#include <Arduino.h>
#include <WiFi.h>
#include <UbidotsEsp32Mqtt.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ubidots parameters BEGIN
const char *UBIDOTS_TOKEN = "BBFF-O7IdMgAv4p0AzptrTKIQGkQr1jTCZZ"; // TOKEN
const char *WIFI_SSID = "Hurodor";                                 // SSID
const char *WIFI_PASS = "123456789";                               //  password
char *DEVICE_LABEL = "Test";                                       //  Device label
char *VARIABLE_LABEL = "random_value";                             // Variable label

// context variabel  "key":"value"
char context[] = "\"test\":\"this_is_new\"";

Ubidots ubidots(UBIDOTS_TOKEN);

// Clock variables
#define NTP_SRV "time-a-b.nist.gov"
#define NTP_TIMEOUT 30  // seconds
#define TIME_TO_SLEEP 5 // seconds to deepsleep

unsigned int wifi_timeout = 30; // seconds
RTC_DATA_ATTR unsigned long lastUpdate = 0;

// current loop counter
RTC_DATA_ATTR int timer = 0;

// storing data between deepsleep:
const int RTC_LIST_SIZE = 20; // how many elements can be saved in rtc memory
RTC_DATA_ATTR int rtc_list_current_index = 0;

RTC_DATA_ATTR float rtc_saved_humidity[RTC_LIST_SIZE];
RTC_DATA_ATTR float rtc_saved_temperature[RTC_LIST_SIZE];
RTC_DATA_ATTR float rtc_saved_THI[RTC_LIST_SIZE];

RTC_DATA_ATTR unsigned long rtc_saved_timestamp[RTC_LIST_SIZE];

// messurments
#define SEALEVELPRESSURE_HPA (1013.25)
float THI;
float temp;
float hum;

// avg calculations
const int sample_size = 100; // how many samples each to calc avg from
float temp_array[sample_size];
float hum_array[sample_size];
float THI_array[sample_size];

const int delay_time = 500; // time between messurments while messuring

Adafruit_BME280 bme;

float avgOfArray(float array[], int array_size)
{
    double sum = 0;
    for (int i = 0; i < array_size; i++)
    {
        sum += array[i];
    }
    return sum / (double)array_size;
}

bool rtcArraysFull(int current_index, int max_size)
{
    if (current_index < max_size)
    {
        return false;
    }
    return true;
}

void connectToWifi(const char *WIFI_SSID, const char *WIFI_PASS)
{

    // local timeout variables
    int timer = 0;
    int timeout = wifi_timeout * 2;

    // try connect
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        timer += 1;
        if (timer >= timeout)
        {
            break;
        };
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println();
        Serial.println("Could not connect to wifi [wifi timed out]");
        return;
    }

    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());
}

void sync_clock()
{

    struct tm tm_now;
    configTime(0, 0, NTP_SRV); // UTC time

    if (!getLocalTime(&tm_now, NTP_TIMEOUT * 1000ULL))
    {
        Serial.println("Unable to sync with NTP server");
        return;
    }

    lastUpdate = time(NULL);
    Serial.println("NTP time updated");
}

void disconctUnwantedServices()
{
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

void connectToUbidots()
{

    ubidots.setup();
    ubidots.reconnect();
}

void pushToUbidots(char varable_label[], char device_label[], int value, char context[], unsigned long timestamp)
{

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

void setupSensor()
{
    Serial.println(F("BME280 test"));

    // default settings
    // (you can also pass in a Wire library object like &Wire2)
    bool status = bme.begin(0x76);
    if (!status)
    {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        goToDeepSleep(); // go sleep and try again when you wake up
    }

    Serial.println("-- Default Test --");
    Serial.println();
}

void takeMessurments()
{

    {
        for (int i = 0; i < sample_size; i++)
        {
            // read data
            hum = bme.readHumidity();
            temp = bme.readTemperature();
            THI = 0.8 * bme.readTemperature() + bme.readHumidity() * 0.01 * (bme.readTemperature() - 14.4) + 46.4;

            // store messurments in list
            hum_array[i] = hum;
            temp_array[i] = temp;
            THI_array[i] = THI;

            delay(delay_time); // time between messurments
        }

        // save avg of current messurments in rtc memory
        rtc_saved_humidity[rtc_list_current_index] = avgOfArray(hum_array, sample_size);
        rtc_saved_temperature[rtc_list_current_index] = avgOfArray(temp_array, sample_size);
        rtc_saved_THI[rtc_list_current_index] = avgOfArray(THI_array, sample_size);

        // save coresponding timestamp
        rtc_saved_timestamp[rtc_list_current_index] = time(NULL);
    }
}

void sendSavedDataToUbidots()
{
    // send only when full list
    // make sure connection to ubidots is estahblished before sending!
    for (int i = 0; i < RTC_LIST_SIZE; i++)
    {

        // push humidity
        pushToUbidots(
            VARIABLE_LABEL,
            DEVICE_LABEL,
            rtc_saved_humidity[i],
            context,
            rtc_saved_timestamp[i]);

        // push temeprature
        pushToUbidots(
            VARIABLE_LABEL,
            DEVICE_LABEL,
            rtc_saved_temperature[i],
            context,
            rtc_saved_timestamp[i]);

        // push THI
        pushToUbidots(
            VARIABLE_LABEL,
            DEVICE_LABEL,
            rtc_saved_THI[i],
            context,
            rtc_saved_timestamp[i]);
        // delay(delay_time);  delay in push-function
    }
}

void setup()
{
    Serial.begin(115200);

    // Disconect wifi and bluethoot for powersaving
    disconctUnwantedServices();

    // setup sensor
    setupSensor();
}

void loop()
{

    takeMessurments();
    Serial.print("current index");
    Serial.println(rtc_list_current_index);

    // if list is full
    if (rtc_list_current_index >= RTC_LIST_SIZE)
    {

        // connect to wifi
        connectToWifi(WIFI_SSID, WIFI_PASS);

        // if wifi timed_out
        if (WiFi.status() != WL_CONNECTED)
        {
            // throw package and start new sekvens
            rtc_list_current_index = 0; // discus if it is better to not thorw package
            goToDeepSleep();
        }

        // mabye only sync if needed
        sync_clock();
        connectToUbidots();

        // send data
        sendSavedDataToUbidots();

        // reset index counter
        rtc_list_current_index = 0;

        goToDeepSleep();
    }

    // update index counter
    rtc_list_current_index += 1;
    // go sleep
    goToDeepSleep();
}

