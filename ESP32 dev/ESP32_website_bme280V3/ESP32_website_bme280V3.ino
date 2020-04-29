/*
  WORKING AP 2020
  Based on:
  https://randomnerdtutorials.com/cloud-weather-station-esp32-esp8266/#more-92139

     reads a setup file on the SD card "setup.js"
       
     ****** On upload, press and hold BOOT button till the bootloader is started
       
        ESP32  micro SD
        GND     GND
        +3.3v   Vcc
        D19     MISO
        D23     MOSI
        D18     SCK
        D05     CS
       
     * NOTE:   reads setup.js (see end of file for example), gets ssid, password, and location, then logs onto wifi network
               gets BME280 Temperature, Humidity, Pressure, adjusts time to Australia
               serves temp and humidity and pressure upto my website anthonyjparker.com/
*/
    // Import required libraries
   #include "FS.h"
   #include "SD.h"                         // CS is on G05
   #include "SPI.h"

   #include <WiFiMulti.h>
   //#include "WiFi.h"
   #include <WiFiUdp.h>
   #include "ESPAsyncWebServer.h"
   #include <ArduinoJson.h>
      
   #include <WiFi.h>
   #include <HTTPClient.h>
   
   #include <Wire.h>
   #include <Adafruit_Sensor.h>
   #include <Adafruit_BME280.h>

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
    
    char filename1[20] = "/setup.js";           // NOTE: SD card "SETUP.JS" filename
    void ReadSetupFile(char filename[20]);
    void listDir(fs::FS &fs, const char * dirname, uint8_t levels);

    WiFiMulti wifiMulti;
    // Create AsyncWebServer object on port 80
    AsyncWebServer server(80);
    
    File myFile;    //myFile (setup.js)
  
    //Your Domain name with URL path or IP address with path
    const char* serverName = "http://anthonyjparker.com/esp-post-data.php";
    
    // Keep this API Key value to be compatible with the PHP code provided in the project page.
    // If you change the apiKeyValue value, the PHP file /esp-post-data.php also needs to have the same key
    String apiKeyValue = "tPmAT5Ab3j7F9";
    String sensorName = "BME280";
    String sensorLocation = "Bullarto South";
    
    char sensLocatArray[5][40];                         // added Station Location array ***************
    
    #define SEALEVELPRESSURE_HPA (1013.25)
    
    Adafruit_BME280 bme;  // I2C
    
    // the following variables are unsigned longs because the time, measured in
    // milliseconds, will quickly become a bigger number than can be stored in an int.
    unsigned long lastTime = 0;
    // Timer set to 10 minutes (600000)
    unsigned long timerDelay = 600000;
    // Set timer to 30 seconds (30000)
    //unsigned long timerDelay = 30000;

void setup() {
    Serial.begin(115200);
    delay(100);
    // (you can also pass in a Wire library object like &Wire2)
    bool status;
    status = bme.begin(0x76);
    if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring or change I2C address!");
      while (1);
    }

    for (int i = 0; i < 5; i++) {
    strcpy(sensLocatArray[i],"Location 1"); // put text in the location array
    }
    
  //SD card setup
   if (!SD.begin()) {
        Serial.println("Card Mount Failed");
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
      Serial.printf("SD Card Size: %lluMB\n", cardSize);
    
      listDir(SD, "/", 0);

      Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
      Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    
      ReadSetupFile(filename1);
      delay(200);
      
      uint8_t stat = wifiMulti.run();
      while (stat != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
      if (stat == WL_CONNECTED) {
            Serial.println("");
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
       }
 
      Serial.print("Timer set to ");
      Serial.print(timerDelay / 1000);
      Serial.println(" seconds (timerDelay variable), it will take this many seconds before publishing the first reading.");
    }

void loop() {
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Prepare your HTTP POST request data
      String httpRequestData = "api_key=" + apiKeyValue + "&sensor=" + sensorName
                            + "&location=" + sensLocatArray[0] + "&value1=" + String(bme.readTemperature())
                            + "&value2=" + String(bme.readHumidity()) + "&value3=" + String(bme.readPressure()/100.0F) + "";
      Serial.print("httpRequestData: ");
      Serial.println(httpRequestData);

      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode>0) {
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
  }
}
// *****************************************************************************
// void ReadSetupFile(char filename[20]) (myFile)
// *****************************************************************************
    void ReadSetupFile(char filename[20]){
      //read network credentials from SD card
      myFile = SD.open(filename);//"/setup.js");
      if (!myFile) {
        Serial.println("setup.js file doesn't exist. Save it on SD card and restart.");
        return;
      }
      Serial.printf("setup.js file size: %d\r\n", myFile.size());
    
      //Arduino JSON uses a preallocated memory pool to store the JsonObject tree, this is done by the StaticJsonBuffer. You can use ArduinoJson Assistant to compute the exact buffer size, but for this example 200 is enough.
      const size_t capacity = JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(1) + 3 * JSON_OBJECT_SIZE(2) + 130;
      //DynamicJsonDocument doc(capacity);
      StaticJsonDocument<512> doc;
    
      char* json = new char [myFile.size() + 1];
      myFile.read((uint8_t*)json, myFile.size());
      json[myFile.size()] = 0;  //make a CString
      myFile.close();
    
      //const char* json = "{\"WifiNetwork\":[{\"ssid\":\"iiNet7F89AA\",\"password\":\"7E3CK5FQJTAY576\"},{\"ssid\":\"officenet\",\"password\":\"admin1\"},{\"ssid\":\"iPhone(Johny)\",\"password\":\"12345\"}]}";
      DeserializationError  err = deserializeJson(doc, json);
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
    
      if (wifi_net_no > 10) {
        Serial.println("too many WiFi networks on SD card, 10 max");
        wifi_net_no = 10;
      }
      for (int i = 0; i < wifi_net_no; i++) {
        String ssid = WifiNetwork[i]["ssid"];
        String password = WifiNetwork[i]["password"];
        const char* sensorLocation = WifiNetwork[i]["sensorLocation"];          // added Station Location***************
        strcpy(sensLocatArray[i],sensorLocation);                               // added Station Location***************
        
        wifiMulti.addAP(ssid.c_str(), password.c_str());
    
        //Serial.printf("WiFi %d: SSID: \"%s\" ; PASSWORD: \"%s\" ; LOCATION: \"%s\"\r\n", i, ssid.c_str(), password.c_str(), sensorLocation);
        Serial.printf("LOCATION: \"%s\"\r\n", i, sensorLocation);
      }
      Serial.print("Device location is: ");
      Serial.println(sensLocatArray[0]);
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
 *  {
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
 */
