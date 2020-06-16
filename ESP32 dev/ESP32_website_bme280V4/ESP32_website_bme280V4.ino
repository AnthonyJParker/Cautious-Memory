/*
  Name: ESP32_website_bme280V4
  Purpose: reads setup.js (see end of file for example), for wifi details, (gets ssid, password, and location),
  then logs onto wifi network, gets BME280 Temperature, Humidity, Pressure,
  adjusts time to Australia, serves temp and humidity and pressure upto my website anthonyjparker.com/

 ****** On upload, press and hold BUT button till the bootloader is started

  Based on:  https://randomnerdtutorials.com/cloud-weather-station-esp32-esp8266/#more-92139
             Using ESP32 DevKit (Not ESP32 OLED, which has different SD libraries)
        ESP32  micro SD
        GND     GND
        +3.3v   Vcc (NOTE: microSD adapter uses 5v)
        D19     MISO
        D23     MOSI
        D18     SCK
        D05     CS

  @author Anthony Parker
  @version 3.0 12/06/2020

  NOTE: watchdog timer, corrections to Humidity, Working AP

  TODO: power save during deep sleep, 
*/
// Import required libraries
#include "FS.h"
#include "SD.h"                         // CS is on G05
#include "SPI.h"

#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>

#include <WiFi.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "esp_deep_sleep.h"

//BME correction factors for polynomial. For no correction set: B=1, D=C=A=0
#define CORR_D 0  //x^3 
#define CORR_C 0  //x^2
#define CORR_B  1 //x
#define CORR_A  0  //x^0

#define stat_LED 2 //pin of status LED (blue) on the ESP32 board, will be high when ESP active
//#define PINVCC 23 //pin of VCC of BME280 - will only briefly be high to power bme280

#define SEALEVELPRESSURE_HPA (1013.25)

// Deep sleep, based on https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
unsigned long time_to_sleep = 60;        // Time ESP32 will go to sleep (in seconds), before pushing another data value to cloud
unsigned long time_to_sleep_error = 10;  //in case of error try again faster

// Replace with your network credentials
typedef struct {                        // struct for the network details
  char ssid[30];
  char password[30];
  char sensorLocation[30];              // struct for the Station details, Name/Location
} WifiNetwork;

struct WifiConfig {                     // struct for the networks available
  WifiNetwork net[10];
};

WifiConfig wifi_conf;

int wifi_net_no = 0;

const char *filename1 = "/setup.js";           // NOTE: SD card "SETUP.JS" filename
void ReadSetupFile(const char *filename, WifiConfig &wifi_conf );
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void print_wakeup_reason();
void goToSleep(unsigned long sleepTime);
//float correctHumidity(float humMeasured);

// watchdog timer to reset in case it gets stuck, watchdog will trigger reset of ESP
// based on https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Timer/WatchdogTimer/WatchdogTimer.ino
const int wdtTimeout = 660000;  //time in ms to trigger the watchdog in case something gets stuck
hw_timer_t *timer = NULL;       // 10 + 1 min x 60 x 1000 = 660000
void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

const char compile_date[] = __DATE__ " " __TIME__;

WiFiMulti wifiMulti;
AsyncWebServer server(80);               // Create AsyncWebServer object on port 80

File myFile;    //myFile (setup.js)

//Your Domain name with URL path or IP address with path
const char* serverName = "http://anthonyjparker.com/esp-post-data.php";

// Keep this API Key value to be compatible with the PHP code provided in the project page.
// If you change the apiKeyValue value, the PHP file /esp-post-data.php also needs to have the same key
String apiKeyValue = "tPmAT5Ab3j7F9";

String sensorName = "BME280";
String sensorLocation = "Fafa Island";
String sensorHome = "deviceSpot";                   // for the http POST data to SQL
char sensLocatArray[5][40];                         // added Station SSID, PSWRD and Location array
char sensSSIDArray[5][40];
char sensPSWRDArray[5][40];

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;  // I2C
bool BMEworking = true; //if initalization fails, will be set to false later and error message pushed to server

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
unsigned long timerDelay = 600000;
// Set timer to 30 seconds (30000)
//unsigned long timerDelay = 30000;

void setup() {
  //set up watchdog, first thing // try to  move further up in code
  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt

  //btStop(); //stop bluetooth -does not need to consume current

  pinMode(stat_LED, OUTPUT);
  digitalWrite(stat_LED, HIGH); //turn on status LED

  Serial.begin(115200);
  bool status;
  status = bme.begin(0x76);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring or change I2C address!");
    //BMEworking = false;
    while (1);
  }

  if (BMEworking) {
    bme.setSampling(Adafruit_BME280::MODE_FORCED, // setting BME to weather station configuration
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
    }

  //Print the wakeup reason for ESP32
  //print_wakeup_reason();  //delete if compiled

  for (int i = 0; i < 5; i++) {
    strcpy(sensSSIDArray[i], "123456789");
    strcpy(sensPSWRDArray[i], "password");
    strcpy(sensLocatArray[i], "location 1"); // put text in the location array
  }

  //SD card setup
  if (!SD.begin()) {
    Serial.println(" ");
    Serial.println("***** Card Mount Failed *****");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %llu MB\n", cardSize);

  listDir(SD, "/", 0);

  Serial.printf("Total space: %llu MB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %llu MB\n", SD.usedBytes() / (1024 * 1024));

  ReadSetupFile(filename1, wifi_conf); // read "/setup.js" on the SD card and save wifi details

  uint8_t stat = wifiMulti.run();
  while (stat != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if (stat == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected to: ");
    Serial.println(WiFi.SSID());              // Tell us what network we're connected to
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  for (int i = 0; i < 5; i++) {
    Serial.print("checking device location.....");
    Serial.println(i);
    //if( WiFi.SSID() == sensSSIDArray[i] ) {
    if (strcmp(WiFi.SSID().c_str(), sensSSIDArray[i] ) == 0)  { // check which wifi we connected to and get site location
      Serial.print("Device location is: ");
      Serial.println(sensLocatArray[i]);
      sensorHome = sensLocatArray[i]; // Get actual sensor location/wifi network for the http POST data
    }
  }

  Serial.print("ESP32_website_bme280V4.ino programmed on: ");
  Serial.println(compile_date);
  Serial.print("The ESP32 DevKit has the WatchDog timer (wdtTimeout variable) set to ");
  Serial.print(wdtTimeout / (1000 * 60));
  Serial.println(" minutes.");
  Serial.println("Using a BME280 sensor set to weather station (Forced Mode).");
  Serial.print("It will take ");
  Serial.print(timerDelay / (1000 * 60));
  Serial.println(" minutes (timerDelay variable), before publishing the first reading.");


}//~ end setup loop

void loop() {
  timerWrite(timer, 0);                 //reset timer (feed watchdog)
  
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    digitalWrite(stat_LED, HIGH);
    bme.takeForcedMeasurement();       // has no effect in normal mode, needed for Forced Mode
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Prepare your HTTP POST request data
      String httpRequestData = "api_key=" + apiKeyValue + "&sensor=" + sensorName
                               + "&location=" + sensorHome + "&value1=" + String(bme.readTemperature())
                               + "&value2=" + String(correctHumidity(bme.readHumidity())) + "&value3=" + String(bme.readPressure() / 100.0F) + "";
      Serial.print("httpRequestData: ");
      Serial.println(httpRequestData);

      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
    digitalWrite(stat_LED, LOW);
  }
  //goToSleep(time_to_sleep);

}//~ end loop

//***************************** Functions ***********************************************//
//***************************************************************************************//

// *****************************************************************************
// void ReadSetupFile(const char* filename, WifiConfig &wifi_conf)
// *****************************************************************************
void ReadSetupFile(const char* filename, WifiConfig &wifi_conf) {
  //read network credentials from SD card
  myFile = SD.open(filename);                    //"/setup.js"
  if (!myFile) {
    Serial.println("setup.js file doesn't exist. Save it on SD card and restart.");
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
    strcpy(sensLocatArray[i], WifiNetwork[i]["sensorLocation"]);                              // added Station Location***************

    wifiMulti.addAP(ssid.c_str(), password.c_str());

    Serial.printf("WiFi %d: SSID: \"%s\" ; PASSWORD: \"%s\" ; LOCATION: \"%s\"\r\n", i, ssid.c_str(), password.c_str(), sensorLocation.c_str());
    //Serial.printf("LOCATION: \"%s\"\r\n", i, sensorLocation.c_str());
  }
  myFile.close();
}
// *****************************************************************************
// void print_wakeup_reason()
// *****************************************************************************
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

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
// *****************************************************************************
// void goToSleep(unsigned long sleepTime)
// *****************************************************************************
void goToSleep(unsigned long sleepTime)   {
  esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
  esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF); //all RTC Peripherals to be powered down in sleep

  WiFi.mode(WIFI_OFF); //https://esp32.com/viewtopic.php?t=6744
  // btStop(); //stop bluetooth -does not need to consume current //delefe if compiles without error
  esp_deep_sleep_start();
}
// *****************************************************************************
// float correctHumidity(float humMeasured)
// *****************************************************************************
float correctHumidity(float humMeasured) {
  //  corrected = d*x^3 + c*x^2 + b*x + a; x=measured humidity
  float correctedHumidity = CORR_D * pow(humMeasured, 3) + CORR_C * pow(humMeasured, 2) + CORR_B * pow(humMeasured, 1) + CORR_A;

  return correctedHumidity;
}
// *****************************************************************************
// void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
// *****************************************************************************
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
                "sensorLocation":"Yarravile"
        },
        {
                "ssid":"iPhone(Johny)",
                "password":"12345",
                "sensorLocation":"Pontrieux"
        }
    ]
  }
*/
