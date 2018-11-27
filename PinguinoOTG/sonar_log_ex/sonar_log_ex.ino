/*
  HC-SR04 sonar distance measure test

 UECIDE    : v0.8.9-pre14
 Hardware  : Olimex Pinguino32 OTG
 Core      : chipKIT
 Compiler  : PIC32 Compiler version 1.43
 Programmer: pic32prog
 Other     : HC-SR04 sonar sensor conected to send the trigger pulse on pin 2 and wait for a response/echo on pin 3
  
 This example code is in the public domain.
 */
# include <Pinger.h>
# include <DSPI.h>
# include <SPI.h>
# include <SdFat.h>                                  // use SDfat faster library

void Update_File_Exist(char filename[20],int mF); // update file name
void open_txt();

char textname[20] = "TXT00000.TXT"; // SD card filename

// Default SD chip select for PinguinoOTG 36
const int chipSelect_SD_default = 36; // Change to suit your board
// chipSelect_SD can be changed if you do not use default CS pin
const int chipSelect_SD = chipSelect_SD_default;

#define LOG_COLUMN_HEADER  "distance mm" // printed at the top of the file.
#define MAX_LOG_FILES 100                                 // Number of log files that can be made

SdFat SD;                                                               // instance of SDfat i.e. SD
File myFile1, myFile2;
// Send the trigger pulse on pin 2 and wait for a response on pin 3.
Pinger pinger(2, 3);

void setup() {
  Serial.begin(9600);
  delay(3000);
  Serial.println("Pinger log example");
  
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
}

void loop() {
    // The pinger responds in cm
    float mm;
    float cm = pinger.ping();
    mm = cm * 10;
    // Print it.
    Serial.print(mm);
    Serial.println(" mm");
    
      myFile1 = SD.open(textname, FILE_WRITE);
       // if the file opened okay, write to it:
      if (myFile1) {
        Serial.print("Writing data to txt file....");
        
        myFile1.print(mm); // sonar distance data
        myFile1.println(" ");
        myFile1.close();

        Serial.println("done");
      } else {
        // if the file didn't open, print an error:
        Serial.println("error opening txt file.....");
      }
    // Don't do it too often or there will just be too much data
    // for the human brain to take in.
    delay(400);
}
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
