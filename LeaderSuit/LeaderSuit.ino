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


//FLICKER
bool bFlicker = false;
unsigned int flickerInterval = 100;
int shortFlickerInterval = 100;
int longFlickerInterval = 500;
float lowFlickerBright = 0.5;
float highFlickerBright = 1.0;
float flickerBrightness = 1.0;

int flickerColor[] = {255, 100, 0};

unsigned long lastFlickerTime = 0;

bool flickerStates[] = { false, false, false, false, false };

//SINE WAVE
bool bSineWave = false;
float sineSpeed = 3.0;
float lowBright = 0.1;
float highBright = 1.0;

//TRAVELLING WAVE
bool bTravelWave = false;
float phaseDiff = TWO_PI;

//----------LED COLORS----------
const int orange[]    = {255,  43,   0};
const int amber[]     = {255,  70,   0};
const int yellow[]    = {255, 112,   0};
const int lightBlue[] = {135, 210, 231};

int waveColor[] = {255, 43, 0};


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
    pulseHeights[i] = 0 - (i / (float)NUMPULSES) * maxHeight;
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
        p - speed
        s - speed
        t - overall phase diff
        f - min, max interval
        SPACE - turn off testing mode

    */

    bTestingMode = true;

    if ( prefix == 'p' ) {

      pulseSpeed = inputString.substring(1, inputString.length()).toFloat();

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

      }


    } else if ( prefix == 't' ) {

      //if receive only 's' with no number shut it off
      if ( inputString.length() == 1 ) {
        bTravelWave = false;
      } else {

        bTravelWave = true;

        phaseDiff = inputString.substring(1, inputString.length()).toFloat();
        Serial.print("Phase Difference: ");
        Serial.println( phaseDiff );

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
      bTestingMode = false;
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

    //SEGMENT DURATIONS
    //Each segment time starts at the end of the previous segment
    //and lasts as long as the number added to it.
    //i.e. int segment3_End = segment2_End + 3000; means segment 3 lasts 3 seconds

    int segment0_End = 4000;
    int segment1_End =  segment0_End + 4000;
    int segment2_End =  segment1_End + 4000;
    int segment3_End =  segment2_End + 4000;
    int segment4_End =  segment3_End + 4000;
    int segment5_End =  segment4_End + 4000;
    int segment6_End =  segment5_End + 4000;
    int segment7_End =  segment6_End + 4000;
    int segment8_End =  segment7_End + 4000;
    int segment9_End =  segment8_End + 4000;

    //time since starting the animation
    unsigned long timeNow = millis() - animationStartTime;

    //--------------------SEGMENT 0--------------------
    if ( timeNow < segment0_End ) {

      Serial.println("[Segment0] waiting with indicator");

      //All LEDs off
      for (int i = 0; i < NUMSTRIPS; i++) {
        stripList[i].setAllColor(0, 0, 0);
      }

      //except a dim red indicator on the
      //bottom of the left arm
      //Strip #1, LED # 33
      stripList[1].setColorAt(33, 10, 0, 0);

    }

    //--------------------SEGMENT 1--------------------
    else if ( timeNow < segment1_End ) {

      Serial.println("[Segment1] slow fade up");

      //All LEDs fade up from 0 to 50% brightness
      int fadeUpDuration = 3000;
      float br = map_clamp( timeNow, segment0_End, segment0_End + fadeUpDuration, 0.0, 0.5);

      for (int i = 0; i < NUMSTRIPS; i++) {
        stripList[i].setAllColor(orange[0], orange[1], orange[2]);
        stripList[i].setBrightness(br);
      }

    }

    //--------------------SEGMENT 2--------------------
    else if ( timeNow < segment2_End ) {

      Serial.println("[Segment2] Flicker");

      //turn off all other effects
      allEffectsOff();
      
      //turn on flicker
      bFlicker = true;

      //set flicker characteristics
      //range in time of flicker
      shortFlickerInterval = 20;
      longFlickerInterval = 80;
      
      //range in brightness of flicker
      lowFlickerBright = 0.50;
      highFlickerBright = 0.75;

      //set flicker color (0,1,2 = R,G,B)
      flickerColor[0] = orange[0];
      flickerColor[1] = orange[1];
      flickerColor[2] = orange[2];

      
    }

    //--------------------SEGMENT 3--------------------
    else if ( timeNow < segment3_End ) {
    }

    //--------------------SEGMENT 4--------------------
    else if ( timeNow < segment4_End ) {
    }

    //--------------------SEGMENT 5--------------------
    else if ( timeNow < segment5_End ) {
    }

    //--------------------SEGMENT 6--------------------
    else if ( timeNow < segment6_End ) {
    }

    //--------------------SEGMENT 7--------------------
    else if ( timeNow < segment7_End ) {
    }

    //--------------------SEGMENT 8--------------------
    else if ( timeNow < segment8_End ) {
    }

    //-------------------SEGMENT 9--------------------
    else if ( timeNow < segment9_End ) {
    }



  }





  //---------------SINE ANIMATION---------------
  if ( bSineWave ) {

    setAllDummyMode(true);

    //go through all the LEDs and set them to pulse in phase

    float range = highBright - lowBright;
    float midBright = lowBright + range / 2.0f;

    //sine wave applied to ALL LEDs
    float sine = midBright + range / 2.0f * sin( sineSpeed * millis() / 1000.f );

    //go through all strips and set them to the sine wave
    //all in sync with no phase difference (stationary wave)
    for (int i = 0; i < NUMSTRIPS; i++) {
      stripList[i].setAllColor(orange[0] * sine, orange[1] * sine, orange[2] * sine);
    }

    //    Serial.println(sine);


  }


  //---------------TRAVELLING WAVE ANIMATION---------------
  if ( bTravelWave ) {

    setAllDummyMode(true);

    highBright = 0.0;
    lowBright = 1.0;

    float range = highBright - lowBright;
    float midBright = lowBright + range / 2.0f;

    //define now but set within the for loop
    //since each LED is given a different phase
    float sine;

    sineSpeed = 6;

    //set the value of each of the leds to the sine with the phase proportional to
    //their height
    for (int i = 0; i < NUMSTRIPS; i++) {
      for ( int j = 0; j < stripList[i].numLEDs; j++) {

        float phaseShift = map( stripList[i].heights[j], 0, maxHeight, 0.0f, phaseDiff * 1000.0f) / 1000.0f;

        sine = midBright + range / 2.0f * sin( sineSpeed * millis() / 1000.f + phaseShift);

        stripList[i].setColorAt( j, lightBlue[0] * sine, lightBlue[1] * sine, lightBlue[2] * sine);

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
            if (stripList[i].heights[j] <= pulseHeights[w] && stripList[i].heights[j] >= pulseHeights[w] - pulseSpeed) {
              stripList[i].triggerFlashAt(j, orange[0], orange[1], orange[2]);
            }
          }
        }

        if ( pulseHeights[w] > maxHeight) pulseHeights[w] = 0;

      }

    }

  }


  //---------------FLICKER ANIMATION---------------
  if ( bFlicker ) {

    //randomly pick one or two strips to light then pick
    //another random interval for the next time
    if ( millis() - lastFlickerTime > flickerInterval ) {

      //set all states to false
      for (int i = 0; i < NUMSTRIPS; i++) {
        flickerStates[i] = false;
      }

      int randNum = floor( random(5) );
      flickerStates[randNum] = true;

      flickerBrightness = random( lowFlickerBright, highFlickerBright );

      lastFlickerTime = millis();

      flickerInterval = random(shortFlickerInterval, longFlickerInterval);

      //      Serial.print("New Interval: ");
      //      Serial.println(flickerInterval);
    }


    for (int i = 0; i < NUMSTRIPS; i++) {

      if ( flickerStates[i] ) {
        stripList[i].setAllColor(flickerColor[0], flickerColor[1], flickerColor[2]);
        stripList[i].setBrightness(flickerBrightness);
      } else {
        stripList[i].setAllColor(0, 0, 0);
      }

    }

  }


  //DEBUG PRINTING
  //    Serial.print(stripList[2].timeNow );
  //    Serial.print(", ");
  //    Serial.print(stripList[2].rTarget );
  //    Serial.print(", ");
  //    Serial.print(stripList[2].gTarget );
  //    Serial.print(", ");
  //    Serial.println(stripList[2].bTarget );
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

  bPulse = true;

  bStopPulse = false;
  pulseStartTime = millis();

  for (int i = 0; i < NUMSTRIPS; i++) {
    stripList[i].startPulseEvent();
  }

}

void allEffectsOff() {

  bPulse = false;
  bSineWave = false;
  bTravelWave = false;
  bFlicker = false;

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

  float minClamp = min(minOut, maxOut) * 1000.0f;
  float maxClamp = max(minOut, maxOut) * 1000.0f;

  return constrain( map(v, minIn, maxIn, minOut * 1000.0f, maxOut * 1000.0f) , minClamp, maxClamp) / 1000.0f;

}

float getLerped(float start, float end, float _pct) {
  float precision = 1000;

  float lerped;
  lerped = start * precision + (end * precision - start * precision) * _pct;
  return lerped / precision;
}


