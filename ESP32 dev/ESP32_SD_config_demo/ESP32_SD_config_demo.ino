/*
 * ESP32 reads SD card for a "/setup.js" file
	reads the wifi credentials and location
	prints toSerial the network we connected to and the coresponding Location/sensor name
      
	see end of file for example "/setup.js"
 * 
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

// Our configuration structure.
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

    const char compile_date[] = __DATE__ " " __TIME__;

    WiFiMulti wifiMulti;
    // Create AsyncWebServer object on port 80
    AsyncWebServer server(80);
    
    File myFile;    //myFile (setup.js)

    String sensorLocation = "Fafa island";
	char sensPSWRDArray[5][40];
	char sensSSIDArray[5][40];
    char sensLocatArray[5][40];                         // added Station Location array

  void setup() {
    Serial.begin(115200);

    for (int i = 0; i < 5; i++) {
    strcpy(sensPSWRDArray[i],"password");
    strcpy(sensSSIDArray[i],"123456789");
    strcpy(sensLocatArray[i],"Location 1"); // put text in the location array
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
    
      ReadSetupFile(filename1, wifi_conf);
      
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
	   
       Serial.print("checking device location.....");
	   
      for (int i = 0; i < 5; i++) {
        //Serial.print("checking device location.....");
        //Serial.println(i);
            if(strcmp(WiFi.SSID().c_str(), sensSSIDArray[i] ) == 0)  { // check which wifi we connected to and get site location
            Serial.print("Device location is: ");
            Serial.println(sensLocatArray[i]); 
          }
      }
 
      Serial.print("ESP32 SD config file demo programmed on: ");
      Serial.println(compile_date);
     
    }//~ end setup loop   

void loop() {
  // not used in this example
}

// *****************************************************************************
// void ReadSetupFile(const char* filename, WifiConfig &wifi_conf)
// *****************************************************************************
    void ReadSetupFile(const char* filename, WifiConfig &wifi_conf){
      //read network credentials from SD card
      myFile = SD.open(filename);                             //"/setup.js"
      if (!myFile) {
        Serial.println("setup.js file doesn't exist. Save it on SD card and restart.");
        return;
      }
      Serial.printf("/setup.js file size: %d\r\n", myFile.size());
    
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
        //String sensorLocation = WifiNetwork[i]["sensorLocation"];               // used and defined above for the SD array
        strcpy(sensSSIDArray[i],WifiNetwork[i]["ssid"]);
        strcpy(sensPSWRDArray[i],WifiNetwork[i]["password"]);
        strcpy(sensLocatArray[i],WifiNetwork[i]["sensorLocation"]);               // added Station Location***************
        
        wifiMulti.addAP(ssid.c_str(), password.c_str());
    
        Serial.printf("WiFi %d: SSID: \"%s\" ; PASSWORD: \"%s\" ; LOCATION: \"%s\"\r\n", i, ssid.c_str(), password.c_str(), sensorLocation.c_str());
      }

      myFile.close();
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

    /*
	Put on the SD card
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
