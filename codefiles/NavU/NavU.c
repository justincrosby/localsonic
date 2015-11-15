/* 
 * File:   NavU.c
 * Author: Justin
 *
 * This file contains the firmware for the NavU
 *
 * GPIO pin numbers    wiringPi pin numbers
 * ------------------  ------------------
 * |         |01|02| | |         |3v|5v| |
 * |         |03|04| | |         |08|5v| |
 * |         |05|06| | |         |09|0v| |
 * |         |07|08| | |         |07|15| |
 * |         |09|10| | |         |0v|16| |
 * |         |11|12| | |         |00|01| |
 * |         |13|14| | |         |02|0v| |
 * |         |15|16| | |         |03|04| |
 * |         |17|18| | |         |3v|05| |
 * |         |19|20| | |         |12|0v| |
 * |         |21|22| | |         |13|06| |
 * ................... ...................
 * ------------------- -------------------
 * wiringPi pins 0-7 are general purpose IO pins
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wiringPi.h>

#include "map.h"
#include "sounds.h"

// these are arbitrary assignments for now
#define LED_PIN 0
#define START_PIN 1
#define VOLUP_PIN 8
#define VOLDOWN_PIN 9
#define EMITTER_1 7
#define EMITTER_2 2
#define EMITTER_3 3
#define EMITTER_4 4
#define EMITTER_5 5
// number of periods to emit ultrasonic
#define NUM_CYCLES 10
// period in microseconds (25 is 40kHz)
#define PERIOD 25


// GLOBAL VARIABLES
// default volume is 80%
int vol = 80;
// inline function so that the compiler does inline expansion
// need to be as fast as possible, normal function calls take too long
void inline usGen(int emitterNum){
    int i;
    for(i = 0; i<NUM_CYCLES; i++){
	digitalWrite(emitterNum, LOW);
	delayMicroseconds(PERIOD/2);
	digitalWrite(emitterNum, HIGH);
	delayMicroseconds(PERIOD/2);
    }
}
void playAudio(const char filename[100]){
    char str[100];
    // create a command string, something like 
    // "aplay /root/test.wav" and run it using system cmd
    strcpy(str, "aplay -N ");
    strcat(str, filename);
    system(str);
}
void setVolume(){
    char str[100];
    char volume[5];
    // create a command string, something like 
    // "amixer cset numid=1 -- 80%" and run it using system cmd
    strcpy(str, "amixer cset numid=1 --  ");
    // turn int into string before concatenating
    sprintf(volume, "%d", vol);
    strcat(str, volume);
    strcat(str, "%");
    system(str);
    // play volume sound to acknowledge volume change
    playAudio(VOLUME_SOUND);
}
void userFeedback(int location, float distance, int direction){
    // play a sequence of files that give the user their location
    // i.e. "<location> is <distance> to your <direction>
    // location is given as a unique integer value which maps to a
    // filename in sounds[]
    // distance is given in an integer number +/- 0.5m
    // direction is given as 0 or 1, 0 corresponding to left, 1 is right
    
    // do some calculations first - need to play the sounds quickly
    int intpart = (int)distance;
    float decpart = distance - intpart;
    
    // location name first
    playAudio(map[location]);
    // connecting words
    playAudio(IS_SOUND);
    // play distance
    playAudio(sounds[intpart]);
    // if there's a decimal part play it
    if(decpart != 0){
	playAudio(SOUND_05);
    }
    // connecting words
    playAudio(TOYOUR_SOUND);
    // direction: left or right
    playAudio(lr[direction]);
}
int main(int argc, char** argv) {
    // initial wiringPi setup, run once
    wiringPiSetup();
    pinMode(LED_PIN, OUTPUT);
    pinMode(EMITTER_1, OUTPUT);
    pinMode(EMITTER_2, OUTPUT);
    pinMode(EMITTER_3, OUTPUT);
    pinMode(EMITTER_4, OUTPUT);
    pinMode(EMITTER_5, OUTPUT);
    pinMode(START_PIN, INPUT);
    pinMode(VOLUP_PIN, INPUT);
    pinMode(VOLDOWN_PIN, INPUT);
    // set the initial state
    digitalWrite(EMITTER_1, LOW);
    digitalWrite(EMITTER_2, LOW);
    digitalWrite(EMITTER_3, LOW);
    digitalWrite(EMITTER_4, LOW);
    digitalWrite(EMITTER_5, LOW);
    // set initial volume level
    setVolume();
    
    return 0;
    while(1){
	char filename[100];
	bool volUp, volDown;
	// wait til a button is pressed then continue
	while(!digitalRead(START_PIN) && 
		!(volUp = digitalRead(VOLUP_PIN)) &&
		!(volDown = digitalRead(VOLDOWN_PIN))){}
	// if the volume buttons were pressed do the appropriate operation
	// and start again from the beginning of the loop
	if(volUp){
	    vol += 5;
	    setVolume();
	    continue;
	}
	if(volDown){
	    vol -= 5;
	    setVolume();
	    continue;
	}
	// button testing
	digitalWrite(LED_PIN, HIGH);
	delay(1000);
	digitalWrite(LED_PIN, LOW);
	continue;
	
	// send radio signal
	usGen(EMITTER_1);
	// wait to receive radio signal
	// send radio signal
	usGen(EMITTER_2);
	// wait to receive radio signal
	// send radio signal
	usGen(EMITTER_3);
	// wait to receive radio signal
	// send radio signal
	usGen(EMITTER_4);
	// wait to receive radio signal
	// send radio signal
	usGen(EMITTER_5);
	// wait to receive radio signal
	// calculate the closest distance
	// do logic to decide which audio file to play, save into filename
	
	playAudio(filename);
    }

    return (EXIT_SUCCESS);
}

