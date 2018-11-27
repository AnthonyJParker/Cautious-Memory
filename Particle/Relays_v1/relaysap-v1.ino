/*
 * FILE:        DHT_example.cpp
 * VERSION:     0.4
 * PURPOSE:     Example that uses DHT library with two sensors
 * LICENSE:     GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 * Example that start acquisition of DHT sensor and allows the
 * loop to continue until the acquisition has completed
 * It uses DHT.acquire and DHT.acquiring
 *
 * Change DHT_SAMPLE_TIME to vary the frequency of samples
 *
 * Scott Piette (Piette Technologies) scott.piette@gmail.com
 *      January 2014        Original Spark Port
 *      October 2014        Added support for DHT21/22 sensors
 *                          Improved timing, moved FP math out of ISR
 *      September 2016      Updated for Particle and removed dependency
 *                          on callback_wrapper.  Use of callback_wrapper
 *                          is still for backward compatibility but not used
 * ScruffR
 *      February 2017       Migrated for Libraries 2.0
 *                          Fixed blocking acquireAndWait()
 *                          and previously ignored timeout setting
 *                          Added timeout when waiting for Serial input
 *
 * With this library connect the DHT sensor to the following pins
 * Spark Core: D0, D1, D2, D3, D4, A0, A1, A3, A5, A6, A7
 * Particle  : any Pin but D0 & A5
 * See docs for more background
 *   https://docs.particle.io/reference/firmware/photon/#attachinterrupt-
 */

#include "PietteTech_DHT.h"  

 // system defines
#define DHTTYPE  DHT11              // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   D4           	    // Digital pin for communications
#define DHT_SAMPLE_INTERVAL   2000  // Sample every two seconds

/*
 * NOTE: Use of callback_wrapper has been deprecated but left in this example
 *       to confirm backwards compabibility.  Look at DHT_2sensor for how
 *       to write code without the callback_wrapper
 */
 
 // Function Protoype
int led_on(String command);
char temp[8];
char humi[8];

int led7 = D7 ;// This one is the built-in tiny one to the right of the USB jack
int led0 = D0 ;//Relays
int led1 = D1 ;
int led2 = D2 ;
int led3 = D3 ;
 //declaration
void dht_wrapper(); // must be declared before the lib initialization

// Lib instantiate
PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);

// globals
unsigned int DHTnextSampleTime;	    // Next time we want to start sample
bool bDHTstarted;		    // flag to indicate we started acquisition
int n;                              // counter

void setup()
{
  Serial.begin(9600);
  // register the Spark function
  Spark.function("led_on", led_on);

  // register the Spark variable
  Spark.variable("temp", temp);
  Spark.variable("humi", humi);

  // set output on Relay PORTS and D7
  pinMode(led7, OUTPUT);
  pinMode(led0, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  DHTnextSampleTime = 0;  // Start the first sample immediately
}


/*
 * NOTE:  Use of callback_wrapper has been deprecated but left in this example
 * to confirm backwards compatibility.
 */
 // This wrapper is in charge of calling
 // must be defined like this for the lib work
void dht_wrapper() {
  DHT.isrCallback();
}

void loop()
{
  // Check if we need to start the next sample
  if (millis() > DHTnextSampleTime) {
    if (!bDHTstarted) {		// start the sample
      Serial.print("\n");
      Serial.print(n);
      Serial.print(": Retrieving information from sensor: ");
      DHT.acquire();
      bDHTstarted = true;
    }

    if (!DHT.acquiring()) {		// has sample completed?

        // get DHT status
      int result = DHT.getStatus();

      Serial.print("Read sensor: ");
      switch (result) {
      case DHTLIB_OK:
        Serial.println("OK");
        break;
      case DHTLIB_ERROR_CHECKSUM:
        Serial.println("Error\n\r\tChecksum error");
        break;
      case DHTLIB_ERROR_ISR_TIMEOUT:
        Serial.println("Error\n\r\tISR time out error");
        break;
      case DHTLIB_ERROR_RESPONSE_TIMEOUT:
        Serial.println("Error\n\r\tResponse time out error");
        break;
      case DHTLIB_ERROR_DATA_TIMEOUT:
        Serial.println("Error\n\r\tData time out error");
        break;
      case DHTLIB_ERROR_ACQUIRING:
        Serial.println("Error\n\r\tAcquiring");
        break;
      case DHTLIB_ERROR_DELTA:
        Serial.println("Error\n\r\tDelta time to small");
        break;
      case DHTLIB_ERROR_NOTSTARTED:
        Serial.println("Error\n\r\tNot started");
        break;
      default:
        Serial.println("Unknown error");
        break;
      }

      Serial.print("Humidity (%): ");
      Serial.println(DHT.getHumidity(), 2);

      Serial.print("Temperature (oC): ");
      Serial.println(DHT.getCelsius(), 2);

      Serial.print("Temperature (oF): ");
      Serial.println(DHT.getFahrenheit(), 2);

      Serial.print("Temperature (K): ");
      Serial.println(DHT.getKelvin(), 2);

      Serial.print("Dew Point (oC): ");
      Serial.println(DHT.getDewPoint());

      Serial.print("Dew Point Slow (oC): ");
      Serial.println(DHT.getDewPointSlow());

	sprintf(temp,"%3.2f", DHT.getCelsius());

    sprintf(humi,"%3.2f", DHT.getHumidity());

	Serial.print("Humidity (%): ");
	Serial.println(DHT.getHumidity(), 2);

	Serial.print("Temperature (oC): ");
	Serial.println(DHT.getCelsius(), 2);
	
      n++;  // increment counter
      bDHTstarted = false;  // reset the sample flag so we can take another
      DHTnextSampleTime = millis() + DHT_SAMPLE_INTERVAL;  // set the time for next sample
    }
  }
}



// this function automagically gets called upon a matching POST request
int led_on(String command)
{

  // look for the matching argument "on" or "off" <-- max of 64 characters long
  if(command == "0h")
  {
    digitalWrite(led0, HIGH);   // Turn ON the LED pin
    return 1;
  }
  if(command == "0l")
  {
    digitalWrite(led0, LOW);   // Turn of the LED pin
    return 2;
  }

 if(command == "1h")
  {
    digitalWrite(led1, HIGH);   // Turn ON the LED pin
    return 3;
  }
  if(command == "1l")
  {
    digitalWrite(led1, LOW);   // Turn of the LED pin
    return 4;
  }


 if(command == "2h")
  {
    digitalWrite(led2, HIGH);   // Turn ON the LED pin
    return 5;
  }
  if(command == "2l")
  {
    digitalWrite(led2, LOW);   // Turn of the LED pin
    return 6;
  }


   if(command == "3h")
  {
    digitalWrite(led3, HIGH);   // Turn ON the LED pin
    return 7;
  }
  if(command == "3l")
  {
    digitalWrite(led3, LOW);   // Turn of the LED pin
    return 8;
  }


   if(command == "7h")
  {
    digitalWrite(led7, HIGH);   // Turn ON the LED pin
    return 9;
  }
  if(command == "7l")
  {
    digitalWrite(led7, LOW);   // Turn of the LED pin
    return 10;
  }

  else {

    return 11;
       }

}
