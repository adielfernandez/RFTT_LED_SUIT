#include "Arduino.h"
#include "SuitStrip.h"

SuitStrip::SuitStrip(int _id) {

//STRIP SEGMENTS AND THEIR IDs
//TOP LEFT  (FRONT FACING) (87)   = 0
//TOP RIGHT				   (87)	  = 1
//BOTTOM LEFT			   (60)   = 2
//BOTTOM RIGHT			   (60)   = 3
//Helmet 				   (48)   = 4

	ID = _id;

	//our arrays are sometimes larger than we need so initialize
	//them all to 0 before filling with appropriate values
	for(int i = 0; i < MAX_LEDS; i++){
		redArray[i] = 0;
		greenArray[i] = 0;
		blueArray[i] = 0;
		redOut[i] = 0;
		greenOut[i] = 0;
		blueOut[i] = 0;
		// triggerStates[i] = false;
		lastTriggerTimes[i] = 0;
	}

	if(ID <= 1){
		numLEDs = 87;

		//store the known heights in an array
		int h[] = { 567,579,590,601,613,625,636,648,659,671,681,693,704,718,728,
					740,749,760,771,782,783,772,761,750,740,728,717,705,695,683,
					672,661,650,510,521,533,544,555,567,578,644,655,667,677,689,
					700,711,722,734,744,756,767,778,787,787,787,787,777,765,754,
					742,731,720,709,697,686,675,663,652,640,629,618,607,595,584,
					572,561,549,538,527,515,505,493,482,470,459,448 };

		//fill the global heights array with he known heights.
		//Easier this way since the dynamic array allocation doesn't
		//support bracket{} style initialization
		for(int i = 0; i < numLEDs; i++){
			heights[i] = h[i];
		}

	} else if(ID <= 3){
		numLEDs = 60;

		int h[] = { 216,227,239,250,261,273,284,296,307,318,330,341,353,367,375,
					383,391,399,407,407,400,391,383,375,367,352,341,329,317,306,
					295,284,272,262,251,239,228,216,6,17,28,40,51,63,75,85,106,
					114,122,122,113,105,85,74,63,51,39,28,17,5 };

		//fill the global heights array with he known heights.
		//Easier this way since the dynamic array allocation doesn't
		//support bracket{} style initialization
		for(int i = 0; i < numLEDs; i++){
			heights[i] = h[i];
		}

	} else {
		numLEDs = 48;

		// all helmet leds at top
		int h[] = { 1084,1071,1060,1049,1037,1026,1014,1003,992,980,969,957,946,
					935,854,865,877,888,899,911,921,921,921,921,922,921,921,921,
					911,899,888,877,865,854,935,946,958,970,981,992,1003,1014,
					1026,1037,1049,1060,1071,1083 };

		for(int i = 0; i < numLEDs; i++){
			heights[i] = h[i];
		}
		
	}


	//set initial states
	for(int i = 0; i < numLEDs; i++){
		redArray[i] = 0;
		greenArray[i] = 0;
		blueArray[i] = 0;

		redOut[i] = 0;
		greenOut[i] = 0;
		blueOut[i] = 0;

		// triggerStates[i] = false;
	}




	animationStart = 0;

	brightnessPct = 1.0;

	rTarget = 0;
    gTarget = 0;
    bTarget = 0;

    pulseStartTime = 0;
    bPulseEvent = false;

    bDummyMode = true;

}





void SuitStrip::triggerFlashAt(int i, int r, int g, int b){

	redArray[i] = r;
	greenArray[i] = g;
	blueArray[i] = b;

	// triggerStates[i] = true;
	lastTriggerTimes[i] = millis();

}

void SuitStrip::startPulseEvent(){
	pulseStartTime = millis();
	bPulseEvent = true;


	for(int i = 0; i < numLEDs; i++){
		redArray[i] = 0;
		greenArray[i] = 0;
		blueArray[i] = 0;
		redOut[i] = 0;
		greenOut[i] = 0;
		blueOut[i] = 0;
	}
	// for(int i = 0; i < numLEDs; i++){
	// 	triggerStates[i] = false;
	// }

}

void SuitStrip::endPulseEvent(){
	bPulseEvent = false;

	// for(int i = 0; i < numLEDs; i++){
	// 	triggerStates[i] = false;
	// }

}



void SuitStrip::updateVals() {


	if( !bDummyMode ){

		if( bPulseEvent ){

			//during wave event all LEDs will be fading down
			//but select ones will flash brighter (then fade down)
			float fadeDownSpeed = 0.05;

			for(int i = 0; i < numLEDs; i++){
				redArray[i] = getLerped(redArray[i], 0, fadeDownSpeed);
				greenArray[i] = getLerped(greenArray[i], 0, fadeDownSpeed);
				blueArray[i] = getLerped(blueArray[i], 0, fadeDownSpeed);

				redOut[i] = redArray[i];
				greenOut[i] = greenArray[i];
				blueOut[i] = blueArray[i];	
			}

		} 


	} else {

		//lerp all the out colors to their targets
		float lerpSpeed = 0.2;
		for(int i = 0; i < numLEDs; i++){
			redOut[i] = getLerped(redOut[i], redArray[i], lerpSpeed);
			greenOut[i] = getLerped(greenOut[i], greenArray[i], lerpSpeed);
			blueOut[i] = getLerped(blueOut[i], blueArray[i], lerpSpeed);
		}

	}






	brightnessPct = constrain(brightnessPct, 0.0, 1.0);
}

int SuitStrip::getRed(int i){  
	if(i >= 0 && i < numLEDs){
	 	return redOut[i] * brightnessPct;
	} else {
		return -1;
	}
}

int SuitStrip::getGreen(int i){
if(i >= 0 && i < numLEDs){
	 	return greenOut[i] * brightnessPct;
	} else {
		return -1;
	}
}

int SuitStrip::getBlue(int i){
if(i >= 0 && i < numLEDs){
	 	return blueOut[i] * brightnessPct;
	} else {
		return -1;
	}
}

//Arduino friendly (i.e. floating point friendly) Lerp method
float SuitStrip::getLerped(float start, float end, float _pct){
	float precision = 1000;

	float lerped;
	lerped = start * precision + (end * precision - start * precision) * _pct;
	return lerped/precision;
}

//not used, mostly for debug
void SuitStrip::setAllColor(int _r, int _g, int _b){
	for(int i = 0; i < numLEDs; i++){
		redArray[i] = _r;
		greenArray[i] = _g;
		blueArray[i] = _b;
	}
}

void SuitStrip::setColorAt(int i, int r, int g, int b){
	redArray[i] = r;
	greenArray[i] = g;
	blueArray[i] = b;
}

void SuitStrip::setBrightness(float br){
	brightnessPct = constrain(br, 0.0, 1.0);
}

//Arduino friendly (i.e. floating point friendly) map + constrain method
float SuitStrip::map_clamp(float v, float minIn, float maxIn, float minOut, float maxOut){

	float minClamp = min(minOut, maxOut) * 1000.0f;
	float maxClamp = max(minOut, maxOut) * 1000.0f;

	return constrain( map(v, minIn, maxIn, minOut * 1000.0f, maxOut * 1000.0f) , minClamp, maxClamp)/1000.0f;
}
