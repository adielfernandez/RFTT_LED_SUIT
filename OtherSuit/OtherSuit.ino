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

//----------STRIP OBJECT SETUP----------
#define NUMSTRIPS 5

SuitStrip stripList[NUMSTRIPS] = {
  SuitStrip(0),   //TOP LEFT (FRONT FACING)
  SuitStrip(1),   //TOP RIGHT
  SuitStrip(2),   //BOTTOM LEFT
  SuitStrip(3),   //BOTTOM RIGHT
  SuitStrip(4)    //HELMET
};


//----------ANIMATION STUFF----------

//PULSE
bool bPulse = false;
bool bStopPulse = false;

float pulseSpeed = 0.5;
int maxHeight = 1110;
unsigned long pulseStartTime = 0;

#define NUMPULSES 4
float pulseHeights[NUMPULSES];

int pulseColor[] = {255, 100, 0};
float pulseBrightness = 1.0;


//FLICKER
bool bFlicker = false;
unsigned int flickerInterval = 100;
int shortFlickerInterval = 100;
int longFlickerInterval = 500;
float lowFlickerBright = 0.05;
float highFlickerBright = 1.0;
float flickerBrightness = 1.0;

int flickerColorChoice = 0;
int flickerColor1[] = {255, 100, 0};
int flickerColor2[] = {100, 100, 255};
int currentFlickerCol[] = {255, 255, 255};

unsigned long lastFlickerTime = 0;

bool flickerStates[] = { false, false, false, false, false };

//SINE WAVE
bool bSineWave = false;
float sineTime = 0;
float sineSpeed = 0.05;
float sineLowBright = 0.1;
float sineHighBright = 1.0;

int sineColor[] = {255, 100, 0};

//TRAVELLING WAVE
bool bTravelWave = false;
float travelSineTime = 0;
float travelPhaseDiff = TWO_PI;
float travelSineSpeed = 0.05;
float travelLowBright = 0.0f;
float travelHighBright = 1.0f;

int travelColor[] = {255, 100, 0};

//----------LED COLORS----------
const int orange[]    = {255,  43,   0};
const int amber[]     = {255,  70,   0};
const int yellow[]    = {255, 112,   0};
//const int lightBlue[] = {135, 210, 231};
const int lightBlue[] = {110, 180, 255};

bool bTestingMode = false;
bool bAnimating = false;
unsigned long animationStartTime = 0;


//-----Button input-----
//NOTE: Input is a latching switch,
//not a momentary press. So state changes based on a
//input CHANGE event, i.e. rising or falling edge

int lastButtonState;
int buttonPin = 22;

unsigned int debounceTime = 10;
unsigned long lastButtonCheck = 0;


void setup() {

  //enable serial
  Serial.begin(9600);

  //start the strips
  leds.begin();
  leds.show();

  //setup pulses
  for (int i = 0; i < NUMPULSES; i++) {
    pulseHeights[i] = 0 + (i / (float)NUMPULSES) * maxHeight;
  }

  //setup input
  pinMode(buttonPin, INPUT_PULLUP);

}

void loop() {

  //---------------------------------------------
  //---------------BUTTON HANDLING---------------
  //---------------------------------------------

  //NOTE: input is a LATCHING switch, not a momentary pushbutton
  //so we're looking for any state change as a trigger, either rising or falling edge
  if ( millis() - lastButtonCheck > debounceTime ) {

    int currentButtonState = digitalRead(buttonPin);

    if ( currentButtonState != lastButtonState ) {

      //flip the animation state
      bAnimating = !bAnimating;

      //Print out state
      Serial.print("State Switched to: ");
      Serial.println(bAnimating);

      //if we're switching INTO animating mode, turn off all
      //effects and testing mode
      if ( bAnimating == 1 ) {
        allEffectsOff();

        //Also start the sine timer over
        //but start from PI/2 so sine starts at 0
        sineTime = -PI / 2.0f;

        bTestingMode = false;
      }

    }

    //store this state as the last one for the next time we check
    lastButtonState = currentButtonState;
    lastButtonCheck = millis();

  }

  //Ignore the button and stop any animation for the first half a second
  //to wait for any noise to quiet down while the teensy starts up
  //to prevent accidentally reading a state change
  if ( millis() < 500 ) {
    bAnimating = false;
  }


  //----------------------------------------------------
  //---------------SERIAL INPUT FOR DEBUG---------------
  //----------------------------------------------------
  if (stringComplete) {
    char prefix = inputString[0];

    //set all effects to off before turning one on
    allEffectsOff();

    //echo back serial input
    Serial.println(inputString);

    /*
        p - speed   (good val = 5)
        s - speed   (good val = 0.05)
        t - overall phase diff,speed  (good vals = 40, 0.05)
        f - min, max interval  (good vals = 50,100)
        SPACE - turn off testing mode

    */

    bTestingMode = true;

    if ( prefix == 'p' ) {

      int firstComma = inputString.indexOf(',');
      pulseSpeed = inputString.substring(1, firstComma).toFloat();
      pulseBrightness = inputString.substring(firstComma + 1, inputString.length()).toFloat();

      Serial.print("Pulse Speed: ");
      Serial.print( pulseSpeed );
      Serial.print(", brightness: ");
      Serial.println( pulseBrightness );

      startPulse();

    } else if ( prefix == 's' ) {

      //if receive only 's' with no number shut it off
      if ( inputString.length() == 1 ) {
        bSineWave = false;
      } else {

        bSineWave = true;

        sineSpeed = inputString.substring(1, inputString.length()).toFloat();
        Serial.print("Sine Speed: ");
        Serial.println( sineSpeed );

        //        sineColor[0] = lightBlue[0];
        //        sineColor[1] = lightBlue[1];
        //        sineColor[2] = lightBlue[2];
      }


    } else if ( prefix == 't' ) {

      //if receive only 's' with no number shut it off
      if ( inputString.length() == 1 ) {
        bTravelWave = false;
      } else {

        bTravelWave = true;

        int firstComma = inputString.indexOf(',');
        travelPhaseDiff = inputString.substring(1, firstComma).toFloat();
        travelSineSpeed = inputString.substring(firstComma + 1, inputString.length()).toFloat();
        Serial.print("Phase Difference: ");
        Serial.print( travelPhaseDiff );
        Serial.print(", speed: ");
        Serial.println( travelSineSpeed);

      }


    } else if ( prefix == 'f' ) {

      if ( inputString.length() == 1 ) {
        bFlicker = false;
      } else {

        bFlicker = true;

        int firstComma = inputString.indexOf(',');
        shortFlickerInterval = inputString.substring(1, firstComma).toInt();
        longFlickerInterval = inputString.substring(firstComma + 1, inputString.length()).toInt();

        Serial.print("Interval: ");
        Serial.print( shortFlickerInterval );
        Serial.print(", ");
        Serial.println( longFlickerInterval );

      }

    } else if ( prefix == ' ' ) {

      //turn off testing mode if we get a space
      bTestingMode = !bTestingMode;
    }

    stringComplete = false;
    inputString = "";

  }



  //-----------------------------------------------
  //---------------ANIMATION CONTROL---------------
  //-----------------------------------------------

  //Actual segment animation

  if ( !bAnimating ) {

    //we're not animating, all effects off, all lights off
    if ( !bTestingMode ) {
      allEffectsOff();

      for (int i = 0; i < NUMSTRIPS; i++) {
        stripList[i].setAllColor(0, 0, 0);
      }
    }

    //if we're not animating, keep logging the animationStartTime
    //so it's current the next time we DO start animating

    animationStartTime = millis();



  } else {


    //------------------------------------------------
    //---------------ANIMATION TIMELINE---------------
    //------------------------------------------------

    int startDelay = 3000;

    //SEGMENT TIME STAMPS (mult. by 1000 to get milliseconds)
//    unsigned long segment0_End  = 1000 * 12.15;  //12.15 SECONDS
//    unsigned long segment1_End  = 1000 * 52;
//    unsigned long segment2_End  = 1000 * 72.10;
//    unsigned long segment3_End  = 1000 * 85.19;
//    unsigned long segment4_End  = 1000 * 98.02;
//    unsigned long segment5_End  = 1000 * 129.20;
//    unsigned long segment6_End  = 1000 * 138;
//    unsigned long segment7_End  = 1000 * 203;
//    unsigned long segment8_End  = 1000 * 230;
//    unsigned long segment9_End  = 1000 * 289.27;
//    unsigned long segment10_End = 1000 * 337.1;
//    unsigned long segment11_End = 1000 * 360;
//    unsigned long segment12_End = 10000000;  //"forever"

    //--------------------------------------------
    //  DEFINE FASTER TIME SEGMENTS FOR DEBUGGING
    // Be sure to comment out the variables above!
    //--------------------------------------------

    //time for each segment
    int d = 6000;

    unsigned long segment0_End  = d * 1;
    unsigned long segment1_End  = d * 2;
    unsigned long segment2_End  = d * 3;
    unsigned long segment3_End  = d * 4;
    unsigned long segment4_End  = d * 5;
    unsigned long segment5_End  = d * 6;
    unsigned long segment6_End  = d * 7;
    unsigned long segment7_End  = d * 8;
    unsigned long segment8_End  = d * 9;
    unsigned long segment9_End  = d * 10;
    unsigned long segment10_End = d * 11;
    unsigned long segment11_End = d * 12;
    unsigned long segment12_End = d * 13;
    unsigned long segment13_End = 10000000;


    //time since starting the animation PLUS the intentional delay
    unsigned long timeNow;

    bool bDelaying;

    if ( millis() - animationStartTime < startDelay ) {
      bDelaying = true;
    } else {
      bDelaying = false;
    }


    if ( bDelaying ) {
      timeNow = 0;
      Serial.println("Delaying start...");
    } else {
      timeNow = millis() - animationStartTime - startDelay;
    }




    //--------------------SEGMENT 0--------------------
    if ( bDelaying || timeNow < segment0_End ) {

      //turn off all other effects
      allEffectsOff();

      if ( !bDelaying ) Serial.println("[Segment0] waiting with indicator");

      float indicatorBrightness = 0.1; //out of one

      //All LEDs off
      for (int i = 0; i < NUMSTRIPS; i++) {
        stripList[i].setAllColor(0, 0, 0);
      }

      //except a dim indicator on the
      //bottom of the left arm
      //Strip #1, LED # 33
      //if we're in the delay period, light RED
      //if we're in the actual segment0 period, light GREEN
      if ( bDelaying ) {
        stripList[1].setColorAt(33, 255 * indicatorBrightness, 0, 0);
      } else {
        stripList[1].setColorAt(33, 255 * indicatorBrightness, 255 * indicatorBrightness, 0);
      }

    }

    //--------------------SEGMENT 1--------------------
    else if ( timeNow < segment1_End ) {

      //turn off all other effects
      allEffectsOff();

      Serial.println("[Segment1] Slow traveling wave");

      bTravelWave = true;
      travelSineSpeed = 0.09;

      //range of brightness
      travelLowBright = 0.0;
      travelHighBright = 0.5;

      //density of waves
      //correlated but not equal to num waves
      //(value of 50 is not equal to 50 waves)
      travelPhaseDiff = 50;

      travelColor[0] = lightBlue[0];
      travelColor[1] = lightBlue[1];
      travelColor[2] = lightBlue[2];

    }

    //--------------------SEGMENT 2--------------------
    else if ( timeNow < segment2_End ) {

      Serial.println("[Segment2] 4 Pulses moving up");

      startPulse();

      pulseSpeed = 4;
      pulseBrightness = 0.4;

      pulseColor[0] = lightBlue[0];
      pulseColor[1] = lightBlue[1];
      pulseColor[2] = lightBlue[2];

    }

    //--------------------SEGMENT 3--------------------
    else if ( timeNow < segment3_End ) {

      Serial.println("[Segment3] 4 Pulses moving up");

      startPulse();

      pulseSpeed = 3;
      pulseBrightness = 0.7;

      pulseColor[0] = lightBlue[0];
      pulseColor[1] = lightBlue[1];
      pulseColor[2] = lightBlue[2];


    }

    //--------------------SEGMENT 4--------------------
    else if ( timeNow < segment4_End ) {

      //turn off all other effects
      allEffectsOff();

      Serial.println("[segment4] All Off");

      //set all to black
      for (int i = 0; i < NUMSTRIPS; i++) {
        stripList[i].setAllColor(0, 0, 0);
      }

    }

    //--------------------SEGMENT 5--------------------
    else if ( timeNow < segment5_End ) {

      //turn off all other effects
      allEffectsOff();

      Serial.println("[Segment5] Fade In & Out");

      //All LEDs fade up from 0 to 50% brightness
      bSineWave = true;
      sineSpeed = 0.02;

      //range of brightness
      sineLowBright = 0.05;
      sineHighBright = 0.6;

      sineColor[0] = lightBlue[0];
      sineColor[1] = lightBlue[1];
      sineColor[2] = lightBlue[2];
      
    }

    //--------------------SEGMENT 6--------------------
    else if ( timeNow < segment6_End ) {

      Serial.println("[Segment6] 4 Pulses moving up");

      startPulse();

      pulseSpeed = 4;
      pulseBrightness = 1.0;

      pulseColor[0] = lightBlue[0];
      pulseColor[1] = lightBlue[1];
      pulseColor[2] = lightBlue[2];
      
    }

    //--------------------SEGMENT 7--------------------
    else if ( timeNow < segment7_End ) {

      //turn off all other effects
      allEffectsOff();

      Serial.println("[Segment7] Slow traveling wave");

      bTravelWave = true;
      travelSineSpeed = 0.04;

      //range of brightness
      travelLowBright = 0.0;
      travelHighBright = 1.0;

      //density of waves
      //correlated but not equal to num waves
      //(value of 50 is not equal to 50 waves)
      travelPhaseDiff = 50;

      travelColor[0] = lightBlue[0];
      travelColor[1] = lightBlue[1];
      travelColor[2] = lightBlue[2];
      
    }

    //--------------------SEGMENT 8--------------------
    else if ( timeNow < segment8_End ) {

      //turn off all other effects
      allEffectsOff();

      Serial.println("[Segment8] Slow traveling wave");

      bTravelWave = true;
      travelSineSpeed = 0.04;

      //range of brightness
      travelLowBright = 0.0;
      travelHighBright = 0.75;

      //density of waves
      //correlated but not equal to num waves
      //(value of 50 is not equal to 50 waves)
      travelPhaseDiff = 50;

      travelColor[0] = lightBlue[0];
      travelColor[1] = lightBlue[1];
      travelColor[2] = lightBlue[2];
      
    }

    //-------------------SEGMENT 9--------------------
    else if ( timeNow < segment9_End ) {

      Serial.println("[Segment9] 4 Pulses moving up");

      startPulse();

      pulseSpeed = 3;
      pulseBrightness = 1.0;

      pulseColor[0] = lightBlue[0];
      pulseColor[1] = lightBlue[1];
      pulseColor[2] = lightBlue[2];
      
    }

    //-------------------SEGMENT 10--------------------
    else if ( timeNow < segment10_End ) {

      Serial.println("[Segment10] 4 Pulses moving up");

      startPulse();

      pulseSpeed = 8;
      pulseBrightness = 1.0;

      pulseColor[0] = lightBlue[0];
      pulseColor[1] = lightBlue[1];
      pulseColor[2] = lightBlue[2];
      
    }

    //-------------------SEGMENT 11--------------------
    else if ( timeNow < segment11_End ) {

      //turn off all other effects
      allEffectsOff();

      Serial.println("[Segment11] Glitch");

      //turn on flicker
      bFlicker = true;

      //set flicker characteristics
      //range of flicker durations
      shortFlickerInterval = 20;
      longFlickerInterval = 80;

      //range in brightness of flicker
      lowFlickerBright = 0.80;
      highFlickerBright = 1.0;

      //set flicker color (0,1,2 = R,G,B)
      flickerColor1[0] = orange[0];
      flickerColor1[1] = orange[1];
      flickerColor1[2] = orange[2];

      //manual set to white, one of the 4 color words
      //can be used also, as above
      flickerColor2[0] = orange[0];
      flickerColor2[1] = orange[1];
      flickerColor2[2] = orange[2];
      
    }

    //-------------------SEGMENT 12--------------------
    else if ( timeNow < segment12_End ) {

      //turn off all other effects
      allEffectsOff();

      Serial.println("[Segment12] Fade In & Out");

      //All LEDs fade up from 0 to 50% brightness
      bSineWave = true;
      sineSpeed = 0.05;

      //range of brightness
      sineLowBright = 0.2;
      sineHighBright = 1.0;

      sineColor[0] = orange[0];
      sineColor[1] = orange[1];
      sineColor[2] = orange[2];
      
    }



  }



  //---------------SINE ANIMATION---------------
  if ( bSineWave ) {

    setAllDummyMode(true);

    //go through all the LEDs and set them to pulse in phase

    float range = sineHighBright - sineLowBright;
    float midBright = sineLowBright + range / 2.0f;

    sineTime += sineSpeed;

    //sine wave applied to ALL LEDs
    float sine = midBright + range / 2.0f * sin( sineTime );

    //go through all strips and set them to the sine wave
    //all in sync with no phase difference (stationary wave)
    for (int i = 0; i < NUMSTRIPS; i++) {
      stripList[i].setAllColor(sineColor[0] * sine, sineColor[1] * sine, sineColor[2] * sine);
    }

    //    Serial.println(sine);


  }


  //---------------TRAVELLING WAVE ANIMATION---------------
  if ( bTravelWave ) {

    setAllDummyMode(true);

    float range = travelHighBright - travelLowBright;
    float midBright = travelLowBright + range / 2.0f;

    //define now but set within the for loop
    //since each LED is given a different phase
    travelSineTime +=  travelSineSpeed;


    float sine;

    //set the value of each of the leds to the sine with the phase proportional to
    //their height
    for (int i = 0; i < NUMSTRIPS; i++) {
      for ( int j = 0; j < stripList[i].numLEDs; j++) {

        float phaseShift = map( stripList[i].heights[j], 0, maxHeight, 0.0f, travelPhaseDiff * 1000.0f) / 1000.0f;

        sine = midBright + range / 2.0f * sin( travelSineTime - phaseShift);

        stripList[i].setColorAt( j, travelColor[0] * sine, travelColor[1] * sine, travelColor[2] * sine);

        //        if (i == 0 && j == 0) Serial.println(sine);


      }
    }



  }


  //---------------PULSE ANIMATION---------------
  if ( bPulse ) {

    setAllDummyMode(false);

    if ( !bStopPulse ) {

      for (int i = 0; i < NUMPULSES; i++) {
        pulseHeights[i] += pulseSpeed;
      }


      //go through each wave and check LEDs
      for (int w = 0; w < NUMPULSES; w++) {

        //check if any leds are below this position and trigger
        //them if they havent already been flashed
        for (int i = 0; i < NUMSTRIPS; i++) {

          //go through each LED in that strip
          for (int j = 0; j < stripList[i].numLEDs; j++) {
            if (stripList[i].heights[j] <= pulseHeights[w] && stripList[i].heights[j] >= pulseHeights[w] - abs(pulseSpeed)) {
              stripList[i].triggerFlashAt(j, pulseColor[0] * pulseBrightness, pulseColor[1] * pulseBrightness, pulseColor[2] * pulseBrightness);
            }
          }
        }

        if ( pulseHeights[w] > maxHeight) pulseHeights[w] = 0;
        if ( pulseHeights[w] < 0) pulseHeights[w] = maxHeight;

      }

      //      Serial.println( pulseHeights[0] );

    }

  }


  //---------------FLICKER ANIMATION---------------
  if ( bFlicker ) {

    setAllDummyMode(true);

    //randomly pick one or two strips to light then pick
    //another random interval for the next time
    if ( millis() - lastFlickerTime > flickerInterval ) {

      //set all states to false
      for (int i = 0; i < NUMSTRIPS; i++) {
        flickerStates[i] = false;
      }

      int randNum = floor( random(5) );
      flickerStates[randNum] = true;

      //multiply by 100, get random, then divide by 100 to get floats
      flickerBrightness = random( lowFlickerBright * 100, highFlickerBright * 100) / 100.0f;
      flickerInterval = random(shortFlickerInterval, longFlickerInterval);

      //choose a new random color
      int randCol = floor( random(2) );

      if (randCol == 0 ) {
        currentFlickerCol[0] = flickerColor1[0];
        currentFlickerCol[1] = flickerColor1[1];
        currentFlickerCol[2] = flickerColor1[2];
      } else {
        currentFlickerCol[0] = flickerColor2[0];
        currentFlickerCol[1] = flickerColor2[1];
        currentFlickerCol[2] = flickerColor2[2];
      }


      lastFlickerTime = millis();

    }


    for (int i = 0; i < NUMSTRIPS; i++) {

      if ( flickerStates[i] ) {
        stripList[i].setAllColor(currentFlickerCol[0], currentFlickerCol[1], currentFlickerCol[2]);
        stripList[i].setBrightness(flickerBrightness);
      } else {
        stripList[i].setAllColor(0, 0, 0);
      }

    }

  }


  //DEBUG PRINTING
  //    Serial.print(stripList[0].getRed(0));
  //    Serial.print(", ");
  //    Serial.print(stripList[0].getGreen(0));
  //    Serial.print(", ");
  //    Serial.println(stripList[0].getBlue(0));
  //    delay(10);



  //update the strip objects data containers
  for (int i = 0; i < NUMSTRIPS; i++) {
    stripList[i].updateVals();
  }

  //assign strip object data to actual LEDs
  for (int i = 0; i < leds.numPixels(); i++) {

    //get the strip number and led number from the total LED number
    int stripNum = floor(i / ledsPerStrip);
    int thisLED = i % ledsPerStrip;

    //The OCTOLED library assumes there are the max number
    //of LEDs in each strip, so make sure we don't go out of bounds
    //when trying to access the color values
    if ( thisLED < stripList[stripNum].numLEDs ) {
      leds.setPixel(i, stripList[stripNum].getRed(thisLED),
                    stripList[stripNum].getGreen(thisLED),
                    stripList[stripNum].getBlue(thisLED)   );
    }

  }

  leds.show();
}

void startPulse() {

  //if we're not already pulsing, set up the pulse
  if ( !bPulse ) {

    //turn off other effects
    allEffectsOff();

    for (int i = 0; i < NUMSTRIPS; i++) {
      stripList[i].startPulseEvent();
    }

    bPulse = true;

    bStopPulse = false;
    pulseStartTime = millis();
  }





}

void allEffectsOff() {

  bPulse = false;
  bSineWave = false;
  bTravelWave = false;
  bFlicker = false;

  //also go through and set all strip brightnesses to 1.0
  //just in case they were changed before
  for (int i = 0; i < NUMSTRIPS; i++) {
    stripList[i].setBrightness(1.0);
  }

}

void setAllDummyMode(bool state) {

  for (int i = 0; i < NUMSTRIPS; i++) {
    stripList[i].bDummyMode = state;
  }

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

//-----------------------------------------------------------
//--------------------CONVENIENCE METHODS--------------------
//-----------------------------------------------------------

//Arduino sucks at floating point math, these methods help

float map_clamp(float v, float minIn, float maxIn, float minOut, float maxOut) {

  float minClamp = min(minOut, maxOut) * 1000000.0f;
  float maxClamp = max(minOut, maxOut) * 1000000.0f;

  return constrain( map(v, minIn, maxIn, minOut * 1000000.0f, maxOut * 1000000.0f) , minClamp, maxClamp) / 1000000.0f;

}

float getLerped(float start, float end, float _pct) {
  float precision = 1000;

  float lerped;
  lerped = start * precision + (end * precision - start * precision) * _pct;
  return lerped / precision;
}


