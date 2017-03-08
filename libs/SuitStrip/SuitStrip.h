#ifndef SuitStrip_h
#define SuitStrip_h

#include "Arduino.h"

#define MAX_LEDS 87


class SuitStrip
{
  public:
    SuitStrip(int _id);
    void setup();
    void updateVals();


    int getRed(int i);
    int getBlue(int i);
    int getGreen(int i);

    //convenience method for maths and stuff
    float getLerped(float start, float end, float pct);

    void triggerFlashAt(int i, int r, int g, int b);
    void startPulseEvent();
    void endPulseEvent();

    void setColorAt(int i, int r, int g, int b);
    void setAllColor(int r, int g, int b);
    void setBrightness(float b);



    float map_clamp(float v, float minIn, float maxIn, float minOut, float maxOut);


//private:

    int ID;
    
    int numLEDs;

    //heights array
    int heights[MAX_LEDS];

    //CURRENT colors array
    float redArray[MAX_LEDS];
    float greenArray[MAX_LEDS];
    float blueArray[MAX_LEDS];

    //TARGET colors array
    float redTargets[MAX_LEDS];
    float greenTargets[MAX_LEDS];
    float blueTargets[MAX_LEDS];  

    unsigned long lastTriggerTimes[MAX_LEDS]; 

    unsigned long animationStart;

    //master control of Light brightness
    //affected by oscillations and
    //and values dependent on states
    float brightnessPct;

    float rTarget;
    float gTarget;
    float bTarget;
    unsigned long timeNow;

    bool bPulseEvent;
    unsigned long pulseStartTime;

    //don't do anything here
    //all action from main arduino
    //sketch
    bool bDummyMode;

};

#endif