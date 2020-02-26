/*
  BME280 temperature and humidity sensor test
 
  This example shows how to read and write data to and from an SD card file   
  ****** On upload, press and hold BUT button till the bootloader is started
   
    ESP32  micro SD
    GND     GND
    +3.3v   Vcc
    D19     MISO
    D23     MOSI
    D18     SCK
    D05     CS
    
 edited for the ESP32
 NOTE: working AP 
 */

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "FS.h"
#include "SD.h"                         // CS is on G05
#include "SPI.h"

void Update_File_Exist(char filename[20]); // NOTE: file name, Update_File_Exist(kmlname,1); i.e. kmlname, myFile1 update
void open_txt();

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

char textname[20] = "/BME2000.TXT"; // SD card filename

int tim = 0;
float t;   // temperature C
float ht;  // height above sea level m
float h;   // humidity %RH
float p;   // pressure hPa


// Default SD chip select for Pinguino Micro 51, OTG 36
const int chipSelect_SD_default = 5; // Change to suit your board
// chipSelect_SD can be changed if you do not use default CS pin
const int chipSelect_SD = chipSelect_SD_default;

#define SEALEVELPRESSURE_HPA (1020.10)//http://www.bom.gov.au/products/IDV60801/IDV60801.95936.shtml

Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

#define LOG_COLUMN_HEADER  "time," "temperature C," "humidity %RH," "Pressure mB," "height ASL"
#define MAX_LOG_FILES 100    // NOTE: Number of log files that can be made

unsigned long delayTime;
File myFile1;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println(F("BME280 test"));
  
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
  
  Update_File_Exist(textname);
  
  unsigned status;
  
  status = bme.begin();
      if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    } 
    Serial.println("-- Default Test --");
    delayTime = 1000;

    Serial.println();
}

void loop() {
  // Wait a few seconds between measurements.
    delay(delayTime);

    Serial.print("Temperature = ");
    t = bme.readTemperature();
    Serial.print(t);
    Serial.println(" *C");

    Serial.print("Humidity = ");
    h = bme.readHumidity();
    Serial.print(h);
    Serial.println(" %");

    Serial.print("Pressure = ");
    p = (bme.readPressure() / 100.0F);
    Serial.print(p);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    ht = bme.readAltitude(SEALEVELPRESSURE_HPA);
    Serial.print(ht);
    Serial.println(" m");

    Serial.println();
    delay(50);
    write_txt();
    /*
      myFile1 = SD.open(testname1, FILE_APPEND);
       // if the file opened okay, write to it:
      if (myFile1) {
        Serial.print("Writing data to txt file....");

        myFile1.print(tim);
        myFile1.print(",");
        myFile1.print(t);
        myFile1.print(",");
        myFile1.print(h);
        myFile1.print(",");
        myFile1.print(p);
        myFile1.print(",");
        myFile1.print(ht);
        myFile1.println(" ");
        myFile1.close();
        
        delay(50);
        Serial.println("done");
      } else {
        // if the file didn't open, print an error:
        Serial.println("error opening txt file.....");
      }
      */

      tim = tim + delayTime; // update time
  
}// end main loop

// *****************************************************************************
// void Update_File_Exist(char filename[20]) Looks through the log files already present on a card,
//  and creates a new file with an incremented file index.
//  filename (TXT00000.TXT) and 1 i.e. myFile1
// *****************************************************************************
void Update_File_Exist(char filename[20])
{
  for (uint8_t i = 0; i < MAX_LOG_FILES; i++)
  {
    filename[6] = (i / 10) + '0';                                // Set logFileName to "DD_MMvXX.txt":
    filename[7] = (i % 10) + '0';

    if (!SD.exists(filename))
      break;                                                     // We found our index
    Serial.print(filename);
    Serial.println( F(" exists") );
  }

  myFile1 = SD.open(filename, FILE_WRITE);                      //--- Create the TXT file
  if (myFile1) {
    Serial.print("Writing text header...");                       //--- if the file opened okay, write to it:
    open_txt();                                                   // writes start of txt file
    Serial.println("done.");
  }
  Serial.print( F("File name: ") );
  Serial.println(filename);
}//~

// *****************************************************************************
// void open_txt() write column headers to the text file
// *****************************************************************************
void open_txt() {
  myFile1.println(LOG_COLUMN_HEADER);
  myFile1.close();
}//~

// *****************************************************************************
//  void write_txt() write temp/humidity data to the text file (myFile1)
//  *****************************************************************************
void write_txt() {
  char buf[256];
  myFile1 = SD.open(textname, FILE_APPEND);
    
  if (myFile1) {
    Serial.println("Writing data to text file...");
    myFile1.print(tim);
    myFile1.print(",");
    myFile1.print(t);
    myFile1.print(",");
    myFile1.print(h);
    myFile1.print(",");
    myFile1.print(p);
    myFile1.print(",");
    myFile1.print(ht);
    myFile1.println(" ");
    myFile1.close();

    delay(50);
    Serial.println("done");
  } else {
    Serial.println("error opening TXT file.....");                        // if the file didn't open, print an error:
  } // end text file loop
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
