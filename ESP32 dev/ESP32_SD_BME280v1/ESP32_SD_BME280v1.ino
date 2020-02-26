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

#include <SD.h>        // include the SD library:

void TXT_Test_File_Exist();
void open_txt();

char testname1[20] = "/BME2000.TXT"; // SD card filename
char *textF = "0123456789"        ; // SD card filename increment
unsigned char loop1, loop2        ; // SDS card loop
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

unsigned long delayTime;
File myFile1;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println(F("BME280 test"));
  
  SD.begin(chipSelect_SD);
  Serial.println("SD initialization done.");
  delay(2000);
  
  TXT_Test_File_Exist(); // check if a test file exists and update file number
  myFile1 = SD.open(testname1, FILE_WRITE);
  open_txt();
  
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

      tim = tim + delayTime; // update time
  
}// end main loop
// *****************************************************************************
// void TXT_Test_File_Exist()
// *****************************************************************************
 void TXT_Test_File_Exist()  // Tests file exists, and if so updates filename
  {
     char Bslash[] = "\r\n";
     for(loop1 = 0; loop1 <= 9; loop1++)   // outer tens loop
     {
       for(loop2 = 0; loop2 <= 9; loop2++) // inner ones loop
       {
           //--- "GPS00000.TXT"
        testname1[6] = textF[loop1];        //current filename tens
        testname1[7] = textF[loop2];        //current filename ones
       if (SD.exists(testname1))
        { 
           //Serial.println("\r\nfound the file\r\n");
           Serial.print("...found the file....");
           Serial.println(testname1);
           //--- file has been found update filename
        }
       else { //--- file was not found ie filename ok
          testname1[6] = textF[loop1];
          testname1[7] = textF[loop2];       //current filename
          //Serial.println("\r\ncreate the file\r\n");
          Serial.print("..create the file....");
          Serial.println(testname1);
          //--- Create the data file
          myFile1 = SD.open(testname1, FILE_WRITE);
           //--- if the file opened okay, write to it:
           if (myFile1) {
            Serial.print("Writing text header...");
            open_txt();
            Serial.println("done.");
            } 
           return;
           }// loop2 end
       }//loop1 end
     }// end if
  }//~
  // *****************************************************************************
// void open_kml()
// *****************************************************************************
void open_txt()
{
    myFile1.print("time");
    myFile1.print(",");
    myFile1.print("temp C");
    myFile1.print(",");
    myFile1.print("humidity %RH");
    myFile1.print(",");
    myFile1.print("pressure hPa");
    myFile1.print(",");
    myFile1.print("height ASL");
    myFile1.println(" ");
    myFile1.close();
}
