/*
  PIC32 Neopixel Library Demo                    
  A simple to use library for addressable LEDs like the WS2812                             

 UECIDE    : v0.8.9-pre14
 Hardware  : Olimex Pinguino32 OTG
 Core      : chipKIT
 Compiler  : PIC32 Compiler version 1.43
 Programmer: pic32prog
 Other     : WS2812 RGB LED strip of 16 leds with the data line on pin (4)
  
  This library is protected under the GNU GPL v3.0 license          
  http://www.gnu.org/licenses/ 
 */
#include <PICxel.h>

#define number_of_LEDs 16
#define LED_pin 16                    //  on pin 16 (UEXT pin5) or you choose
#define millisecond_delay 50

//PICxel constructor(uint8_t # of LEDs, uint8_t pin #, color_mode GRB or HSV);
PICxel strip(number_of_LEDs, LED_pin, GRB);

void setup(){
  strip.begin();
  strip.setBrightness(50);
  strip.clear();
}

void loop(){
  for(int i=0; i<strip.getNumberOfLEDs(); i++){
    strip.GRBsetLEDColor(i, 255, 0, 0);//Red
    strip.refreshLEDs();
    delay(millisecond_delay);
  }
  
  for(int i=strip.getNumberOfLEDs(); i>=0; i--){
    strip.GRBsetLEDColor(i, 155, 155, 0);//Orange
    strip.refreshLEDs();
    delay(millisecond_delay);
  }

  for(int i=0; i<strip.getNumberOfLEDs(); i++){
    strip.GRBsetLEDColor(i, 0, 100, 155);//Yellow
    strip.refreshLEDs();
    delay(millisecond_delay);
  }
  
  for(int i=strip.getNumberOfLEDs(); i>=0; i--){
    strip.GRBsetLEDColor(i, 0, 255, 0);//Green
    strip.refreshLEDs();
    delay(millisecond_delay);
  }

  for(int i=0; i<strip.getNumberOfLEDs(); i++){
    strip.GRBsetLEDColor(i, 0, 0, 255);//Blue
    strip.refreshLEDs();
    delay(millisecond_delay);
  }
  
  for(int i=strip.getNumberOfLEDs(); i>=0; i--){
    strip.GRBsetLEDColor(i, 150, 0, 150);//Indigo
    strip.refreshLEDs();
    delay(millisecond_delay);
  }

    for(int i=0; i<strip.getNumberOfLEDs(); i++){
    strip.GRBsetLEDColor(i, 150, 0, 200);//Violet
    strip.refreshLEDs();
    delay(millisecond_delay);
  }
}



















