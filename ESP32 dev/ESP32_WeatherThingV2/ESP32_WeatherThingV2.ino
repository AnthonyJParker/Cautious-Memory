/*
  Name       : ESP32_WeatherThingV2         ex, ESP32_website_bme280_deepsleepV4
  Description: reads setup.js (see end of file for example), for wifi details, (gets ssid, password, and location),
               then logs onto wifi network, gets Temperature, Humidity, Pressure,
               adjusts time to Australia, serves temp and humidity and pressure upto my website anthonyjparker.com/
  Based on   :  https://randomnerdtutorials.com/cloud-weather-station-esp32-esp8266/#more-92139
  Board      : ESP32 DevKit 30 pin (Not ESP32 OLED, which has different SD libraries)

  IDE        : Arduino_1.8.13
  @author Anthony Parker, Working July 2020
  @version 0.5 2/08/2020

  NOTE: saves ssid / password / location to RTC memory preserved across deep sleep
        and will use these if the SD fails
        saves data and wakeup reason to LOG.TXT, also contacts Time Server for time/day/date
  TODO: comment out line:188 print_wakeup_reason
        see lines: 62,63,64 (WiFi hard code credentials) 
        see lines: 214 (Hard code WiFi credential setup) 215,216,217 (SD card WiFi setup file) and 284 (save data to Log file on SD)
        ESP32 EZSBC Dev board: R Led (16), G Led (17), B Led (18),
        https://www.ezsbc.com/index.php/wifi01-bat.html#.Xv7SASgzbb0

        DHT22 sensor powered by ESP32 pins, -ve(13), data(14), +ve(15)
        BMP280 in place of BME280
  ****** On upload, press and hold BUT button till the bootloader is started
*/
// Import required libraries
#include "FS.h"                     // SD card
#include "SD.h"                     // CS is on G05
#include "SPI.h"
#include <NTPClient.h>              // Time Server
#include <WiFiMulti.h>              // multiple WiFi Networks
#include <WiFiUdp.h>
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>            // read a setup file "/setup.js"
#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>                // DS18 temp sensor
#include <Wire.h>
#include <Adafruit_BME280.h>        // BME280 temp/hunidity/pressuresensor
#include <Adafruit_BMP280.h>        // BMP280 temp/pressure sensor
#include <rom/rtc.h>
#include "esp_deep_sleep.h"
#include "driver/adc.h"             // to turn off adc for lowest power usage

#include <DHT.h>                    // DHT22 temp/humidity sensor
#include <DHT_U.h>
#define DHTPIN 14                   // Digital pin connected to the DHT sensor (see below for power pins 13, 14, 15) 
#define DHTTYPE    DHT22

//BME correction factors for polynomial. For no correction set: B=1, D=C=A=0
#define CORR_D 0                    // x^3 
#define CORR_C 0                    // x^2
#define CORR_B  1                   // x
#define CORR_A  0                   // x^0

#define stat_LED 2                  // pin of status LED (blue) on the ESP32 board, will be high when ESP active
#define PINVCC1 25                  // 23//pin of VCC of BME280 - will only briefly be high to power bme280 (not used yet)
#define PINVCC2 26                  // pin of VCC of BME280 - will only briefly be high to power bme280
#define PINDHTP 15                  // pins for a DHT22 sensor
#define PINDHTN 13
#define SEALEVELPRESSURE_HPA (1013.25)

// Wifi credentials 
const char *WIFI_SSID = "iiNet7F89AA";
const char *WIFI_PASSWORD = "7E3CK5FQJTAY576";
const char *WIFI_LOCATION = "Bullarto_South";

// Deep sleep, based on https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
unsigned long time_to_sleep = 600;      // Time ESP32 will go to sleep (in seconds), before pushing another data value to cloud
unsigned long time_to_sleep_error = 10; // in case of deepsleep error try again faster

float Temp_temp;                        // variables to store the temp etc
float Temp_humidity;
float Temp_pressure;
float Temp_altitude;
float TEMPD_CORRECT = 0.36;              // temp correction for: DHT22(-0.36), BME280(-1.66), BMP280(-0.34)
float TEMPP_CORRECT = 0.34;
float TEMPE_CORRECT = 1.66;

// struct for the network details
typedef struct {                        // struct for the network details
  char ssid[30];
  char password[30];
  char sensorLocation[30];              // struct for the Station details, Name or Location
} WifiNetwork;

struct WifiConfig {                     // struct for the networks available
  WifiNetwork net[10];
};

WifiConfig wifi_conf;
int wifi_net_no = 0;

const char *p_project = "ESP32_WeatherThingV2.ino";             // Project name
const uint8_t version_hi = 0;                                   // Project version
const uint8_t version_lo = 5;                                   // Project revision
const char compile_date[] = __DATE__ " " __TIME__;

const char *filename1 = "/setup.js";                            // SD card "SETUP.JS" filename contains Network credentials
const char *filename2 = "/log.txt";                             // log filename on SD card

void ReadSetupFile(const char *filename, WifiConfig &wifi_conf );
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void getTimeStamp();
void write_txt(const char* filename);
void print_wakeup_reason();
void goToSleep(unsigned long sleepTime);
void SD_setup();
void WiFiMulti_connect();     // read setup file on SD
void WiFi_connect();          // hard coded WiFi credentials
//void DS18S_setup();
void DHT22_setup();
void BMP280_setup();
void BME280_setup();
//void DS18S_sample();
void DHT22_sample();
void BMP280_sample();
void BME280_sample();

// watchdog timer to reset in case it gets stuck, watchdog will trigger reset of ESP
// based on https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Timer/WatchdogTimer/WatchdogTimer.ino
const int wdtTimeout = 660000;                // time in ms to trigger the watchdog in case something gets stuck
hw_timer_t *timer = NULL;                     // 10 + 1 min x 60 x 1000 = 660000
void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

WiFiMulti wifiMulti;
AsyncWebServer server(80);                    // Create AsyncWebServer object on port 80

File myFile, myFile2;                         // myFile(setup.js), myfile2(LOG.TXT)

//Your Domain name with URL path or IP address with path
const char* serverName = "http://my_server_data.php";

// Keep this API Key value to be compatible with the PHP code provided in the project page.
// If you change the apiKeyValue value, the PHP file /esp-post-data.php also needs to have the same key
String apiKeyValue = "tPmAT5Ab3j7F9";

String sensorName     = "BME280";             // for the http POST data to SQL
String sensorLocation = "Fafa Island";
String sensorHome     = "DeviceSpot";
char sensLocatArray[5][40];                   // added Station SSID, PSWRD and Location array
char sensSSIDArray[5][40];
char sensPSWRDArray[5][40];

String Wakeup_cause;                          // variable to hole wakeup_reason for LOG.TXT
RTC_DATA_ATTR char RTC_ssid[33];
RTC_DATA_ATTR char RTC_password[33];
RTC_DATA_ATTR char RTC_location[33];

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

Adafruit_BMP280 bmp; // use I2C interface
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();

Adafruit_BME280 bme;                          // I2C (address 0x76)
bool BMEworking = true;

int bme_flag = 0;                             // flags to set if sensor exists
int bmp_flag = 0;
int dht_flag = 0;
int ds8_flag = 0;

WiFiUDP ntpUDP;                               // Define NTP Client to get time
NTPClient timeClient(ntpUDP);                 // NTPClient timeClient(ntpUDP, "oceania.pool.ntp.org", 36000, 60000);

String dayStamp;                              // Variables to save date and time
String timeStamp;

void splash(void)
{
  Serial.println(" ");
  Serial.print("AJP Project: ");
  Serial.println(p_project);
  Serial.print("Version ");
  Serial.print(version_hi);
  Serial.print('.');
  Serial.print(version_lo);
  Serial.print(" Compiled: ");
  Serial.println(compile_date);
  Serial.println(" ");
}

void setup() {
  //set up watchdog, first thing                    // try to  move further up in code
  timer = timerBegin(0, 80, true);                  // timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  // attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); // set time in us
  timerAlarmEnable(timer);                          // enable interrupt

  btStop();                                         // stop bluetooth -does not need to consume current

  pinMode(stat_LED, OUTPUT);
  digitalWrite(stat_LED, HIGH);                     // turn on status LED
  //pinMode(PINVCC1, OUTPUT);
  //digitalWrite(PINVCC1, HIGH);                    // turn on SD/BME280 VCC pin1
  pinMode(PINDHTP, OUTPUT);
  digitalWrite(PINDHTP, HIGH);                      // turn on DHT positive pin
  pinMode(PINDHTN, OUTPUT);
  digitalWrite(PINDHTN, LOW);                       // set DHT negitive pin

  Serial.begin(115200);
  splash();
  DHT22_setup();
  BMP280_setup();
  BME280_setup();

  print_wakeup_reason();                      // delete if compiled, Print the wakeup reason for ESP32
  WiFi_connect();                            // Hard code the WiFi credentials
  // SD_setup();
  // ReadSetupFile(filename1, wifi_conf);               // read "/setup.js" on the SD card and save wifi details
  // WiFiMulti_connect();

  timeClient.begin();                                 // Initialize a NTPClient to get time
  timeClient.setTimeOffset(36000);// Set offset time in seconds for your timezone: GMT+10 = 36000, GMT+11 = 39600, GMT 0 = 0
  Serial.print("Contacting Time Server....");
  getTimeStamp();
  timeClient.update();

  Serial.println(" ");
  Serial.print("The ESP32 DevKit has the WatchDog timer (wdtTimeout variable) set to ");
  Serial.print(wdtTimeout / (1000 * 60));
  Serial.println(" minutes.");
  Serial.println("Using various sensors, DHT22, BME280, BMP280 uploading to web via HTTP POST.");
  Serial.println("DHT22 on pins -ve(13), data(14), +ve(15) ");
  Serial.println("BMP280 on pins SDA(21), SCL(22) ");
  Serial.println("BME280 on pins SDA(21), SCL(22) ");

  delay(500);
}//~ end setup

void loop() {
  timerWrite(timer, 0);                           // reset timer (feed watchdog)

  //Send an HTTP POST request (at the wake up from DeepSleep)
  digitalWrite(stat_LED, HIGH);

  DHT22_sample();
  BMP280_sample();
  BME280_sample();

  // need to check that we have a valid temperature, humidity, pressure

  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);                          // Your Domain name with URL path or IP address with path

    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // float seaLevelForAltitude(float altitude, float pressure);
    // Prepare your HTTP POST request data
    String httpRequestData = "api_key=" + apiKeyValue + "&sensor=" + sensorName
                             + "&location=" + sensorHome + "&value1=" + String(Temp_temp)
                             + "&value2=" + String(correctHumidity(Temp_humidity)) + "&value3=" + String(Temp_pressure) + "";
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);

    int httpResponseCode = http.POST(httpRequestData); // Send HTTP POST request

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();                                        // Free resources
  }
  else {
    Serial.println("WiFi Disconnected");
  }
  digitalWrite(stat_LED, LOW);
  getTimeStamp();                                      // get date / time

  // write_txt(filename2);                                // write data to LOG.TXT

  //digitalWrite(PINVCC1, LOW);                        // turn off SD/BME280 VCC pin2
  adc_power_off();                                     // turn off adc for power saving
  goToSleep(time_to_sleep);
}//~ end loop


/*********************************  FUNCTIONS  ****************************************/

/***************************************************************************************
   void ReadSetupFile(const char* filename, WifiConfig &wifi_conf)
   Requires: SD card
              #include "FS.h"
              #include "SD.h"                         // CS is on G05
              #include "SPI.h"
              const char *filename1 = "/setup.js";
              File myFile
***************************************************************************************/
void ReadSetupFile(const char* filename, WifiConfig &wifi_conf) {
  for (int i = 0; i < 5; i++) {
    strcpy(sensSSIDArray[i], "123456789");
    strcpy(sensPSWRDArray[i], "password");
    strcpy(sensLocatArray[i], "location 1");         // put text in the location array
  }

  //read network credentials from SD card
  myFile = SD.open(filename);                      // "/setup.js"
  if (!myFile) {                                   // If no SD card
    Serial.println("setup.js file doesn't exist. Save it on the SD card.");
    Serial.print("Retrieved from RTC memory: ");   // If no SD card attached get ssid, password and location from RTC memory
    Serial.println(RTC_ssid);
    Serial.print("Retrieved from RTC memory: ");
    Serial.println(RTC_password);
    Serial.print("Retrieved from RTC memory: ");
    Serial.println(RTC_location);
    strcpy(sensLocatArray[0], RTC_location);
    strcpy(sensLocatArray[1], RTC_location);
    strcpy(sensLocatArray[2], RTC_location);
    wifiMulti.addAP(RTC_ssid, RTC_password);       // add retrieved ssid and password to WiFimulti
    return;
  }
  Serial.printf("setup.js file size: %d\r\n", myFile.size());

  //Arduino JSON uses a preallocated memory pool to store the JsonObject tree, this is done by the StaticJsonBuffer. You can use ArduinoJson Assistant to compute the exact buffer size, but for this example 200 is enough.
  const size_t capacity = JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(1) + 3 * JSON_OBJECT_SIZE(2) + 130;
  //DynamicJsonDocument doc(capacity);
  StaticJsonDocument<512> doc;

  DeserializationError  err = deserializeJson(doc, myFile);
  switch (err.code())
  {
    case DeserializationError::Ok:
      Serial.print(F("Deserialization succeeded"));
      break;
    case DeserializationError::InvalidInput:
      Serial.print(F("Invalid input!"));
      break;
    case DeserializationError::NoMemory:
      Serial.print(F("Not enough memory"));
      break;
    default:
      Serial.print(F("Deserialization failed"));
      break;
  }

  JsonArray WifiNetwork = doc["WifiNetwork"];

  int wifi_net_no = WifiNetwork.size();
  Serial.println();
  Serial.printf("number of Wi-Fi networks: %d\r\n", wifi_net_no);

  if (wifi_net_no > 5) {
    Serial.println("too many WiFi networks on SD card, 5 max");
    wifi_net_no = 5;
  }
  for (int i = 0; i < wifi_net_no; i++) {
    String ssid = WifiNetwork[i]["ssid"];
    String password = WifiNetwork[i]["password"];
    String sensorLocation = WifiNetwork[i]["sensorLocation"];
    strcpy(sensSSIDArray[i], WifiNetwork[i]["ssid"]);
    strcpy(sensPSWRDArray[i], WifiNetwork[i]["password"]);
    strcpy(sensLocatArray[i], WifiNetwork[i]["sensorLocation"]);                // added Station Location

    wifiMulti.addAP(ssid.c_str(), password.c_str());                            // add list of ssid/passwords to WiFi multi

    Serial.printf("WiFi %d: SSID: \"%s\" ; PASSWORD: \"%s\" ; LOCATION: \"%s\"\r\n", i, ssid.c_str(), password.c_str(), sensorLocation.c_str());
    //Serial.printf("LOCATION: \"%s\"\r\n", i, sensorLocation.c_str());
  }
  myFile.close();
}
/***************************************************************************************
   void print_wakeup_reason()
   Requires:

***************************************************************************************/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  Wakeup_cause = wakeup_reason;                              // variable for wakeup_reason on LOG.TXT
  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

/***************************************************************************************
   void goToSleep(unsigned long sleepTime)
   Requires:

***************************************************************************************/
void goToSleep(unsigned long sleepTime)   {
  Serial.print("Going into DeepSleep for ");
  Serial.print(time_to_sleep / 60);
  Serial.println(" minutes");

  WiFi.disconnect(true);//https://www.savjee.be/2019/12/esp32-tips-to-increase-battery-life/
  WiFi.mode(WIFI_OFF);  //
  btStop();             //
  adc_power_off();      //
  //esp_wifi_stop();      //
  //esp_bt_controller_disable();//

  esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
  //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF); //all RTC Peripherals to be powered down in sleep
  //WiFi.mode(WIFI_OFF); //https://esp32.com/viewtopic.php?t=6744
  // btStop(); //stop bluetooth -does not need to consume current, delefe if compiles without error
  esp_deep_sleep_start();
}

/***************************************************************************************
   float correctHumidity(float humMeasured)
   Requires:

***************************************************************************************/
float correctHumidity(float humMeasured) {
  //  corrected = d*x^3 + c*x^2 + b*x + a; x=measured humidity
  float correctedHumidity = CORR_D * pow(humMeasured, 3) + CORR_C * pow(humMeasured, 2) + CORR_B * pow(humMeasured, 1) + CORR_A;

  return correctedHumidity;
}

/***************************************************************************************
   void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
   Requires:
              #include "FS.h"
              #include "SD.h"                         // CS is on G05
              #include "SPI.h"
              const char *filename1 = "/setup.js";
              File myFile
***************************************************************************************/
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

/***************************************************************************************
   void getTimeStamp()  Function to get date and time from NTPClient
   Requires:

***************************************************************************************/
void getTimeStamp() {
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The time comes with the following format:
  //getDay  KEYWORD2, getHours  KEYWORD2, getMinutes  KEYWORD2, getSeconds  KEYWORD2, getFormattedTime  KEYWORD2
  dayStamp = timeClient.getDay();

  if ( dayStamp == "0") {
    dayStamp = "Sunday";
  }
  if ( dayStamp == "1") {
    dayStamp = "Monday";
  }
  if ( dayStamp == "2") {
    dayStamp = "Tuesday";
  }
  if ( dayStamp == "3") {
    dayStamp = "Wednesday";
  }
  if ( dayStamp == "4") {
    dayStamp = "Thursday";
  }
  if ( dayStamp == "5") {
    dayStamp = "Friday";
  }
  if ( dayStamp == "6") {
    dayStamp = "Saturday";
  }

  Serial.print(dayStamp);
  Serial.print(" : ");
  timeStamp = timeClient.getFormattedTime();
  Serial.println(timeStamp);
}

/***************************************************************************************
  void write_txt(const char* filename) write temp/humidity data to the text file (myFile2)
  Requires:
            #include "FS.h"
            #include "SD.h"                         // CS is on G05
            #include "SPI.h"
            const char *filename1 = "/lpg.txt";
            File myFile
***************************************************************************************/
void write_txt(const char* filename) {
  char buf[256];
  myFile2 = SD.open(filename, FILE_APPEND);

  if (myFile2) {
    Serial.print("Writing data to LOG.TXT....");
    myFile2.print(dayStamp);
    myFile2.print(",");
    myFile2.print(timeStamp);
    myFile2.print(",");
    myFile2.print(String(Temp_temp));
    myFile2.print(",");
    myFile2.print(String(correctHumidity(Temp_humidity)));
    myFile2.print(",");
    myFile2.print(String(Temp_pressure));
    myFile2.print(",");
    myFile2.print(Wakeup_cause);
    myFile2.println(" ");
    myFile2.close();
    delay(50);
    Serial.println("done");
  } else {
    Serial.println("error opening LOG.TXT file.....");                        // if the file didn't open, print an error:
  } // end text file loop
}//~

/***************************************************************************************
   void SD_setup()
   Requires:
            #include "FS.h"
            #include "SD.h"                         // CS is on G05
            #include "SPI.h"
***************************************************************************************/
void SD_setup() {
  if (!SD.begin()) {                                 // SD card setup
    Serial.println(" ");
    Serial.println("***** Card Mount Failed *****");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.printf("Total space: %llu MB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space : %llu MB\n", SD.usedBytes() / (1024 * 1024));
}

/***************************************************************************************
   void WiFiMulti_connect()
   Requires:
            #include
***************************************************************************************/
void WiFiMulti_connect() {

  uint8_t stat = wifiMulti.run();                    // connect to nearest WiFi network from setup file
  while (stat != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if (stat == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected to: ");
    Serial.println(WiFi.SSID());                     // Tell us what network we're connected to
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  for (int i = 0; i < 5; i++) {
    Serial.print("Checking device location...");     // Tell us what sensor loacation
    Serial.print(i);

    if (strcmp(WiFi.SSID().c_str(), sensSSIDArray[i] ) == 0)  { // check which wifi we connected to and get site location
      Serial.print(" Device location is: ");
      Serial.println(sensLocatArray[i]);
      sensorHome = sensLocatArray[i];                 // Get actual sensor location/wifi network for the http POST data
      strcpy(RTC_ssid, sensSSIDArray[i]);             // save ssid and password to RTC memory
      //Serial.print("RTC memory ssid saved: ");
      //Serial.println(RTC_ssid);
      strcpy(RTC_password, sensPSWRDArray[i]);        // save ssid and password to RTC memory
      //Serial.print("RTC memory password saved: ");
      //Serial.println(RTC_password);
      strcpy(RTC_location, sensLocatArray[i]);        // save ssid and password to RTC memory
      //Serial.print("RTC memory location saved: ");
      //Serial.println(RTC_location);
      Serial.println("");
      break;
    }
  }
}
/***************************************************************************************
   void WiFi_connect()
   Requires:
              #include <Adafruit_Sensor.h>

***************************************************************************************/
void WiFi_connect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Only try 15 times to connect to the WiFi
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 15) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print(F("WiFi connected to: "));
    Serial.println(WiFi.SSID());                     // Tell us what network we're connected to
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("Device location is: "));
    Serial.println(WIFI_LOCATION);
  }

  // If we still couldn't connect to the WiFi, go to deep sleep for a minute and try again.
  if (WiFi.status() != WL_CONNECTED) {
    esp_sleep_enable_timer_wakeup(1 * 60L * 1000000L);
    esp_deep_sleep_start();
  }
  sensorHome = WIFI_LOCATION;
}
/***************************************************************************************
   void DHT22_setup()
   Requires:
              #include <Adafruit_Sensor.h>
              #include <DHT.h>
              #include <DHT_U.h>
              #define DHTPIN 2
              #define DHTTYPE    DHT22
              DHT_Unified dht(DHTPIN, DHTTYPE);
              uint32_t delayMS;
***************************************************************************************/
void DHT22_setup() {

  dht.begin();
  Serial.print(F("DHT22 Unified Sensor setup...."));
  sensor_t sensor;                                   // Print temperature sensor details.
  dht.temperature().getSensor(&sensor);
  /*
    Serial.println(F("Temperature Sensor"));
    Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
    Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
    Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
    Serial.println(F("------------------------------------"));
  */
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  /*
    Serial.println(F("Humidity Sensor"));
    Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
    Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
    Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  */
  Serial.println("ok");
  delayMS = sensor.min_delay / 1000;          // Set delay between sensor readings based on sensor details.
}

/***************************************************************************************
   void BMP280_setup()
   Requires:
              #include <Wire.h>
              #include <Adafruit_Sensor.h>
              #include <Adafruit_BMP280.h>
              #define SEALEVELPRESSURE_HPA (1013.25)
              Adafruit_BMP280 bmp; // use I2C interface
              Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
              Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
***************************************************************************************/
void BMP280_setup() {
  bool status;
  status = bmp.begin();
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    bmp_flag = 1;
  } else {

    /* Default settings from datasheet. */
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

    Serial.print(F("BMP280 Sensor setup...."));
    Serial.println("ok");
  }
}

/***************************************************************************************
   void BME280_setup()
   Requires:
              #include <Wire.h>
              #include <Adafruit_Sensor.h>
              #include <Adafruit_BME280.h>
              #define SEALEVELPRESSURE_HPA (1013.25)
              Adafruit_BME280 bme;                          // I2C
              bool BMEworking = true;
***************************************************************************************/
void BME280_setup() {
  bool status;
  status = bme.begin(0x76);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring or change I2C address!");
    bme_flag = 1;
  } else {

    if (BMEworking) {
      bme.setSampling(Adafruit_BME280::MODE_FORCED, // setting BME to weather station configuration
                      Adafruit_BME280::SAMPLING_X1, // temperature
                      Adafruit_BME280::SAMPLING_X1, // pressure
                      Adafruit_BME280::SAMPLING_X1, // humidity
                      Adafruit_BME280::FILTER_OFF   );
    }

    Serial.print(F("BME280 Sensor setup...."));
    Serial.println("ok");
  }
}

/***************************************************************************************
   void DHT22_sample()
   Requires:

***************************************************************************************/
void DHT22_sample() {
  sensors_event_t event; // Get temperature event and print its value.
  dht.temperature().getEvent(&event);

  if (isnan(event.temperature)) {
    Serial.println(F("------------------------------------"));
    Serial.println("No DHT22 sensor");
    Serial.println(F("------------------------------------"));
    dht_flag = 1;
  }
  else {
    Serial.println(F("------------------------------------"));
    Serial.println(F("DHT22 Sensor sample"));
    Serial.print(F("Temperature   : "));
    Temp_temp = event.temperature - TEMPD_CORRECT;
    Serial.print(Temp_temp);
    Serial.println(F("°C"));
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    //Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity      : "));
    Temp_humidity = event.relative_humidity;
    Serial.print(Temp_humidity);
    Serial.println(F("%"));

    Serial.print(F("Dummy pressure: "));
    //Temp_pressure = 900;                                       // dummy value if DHT22 is the only sensor
    //Serial.print(Temp_pressure);
    Serial.println(F("hPa"));
    Serial.println(F("------------------------------------"));
  }
}

/***************************************************************************************
   void BMP280_sample()
   Requires:

***************************************************************************************/
void BMP280_sample() {
  sensors_event_t temp_event, pressure_event;
  bmp_temp->getEvent(&temp_event);
  bmp_pressure->getEvent(&pressure_event);

  if (bmp_flag == 1) {
    Serial.println(F("------------------------------------"));
    Serial.println("No BMP280 sensor");
    Serial.println(F("------------------------------------"));
  } else {
    Serial.println(F("------------------------------------"));
    Serial.println(F("BMP280 Sensor sample"));
    Serial.print(F("Temperature   : "));
    Temp_temp = temp_event.temperature - TEMPP_CORRECT;
    Serial.print(Temp_temp);
    Serial.println(" °C");

    Serial.print(F("Dummy humidity: "));
    //Temp_humidity = 99;                                       // dummy value if BMP280 is the only sensor
    //Serial.print(Temp_humidity);
    Serial.println(F("%"));

    Serial.print(F("Pressure      : "));
    Temp_pressure = pressure_event.pressure;
    Serial.print(Temp_pressure);
    Serial.println(" hPa");
    Serial.println(F("------------------------------------"));
  }
}

/***************************************************************************************
   void BME280_sample()
   Requires:

***************************************************************************************/
void BME280_sample() {

  bme.takeForcedMeasurement();         // has no effect in normal mode, needed for Forced Mode

  if (bme_flag == 1) {
    Serial.println(F("------------------------------------"));
    Serial.println("No BME280 sensor");
    Serial.println(F("------------------------------------"));
  } else {
    Serial.println(F("------------------------------------"));
    Serial.println(F("BME280 Sensor sample"));
    Serial.print(F("Temperature: "));
    Temp_temp = bme.readTemperature() - TEMPE_CORRECT;
    Serial.print(Temp_temp);
    Serial.println(" °C");
    Serial.print(F("Humidity   : "));
    Temp_humidity = bme.readHumidity();
    Serial.print(Temp_humidity);
    Serial.println(" %RH");
    Serial.print(F("Pressure   : "));
    Temp_pressure = (bme.readPressure() / 100.0F);
    Serial.print(Temp_pressure);
    Serial.println(" hPa");
    Serial.println(F("------------------------------------"));
  }
}
/* example setup.js
    {
    "WifiNetwork":[
        {
                "ssid":"iiNet7F89AA",
                "password":"7E3CK5FQJTAY576",
                "sensorLocation":"Bullarto South"
        },
        {
                "ssid":"officenet",
                "password":"admin1",
                "sensorLocation":"Yarraville"
        },
        {
                "ssid":"iPhone(Johny)",
                "password":"12345",
                "sensorLocation":"Pontrieux"
        }
    ]
  }

  also connections for the SD
        ESP32  micro SD
        GND     GND
        +3.3v   Vcc (NOTE: microSD adapter uses 5v)
        D19     MISO
        D23     MOSI
        D18     SCK
        D05     CS

*/
