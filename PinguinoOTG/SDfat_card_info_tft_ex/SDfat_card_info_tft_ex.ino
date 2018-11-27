/*
 Compare to SDBench.ino using chipkit SD library
  Note: waits for serial line 31 
  
 UECIDE    : v0.8.9-pre14
 Hardware  : Olimex Pinguino32 OTG
 Core      : chipKIT
 Compiler  : PIC32 Compiler version 1.43
 Programmer: pic32prog
 Other     : TFT shield (320x480) (you may need to check the driver chip for your display e.g. ILI9481)
  
 This example code is in the public domain.
 */
#include <DSPI.h>
#include <SPI.h>
#include <SdFat.h>
#include <ILI9481.h>
#include <Liberation.h>

ILI9481 tft(A2, A1, A0, A3, A4, 8, 9, 2, 3, 4, 5, 6, 7); 

const int chipSelect_SD_default = 36;          // 10 on tft shield
const int chipSelect_SD = chipSelect_SD_default;

SdFat SD;
File myFile;

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  Serial.print("Initializing SD card...");

  pinMode(chipSelect_SD_default, OUTPUT);
  digitalWrite(chipSelect_SD_default, HIGH);

  pinMode(chipSelect_SD, OUTPUT);
  digitalWrite(chipSelect_SD, HIGH);
     
  tft.initializeDevice();
  delay(20);
  tft.setBacklight(255);
  tft.setTextWrap(false);
  tft.setRotation(1);
  tft.fillScreen(Color::Black);
  tft.setTextColor(Color::Green);

  tft.setFont(Fonts::Liberation20);
  tft.setCursor(5, 10);
  tft.println("......SDfat card speed test.....");
  tft.setFont(Fonts::Liberation18);
  tft.setCursor(5, 45);
  tft.print("SDfat Ver: ");
  tft.print(SD_FAT_VERSION);
  tft.print(" CS pin: ");
  tft.println(chipSelect_SD);
  delay(1000);
  
   if (!SD.cardBegin(chipSelect_SD, SPI_FULL_SPEED)) {
   Serial.println("initialization failed!");
   tft.setCursor(5, 70);
   tft.println("initialization failed!");
   return;
  }

  SD.begin(chipSelect_SD);
  Serial.println("SD initialization done.");
   tft.setCursor(5, 70);
   tft.println("SD initialization done.");
  
  if (SD.exists("/test.txt")) {
   Serial.println("Deleting existing file");
    SD.remove("/test.txt");
  }
  
  myFile = SD.open("/test.txt", FILE_WRITE);
  
  if (myFile) {
    char buf[1024];
    for(int i = 0; i < 1024; i++) {
      buf[i] = rand();
    }
    Serial.print("Writing 10MB to test.txt...");
    tft.setCursor(5, 95);
    tft.println("Writing 10MB to test.txt...");

    uint32_t startTime = millis();
    uint32_t bytes = 0;
    while (bytes < 10485760UL) {
      myFile.write((const uint8_t *)buf, 1024);
      bytes += 1024;
    }
  
    myFile.close();
    uint32_t endTime = millis();
    Serial.println("done.");
    Serial.print("Time taken: ");
    tft.setCursor(5, 120);
    tft.print("done...Time taken: ");
    tft.print(endTime - startTime );
    tft.print(" ms");
    Serial.print(endTime - startTime);
    Serial.println("ms");
  } else {
    Serial.println("error opening test.txt");
  }
  
  myFile = SD.open("/test.txt");
  if (myFile) {
    Serial.println("Reading data from test.txt:");
    tft.setCursor(5, 145);
    tft.println("Reading data from test.txt:");
    uint32_t startTime = millis();
   
    while (myFile.available()) {
      (void)myFile.read();
    }
    myFile.close();
    uint32_t endTime = millis();
    Serial.println("done.");
    Serial.print("Time taken: ");
    tft.setCursor(5, 170);
    tft.print("done...Time taken: ");
    tft.print(endTime - startTime );
    tft.print(" ms");
    Serial.print(endTime - startTime);
    Serial.println("ms");
  } else {
    Serial.println("error opening test.txt");
    tft.setCursor(5, 170);
    tft.print("error opening test.txt");
  }
}

void loop()
{
}
