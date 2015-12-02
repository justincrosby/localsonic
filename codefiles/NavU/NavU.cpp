/* 
 * File:   NavU.cpp
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
#include <string.h>
#include <wiringPi.h>
#include <time.h>
#include <bitset>
#include <iostream>
#include "/home/pi/rf24libs/RF24/RF24.h"

#include "map.h"
#include "sounds.h"
#include "defs.h"

using namespace std;
// these are arbitrary assignments for now
#define START_PIN 1
#define VOLUP_PIN 8
#define VOLDOWN_PIN 9
#define EMITTER_1 7
#define EMITTER_2 2
#define EMITTER_3 3
#define EMITTER_4 4
#define EMITTER_5 5
// number of periods to emit ultrasonic
#define NUM_CYCLES 200
// period in microseconds (25 is 40kHz)
#define PERIOD 25
#define NUM_NODES 2
#define SAMPLE_DELAY 500
#define LEFT 0
#define RIGHT 1


// GLOBAL VARIABLES
// default volume is 80%
int vol = 80;
RF24 radio(22,0);
const uint8_t writePipe[6] = "1Pipe";
const uint8_t readPipe[2][6] = {"2Pipe", "3Pipe"};
const int emitters[5] = {EMITTER_1, EMITTER_2, EMITTER_3, EMITTER_4, EMITTER_5};
// inline function so that the compiler does inline expansion
// need to be as fast as possible, normal function calls take too long
void inline ping(int emitterNum, char receiverNum){
    for(int i = 0; i < NUM_SAMPLES; i++){
	radio.writeBlocking(&receiverNum, sizeof(receiverNum), 1000);
	radio.txStandBy(1000);
	for(int j = 0; j < NUM_CYCLES; j++){
	    digitalWrite(emitterNum, LOW);
	    delayMicroseconds(PERIOD/2);
	    digitalWrite(emitterNum, HIGH);
	    delayMicroseconds(PERIOD/2);
	}
	delay(SAMPLE_DELAY);
    }
}
uint16_t inline receiveData(char receiverNum){
    struct timespec currTime;
    long int startTime, endTime;
    uint16_t data;
    radio.openReadingPipe(1,readPipe[(int)receiverNum]);
    radio.startListening();
    clock_gettime(CLOCK_REALTIME, &currTime);
    startTime = currTime.tv_nsec;
    while(!radio.available()){
		clock_gettime(CLOCK_REALTIME, &currTime);
        endTime = currTime.tv_nsec;
		if(endTime - startTime >= RECEIVE_TIMEOUT){
		    radio.stopListening();
		    return 0xFFFF;
		}
    }
    radio.read(&data, sizeof(data));
    radio.stopListening();
    return data;
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
    //pinMode(LED_PIN, OUTPUT);
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
    // setup radio    
    radio.begin();
    radio.printDetails();
    radio.openWritingPipe(writePipe);
    
    
    while(1){
		char data;
		uint16_t nodeData[NUM_NODES * NUM_EMITTERS];
		int location[NUM_NODES * NUM_RECEIVERS_PER_NODE][NUM_EMITTERS];
		float distance[NUM_NODES * NUM_RECEIVERS_PER_NODE][NUM_EMITTERS];
		int closestNode;
		float closestDistance = 100;
		int closestEmitter;
		bool volUp, volDown;
		// wait til a button is pressed then continue
		while(!digitalRead(START_PIN) && 
			!(volUp = digitalRead(VOLUP_PIN)) &&
			!(volDown = digitalRead(VOLDOWN_PIN))){}
		// if the volume buttons were pressed do the appropriate operation
		// and start again from the beginning of the loop
		if(volUp){
		    if(vol <= 100){
				vol += 5;
				setVolume();
		    }
		    continue;
		}
		if(volDown){
		    if(vol >= 0){
				vol -= 5;
				setVolume();
		    }
		    continue;
		}
		// iterate through each node and each emitter
		// pinging and reading the return data
		for(data = 0; data < NUM_NODES; data++){
		    radio.openReadingPipe(1, readPipe[(int)data]);
		    for(int i = 0; i < NUM_EMITTERS; i++){
				// send radio signal
				ping(emitters[i], data);
				// wait to receive radio signal
				nodeData[(int)data * NUM_EMITTERS + i] = 
					receiveData(data);
		    }
		    radio.closeReadingPipe(1);
		}
		// debugging purposes
	//	uint16_t testData = 0x0A40;
	//	for(int i = 0; i < NUM_NODES; i++){
	//	    for(int j = 0; j < NUM_EMITTERS; j++){
	//		nodeData[i * NUM_EMITTERS + j] = testData;
	//		testData += 0x0101;
	//	    }
	//	    testData += 0x8080;
	//	}
	//	for(int i = 0; i < NUM_NODES * NUM_EMITTERS; i++){
	//	    bitset<8> x(nodeData[i]);
	//	    bitset<8> y(nodeData[i] >> 8);
	//	    cout << y << " | " << x << endl;
	//	}
	//	cout << endl;
		for(int i = 0; i < NUM_NODES; i++){
		    for(int j = 0; j < NUM_EMITTERS; j++){
				// take the read data and split it up into two halves
				// each half corresponds to one of the receivers
				char receiver[NUM_RECEIVERS_PER_NODE] = 
				    {((char)(nodeData[i * NUM_EMITTERS + j] >> 8)), 
				    ((char)(nodeData[i * NUM_EMITTERS + j] & 0xFF))};
				// take bits 7-6 of each byte as the node ID
				// take bits 5-1 of each byte as the integer distance
				// take bit 0 of each byte as the flag for a half increment
				for(int k = 0; k < NUM_RECEIVERS_PER_NODE; k++){
				    location[i * NUM_RECEIVERS_PER_NODE + k][j] = 
						(int)(receiver[k] >> 6);
				    distance[i * NUM_RECEIVERS_PER_NODE + k][j] = 
						(float)((receiver[k] >> 1) & 0x1F);
				    if((receiver[k] & 1)){
						distance[i * NUM_RECEIVERS_PER_NODE + k][j] += 0.5;
				    }
				}
		    }
		}
		// debugging purposes
	//	cout << endl;
	//	for(int i = 0; i < NUM_NODES * NUM_RECEIVERS_PER_NODE; i++){
	//	    for(int j = 0; j < NUM_EMITTERS; j++){
	//		cout << location[i][j] << " | " << distance[i][j] << endl;
	//	    }
	//	}
		// iterate through all the data and determine the shortest distance
		// record the distance, node ID, and emitter number of
		// the closest distance
		for(int i = 0; i < NUM_NODES * NUM_RECEIVERS_PER_NODE; i++){
		    for(int j = 0; j < NUM_EMITTERS; j++){
				if(distance[i][j] < closestDistance){
				    closestDistance = distance[i][j];
				    closestNode = location[i][j];
				    closestEmitter = j + 1;
				}
		    }
		}
		// debugging purposes
	//	cout << closestDistance << " " << closestNode << " " << closestEmitter;
		
		// emitter numbers 1-2 are on the right side
		if(closestEmitter < 2){
		    userFeedback(closestNode, closestDistance, RIGHT);
		}
		// emitter numbers 3-4 are on the left side
		else if(closestEmitter < 5){
		    userFeedback(closestNode, closestDistance, LEFT);
		}
		else{
		    // closest emitter is 5 so the user is below the node
		}
    }

    return (EXIT_SUCCESS);
}


