#include <SuitStrip.h>
#include <OctoWS2811.h>

//----------OCTO LED STUFF----------

//largest strip has 87 LEDs others are 
//less but will give values of 0 for extras
const int ledsPerStrip = 87;

DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];

const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);


//----------SERIAL STRING PARSING----------
boolean stringComplete = false;
String inputString = "";

int r = 255;
int g = 255;
int b = 255;
float bright = 1.0;


//----------STRIP OBJECT SETUP----------
#define NUMSTRIPS 5

SuitStrip stripList[NUMSTRIPS] = {
  SuitStrip(0),   //TOP LEFT (FRONT FACING) 
  SuitStrip(1),   //TOP RIGHT
  SuitStrip(2),   //BOTTOM LEFT
  SuitStrip(3),   //BOTTOM RIGHT
  SuitStrip(4)    //HELMET
};



void setup() {

  Serial.begin(9600);


  leds.begin();
  leds.show();




}

void loop() {

  if (stringComplete) {


    char prefix = inputString[0];

    Serial.println(inputString);

    /*
       Prefixes:
          c = color set
          b = brightness set
          a = animation set


    */

    if (prefix == 'c') {

      //now let's parse it. First find the place of the first comma
      int firstComma = inputString.indexOf(',');

      //then find the second one by search AFTER the first comma's position
      int secondComma = inputString.indexOf(',', firstComma + 1);

      //then use the comma positions to divide the string
      r = inputString.substring(1, firstComma).toInt();
      g = inputString.substring(firstComma + 1, secondComma).toInt();
      b = inputString.substring(secondComma + 1, inputString.length()).toInt();

    } else if ( prefix == 'b' ) {

      bright = inputString.substring(1, inputString.length()).toFloat();
      
    }

    stringComplete = false;
    inputString = "";
  }

  int currentRed = r * bright;
  int currentGreen = g * bright;
  int currentBlue = b * bright;
  
  Serial.print("RED: ");
  Serial.print(currentRed);
  Serial.print(", GREEN: ");
  Serial.print(currentGreen);
  Serial.print(", BLUE: ");
  Serial.println(currentBlue); 
  delay(10);


  for (int i = 0; i < leds.numPixels(); i++) {

    int stripNum = floor(i / ledsPerStrip);
//    leds.setPixel(i, stripList[stripNum].getRed(),
//                  stripList[stripNum].getGreen(),
//                  stripList[stripNum].getBlue());
    leds.setPixel(i, currentRed, currentGreen, currentBlue);
  }

  leds.show();
}



void serialEvent()
{
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
      return;
    } else {
      // add it to the inputString:
      inputString += inChar;
    }
  }
}


