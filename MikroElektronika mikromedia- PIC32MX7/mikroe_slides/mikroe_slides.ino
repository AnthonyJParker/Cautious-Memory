/*  mikroe mx7 tft touchscreen demo with bmp images
 *   scrolls through bmp files in folder slides
 *   
 *   working A Parker 2017
 *   
 UECIDE     : v0.8.9-pre14
 Hardware   : mikroe-mikromedia-795
 Core       : chipKIT
 Compiler   : PIC32 Compiler version 1.43
 Programmer : pic32prog
 Other      : 
 Board      : https://shop.mikroe.com/mikromedia-4-pic32mx7
 */
#include <DSPI.h>
#include <SD.h>
#include <BMPFile.h>
#include <Liberation.h>
# include <SSD1963.h>
# include <Wire.h>
//# include <SPI.h>
# include <Adafruit_STMPE610.h>

Adafruit_STMPE610 touch = Adafruit_STMPE610(-1);

SSD1963 tft(31, 52, 53, 92, 33, 64, 65, 66, 67, 68, 69, 70, 71, 96, 97, 81, 80, 60, 61, 54, 55,TFTBUS24);

const uint8_t SD_CHIP_SELECT = 6;

#define TS_MINX 150
#define TS_MINY 250
#define TS_MAXX 3800
#define TS_MAXY 3800

void squareButtons(int xp, int yp, int w, int h, char *string, char *bColor);
void roundButtons(int xp, int yp, int r, char *string, char *bColor);
void cardTest();
void fileExists();

int x;
int y;
int z;

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
File myFile, myFile1;

void setup() {
    Wire.begin();                             // join i2c bus (address optional for master)
    Serial.begin(9600);                   // NOTE: output to the serial console
    delay(4000);
      
    tft.initializeDevice(SSD1963::MIKROMEDIA);
    tft.setBacklight(120);
  	//tft.setRotation(0);
  	tft.fillScreen(Color::Black);
  	//tft.setFont(Fonts::Topaz);
    tft.setFont(Fonts::Liberation14);
  	tft.setTextColor(Color::Yellow, Color::Black);	
  	tft.setCursor(10, 30);
  	tft.print("Initializing sd card...");

     if (! touch.begin(0x44)) {
    Serial.println("STMPE not found!");
    while(1) {
      delay(500);
    }
  }
    delay(3000);
    pinMode(SD_CHIP_SELECT, OUTPUT);
    digitalWrite(SD_CHIP_SELECT, HIGH);

     if (!SD.begin(SD_CHIP_SELECT)) {
     Serial.println("initialization failed!");
     return;
    }

    SD.begin(SD_CHIP_SELECT);
    Serial.println("SD initialization done.");
    cardTest();
    tft.setCursor(10, 60);
    tft.print("SD initialization done.");

  squareButtons(20  ,220,120,40,"Up","Red");
  squareButtons(340,220,120,40,"Down","Green");
  delay(2000);
}

void loop() {
 	myFile = SD.open("/slides");
	dir_t p;
	myFile.seek(0);
	while (myFile.readDir(&p)) { 
		if (p.name[0] == DIR_NAME_FREE) {
			break;
		}
		if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') {
      Serial.println("at continue");
			continue;
		}
		if (!DIR_IS_FILE_OR_SUBDIR(&p)) {
    Serial.println("at file sub continue");
			continue;
		}
 		if (!strncmp((const char *)p.name+8, "BMP", 3)) {
			char fn[15];
			char fullPath[100];
			dirToChar(p, fn);
			sprintf(fullPath, "/slides/%s", fn);
      //fileExists();
      myFile1 = SD.open(fullPath);
  		BMPFile bmp(myFile1);
			uint32_t start = millis();
			bmp.draw(&tft, 0, 0);
			uint32_t end = millis();
			uint32_t len = end - start;
     // tft.setCursor(0,0);
      //tft.println("finished.................");
			myFile1.close();
			delay(5000);
		}
	}
	myFile.close();
}// end loop

void dirToChar(dir_t &p, char *s) {
	memset(s, 0, 14);
	char *ptr = s;
	for (int i = 0; i < 11; i++) {
		if (p.name[i] == ' ') {
			continue;
		}
		if (i == 8) {
			*ptr++ = '.';
		}
		*ptr++ = p.name[i];
	}
}

void squareButtons(int xp,int yp,int w, int h,char *string, char *bColor){
       // note touch sense must match button x and y
       //   tft.fillRoundRect(x, y, width, height, 4, Color::Yellow);
       if(bColor == "Red"){
         tft.fillRoundRect(xp,  yp, w, h, 4, Color::Red);  
         tft.setTextColor(Color::Black, Color::Red);     
       }
       else if (bColor == "Green"){
        tft.fillRoundRect(xp,  yp, w, h, 4, Color::Green);
        tft.setTextColor(Color::Black, Color::Green);
       }
        else if (bColor == "Blue"){
        tft.fillRoundRect(xp,  yp, w, h, 4, Color::Blue);
        tft.setTextColor(Color::Black, Color::Blue);
       }
        else if (bColor == "Yellow"){
        tft.fillRoundRect(xp,  yp, w, h, 4, Color::Yellow);
        tft.setTextColor(Color::Black, Color::Yellow);
       }
//        tft.fillRoundRect(xp,  yp, w, h, 4, Color::Yellow);
//        tft.setTextColor(Color::Black, Color::Yellow);
        tft.setCursor(xp+10, yp+8);
        tft.setFont(Fonts::Liberation16);
        tft.println(string);
}

void roundButtons(int xp,int yp,int r,char *string, char *bColor){
       // note touch sense must match button x and y
       //   tft.fillCircle(x, y, radius, Color::Yellow);
       if(bColor == "Red"){
         tft.fillCircle(xp,  yp, r, Color::Red);  
         tft.setTextColor(Color::Black, Color::Red);     
       }
       else if (bColor == "Green"){
        tft.fillCircle(xp,  yp, r, Color::Green);
        tft.setTextColor(Color::Black, Color::Green);
       }
        else if (bColor == "Blue"){
        tft.fillCircle(xp,  yp, r, Color::Blue);
        tft.setTextColor(Color::Black, Color::Blue);
       }
        else if (bColor == "Yellow"){
        tft.fillCircle(xp,  yp, r, Color::Yellow);
        tft.setTextColor(Color::Black, Color::Yellow);
       }

        tft.setCursor(xp+10, yp+8);
        tft.setFont(Fonts::Liberation16);
        tft.println(string);
}
void cardTest(){

  if (!card.init(SPI_HALF_SPEED, SD_CHIP_SELECT)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card is inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
   Serial.println("Wiring is correct and a card is present."); 
  }
  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition");
    return;
  }
  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);
  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
    
}
void fileExists(){
      tft.setTextColor(Color::Yellow, Color::Black);  
      tft.setCursor(10, 30);
      tft.print("opening bmp file......");
      //char fullPath = "/slides/meth.bmp";
      if (SD.exists("/slides/space2s.bmp")) {
      tft.setCursor(10, 60);
      tft.println("bmp exists..............");
      }
      else {
      tft.setCursor(10, 60);
      tft.println("bmp doesn't exist....");
      }
}
