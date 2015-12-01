/* 
 * File:   beacon.cpp
 * Author: Justin
 *
 * Created on December 1, 2015, 11:00 AM
 */

#include <cstdlib>
#include <wiringPi.h>
#include <time.h>
#include <iostream>
#include <bitset>
#include "/home/pi/rf24libs/RF24/RF24.h"

using namespace std;

#define NUM_SAMPLES 10
#define NUM_EMITTERS 5
#define NUM_RECEIVERS_PER_NODE 2
#define RX_PIN_1 0
#define RX_PIN_2 7
#define NODE_ID 0
#define RECEIVE_TIMEOUT (0.5 * NANOSECONDS_PER_SECOND)
#define NANOSECONDS_PER_SECOND 1E9
// eliminate the large outliers
#define TOLERANCE_1 0.3
// eliminate the smaller outliers
#define TOLERANCE_2 0.15
#define SPEED_OF_SOUND 340

// GLOBAL VARIABLES
RF24 radio(22,0);
const uint8_t readPipe[6] = "1Pipe";
const uint8_t writePipe[2][6] = {"2Pipe", "3Pipe"};

int main(int argc, char** argv) {
	// initial wiringPi setup, run once
	wiringPiSetup();
	pinMode(RX_PIN_1, INPUT);
	pinMode(RX_PIN_2, INPUT);

	radio.begin();
	radio.printDetails();
	radio.openReadingPipe(1,readPipe);
	radio.openWritingPipe(writePipe[NODE_ID]);
	while(1){
		char data;
		struct timespec currTime;
		long int startTime, endTime;
		long int times[NUM_RECEIVERS_PER_NODE][NUM_SAMPLES];
		bool receiver1;
		float distance[2];
		uint16_t sendData = 0;
		int intpart;
		float decpart;

		radio.startListening();
		for(int i = 0; i < NUM_SAMPLES; i++){
			while(!radio.available()){}
			radio.read(&data, sizeof(data));
			if(data != NODE_ID){
				break;
			}
			clock_gettime(CLOCK_REALTIME, &currTime);
			startTime = currTime.tv_nsec;
			while(!(receiver1 = digitalRead(RX_PIN_1)) && !digitalRead(RX_PIN_2)){
				clock_gettime(CLOCK_REALTIME, &currTime);
				endTime = currTime.tv_nsec;
				if(endTime - startTime >= RECEIVE_TIMEOUT){
					times[0][i] = 0;
					times[1][i] = 0;
					break;
				}
			}
			if(receiver1){
				clock_gettime(CLOCK_REALTIME, &currTime);
				endTime = currTime.tv_nsec;
				times[i][0] = endTime - startTime;
				while(!digitalRead(RX_PIN_2)){
					clock_gettime(CLOCK_REALTIME, &currTime);
					endTime = currTime.tv_nsec;
					if(endTime - startTime >= RECEIVE_TIMEOUT){
						times[1][i] = 0;
						break;
					}
				}
			}
			else{
				clock_gettime(CLOCK_REALTIME, &currTime);
				endTime = currTime.tv_nsec;
				times[i][1] = endTime - startTime;
				while(!digitalRead(RX_PIN_1)){
					clock_gettime(CLOCK_REALTIME, &currTime);
					endTime = currTime.tv_nsec;
					if(endTime - startTime >= RECEIVE_TIMEOUT){
						times[0][i] = 0;
						break;
					}
				}
			}
		}
		if(data != NODE_ID){
			delay(1000);
			continue;
		}
		radio.stopListening();
		for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
			float avgTol = 0;
			float average = 0;
			int numSamples = 0;
			for(int j = 0; j < NUM_SAMPLES; j++){
				if(times[i][j] != 0){
					average += times[i][j];
					numSamples++;
				}
			}
			avgTol = average/numSamples;
			average = 0;
			numSamples = 0;
			for(int j = 0; j < NUM_SAMPLES; j++){
				if(times[i][j] != 0 && times[i][j] > avgTol - avgTol * TOLERANCE_1 &&
					times[i][j] < avgTol + avgTol * TOLERANCE_1){
					average += times[i][j];
					numSamples++;
				}
			}
			avgTol = average/numSamples;
			average = 0;
			numSamples = 0;
			for(int j = 0; j < NUM_SAMPLES; j++){
				if(times[i][j] != 0 && times[i][j] > avgTol - avgTol * TOLERANCE_2 &&
					times[i][j] < avgTol + avgTol * TOLERANCE_2){
					average += times[i][j];
					numSamples++;
				}
			}
			distance[i] = average / numSamples / 
				NANOSECONDS_PER_SECOND * SPEED_OF_SOUND;
		}
		for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
			sendData |= NODE_ID + i;
			if(distance[i] == 0){
				sendData <<= 6;
				sendData |= 0x3F;
			}
			else{
				intpart = (int)distance[i];
				decpart = distance[i] - intpart;
				sendData <<= 5;
				sendData |= intpart;
				sendData <<= 1;
				if(decpart < 0.75 && decpart > 0.25){
					sendData |= 1;
				}
			}
			sendData <<= 2;
		}
		radio.writeBlocking(&sendData, sizeof(sendData), 1000);
		radio.txStandBy(1000);
	}
	return 0;
}