/*
  DHT22 temperature and humidity sensor test and prints to serial
 
 UECIDE    : v0.8.9-pre14
 Hardware  : Olimex Pinguino32 OTG
 Core      : chipKIT
 Compiler  : PIC32 Compiler version 1.43
 Programmer: pic32prog
 Other     : DHT22 temperature/humidity sensor on pin (4)...
  
 This example code is in the public domain.
 */
#include "DHT.h"

#define DHTPIN 16        // DHT22 on pin 16 (UEXT pin5) or you choose
# include <DSPI.h>
# include <SPI.h>
# include <SdFat.h>                                  // use SDfat faster library

void Update_File_Exist(char filename[20],int mF); // update file name
void open_txt();

char textname[20] = "TEMP0000.TXT"; // SD card filename
int tim = 0;

// Default SD chip select for PinguinoOTG 36
const int chipSelect_SD_default = 36; // Change to suit your board
// chipSelect_SD can be changed if you do not use default CS pin
const int chipSelect_SD = chipSelect_SD_default;
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);
#define LOG_COLUMN_HEADER  "time," "ROM address," "temp C," "temp F," "humidity %RH," "heat index" // printed at the top of the file.
#define MAX_LOG_FILES 100                                 // Number of log files that can be made

SdFat SD;                                                               // instance of SDfat i.e. SD
File myFile1, myFile2;

void setup() {
  Serial.begin(9600);
  Serial.println("DHTxx test!");
  delay(2000);
  
  pinMode(chipSelect_SD_default, OUTPUT);
  digitalWrite(chipSelect_SD_default, HIGH);

  pinMode(chipSelect_SD, OUTPUT);
  digitalWrite(chipSelect_SD, HIGH);

           if (!SD.cardBegin(chipSelect_SD, SPI_FULL_SPEED)) {
           Serial.println("initialization failed!");
           return;
          }

  SD.begin(chipSelect_SD);
  Serial.println("SD initialization done.");
  delay(2000);
  
  Update_File_Exist(textname,1); // check if a test file exists and update file number
  myFile1 = SD.open(textname, FILE_WRITE);
  dht.begin();
}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {//|| isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  //float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.println(" ");
  //Serial.print(hif);
  //Serial.println(" *F");
  
      myFile1 = SD.open(textname, FILE_WRITE);
       // if the file opened okay, write to it:
      if (myFile1) {
        Serial.print("Writing data to txt file....");

        myFile1.print(tim);
        myFile1.print(",");
        myFile1.print(t);
        myFile1.print(",");
        myFile1.print(f);
        myFile1.print(",");
        myFile1.print(h);
        myFile1.print(",");
        myFile1.print(hic);
        myFile1.println(" ");
        myFile1.close();

        
        delay(50);
        Serial.println("done");
      } else {
        // if the file didn't open, print an error:
        Serial.println("error opening txt file.....");
      }

      tim = tim + 2; // update time
  
}// end main loop
// *****************************************************************************
// void open_txt()
// *****************************************************************************
  void open_txt()
  {
    myFile1.println(LOG_COLUMN_HEADER);
    myFile1.close();
  }
// *****************************************************************************
// void Update_File_Exist(char filename[20],int mF) Looks through the log files already present on a card,
//  and creates a new file with an incremented file index.
//  filename (TXT00000.TXT) and 1 i.e. myFile1
// *****************************************************************************
  void Update_File_Exist(char filename[20],int mF)
  {
    for (uint8_t i=0; i < MAX_LOG_FILES; i++)
    {
      filename[6] = (i/10) + '0';                                                // Set logFileName to "DD_MMvXX.txt":
      filename[7] = (i%10) + '0';
  
      if (!SD.exists(filename))
        break;                                                                       // We found our index
      Serial.print(filename);
      Serial.println( F(" exists") );
    }
          if(mF == 1) {                                                            //--- Create the data file
                  myFile1 = SD.open(filename, FILE_WRITE);
          
                  if(myFile1) {                                                     //--- if the file opened okay, write to it:
                    Serial.print("Writing text header...");
                    open_txt();
                    Serial.println("done.");
                  }
          }
          else if (mF == 2) {
                  myFile2 = SD.open(filename, FILE_WRITE);
          
                  if(myFile2) {                                                     //--- if the file opened okay, write to it:
                    Serial.print("Writing text header...");
                    open_txt();
                    Serial.println("done.");
                  }          
          }
    Serial.print( F("File name: ") );
    Serial.println(filename);
  }//~
