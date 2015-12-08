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

#include "/home/pi/rf24libs/RF24/RF24.h"
#include "defs.h"

using namespace std;

#define RX_PIN_1 0
#define RX_PIN_2 1
#define RX_PIN_3 2
#define RECEIVER_1_ID 0
#define RECEIVER_2_ID 1
#define RECEIVER_3_ID 2
#define NODE_ID 0

// eliminate the large outliers
#define TOLERANCE_1 0.3
// eliminate the smaller outliers
#define TOLERANCE_2 0.15
#define SPEED_OF_SOUND 340
// we have 6 bits for distance measurement 0x3F = 31.5
#define MAX_DISTANCE 15.5
// ~20m
#define MAX_TIME 59000000
// ~0.25m
#define MIN_TIME 735000
// radio timeout in nanoseconds
#define RADIO_TIMEOUT (0.5 * NANOSECONDS_PER_SECOND)
// ultrasonic receiver timeout in nanoseconds
#define RECEIVE_TIMEOUT (0.5 * NANOSECONDS_PER_SECOND)

// GLOBAL VARIABLES
RF24 radio(22,0);
const uint8_t readPipe[2][6] = {"1Pipe", "2Pipe"};
const uint8_t writePipe[2][6] = {"3Pipe", "4Pipe"};
const int receiverIDs[NUM_RECEIVERS_PER_NODE] = {RECEIVER_1_ID, RECEIVER_2_ID, RECEIVER_3_ID};
const int receiverPins[NUM_RECEIVERS_PER_NODE] = {RX_PIN_1, RX_PIN_2, RX_PIN_3};

int main(int argc, char** argv) {
    // initial wiringPi setup, run once
    wiringPiSetup();
    pinMode(RX_PIN_1, INPUT);
    pinMode(RX_PIN_2, INPUT);
    pinMode(RX_PIN_3, INPUT);

    radio.begin();
    // beacons use static reading and writing pipes, open once
    radio.openReadingPipe(1,readPipe[NODE_ID]);
    radio.openWritingPipe(writePipe[NODE_ID]);
    while(1){
        char data;
        struct timespec startTime;
        long int times[NUM_RECEIVERS_PER_NODE][NUM_SAMPLES];
        float distance[NUM_RECEIVERS_PER_NODE];
        char sendData[NUM_RECEIVERS_PER_NODE];
        int intpart;
        float decpart;
        // need to initialize sendData since it'll be bitwise or'd
        for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
            sendData[i] = 0;
        }
        radio.startListening();
        // iterate through each receiver with NUM_SAMPLES per receiver
        for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
            for(int j = 0; j < NUM_SAMPLES; j++){
                // wait for a radio transmission
                clock_gettime(CLOCK_REALTIME, &startTime);
                while(!radio.available()){
                    // if we've hit the receive timeout maybe we missed a transmission
                    // continue to the end of the loop
                    // we only continue if we have received a signal before
                    if(timeDifference(startTime) >= RADIO_TIMEOUT && j != 0){
                        times[i][j] = 0;
                        goto end;
                    }
                }
                // if we've made it this far there is some data to be read
                radio.read(&data, sizeof(data));
                // to make sure we don't have bad data double check against
                // the code being sent
                if(data != CODE){
                    cout << "ERROR: Bad data" << endl;
                    continue;
                }
                clock_gettime(CLOCK_REALTIME, &startTime);
                // wait for the US receiver to detect a signal
                while(!digitalRead(receiverPins[i])){
                    // if we hit the timeout period set both of the times for this transmission to 0
                    // that way we know that the receiver did not receive anything
                    if(timeDifference(startTime) >= RECEIVE_TIMEOUT){
                        times[i][j] = 0;
                        goto end;
                    }
                }
                // if we've broken out of the previous while loop, 
                // we must have detected something
                times[i][j] = timeDifference(startTime);
                // jump out of the loop to the end
                end:
                continue;
            }
        }
        radio.stopListening();
        // iterate through each receiver getting the average time of flight
        for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
            float avgTol = 0;
            float average = 0;
            int numSamples = 0;
            for(int j = 0; j < NUM_SAMPLES; j++){
                cout << "times " << times[i][j] << endl;
                // add samples to the average as long as they aren't too close
                // or too far away to be possible
                if(times[i][j] > MIN_TIME && times[i][j] < MAX_TIME){
                    average += times[i][j];
                    numSamples++;
                }
            }
            // if we don't get any samples within the possible range
            // the emitter must be far away
            if(numSamples == 0){
                distance[i] = MAX_DISTANCE;
                continue;
            }
            avgTol = average/numSamples;
            average = 0;
            numSamples = 0;
            for(int j = 0; j < NUM_SAMPLES; j++){
                // remove the far off outlier values first
                if(times[i][j] > MIN_TIME && times[i][j] < MAX_TIME && 
                     times[i][j] > avgTol - avgTol * TOLERANCE_1 &&
                     times[i][j] < avgTol + avgTol * TOLERANCE_1){
                    average += times[i][j];
                    numSamples++;
                }
            }
            if(numSamples == 0){
                distance[i] = MAX_DISTANCE;
                continue;
            }
            avgTol = average/numSamples;
            average = 0;
            numSamples = 0;
            for(int j = 0; j < NUM_SAMPLES; j++){
                // now that we have a more accurate average remove closer outliers
                if(times[i][j] > MIN_TIME && times[i][j] < MAX_TIME && 
                     times[i][j] > avgTol - avgTol * TOLERANCE_2 &&
                     times[i][j] < avgTol + avgTol * TOLERANCE_2){
                    average += times[i][j];
                    numSamples++;
                }
            }
            if(numSamples == 0){
                distance[i] = MAX_DISTANCE;
                continue;
            }
            distance[i] = average / numSamples / 
                NANOSECONDS_PER_SECOND * SPEED_OF_SOUND;

            cout << "distance :" << distance[i] << endl;
        }
        // format a packet of data to send back to the NavU
        // each receiver needs 8 bits of data transmitted
        for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
            // first 2 bits are the receiver ID
            sendData[i] |= receiverIDs[i];
            intpart = (int)distance[i];
            decpart = distance[i] - intpart;
            if(decpart > 0.75){
                intpart++;
            }
            // next 4 bits are the integer part of the distance
            sendData[i] <<= 4;
            sendData[i] |= intpart;
            // last bit is a flag for a half increment
            sendData[i] <<= 1;
            if(decpart < 0.75 && decpart > 0.25){
                sendData[i] |= 1;
            }
        }
        for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
            radio.writeBlocking(&sendData[i], sizeof(sendData[i]), 1000);
            radio.txStandBy(1000);
        }
    }
    return 0;
}