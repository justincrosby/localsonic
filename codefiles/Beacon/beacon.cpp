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
#include "defs.h"

using namespace std;

#define RX_PIN_1 0
#define RX_PIN_2 7
#define NODE_ID 0
#define RECEIVER_1_ID 0
#define RECEIVER_2_ID 1

// eliminate the large outliers
#define TOLERANCE_1 0.3
// eliminate the smaller outliers
#define TOLERANCE_2 0.15
#define SPEED_OF_SOUND 340

// GLOBAL VARIABLES
RF24 radio(22,0);
const uint8_t readPipe[6] = "1Pipe";
const uint8_t writePipe[2][6] = {"2Pipe", "3Pipe"};
const int receiverIDs[NUM_RECEIVERS_PER_NODE] = {RECEIVER_1_ID, RECEIVER_2_ID};

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
		float distance[NUM_RECEIVERS_PER_NODE];
		// set this data type for the number of receivers * 4
		uint16_t sendData = 0;
		int intpart;
		float decpart;

		radio.startListening();
		for(int i = 0; i < NUM_SAMPLES; i++){
			// wait for a radio transmission
			// if the radio signal was not meant for this node we need to stop listening
			while(!radio.available()){}
			radio.read(&data, sizeof(data));
			if(data != NODE_ID){
				break;
			}
			clock_gettime(CLOCK_REALTIME, &currTime);
			startTime = currTime.tv_nsec;
			// wait for one of the US receivers to detect a signal
			while(!(receiver1 = digitalRead(RX_PIN_1)) && !digitalRead(RX_PIN_2)){
				clock_gettime(CLOCK_REALTIME, &currTime);
				endTime = currTime.tv_nsec;
				// if we hit the timeout period set both of the times for this transmission to 0
				// that way we know that the receiver did not receive anything
				if(endTime - startTime >= RECEIVE_TIMEOUT){
					times[0][i] = 0;
					times[1][i] = 0;
					break;
				}
			}
			// if we've broken out of the previous while loop, 
			// one of the receivers must have detected something
			if(receiver1){
				clock_gettime(CLOCK_REALTIME, &currTime);
				endTime = currTime.tv_nsec;
				times[0][i] = endTime - startTime;
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
				times[1][i] = endTime - startTime;
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
		// make sure the signal was meant for this node
		// if not wait for the other node to finish receiving before listening again
		if(data != NODE_ID){
			delay(1000);
			continue;
		}
		radio.stopListening();
		// iterate through each receiver getting the average time of flight
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
			if(numSamples == 0){
				continue;
			}
			avgTol = average/numSamples;
			average = 0;
			numSamples = 0;
			for(int j = 0; j < NUM_SAMPLES; j++){
				// remove the far off outlier values first
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
				// now that we have a more accurate average remove closer outliers
				if(times[i][j] != 0 && times[i][j] > avgTol - avgTol * TOLERANCE_2 &&
					times[i][j] < avgTol + avgTol * TOLERANCE_2){
					average += times[i][j];
					numSamples++;
				}
			}
			distance[i] = average / numSamples / 
				NANOSECONDS_PER_SECOND * SPEED_OF_SOUND;
		}
		// format a packet of data to send back to the NavU
		// each receiver needs 8 bits of data transmitted
		for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
			sendData |= receiverIDs[i];
			// if the distance is 0 we didn't get a signal
			// so set the distance to the farthest distance possible
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