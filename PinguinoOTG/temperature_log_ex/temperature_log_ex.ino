/*
  This example shows how to save data from multiple DS18S20 sensors on one pin and dsplay on tft shield

 UECIDE    : v0.8.9-pre14
 Hardware  : Olimex Pinguino32 OTG
 Core      : chipKIT
 Compiler  : PIC32 Compiler version 1.43
 Programmer: pic32prog
 Other     : TFT shield (320x480) (you may need to check the driver chip for your display e.g. ILI9481),
             and 2x DS18S20 temperature sensors on pin 16 (UEXT pin 5)
 
 http://www.pjrc.com/teensy/td_libs_OneWire.html

 The DallasTemperature library can do all this work for you!
 http://milesburton.com/Dallas_Temperature_Control_Library 
 This example code is in the public domain. 
 */
# include <OneWire.h>
# include <DSPI.h>
# include <SPI.h>
# include <SdFat.h>                                  // use SDfat faster library
# include <ILI9481.h>
# include <Liberation.h>

ILI9481 tft(A2, A1, A0, A3, A4, 8, 9, 2, 3, 4, 5, 6, 7);

OneWire  ds(16);                                                  // DS18S20 on pin 16 (UEXT pin5)

void print_TFT();                                                     // function to print values on tft lcd
void Update_File_Exist(char filename[20],int mF); // update file name, Update_File_Exist(textname,1); i.e. textname, myFile1
void open_txt();
void write_txt();
void ROM_update();                                               // sprintf the ROM address of the sensor

char textname[20] = "TEMP0000.TXT";             // SD card filename
int tim                 = 0;
float lastTemp    = 0;
char buf[50];
float celsius, fahrenheit;
float inside          = 0;
float outside        = 0;
float maxITemp   = 0;
float minITemp    = 30;
float maxOTemp = 0;
float minOTemp  = 30;
int count              = 0; 
int Tdelay            = 2000;
int DEBUG          = 0;

const int chipSelect_SD_default = 36;               // Change to suit your board PinguinoOTG 36
const int chipSelect_SD = chipSelect_SD_default;

#define LOG_COLUMN_HEADER   "time,"  "ROM index,"  "celsius"
#define MAX_LOG_FILES 100                                 // Number of log files that can be made

SdFat SD;                                                                 // instance of SDfat i.e. SD
File myFile1, myFile2;                                              // myFile for the files

void setup(void) {
  
    Serial.begin(9600);
    Serial.println("Temerature DS18S20 log v0.01");
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
  
    tft.initializeDevice();
    delay(20);
    tft.setBacklight(255);
    tft.setTextWrap(false);
    tft.setRotation(1);
    tft.fillScreen(Color::Black);
    tft.setTextColor(Color::Green);
    tft.setCursor(5, 25);
    tft.setFont(Fonts::Liberation22);
    tft.print("Temperature logging v0.1");
    tft.setCursor(5, 80);
    tft.print("Inside:");
    tft.setCursor(260, 80);
    tft.print("Outside:");
    delay(2000);
}

void loop(void) {
        
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
    
      if ( !ds.search(addr)) {
        Serial.println("No more addresses.");
        Serial.println();
        ds.reset_search();
        delay(250);
        return;
      }

     ROM_update(addr);

      if (OneWire::crc8(addr, 7) != addr[7]) {
          Serial.println("CRC is not valid!");
          return;
      }
      Serial.println();
 
      switch (addr[0]) {                                // the first ROM byte indicates which chip
        case 0x10:
          Serial.println("  Chip = DS18S20");  // or old DS1820
          type_s = 1;
          break;
        case 0x28:
          Serial.println("  Chip = DS18B20");
          type_s = 0;
          break;
        case 0x22:
          Serial.println("  Chip = DS1822");
          type_s = 0;
          break;
        default:
          Serial.println("Device is not a DS18x20 family device.");
          return;
      } 

      ds.reset();
      ds.select(addr);
      ds.write(0x44,1);                                   // start conversion, with parasite power on at the end
      
      delay(1000);                                          // maybe 750ms is enough, maybe not
      // we might do a ds.depower() here, but the reset will take care of it.
      
      present = ds.reset();
      ds.select(addr);    
      ds.write(0xBE);                                       // Read Scratchpad
    
    if (DEBUG == 1) {
      Serial.print("  Data = ");
      Serial.print(present,HEX);
      Serial.print(" ");
    }
      for ( i = 0; i < 9; i++) {                          // we need 9 bytes
        data[i] = ds.read();
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
          if (DEBUG == 1) {
      Serial.print(" CRC=");
      Serial.print(OneWire::crc8(data, 8), HEX);
      Serial.println();
          }
// -------------------- convert the data to actual temperature
        unsigned int raw = (data[1] << 8) | data[0];
        if (type_s) {
          raw = raw << 3;                                      // 9 bit resolution default
          if (data[7] == 0x10) {
            raw = (raw & 0xFFF0) + 12 - data[6];  // count remain gives full 12 bit resolution
          }
        } else {
          byte cfg = (data[4] & 0x60);                  // default is 12 bit resolution, 750 ms conversion time
          if (cfg == 0x00) raw = raw << 3;          // 9 bit resolution, 93.75 ms
          else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
          else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
        }
        celsius = (float)raw / 16.0;
        //fahrenheit = celsius * 1.8 + 32.0;
        Serial.print("  Temperature = ");
        Serial.print(celsius);
        Serial.println(" Celsius, ");
    
        if (count == 0){
          inside = celsius;
          count = 1;
          }
        else if (count == 1){
          outside = celsius;
          count = 0;            
          }
     
         write_txt();                            // save on SD also print save in lower corner 
         lastTemp = inside;
         
         if (inside >= maxITemp){
                  maxITemp = inside;
         }
         if (inside <= minITemp){
                  minITemp = inside; 
         }
         if (outside >= maxOTemp){
                  maxOTemp = outside;
         }
         if (outside <= minOTemp){
                  minOTemp = outside; 
         }
       
        tim = tim + Tdelay; // update time
        delay(Tdelay); 
        print_TFT();      
  } // end main loop
//----------------------------------------------------Functions-------------------------------------------------

// *****************************************************************************
// void ROM_update(byte addr[8]);
// *****************************************************************************
  void ROM_update(byte addr[8]){
    //char buf[50];
    //sprintf(buf,"ROM=%d %d %d %d %d %d %d %d",addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],addr[8]);
    sprintf(buf,"%d",addr[2]); // short version seems to be addr[2] is different for each chip
    Serial.println(buf);
  }//~
// *****************************************************************************
// void Update_File_Exist(char filename[20],int mF) Looks through the log files already present on a card,
//  and creates a new file with an incremented file index.
//  filename (TXT00000.TXT) and 1 i.e. myFile1
// *****************************************************************************
  void Update_File_Exist(char filename[20],int mF)
  {
    for (uint8_t i=0; i < MAX_LOG_FILES; i++)
    {
      filename[6] = (i/10) + '0';                                       // Set logFileName to "DD_MMvXX.txt":
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
          else if (mF == 2) {                                               // change details to suit myFile2
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
// *****************************************************************************
// void open_txt()
// *****************************************************************************
  void open_txt()
  {
      myFile1 = SD.open(textname, FILE_WRITE);
          if (myFile1) {
          Serial.print("Writing header to txt file....");
          
           myFile1.println(LOG_COLUMN_HEADER);
           myFile1.close();
           delay(50);
          Serial.println("done");
        } else {
          Serial.println("error opening txt file.....");
        }
  }//~
// *****************************************************************************
// void write_txt()
// *****************************************************************************
  void write_txt()
  {
      myFile1 = SD.open(textname, FILE_WRITE);
        if (myFile1) {
          Serial.print("Writing data to txt file....");
  
          myFile1.print(tim);
          myFile1.print(",");
          myFile1.print(buf);
          myFile1.print(",");
          myFile1.print(celsius);
          myFile1.println(" ");
          myFile1.close();
          delay(50);
         // tft.setCursor(250, 275);
         // tft.print("save");
          Serial.println("done");
        } else {
           Serial.println("error opening txt file.....");
        }
  }//~
// *****************************************************************************
// void print_TFT()
// *****************************************************************************
  void print_TFT() 
  {
  
      if (inside <= 5){                                              // change colour depending upon temerature
            tft.setTextColor(Color::Red);
      }
        else  if (inside >= 5 && inside <= 10){
            tft.setTextColor(Color::Yellow);
      }
         else  if (inside >= 10 && inside <= 15){
            tft.setTextColor(Color::Blue);
      }
         else  if (inside >= 15 && inside <= 50){
            tft.setTextColor(Color::Green);
      }
          
      tft.setFont(Fonts::Liberation36); // largest Liberation font
      tft.setCursor(5, 140);                   // print large temps
      tft.print(inside);
      tft.print(" C   ");
      tft.setCursor(250, 140);
      tft.print(outside);
      tft.print(" C ");
      
      tft.setTextColor(Color::Green);    // information on lower screen
      tft.setFont(Fonts::Liberation20);
      tft.setCursor(5, 210);
      if (inside > lastTemp){
      tft.print("trend: warming  ");
      }
       else   if (inside == lastTemp){
      tft.print("trend: steady    ");
      } 
      else if (inside < lastTemp){
      tft.print("trend: cooling   ");
      }
      tft.setCursor(5, 250);
      tft.print("Max: ");
      tft.print(maxITemp);
      tft.print(" C   ");
      tft.setCursor(5, 280);
      tft.print("Min: ");
      tft.print(minITemp);
      tft.print(" C   ");
      tft.setCursor(250, 250);
      tft.print("Max: ");
      tft.print(maxOTemp);
      tft.print(" C   ");
      tft.setCursor(250, 280);
      tft.print("Min: ");
      tft.print(minOTemp);
      tft.print(" C   ");
      
     // tft.setCursor(250, 280);                       // text to say saving on SD
     // tft.print("      ");
      delay(50);
  
  }//~
