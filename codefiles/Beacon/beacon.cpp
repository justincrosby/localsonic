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
#include <math.h>

#include "/home/pi/rf24libs/RF24/RF24.h"
#include "defs.h"

using namespace std;

#define RX_PIN_1 0
#define RX_PIN_2 1
#define RX_PIN_3 2
#define RX_PIN_4 4
#define RX_PIN_5 21
#define RECEIVER_1_ID 0
#define RECEIVER_2_ID 1
#define RECEIVER_3_ID 2
#define RECEIVER_4_ID 3
#define RECEIVER_5_ID 4
#define NODE_ID 0

// eliminate the large outliers
#define TOLERANCE_1 0.25
// eliminate the smaller outliers
#define TOLERANCE_2 0.1
#define SPEED_OF_SOUND 340
// ~20m
#define MAX_TIME 59000000
// ~0.25m
#define MIN_TIME 735000
// radio receive timeout in nanoseconds
#define RADIO_RECEIVE_TIMEOUT (0.5 * NANOSECONDS_PER_SECOND)
// radio send timeout in milliseconds
#define RADIO_SEND_TIMEOUT 2000
// ultrasonic receiver timeout in nanoseconds
#define ULTRASONIC_RECEIVE_TIMEOUT (MAX_DISTANCE / SPEED_OF_SOUND * NANOSECONDS_PER_SECOND)
#define USER_HEIGHT 1

// GLOBAL VARIABLES
RF24 radio(22,0);
const uint8_t readPipe[2][6] = {"1Pipe", "2Pipe"};
const uint8_t writePipe[2][6] = {"3Pipe", "4Pipe"};
const int receiverIDs[5] = {RECEIVER_1_ID, RECEIVER_2_ID, RECEIVER_3_ID, RECEIVER_4_ID, RECEIVER_5_ID};
const int receiverPins[5] = {RX_PIN_1, RX_PIN_2, RX_PIN_3, RX_PIN_4, RX_PIN_5};

int main(int argc, char** argv) {
    // initial wiringPi setup, run once
    wiringPiSetup();
    pinMode(RX_PIN_1, INPUT);
    pinMode(RX_PIN_2, INPUT);
    pinMode(RX_PIN_3, INPUT);
    pinMode(RX_PIN_4, INPUT);
    pinMode(RX_PIN_5, INPUT);

    radio.begin();
    // beacons use static reading and writing pipes, open once
    radio.openReadingPipe(1,readPipe[NODE_ID]);
    radio.openWritingPipe(writePipe[NODE_ID]);
    while(1){
        char data;
        struct timespec startTime;
        long int times[NUM_RECEIVERS_PER_NODE][NUM_EMITTERS][NUM_SAMPLES];
        float distance;
        // set initial values for these in case no emitters are close
        float closestDistance = MAX_DISTANCE;
        int closestEmitter = 0;
        int closestReceiver = 0;
        int intpart;
        float decpart;
        
        radio.startListening();
        // need to initialize sendData since it'll be or'd
        uint16_t sendData = 0;
        for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
            for(int j = 0; j < NUM_EMITTERS; j++){
                for(int k = 0; k < NUM_SAMPLES; k++){
                    // wait for a radio transmission
                    clock_gettime(CLOCK_REALTIME, &startTime);
                    while(!radio.available()){
                        // if we've hit the receive timeout maybe we missed a transmission
                        // continue to the end of the loop
                        // we only continue if we have received a signal before
                        if(timeDifference(startTime) >= RADIO_RECEIVE_TIMEOUT && k != 0){
                            times[i][j][k] = 0;
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
                        // if we hit the timeout period set the time for this transmission to 0
                        // that way we know that the receiver did not receive anything
                        if(timeDifference(startTime) >= ULTRASONIC_RECEIVE_TIMEOUT){
                            times[i][j][k] = 0;
                            goto end;
                        }
                    }
                    // if we've broken out of the previous while loop, 
                    // we must have detected something
                    times[i][j][k] = timeDifference(startTime);
                    // jump out of the loop to the end
                    end:
                    continue;
                }
            }
        }
        radio.stopListening();
        // calculate the closest receiver-emitter distance
        for(int i = 0; i < NUM_RECEIVERS_PER_NODE; i++){
            for(int j = 0; j < NUM_EMITTERS; j++){
                float avgTol = 0;
                float average = 0;
                int numSamples = 0;
                for(int k = 0; k < NUM_SAMPLES; k++){
                    //cout << "times " << times[i][j][k] << endl;
                    // add samples to the average as long as they aren't too close
                    // or too far away to be possible
                    if(times[i][j][k] > MIN_TIME && times[i][j][k] < MAX_TIME){
                        average += times[i][j][k];
                        numSamples++;
                    }
                }
                // if we don't get any samples within the possible range
                // the emitter must be far away
                if(numSamples == 0){
                    //cout << "receiver " << i << " emitter " << j << " distance : 15.5" << endl;
                    continue;
                }
                avgTol = average/numSamples;
                average = 0;
                numSamples = 0;
                for(int k = 0; k < NUM_SAMPLES; k++){
                    // remove the far off outlier values first
                    if(times[i][j][k] > MIN_TIME && times[i][j][k] < MAX_TIME && 
                         times[i][j][k] > avgTol - avgTol * TOLERANCE_1 &&
                         times[i][j][k] < avgTol + avgTol * TOLERANCE_1){
                        average += times[i][j][k];
                        numSamples++;
                    }
                }
                if(numSamples == 0){
                    //cout << "receiver " << i << " emitter " << j << " distance : 15.5" << endl;
                    continue;
                }
                avgTol = average/numSamples;
                average = 0;
                numSamples = 0;
                for(int k = 0; k < NUM_SAMPLES; k++){
                    // now that we have a more accurate average remove closer outliers
                    if(times[i][j][k] > MIN_TIME && times[i][j][k] < MAX_TIME && 
                         times[i][j][k] > avgTol - avgTol * TOLERANCE_2 &&
                         times[i][j][k] < avgTol + avgTol * TOLERANCE_2){
                        average += times[i][j][k];
                        numSamples++;
                    }
                }
                if(numSamples == 0){
                    //cout << "receiver " << i << " emitter " << j << " distance : 15.5" << endl;
                    continue;
                }
                distance = average / numSamples / 
                    NANOSECONDS_PER_SECOND * SPEED_OF_SOUND;
                
                if(distance < closestDistance){
                    closestDistance = distance;
                    closestEmitter = j;
                    closestReceiver = i;
                    //cout << closestDistance << " " << closestEmitter << " " << closestReceiver << endl;
                }
                //cout << "receiver " << i << " emitter " << j << " distance :" << distance << endl;
            }
        }
        if(closestDistance == MAX_DISTANCE){
            sendData = 0xFFFF;
            radio.writeBlocking(&sendData, sizeof(sendData), RADIO_SEND_TIMEOUT);
            radio.txStandBy(RADIO_SEND_TIMEOUT);
            continue;
        }
        // calculate the horizontal distance
        closestDistance = sqrt(pow(closestDistance, 2) - pow(USER_HEIGHT, 2));
        // format a packet of data to send back to the NavU
        // send 16 bits of data
        // first 3 bits are the receiver ID
        sendData |= receiverIDs[closestReceiver];
        // next 3 bits are the emitter number
        sendData <<= 3;
        sendData |= closestEmitter;
        // next 4 bits are the integer part of the distance
        intpart = (int)closestDistance;
        decpart = closestDistance - intpart;
        if(decpart > 0.75){
            intpart++;
        }
        sendData <<= 4;
        sendData |= intpart;
        // last bit is a flag for a half increment
        sendData <<= 1;
        if(decpart < 0.75 && decpart > 0.25){
            sendData |= 1;
        }
        radio.writeBlocking(&sendData, sizeof(sendData), RADIO_SEND_TIMEOUT);
        radio.txStandBy(RADIO_SEND_TIMEOUT);
    }
    return 0;
}