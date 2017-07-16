/*
     Logging GPS data on SD card via UART2 (i.e. UEXT connector) i.e.(Serial1) 
     on PinguinoOTG also with DX TFT display shield    
     NOTE uses SDfat faster storage library
    
     revised  8 July 2017 Anthony Parker
 
       NOTE: requires a file "SETUP.TXT" to be on the SD card
**** SETUP.TXT*****

    ' SETUP.TXT, setup details for PinginoOTGgpslog sketch.
    ' GPS Tracker
    ' For updates and notes etc
    ' Anything after a single quote (') is ignored as a comment.
    ' For example,  TZ +9.5  is Australian Central Standard Time which is nine and a half hours ahead of UTC.
     TZ +10       ' this is Australian Eastern Standard Time.

     KML 5        ' record a Google Earth KML track every five seconds (zero means default at 1 second).
     KMLMARK 60   ' place an interval pin every minute (ie, 60 seconds).
     NMEA 0       ' record a the NMEA data every this number of seconds.

**** END SETUP.TXT****
*/
# include <TinyGPS.h>   									       // include the TinyGPS library:
# include <DSPI.h>
# include <SPI.h>
# include <SdFat.h>                  	 					    // use SDfat faster library

# include <ILI9481.h>
# include <Liberation.h>

ILI9481 tft(A2, A1, A0, A3, A4, 8, 9, 2, 3, 4, 5, 6, 7);

void printTotft();     										                 // function to print values on tft lcd

static void print_date(TinyGPS &gps);
static void print_time(TinyGPS &gps);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_float(float val, float invalid, int len, int prec);
static void Latconvert_float(float val);
static void Lonconvert_float(float val);
void open_kml();
void start_kml();                    						         // at GPS lock write start time and date header
void open_txt();
void write_txt();
void end_kml();
void point_kml();
void readSetupFile();
void Update_File_Exist(char filename[20],int mF); // update file name, Update_File_Exist(kmlname,1); i.e. kmlname, myFile1
void updateSDfile(int pos);
void conv_floatspd(float val);
void conv_floatspc(float val);
void conv_float(float val);

char kmlname[20]  = "GPS00000.KML"; 			// SD card filename
char textname[20] = "TXT00000.TXT"; 				// SD card filename
const int StatusLED =  2          ; 						      // the number of the LED pin
//*****************************************************************************************************************************
// setup file details
//*****************************************************************************************************************************
char filename[20] = "SETUP.TXT";              // SD card "SETUP.TXT" filename
int i;
char *p, SBuf[256];
float TimeZone;
int KmlInterval;
int KmlMarkInterval;                                     // seconds interval for saving the point flag in the kml file
int NmeaInterval;
//************************************************************************************************************
#define GPSBAUD 9600                             //19200 // Olimex GPS baudrate

const int chipSelect_SD_default = 36;        // Change to suit your board // Default SD chip select for Pinguino Micro 51, OTG 36, Sheild 10
const int chipSelect_SD = chipSelect_SD_default;
char sd[32];                                                  // date
char fsd[32];                                                 // filename date
char st[32];                                                   // time
char sc[32];                                                  // course
char sp[32];                                                  // speed
char spd[32];                                                // speed
char spc[32];                                                // heading
char bufflat[32];                                            // char arry for the LAT in ddd mm ss format with S
char bufflon[32];                                           // char arry for the LON in ddd mm ss format with E
int flagStart = 0;                                           // flagstart for start up
float flat, flon, fcourse, fspeed, faltitude;     // float values for latitude, longitude, course, speed
int km, k;                                                      // counter for the KML point flag, and KML interval in the kml file
byte month, day, hour, minute, second, hundredths;
/*****************************************************************************************************************************
 Log files header, footers and placemarks
******************************************************************************************************************************/
#define KML_HEAD    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
#define KML_HEAD1   "<kml xmlns=\"http:\/\/earth.google.com/kml/2.1\"><Document>"
#define KML_BAL1    "  <name>Data+BalloonStyle</name>"
#define KML_BAL2    "  <Style id=\"boat-balloon-style\">"
#define KML_BAL3    "    <BalloonStyle>"
#define KML_BAL4    "      <text>"
#define KML_BAL5    "        <![CDATA["
#define KML_BAL6    "          This is $[name]"
#define KML_BAL7    "          Speed is $[pointSpeed] kts"
#define KML_BAL8    "          Heading is $[pointHeading] degrees"
#define KML_BAL9    "          Date is $[pointDate] GMT"
#define KML_BAL10   "        ]]>"
#define KML_BAL11   "      </text>"
#define KML_BAL12   "    </BalloonStyle>"
#define KML_BAL13   "  </Style>"
#define KML_HEAD2   "<name> My GPS track </name>"
#define KML_HEAD3   "<Style id=\"style1\"><LineStyle><color>990000ff</color><width>4</width></LineStyle></Style><Placemark> <name>Polyline 1</name> <description></description> <styleUrl>#style1</styleUrl> <LineString> <altitudeMode>relative</altitudeMode> <coordinates>"
#define KML_FOOT    "</coordinates></LineString></Placemark>"
#define KML_CLOSE   "</Document></kml>"

#define LOG_COLUMN_HEADER  "date," "time," "longitude," "latitude," "course," "speed," "altitude," "satellites"   // printed at the top of the file.
#define MAX_LOG_FILES 100                     			      // Number of log files that can be made
/******************************************************************************************************************************/
SdFat SD;                                                               // instance of SDfat i.e. SD
File myFile, myFile1, myFile2;                         	      // myFile for the SETUP.TXT, myFile1 for the GPS00000.KML, myFile2 for the TXT00000.TXT
TinyGPS gps;                                                         // set-up instance of GPS

void setup() {
  	Serial.begin(9600);                                              // output to the serial console
  	Serial1.begin(GPSBAUD);                                  // input from the GPS NOTE: OLIMEX GPS baud 9600
  	delay(3000);
  	Serial.println("GPS data logger version 0.01");
   
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
    readSetupFile();                             		      // read "SETUP.TXT" file for operational values

  	Serial.print("Simple TinyGPS library v. ");
  	Serial.println(TinyGPS::library_version());
  
  	tft.initializeDevice();
    delay(20);
  	tft.setBacklight(255);
  	tft.setTextWrap(false);
  	tft.setRotation(1);
  	tft.fillScreen(Color::Black);
  	tft.setTextColor(Color::Green);
    tft.setCursor(5, 15);
    tft.setFont(Fonts::Liberation24);
  	tft.println("GPS log v0.1");
  
     pinMode(StatusLED, OUTPUT); 								// StatusLED pin  
  	 delay(1000);
}// end setup

//--------------------------------  START MAIN LOOP  --------------------------------------------------------------------------------------------------
void loop() {
	int newData = 0;
	unsigned long chars;
	unsigned short sentences, failed;
	char c;

	// For one second we parse GPS data and report some key values
    	for(unsigned long start = millis(); millis() - start < 1000;) {
    		while(Serial1.available()) {
    			c = Serial1.read();
    			//Serial.print(c); 								  // NOTE: uncomment this line if you want to see the GPS data flowing
    			if(gps.encode(c))  								  // Did a new valid sentence come in?
    			{ newData = 1; }
    		}
    	}
//---------------------------------------new data from GPS--------------------------------
	if(newData == 1) {
        digitalWrite(StatusLED, HIGH);            				  // here because I may blink a LED
    		unsigned long age, date, time, chars = 0;
    		print_date(gps);
    		print_time(gps);
    		gps.f_get_position(&flat, &flon, &age);
    
    		Serial.print("LAT=");
        Latconvert_float(flat);
        Serial.print(bufflat);
    		//Serial.print(flat,6); 							  // == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    		Serial.print(" LON=");
        Lonconvert_float(flon);
        Serial.print(bufflon);
    		//Serial.print(flon,6); 							  // == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    		Serial.print(" SAT=");
    		Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
    		Serial.println(" ");
    		//Serial.print(" PREC=");
    		//Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());
   
        printTotft();

	//---------------------------------------first fix file start-------------------------------
    if(flagStart == 0) {  // check for first fix and write kml start flag header

        sprintf(fsd, "%02d_%02dv00.kml", day, month);           // SD card filename for KML google earth file
        strcpy(kmlname,fsd);                                    // make filename with date i.e. 29_05v00.kml
        //Serial.println(kmlname);
        sprintf(fsd, "%02d_%02dv00.txt", day, month);           // SD card filename for text file of all gps data
        strcpy(textname,fsd);                                   // make filename with date i.e. 29_05v00.txt
        //Serial.println(textname);
        
        Update_File_Exist(kmlname,1);                 // check if a test file exists and update file number
        Update_File_Exist(textname,2);                 // check if a test file exists and update file number   
/*
         tft.setCursor(5, 65);
         tft.print(kmlname);
         tft.setCursor(5, 120);
         tft.print(textname);
         delay(2000);
*/
        myFile1 = SD.open(kmlname, FILE_WRITE);                 // if the file opened okay, write to it:
            if(myFile1) {
              Serial.print("Writing start header to kml file....");
              start_kml();
      
              delay(50);
              Serial.println("done");
            } else {
              Serial.println("error opening kml file.....");        // if the file didn't open, print an error:
            }
      } // end flagStart loop

//----------------------------------------KML file write------------------------------------
    if(KmlInterval == 0) {                                    // check if Kml time interval "equals 0 means write every second"
          myFile1 = SD.open(kmlname, FILE_WRITE);                 // if the file opened okay, write to it:
    
          if(myFile1) {
            Serial.print("Writing data to kml file....");
            updateSDfile(191);                                    // update KML file and footer includes km KmlMarkInterval
    
            delay(50);
            Serial.println(" done");
          } else {
            Serial.println("error opening kml file.....");        // if the file didn't open, print an error:
          }
    
          k = 0;
    }                                                                       // end KmlInterval loop

    if(KmlInterval == k) {                                       // check for Kml time interval then write file
          myFile1 = SD.open(kmlname, FILE_WRITE);                 // if the file opened okay, write to it:
    
          if(myFile1) {
            Serial.print("Writing data to kml file....");
            updateSDfile(191);                                     // update KML file and footer includes km KmlMarkInterval
    
            delay(50);
            Serial.println(" done");
          } else {
            Serial.println("error opening kml file.....");        // if the file didn't open, print an error:
          }
    
          k = 0;
    }// end KmlInterval loop

//----------------------------------------TXT file write------------------------------------
        write_txt();                                              // update the text file data
//---------------------------------------------end file writes-----------------------------
    //digitalWrite(2, LOW);                                   // may blink a LED
    km = km + 1;                                              // update the KmlMarkInterval loop counter
    k = k + 1;                                                // update the KmlInterval loop counter
  } // end newdata loop
//---------------------------------------end new data from GPS------------------------------
      digitalWrite(StatusLED, LOW);                               // here because I may blink a LED
    	gps.stats(&chars, &sentences, &failed);
    	//Serial.print(" CHARS=");
    	//Serial.print(chars);
    	Serial.print("SENTENCES=");
    	Serial.print(sentences);
    	Serial.print(" ");
    
    	//Serial.print(" CSUM ERR=");
    	//Serial.println(failed);
    	if(chars == 0)
    	{ Serial.println("** No characters received from GPS: check wiring **"); }

}// end main loop

/*-----------------------------------------------FUNCTIONS------------------------------------------------------------------------------------------------------- */
// *************************************************************************************************************************
// static void print_date(TinyGPS &gps)
// *****************************************************************************
  static void print_date(TinyGPS &gps) {
  	int year;
  	//byte month, day, hour, minute, second, hundredths;
  	unsigned long age;
  	gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  
  	if(age == TinyGPS::GPS_INVALID_AGE)
  	{ Serial.print("********** ******** "); }
  	else {
  		sprintf(sd, "%02d/%02d/%02d", day, month, year);
  		Serial.println(sd); // date and time here
  	}
  
  	print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  	smartdelay(0);
  }//~
// *****************************************************************************
// static void print_float(float val, float invalid, int len, int prec)
// *****************************************************************************
  static void print_float(float val, float invalid, int len, int prec)
  {
    if (val == invalid)
    {
      while (len-- > 1)
        Serial.print('*');
      Serial.print(' ');
    }
    else
    {
      Serial.print(val, prec);
      int vi = abs((int)val);
      int flen = prec + (val < 0.0 ? 2 : 1); // . and -
      flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
      for (int i=flen; i<len; ++i)
        Serial.print(' ');
    }
    smartdelay(0);
  }//~
// *****************************************************************************
// static void Lonconvert_float(float val)
// *****************************************************************************
  static void Lonconvert_float(float val)
  {                                                                    // convert to ddd mm ss
      int degree      = (int)val;
      int minutes     = (int) ( (val - (float)degree) * 60.f);
      int seconds    = (int) ( (val - (float)degree - (float)minutes / 60.f) * 60.f * 60.f );
      char direction = (val < 0 ? 'W' : 'E');
      sprintf (bufflon, " %d %d %d %c", degree, minutes,seconds,direction);
  
    smartdelay(0);
  }//~
// *****************************************************************************
// static void Latconvert_float(float val)
// *****************************************************************************
  static void Latconvert_float(float val)
  {                                                                     // convert to ddd mm ss
      int degree      = (int)val;
      int minutes     = (int) ( (val - (float)degree) * 60.f );
      int seconds    = (int) ( (val - (float)degree - (float)minutes / 60.f) * 60.f * 60.f) ;
      char direction = (val < 0 ? 'S' : 'N');
      sprintf (bufflat, " %d %d %d %c", degree*-1, minutes*-1,seconds*-1,direction);
  
    smartdelay(0);
  }//~
// *****************************************************************************
// static void print_print_time(TinyGPS &gps)
// *****************************************************************************
  static void print_time(TinyGPS &gps) {
  	int year;
  	byte month, day, hour, minute, second, hundredths;
  	unsigned long age;
  	gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  
  	if(age == TinyGPS::GPS_INVALID_AGE)
  	{ Serial.print("********** ******** "); }
  	else {
  		sprintf(st, "%02d:%02d:%02d",
  		        hour, minute, second);
  		Serial.print(st); // date and time here
  	}
  
  	print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  	smartdelay(0);
  }//~
// *****************************************************************************
// static void conv_float(float val)
// *****************************************************************************
  void conv_floatspd(float val) {
    sprintf(spd,"%03.1f",val);
  }//~
// *****************************************************************************
// static void conv_float(float val)
// *****************************************************************************
  void conv_floatspc(float val) {
    sprintf(spc,"%03.1f",val);
  }//~
// *****************************************************************************
// static void conv_float(float val)
// *****************************************************************************
  void conv_float(float val) {
    sprintf(sp,"%03.1f",val);
  }//~
// *****************************************************************************
// static void print_int(unsigned long val, unsigned long invalid, int len)
// *****************************************************************************
  static void print_int(unsigned long val, unsigned long invalid, int len) {
  	char sz[32];
  
  	if(val == invalid)
  	{ strcpy(sz, "*******"); }
  	else
  	{ sprintf(sz, "%ld", val); }
  
  	sz[len] = 0;
  
  	for(int i=strlen(sz); i<len; ++i)
  	{ sz[i] = ' '; }
  
  	if(len > 0)
  	{ sz[len-1] = ' '; }
  
  	Serial.print(sz);
  	smartdelay(0);
  }//~
// *****************************************************************************
// void start_kml() write a placemarke at the start of the kml google earth file
// *****************************************************************************
  void start_kml() { // #define KML_START   "<Placemark><name>Start %s (%s)</name><Point><coordinates>%f,%f</coordinates></Point></Placemark>\n"
  	char buf[256];
  	sprintf(buf,"<Placemark><name>Start %s (%s)</name><Point><coordinates>%f,%f</coordinates></Point></Placemark>", st, sd, flon, flat);                            // format the entry
  
  	if(myFile1) {
  		Serial.print("Current File position for writing: ");
  		Serial.print(myFile1.position());
  		myFile1.seek(myFile1.position()-189);
  		myFile1.println(buf);
  		myFile1.println(KML_HEAD3);
  		end_kml();
  	} else {
  		Serial.println("error opening start_kml");                         // if the file didn't open, print an error:
  	}
  	flagStart = 1;
  }//~
// *****************************************************************************
// void write_txt() write gps data to the text file
// *****************************************************************************
  void write_txt() {
    myFile2 = SD.open(textname, FILE_WRITE);
  
    if(myFile2) {
      Serial.print("Writing data to text file...");
  
      myFile2.print(sd);
      myFile2.print(",");
      myFile2.print(st);
      myFile2.print(",");
      myFile2.print(flon,6);
      myFile2.print(",");
      myFile2.print(flat,6);
      myFile2.print(",");
      conv_floatspc(gps.f_course());
      myFile2.print(spc);
      myFile2.print(",");
      conv_floatspd(gps.f_speed_knots());
      myFile2.print(spd);
      myFile2.print(",");
      conv_float(gps.f_altitude());
      myFile2.print(sp);
      myFile2.print(",");
      myFile2.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
      myFile2.println(" ");
      myFile2.close();
  
      delay(50);
      Serial.println("done");
    } else {
      Serial.println("error opening TXT file.....");                        // if the file didn't open, print an error:
    } // end text file loop
  }//~
// *****************************************************************************
// void open_kml() write the format header for the xml kml google earth file
// *****************************************************************************
  void open_kml() {
    myFile1.println(KML_HEAD);
    myFile1.println(KML_HEAD1);
    myFile1.println(KML_HEAD2);
    myFile1.println(KML_BAL1);
    myFile1.println(KML_BAL2);
    myFile1.println(KML_BAL3);
    myFile1.println(KML_BAL4);
    myFile1.println(KML_BAL5);
    myFile1.println(KML_BAL6);
    myFile1.println(KML_BAL7);
    myFile1.println(KML_BAL8);
    myFile1.println(KML_BAL9);
    myFile1.println(KML_BAL10);
    myFile1.println(KML_BAL11);
    myFile1.println(KML_BAL12);
    myFile1.println(KML_BAL13);
    myFile1.println(KML_FOOT);
    myFile1.println("<Placemark><name>Finish 11:22:33 (01/02/03)</name><Point><coordinates>144.946960,-37.823753</coordinates></Point></Placemark>"); // fake first placemark
    myFile1.println(KML_CLOSE);
    myFile1.close();
  }//~
// *****************************************************************************
// void open_txt() write column headers to the text file
// *****************************************************************************
  void open_txt() {
    myFile2.println(LOG_COLUMN_HEADER);
  	myFile2.close();
  }//~
// *****************************************************************************
// void end_kml() end marker of the kml google earth file
// *****************************************************************************
  void end_kml() {
    char buf[256];
    sprintf(buf,"<Placemark><name>Finish %s (%s)</name><Point><coordinates>%f,%f</coordinates></Point></Placemark>", st, sd, flon, flat); // format the entry
    myFile1.println(KML_FOOT);
    myFile1.println(buf);                                            			// Finish placemark
    myFile1.println(KML_CLOSE);
    myFile1.close();
  }//~
// *****************************************************************************
// void point_kml() formating for a flag box in the kml google earth file
// *****************************************************************************
  void point_kml() {
    char buf[256];
    myFile1.println("</coordinates></LineString></Placemark>");
    myFile1.println("<Placemark>");
    sprintf(buf,"    <name>%s</name>",st);                                   // format the entry time
    myFile1.println(buf);
    myFile1.println("  <styleUrl>#boat-balloon-style</styleUrl>");
    myFile1.println("    <ExtendedData>");
    myFile1.println("      <Data name=\"pointSpeed\">");
    sprintf(buf,"        <value>%s</value>",spd);                            // format the entry speed
    myFile1.println(buf);
    myFile1.println("      </Data>");
    myFile1.println("      <Data name=\"pointHeading\">");
    sprintf(buf,"        <value>%s</value>", spc);                           // format the entry heading
    myFile1.println(buf);
    myFile1.println("      </Data>");
    myFile1.println("      <Data name=\"pointDate\">");
    sprintf(buf,"        <value>%s</value>", sd);                            // format the entry
    myFile1.println(buf);
    myFile1.println("      </Data>");
    myFile1.println("    </ExtendedData>");
    myFile1.println("    <Point>");
    sprintf(buf,"      <coordinates>%f,%f  </coordinates>", flon,flat);      // format the entry
    myFile1.println(buf);
    myFile1.println("    </Point>");
    myFile1.println("  </Placemark>");
    myFile1.println(KML_HEAD3);
  }//~
// *****************************************************************************
// static void smartdelay(unsigned long ms)
// *****************************************************************************
  static void smartdelay(unsigned long ms) {
  	unsigned long start = millis();
  
  	do {
  		while(Serial1.available())
  		{ gps.encode(Serial1.read()); }
  	} while(millis() - start < ms);
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
                                                                                         // Set logFileName to "DD_MMvXX.txt":
      filename[6] = (i/10) + '0';
      filename[7] = (i%10) + '0';
  
      if (!SD.exists(filename))
        break;                                                                       // We found our index
      Serial.print(filename);
      Serial.println( F(" exists") );
    }
          if(mF == 1) {                                                            //--- Create the KML file
                  myFile1 = SD.open(filename, FILE_WRITE);
          
                  if(myFile1) {                                                     //--- if the file opened okay, write to it:
                    Serial.print("Writing kml header...");
                    open_kml();                                                     // writes header to kml file
                    Serial.println("done.");
                  }
          }
          else if (mF == 2) {                                                           //--- Create the TXT file
                  myFile2 = SD.open(filename, FILE_WRITE);
          
                  if(myFile2) {                                                     //--- if the file opened okay, write to it:
                    Serial.print("Writing text header...");
                    open_txt();                                                   // writes start of txt file
                    Serial.println("done.");
                  }          
          }
    //}
    Serial.print( F("File name: ") );
    Serial.println(filename);
  }//~
// *****************************************************************************
// void updateSDfile(int pos), NOTE pos moves file pointer back from the end of file
// *****************************************************************************
  void updateSDfile(int pos) {
    if(myFile1) {
      if(km == KmlMarkInterval) {
              Serial.print("Point File position for writing: ");
              Serial.print(myFile1.position());
              myFile1.seek(myFile1.position()-pos);
              point_kml();
              end_kml();
              km = 0;
      } else {
              Serial.print("Current File position for writing: ");
              Serial.print(myFile1.position());
              myFile1.seek(myFile1.position()-pos);
              myFile1.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
              myFile1.print(",");
              myFile1.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
              myFile1.println(" ");
              end_kml();
            }
    } else {
      Serial.println("error opening updateSDfile");                          // if the file didn't open, print an error:
    }
  }//~
// *****************************************************************************
// void readSetupFile()
// *****************************************************************************
  void readSetupFile() {
    myFile = SD.open(filename);                                              // open the file for reading. note that only one file can be open at a time,
  
    if(myFile) {                                                             // does the SETUP.TXT file exist
      Serial.print("Reading from SETUP file: ");
      Serial.println(filename);
  
      while(myFile.available()) {                                            // load the configuration data from the root of the SD card.  If this fails then it is a fatal error
        char c, pc = 0;
        int ic;
  
        for(ic = i = 0; i < sizeof(SBuf) - 1;) {
          if(!myFile.read(&c,1)) { break; }                                  // read a character
          if(c == '\n' || c == '\r') { ic = false; }                         // if it is a new line it is the end of a comment
          if(c == '\'') { ic = true; }                                       // if it is ' it is the start of a comment
          if(!ic && !(pc == ' ' && c == ' ')) { SBuf[i++] = c; }             // if we are not in a comment and it is not multiple spaces save the char in our buffer
          pc = c;                                                            // this is used to detect multiple space chars (which we do not want)
        }
  
        SBuf[i] = 0;                                                         // terminate the string retrieved
        myFile.close();                                                      // close the file
      }
  
      for(p = SBuf; *p; p++) {                                               // convert the string loaded from the configuration file to a format ready for scanning
        if(!isalnum(*p) && *p != '-' && *p != '.') { *p = ' '; }
  
        *p = toupper(*p);
      }                                                                      // load the configuration data
      if(strstr(SBuf, "TZ ")) { TimeZone = atof(strstr(SBuf, "TZ ") + 3); }
      if(strstr(SBuf, "KML ")) { KmlInterval = atoi(strstr(SBuf, "KML ") + 4); }
      if(strstr(SBuf, "KMLMARK ")) { KmlMarkInterval = atoi(strstr(SBuf, "KMLMARK ") + 7); }
      if(strstr(SBuf, "NMEA ")) { NmeaInterval = atoi(strstr(SBuf, "NMEA ") + 5); }
      //if(TimeZone < -24 || TimeZone > 24 || KmlMarkInterval < 0 || KmlInterval < 0 || NmeaInterval < 0);
    } else {                                                                              // else some default values if NO setup.txt
      TimeZone        = 10;                                                  // +10 GMT
      KmlInterval     = 5;                                                   // k
      KmlMarkInterval = 60;                                                  // km
      NmeaInterval    = 0;                                                   // not used
    }
  
        delay(100);
        Serial.print(" Time Zone        :");
        Serial.println(TimeZone);
        Serial.print(" Kml Interval     :");
        Serial.println(KmlInterval);                                             // k
        Serial.print(" Kml Mark Interval:");
        Serial.println(KmlMarkInterval);                                         // km
        Serial.print(" Nmea Interval    :");
        Serial.println(NmeaInterval);
        delay(200);
  }//~
// *****************************************************************************
// void printTotft()
// *****************************************************************************
  void printTotft() {
    tft.setFont(Fonts::Liberation30);
    tft.setCursor(5, 65);
    tft.print("LAT:  ");
    //tft.println(flat,6);
    tft.println(bufflat);                                                    // print to screen in ddd mm ss format
    tft.setCursor(5, 120);
    tft.print("LON: ");
   // tft.println(flon,6);
    tft.println(bufflon);                                                    // print to screen in ddd mm ss format
    conv_float(gps.f_course());
    tft.setCursor(5, 175);
    tft.print("HDG: ");
    tft.print(sp);
    //tft.printf("%s\xf8",sp);                                               // degree symbol ???????????????????
    tft.println(" T     ");
    tft.println((char)223);                                                  // degree symbol ???????????????????
    conv_float(gps.f_speed_knots());
    tft.setCursor(5, 230);
    tft.print("SPD: ");
    tft.print(sp);
    tft.println(" Kts    ");
    tft.setCursor(5, 280);
    tft.setFont(Fonts::Liberation18);
    tft.print("Date: ");
    tft.print(sd);
    tft.print(" GMT: ");
    tft.println(st);
  }//~
