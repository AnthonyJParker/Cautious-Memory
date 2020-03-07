    /*********
      Complete project details at https://RandomNerdTutorials.com
      NOTE:
      ESP32 Plot BME280 Sensor data.docx
    
     reads a setup file, reads a index.html on the SD card,
       
     ****** On upload, press and hold BUT button till the bootloader is started
       
        ESP32  micro SD
        GND     GND
        +3.3v   Vcc
        D19     MISO
        D23     MOSI
        D18     SCK
        D05     CS
       
       see end of file below for the example html
       then logs onto wifi network
       set BMP280 Temperature, Humidity, Pressure
       saves temp and humidity, pressure data on SD at 5000s interval
       serves temp and humidity and pressure
    
       {
        "WifiNetwork":[
            {
                "ssid":"iiNet7F89AA",
                "password":"7E3CK5FQJTAY576"
                "StatName":"Anthony"                   // struct for the station details, name/location
                "StatLocation":"Bullarto South"
            },
            {
                "ssid":"officenet",
                "password":"admin1"
                "StatName":"Work"                   // struct for the station details, name/location
                "StatLocation":"Footscray"
            },
            {
                "ssid":"iPhone(Johny)",
                "password":"12345"
                "StatName":"Simon"                   // struct for the station details, name/location
                "StatLocation":"Yarraville"
            }
        ]
      }
    * NOTE: 
    * 
    * TODO use internet time to timestamp file create and date and time when saving temperature and humidity data
    */
    // Import required libraries
      #include "FS.h"
      #include "SD.h"                         // CS is on G05
      #include "SPI.h"
      
      #include <WiFiMulti.h>
      #include "WiFi.h"
      #include <NTPClient.h>
      #include <WiFiUdp.h>
      #include "ESPAsyncWebServer.h"
      #include <ArduinoJson.h>
    
      #include <Wire.h>
      #include <Adafruit_Sensor.h>
      #include <Adafruit_BME280.h>
    
    Adafruit_BME280 bme; // I2C
    
    // Replace with your network credentials
    typedef struct {                        // struct for the network details
      char ssid[30];
      char password[30];
      char StatName[30];                    // struct for the Station details, Name/Location
      char StatLocation[30];
    } WifiNetwork;
    
    struct WifiConfig {                     // struct for the networks available
      WifiNetwork net[10];
    };
    
    WifiConfig wifi_conf;
    
    int wifi_net_no = 0;
    char* index_html;
    
    void Update_File_Exist(char filename[20]); // NOTE: file name, Update_File_Exist(kmlname,1); i.e. kmlname, myFile1 update
    char filename1[20] = "/setup.js";           // NOTE: SD card "SETUP.JS" filename
    void ReadSetupFile(char filename[20]);
    void open_txt();                           // text temperature file header
    char textname[20] = "/BME2000.TXT";       // NOTE: SD card filename
    void write_txt();
    char filename2[20] = "/index.html";           // NOTE: SD card "index.html" filename
    void ReadHtmlFile(char filename[20]);
    void getTimeStamp();
    
    void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
    void listDirTime(fs::FS &fs, const char * dirname, uint8_t levels);
    void createDir(fs::FS &fs, const char * path);
    void removeDir(fs::FS &fs, const char * path);
    void readFile(fs::FS &fs, const char * path);
    void writeFile(fs::FS &fs, const char * path, const char * message);
    void appendFile(fs::FS &fs, const char * path, const char * message);
    void renameFile(fs::FS &fs, const char * path1, const char * path2);
    void deleteFile(fs::FS &fs, const char * path);
    void testFileIO(fs::FS &fs, const char * path);
    
    // Define NTP Client to get time
    WiFiUDP ntpUDP;
    //NTPClient timeClient(ntpUDP, "oceania.pool.ntp.org", 36000, 60000);
    NTPClient timeClient(ntpUDP);
    
    // Variables to save date and time
    String dayStamp;
    String timeStamp;
    
    #define LOG_COLUMN_HEADER  "date," "time," "temperature C," "humidity %RH," "Pressure mB"
    #define MAX_LOG_FILES 100    // NOTE: Number of log files that can be made
    
    WiFiMulti wifiMulti;
    
    // Create AsyncWebServer object on port 80
    AsyncWebServer server(80);
    
    File myFile, myFile1, myFile2;    //myFile (setup.js), myFile1 (index.html), myFile2 (TEMP0000.TXT),
    
    String readBME280Temperature() {
      // Read temperature as Celsius (the default)
      float t = bme.readTemperature();
      // Convert temperature to Fahrenheit
      //t = 1.8 * t + 32;
      if (isnan(t)) {    
        Serial.println("Failed to read from BME280 sensor!");
        return "--";
      }
      else {
        Serial.print(t);
        Serial.print(", "); // formatting the serial output
        return String(t);
      }
    }
    
    String readBME280Humidity() {
      float h = bme.readHumidity();
      if (isnan(h)) {
        Serial.println("Failed to read from BME280 sensor!");
        return "--";
      }
      else {
        Serial.print(h);
        Serial.print(", "); // formatting the serial output
        return String(h);
      }
    }
    
    String readBME280Pressure() {
      float p = bme.readPressure() / 100.0F;
      if (isnan(p)) {
        Serial.println("Failed to read from BME280 sensor!");
        return "--";
      }
      else {
        Serial.println(p);
        return String(p);
      }
    }
    
    // Replaces placeholder with BMP280 values
    String processor(const String& var) {
      //Serial.println(var);
      if (var == "TEMPERATURE") {
        return readBME280Temperature();
      }
      else if (var == "HUMIDITY") {
        return readBME280Humidity();
      }
      else if (var == "PRESSURE") {
        return readBME280Pressure();
      }
      return String();
    }

    void setup(){
      // Serial port for debugging purposes
      Serial.begin(115200);
      delay(500);
      
      bool status; 
      // default settings
      // (you can also pass in a Wire library object like &Wire2)
      status = bme.begin(0x76);  
      if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
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
      //createDir(SD, "/mydir");
      //listDir(SD, "/", 0);
      //removeDir(SD, "/mydir");
      //listDir(SD, "/", 2);
      //writeFile(SD, "/Temp.txt", "temperature C,humidity % \n");
      //appendFile(SD, "/hello.txt", "World!\n");
      //readFile(SD, "/hello.txt");
      //deleteFile(SD, "/foo.txt");
      //renameFile(SD, "/hello.txt", "/foo.txt");
      //readFile(SD, "/foo.txt");
      //testFileIO(SD, "/test.txt");
      Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
      Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    
      ReadSetupFile(filename1);
      delay(100);
      ReadHtmlFile(filename2);
    
      Serial.printf(index_html);                      // debug error in reading html******************
        
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
      timeClient.begin();              // Initialize a NTPClient to get time
      // Set offset time in seconds to adjust for your timezone, for example:
      // GMT +1 = 3600
      // GMT +8 = 28800
      // GMT+11 = 39600
      // GMT -1 = -3600
      // GMT 0 = 0
      timeClient.setTimeOffset(39600);
      Serial.println("Contacting Time Server");
      getTimeStamp();
      timeClient.update();
      Serial.println(timeClient.getFormattedTime());
    
      Update_File_Exist(textname);
       
      // Route for root / web page
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html, processor);
      });
      server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", readBME280Temperature().c_str());
      });
      server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", readBME280Humidity().c_str());
      });
      server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/plain", readBME280Pressure().c_str());
      });
    
      // Start server
      server.begin();
      
    }// end setup
 
    void loop(){
      getTimeStamp();
      write_txt();
      delay(5000);
    }
    
    //***************************** Functions ***********************************************//
    //***************************************************************************************//
    // Function to get date and time from NTPClient
    void getTimeStamp() {
      while(!timeClient.update()) {
        timeClient.forceUpdate();
      }
      // The time comes with the following format:
      //getDay  KEYWORD2  //getHours  KEYWORD2  //getMinutes  KEYWORD2  //getSeconds  KEYWORD2  //getFormattedTime  KEYWORD2
    
      dayStamp = timeClient.getDay();
      
    if( dayStamp == "0"){
        dayStamp = "Sunday";
        }
    if( dayStamp == "1"){
        dayStamp = "Monday";
        }
    if( dayStamp == "2"){
        dayStamp = "Tuesday";
        }
    if( dayStamp == "3"){
        dayStamp = "Wednesday";
        }
    if( dayStamp == "4"){
        dayStamp = "Thursday";
        }
    if( dayStamp == "5"){
        dayStamp = "Friday";
        }    
    if( dayStamp == "6"){
        dayStamp = "Saturday"; 
        }
    
      Serial.println(dayStamp);
      timeStamp = timeClient.getFormattedTime();
      Serial.println(timeStamp);
    }
    // *****************************************************************************
    //  void write_txt() write temp/humidity data to the text file (myFile2)
    //  *****************************************************************************
    void write_txt() {
      char buf[256];
      myFile2 = SD.open(textname, FILE_APPEND);
        
      if (myFile2) {
        Serial.println("Writing data to text file...");
        myFile2.print(dayStamp);
        myFile2.print(",");
        myFile2.print(timeStamp);
        myFile2.print(",");
        myFile2.print(readBME280Temperature());
        myFile2.print(",");
        myFile2.print(readBME280Humidity());
        myFile2.print(",");
        myFile2.print(readBME280Pressure());
        myFile2.println(" ");
        myFile2.close();
    
        delay(50);
        Serial.println("done");
      } else {
        Serial.println("error opening TXT file.....");                        // if the file didn't open, print an error:
      } // end text file loop
    }//~
    // *****************************************************************************
    // void open_txt() write column headers to the text file
    // *****************************************************************************
    void open_txt() {
      myFile2.println(LOG_COLUMN_HEADER);
      myFile2.close();
    }//~
    // *****************************************************************************
    // void ReadHtmlFile(char filename[20]) (myFile1)
    // *****************************************************************************
    void ReadHtmlFile(char filename[20]){
      //read html file from SD card
      myFile1 = SD.open("/index.html");
      if (!myFile1) {
        Serial.println("index.html file doesn't exist. Save it on SD card and restart.");
        return;
      }
      Serial.printf("index.html file size: %d\r\n", myFile1.size());
      index_html = new char [myFile1.size() + 1];
      myFile1.read((uint8_t*)index_html, myFile1.size());
      index_html[myFile1.size()] = 0;  //make a CString
      myFile1.close();
    
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
      /*
        const char* WifiNetwork_0_ssid = WifiNetwork[0]["ssid"]; // "mywifinet123"
        const char* WifiNetwork_0_password = WifiNetwork[0]["password"]; // "qwerty"
    
        const char* WifiNetwork_1_ssid = WifiNetwork[1]["ssid"]; // "officenet"
        const char* WifiNetwork_1_password = WifiNetwork[1]["password"]; // "admin1"
    
        const char* WifiNetwork_2_ssid = WifiNetwork[2]["ssid"]; // "iPhone(Johny)"
        const char* WifiNetwork_2_password = WifiNetwork[2]["password"]; // "12345"
      */
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
        String StatName = WifiNetwork[i]["StatName"];                                   // added Station Name and Location***************
        String StatLocation = WifiNetwork[i]["StatLocation"];
        wifiMulti.addAP(ssid.c_str(), password.c_str());
    
        Serial.printf("WiFi %d: SSID: \"%s\" ; PASSWORD: \"%s\"\r\n", i, ssid.c_str(), password.c_str());
        Serial.printf("WiFi %d: NAME: \"%s\" ; LOCATION: \"%s\"\r\n", i, StatName.c_str(), StatLocation.c_str());
      }
    }
    // *****************************************************************************
    // void Update_File_Exist(char filename[20]) Looks through the log files already present on a card,
    //  and creates a new file with an incremented file index.
    //  filename (TXT00000.TXT) and 1 i.e. myFile1
    // *****************************************************************************
    void Update_File_Exist(char filename[20]){
      for (uint8_t i = 0; i < MAX_LOG_FILES; i++)
      {
        filename[6] = (i / 10) + '0';                                // Set logFileName to "DD_MMvXX.txt":
        filename[7] = (i % 10) + '0';
    
        if (!SD.exists(filename))
          break;                                                     // We found our index
        Serial.print(filename);
        Serial.println( F(" exists") );
      }
    
      myFile2 = SD.open(filename, FILE_WRITE);                      //--- Create the TXT file
      if (myFile2) {
        Serial.print("Writing text header...");                       //--- if the file opened okay, write to it:
        open_txt();                                                   // writes start of txt file
        Serial.println("done.");
      }
      Serial.print( F("File name: ") );
      Serial.println(filename);
    }//~
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
    // *****************************************************************************
    void listDirTime(fs::FS &fs, const char * dirname, uint8_t levels){
        Serial.printf("Listing directory: %s\n", dirname);
    
        File root = fs.open(dirname);
        if(!root){
            Serial.println("Failed to open directory");
            return;
        }
        if(!root.isDirectory()){
            Serial.println("Not a directory");
            return;
        }
    
        File file = root.openNextFile();
        while(file){
            if(file.isDirectory()){
                Serial.print("  DIR : ");
                Serial.print (file.name());
                time_t t= file.getLastWrite();
                struct tm * tmstruct = localtime(&t);
                Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
                if(levels){
                    listDir(fs, file.name(), levels -1);
                }
            } else {
                Serial.print("  FILE: ");
                Serial.print(file.name());
                Serial.print("  SIZE: ");
                Serial.print(file.size());
                time_t t= file.getLastWrite();
                struct tm * tmstruct = localtime(&t);
                Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
            }
            file = root.openNextFile();
        }
    }
    // *****************************************************************************
    void createDir(fs::FS &fs, const char * path) {
      Serial.printf("Creating Dir: %s\n", path);
      if (fs.mkdir(path)) {
        Serial.println("Dir created");
      } else {
        Serial.println("mkdir failed");
      }
    }
    // *****************************************************************************
    void removeDir(fs::FS &fs, const char * path) {
      Serial.printf("Removing Dir: %s\n", path);
      if (fs.rmdir(path)) {
        Serial.println("Dir removed");
      } else {
        Serial.println("rmdir failed");
      }
    }
    // *****************************************************************************
    void readFile(fs::FS &fs, const char * path) {
      Serial.printf("Reading file: %s\n", path);
    
      File file = fs.open(path);
      if (!file) {
        Serial.println("Failed to open file for reading");
        return;
      }
    
      Serial.print("Read from file: ");
      while (file.available()) {
        Serial.write(file.read());
      }
      file.close();
    }
    // *****************************************************************************
    void writeFile(fs::FS &fs, const char * path, const char * message) {
      Serial.printf("Writing file: %s\n", path);
    
      File file = fs.open(path, FILE_WRITE);
      if (!file) {
        Serial.println("Failed to open file for writing");
        return;
      }
      if (file.print(message)) {
        Serial.println("File written");
      } else {
        Serial.println("Write failed");
      }
      file.close();
    }
    // *****************************************************************************
    void appendFile(fs::FS &fs, const char * path, const char * message) {
      Serial.printf("Appending to file: %s\n", path);
    
      File file = fs.open(path, FILE_APPEND);
      if (!file) {
        Serial.println("Failed to open file for appending");
        return;
      }
      if (file.print(message)) {
        Serial.println("Message appended");
      } else {
        Serial.println("Append failed");
      }
      file.close();
    }
    // *****************************************************************************
    void renameFile(fs::FS &fs, const char * path1, const char * path2) {
      Serial.printf("Renaming file %s to %s\n", path1, path2);
      if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
      } else {
        Serial.println("Rename failed");
      }
    }
    // *****************************************************************************
    void deleteFile(fs::FS &fs, const char * path) {
      Serial.printf("Deleting file: %s\n", path);
      if (fs.remove(path)) {
        Serial.println("File deleted");
      } else {
        Serial.println("Delete failed");
      }
    }
    // *****************************************************************************
    void testFileIO(fs::FS &fs, const char * path) {
      File file = fs.open(path);
      static uint8_t buf[512];
      size_t len = 0;
      uint32_t start = millis();
      uint32_t end = start;
      if (file) {
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len) {
          size_t toRead = len;
          if (toRead > 512) {
            toRead = 512;
          }
          file.read(buf, toRead);
          len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
      } else {
        Serial.println("Failed to open file for reading");
      }
    
    
      file = fs.open(path, FILE_WRITE);
      if (!file) {
        Serial.println("Failed to open file for writing");
        return;
      }
    
      size_t i;
      start = millis();
      for (i = 0; i < 2048; i++) {
        file.write(buf, 512);
      }
      end = millis() - start;
      Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
      file.close();
    }
