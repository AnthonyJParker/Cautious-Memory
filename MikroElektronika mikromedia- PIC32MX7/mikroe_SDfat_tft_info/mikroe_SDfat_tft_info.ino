/*  mikroe mx7 SD card info on tft demo
 *   SD card information demo, note can take a little while to fully read SD
 *   
 UECIDE     : v0.8.9-pre14
 Hardware   : mikroe-mikromedia-795
 Core       : chipKIT
 Compiler   : PIC32 Compiler version 1.43
 Programmer : pic32prog
 Other      : 
 Board      : https://shop.mikroe.com/mikromedia-4-pic32mx7
*/
# include <SdFat.h>
# include <SSD1963.h>
# include <Liberation.h>
# include <DSPI.h>
# include <Wire.h>
# include <SPI.h>
# include <Adafruit_STMPE610.h>

Adafruit_STMPE610 touch = Adafruit_STMPE610(-1);

 SSD1963 tft(31, 52, 53, 92, 33, 64, 65, 66, 67, 68, 69, 70, 71, 96, 97, 81, 80, 60, 61, 54, 55,TFTBUS24);

int count = 0;
const uint8_t SD_CHIP_SELECT = 6;
const int8_t DISABLE_CHIP_SELECT = -1;

SdFat sd;

int ledPinRed = 49;
int ledPinGreen = 50;
int ledPinBlue = 51;
// serial output steam
ArduinoOutStream cout(Serial);

// global for card size
uint32_t cardSize;

// global for card erase size
uint32_t eraseSize;
//------------------------------------------------------------------------------
// store error strings in flash
#define sdErrorMsg(msg) sdErrorMsg_F(F(msg));
void sdErrorMsg_F(const __FlashStringHelper* str) {
  cout << str << endl;
  if (sd.card()->errorCode()) {
    cout << F("SD errorCode: ");
    cout << hex << int(sd.card()->errorCode()) << endl;
    cout << F("SD errorData: ");
    cout << int(sd.card()->errorData()) << dec << endl;
  }
}
//------------------------------------------------------------------------------
uint8_t cidDmp() {
  cid_t cid;
  if (!sd.card()->readCID(&cid)) {
    sdErrorMsg("readCID failed");
    return false;
  }
  cout << F("\nManufacturer ID: ");
  cout << hex << int(cid.mid) << dec << endl;
  cout << F("OEM ID: ") << cid.oid[0] << cid.oid[1] << endl;
  cout << F("Product: ");
  for (uint8_t i = 0; i < 5; i++) {
    cout << cid.pnm[i];
  }
  cout << F("\nVersion: ");
  cout << int(cid.prv_n) << '.' << int(cid.prv_m) << endl;
  cout << F("Serial number: ") << hex << cid.psn << dec << endl;
  cout << F("Manufacturing date: ");
  cout << int(cid.mdt_month) << '/';
  cout << (2000 + cid.mdt_year_low + 10 * cid.mdt_year_high) << endl;
  cout << endl;
  return true;
}
//------------------------------------------------------------------------------
uint8_t csdDmp() {
  csd_t csd;
  uint8_t eraseSingleBlock;
  if (!sd.card()->readCSD(&csd)) {
    sdErrorMsg("readCSD failed");
    return false;
  }
  if (csd.v1.csd_ver == 0) {
    eraseSingleBlock = csd.v1.erase_blk_en;
    eraseSize = (csd.v1.sector_size_high << 1) | csd.v1.sector_size_low;
  } else if (csd.v2.csd_ver == 1) {
    eraseSingleBlock = csd.v2.erase_blk_en;
    eraseSize = (csd.v2.sector_size_high << 1) | csd.v2.sector_size_low;
  } else {
    cout << F("csd version error\n");
    return false;
  }
  eraseSize++;
  cout << F("cardSize: ") << 0.000512*cardSize;
  cout << F(" MB (MB = 1,000,000 bytes)\n");

  cout << F("flashEraseSize: ") << int(eraseSize) << F(" blocks\n");
  cout << F("eraseSingleBlock: ");
  if (eraseSingleBlock) {
    cout << F("true\n");
  } else {
    cout << F("false\n");
  }
  return true;
}
//------------------------------------------------------------------------------
// print partition table
uint8_t partDmp() {
  cache_t *p = sd.vol()->cacheClear();
  if (!p) {
    sdErrorMsg("cacheClear failed");
    return false;
  }
  if (!sd.card()->readBlock(0, p->data)) {
    sdErrorMsg("read MBR failed");
    return false;
  }
  for (uint8_t ip = 1; ip < 5; ip++) {
    part_t *pt = &p->mbr.part[ip - 1];
    if ((pt->boot & 0X7F) != 0 || pt->firstSector > cardSize) {
      cout << F("\nNo MBR. Assuming Super Floppy format.\n");
      return true;
    }
  }
  cout << F("\nSD Partition Table\n");
  cout << F("part,boot,type,start,length\n");
  for (uint8_t ip = 1; ip < 5; ip++) {
    part_t *pt = &p->mbr.part[ip - 1];
    cout << int(ip) << ',' << hex << int(pt->boot) << ',' << int(pt->type);
    cout << dec << ',' << pt->firstSector <<',' << pt->totalSectors << endl;
  }
  return true;
}
//------------------------------------------------------------------------------
void volDmp() {
  cout << F("\nVolume is FAT") << int(sd.vol()->fatType()) << endl;
  cout << F("blocksPerCluster: ") << int(sd.vol()->blocksPerCluster()) << endl;
  cout << F("clusterCount: ") << sd.vol()->clusterCount() << endl;
  cout << F("freeClusters: ");
  uint32_t volFree = sd.vol()->freeClusterCount();
  cout <<  volFree << endl;
  float fs = 0.000512*volFree*sd.vol()->blocksPerCluster();
  cout << F("freeSpace: ") << fs << F(" MB (MB = 1,000,000 bytes)\n");
  cout << F("fatStartBlock: ") << sd.vol()->fatStartBlock() << endl;
  cout << F("fatCount: ") << int(sd.vol()->fatCount()) << endl;
  cout << F("blocksPerFat: ") << sd.vol()->blocksPerFat() << endl;
  cout << F("rootDirStart: ") << sd.vol()->rootDirStart() << endl;
  cout << F("dataStartBlock: ") << sd.vol()->dataStartBlock() << endl;
/*----------------------------------------- print to tft screen  ---------------------------*/                                      
  tft.fillScreen(Color::Black);
  tft.setTextColor(Color::Green, Color::Black);
  tft.setCursor(10, 20);
  tft.setFont(Fonts::Liberation20);
  tft.println("mikroE mx7 SD card info..");
  
  tft.setTextColor(Color::Yellow, Color::Black);
  tft.setFont(Fonts::Liberation16);
  tft.setCursor(10, 55);
  tft.print("volumne is FAT:   ");
  tft.println(int(sd.vol()->fatType()) );
  tft.setCursor(10, 95);
  tft.print("blocksPerCluster: ");
  tft.println(int(sd.vol()->blocksPerCluster()) );
  tft.setCursor(10, 135);
  tft.print("clusterCount:    ");
  tft.println(sd.vol()->clusterCount()  );
  tft.setCursor(10, 160);
  tft.print("freeSpace:       ");
  tft.print(fs );
  tft.println(" MB" );
  tft.setCursor(10, 185);
  tft.print("rootDirStart:   ");
  tft.println(sd.vol()->dataStartBlock() );
  tft.setCursor(10, 210);
  tft.print("cardSize:       ");
  tft.print(0.000512*cardSize );
  tft.println(" MB");
  tft.println(" (MB = 1,000,000 bytes)");
  
  if (sd.vol()->dataStartBlock() % eraseSize) {
    cout << F("Data area is not aligned on flash erase boundaries!\n");
    cout << F("Download and use formatter from www.sdsd.card()->org/consumer!\n");
  }
}

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  delay(5000);
  Serial.println("mx7 SD card info..");

  if (! touch.begin(0x44)) {
    Serial.println("STMPE not found!");
    while(1) {
      Serial.println("in the while loop");
      delay(500);
    }
  }
  Serial.print("getVersion: 0x"); Serial.println(touch.getVersion(), HEX);
  Serial.print("getMode:    0x"); Serial.println(touch.getMode(), HEX);
  
  tft.initializeDevice(SSD1963::MIKROMEDIA);
  delay(20);
  tft.setBacklight(120);
  tft.fillScreen(Color::Blue);
  tft.setTextWrap(false);
 // tft.setRotation(1);
  tft.setTextColor(Color::Yellow, Color::Blue);
  tft.setCursor(10, 20);
  tft.setFont(Fonts::Liberation20);
  tft.println("mikroE mx7 SD card info..");
  delay(2000);

  pinMode(ledPinRed, OUTPUT);
  pinMode(ledPinGreen, OUTPUT);
  pinMode(ledPinBlue, OUTPUT);
 
  // use uppercase in hex and use 0X base prefix
  cout << uppercase << showbase << endl;

  // pstr stores strings in flash to save RAM
  cout << F("SdFat version: ") << SD_FAT_VERSION << endl;
  if (DISABLE_CHIP_SELECT < 0) {
    cout << F(
           "\nAssuming the SD is the only SPI device.\n"
           "Edit DISABLE_CHIP_SELECT to disable another device.\n");
  } else {
    cout << F("\nDisabling SPI device on pin ");
    cout << int(DISABLE_CHIP_SELECT) << endl;
    pinMode(DISABLE_CHIP_SELECT, OUTPUT);
    digitalWrite(DISABLE_CHIP_SELECT, HIGH);
  }
  cout << F("\nAssuming the SD chip select pin is: ") <<int(SD_CHIP_SELECT);
  cout << F("\nEdit SD_CHIP_SELECT to change the SD chip select pin.\n");
  
  
}

void loop()
{
  digitalWrite(ledPinBlue,HIGH);
  delay(250);   
  digitalWrite(ledPinBlue,LOW); 
  digitalWrite(ledPinGreen,LOW); 
  
  tft.setTextColor(Color::Yellow, Color::Black);
  tft.setFont(Fonts::Liberation16);
  tft.setCursor(10, 55);
  tft.print("Starting test.. ");
  tft.setCursor(10, 245);
  tft.setFont(Fonts::Liberation16);
  tft.print("                                 ");

  uint32_t t = millis();
  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  if (!sd.cardBegin(SD_CHIP_SELECT, SPI_HALF_SPEED)) {
    sdErrorMsg("\ncardBegin failed");
    return;
  }
  t = millis() - t;

  cardSize = sd.card()->cardSize();
  if (cardSize == 0) {
    sdErrorMsg("cardSize failed");
    return;
  }
  cout << F("\ninit time: ") << t << " ms" << endl;
  cout << F("\nCard type: ");
  switch (sd.card()->type()) {
  case SD_CARD_TYPE_SD1:
    cout << F("SD1\n");
    break;

  case SD_CARD_TYPE_SD2:
    cout << F("SD2\n");
    break;

  case SD_CARD_TYPE_SDHC:
    if (cardSize < 70000000) {
      cout << F("SDHC\n");
    } else {
      cout << F("SDXC\n");
    }
    break;

  default:
    cout << F("Unknown\n");
  }
  if (!cidDmp()) {
    return;
  }
  if (!csdDmp()) {
    return;
  }
  uint32_t ocr;
  if (!sd.card()->readOCR(&ocr)) {
    sdErrorMsg("\nreadOCR failed");
    return;
  }
  cout << F("OCR: ") << hex << ocr << dec << endl;
  if (!partDmp()) {
    return;
  }
  if (!sd.fsBegin()) {
    sdErrorMsg("\nFile System initialization failed.\n");
    return;
  }
  volDmp();
  delay(10000);

   tft.fillScreen(Color::Black);
   tft.setTextColor(Color::Yellow, Color::Black);
   tft.setCursor(10, 245);
   tft.setFont(Fonts::Liberation16);

  digitalWrite(ledPinBlue,HIGH);
  delay(250);       
  digitalWrite(ledPinBlue,LOW); 
  delay(250);   
    digitalWrite(ledPinRed,HIGH);
  delay(250);      
  digitalWrite(ledPinRed,LOW); 
  delay(250);   
  
    for (int cnt = 10; cnt >= 0; cnt--) { // delay 10 seconds then rerun
       digitalWrite(ledPinRed,HIGH);
       
       tft.setCursor(10, 245);
       tft.setFont(Fonts::Liberation16);
       tft.print("delaying ");
       tft.print(cnt);
       tft.println(" seconds.....");
       
       Serial.print("delaying ");
       Serial.print(cnt);
       Serial.println(" seconds");
      
      delay(500); 
      digitalWrite(ledPinRed,LOW); 
      delay(500); 
    }
  
}
