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
#include <iostream>
#include <bitset>
#include "/home/pi/rf24libs/RF24/RF24.h"

#include "map.h"
#include "sounds.h"
#include "defs.h"

using namespace std;
// these are arbitrary assignments for now
#define START_PIN 1
#define VOLUP_PIN 8
#define VOLDOWN_PIN 9
#define EMITTER_1 0
#define EMITTER_2 2
#define EMITTER_3 3
#define EMITTER_4 4
#define EMITTER_5 5
// number of periods to emit ultrasonic
#define NUM_CYCLES 200
// period in microseconds (25 is 40kHz)
#define PERIOD 25
#define NUM_NODES 2
// delay between pings in milliseconds
#define SAMPLE_DELAY 40
// radio receive timeout in nanoseconds
#define RADIO_RECEIVE_TIMEOUT (0.5 * NANOSECONDS_PER_SECOND)
// radio send timeout in milliseconds
#define RADIO_SEND_TIMEOUT 500
#define LEFT 0
#define RIGHT 1



// GLOBAL VARIABLES
// default volume is 80%
int vol = 80;
RF24 radio(22,0);
const uint8_t writePipe[2][6] = {"1Pipe", "2Pipe"};
const uint8_t readPipe[2][6] = {"3Pipe", "4Pipe"};
const int emitters[5] = {EMITTER_1, EMITTER_2, EMITTER_3, EMITTER_4, EMITTER_5};
// inline function so that the compiler does inline expansion
// need to be as fast as possible, normal function calls take too long
inline void ping(int emitterNum){
    char data = CODE;
    for(int i = 0; i < NUM_SAMPLES; i++){
        // send a blocking write
        // program does not continue until it is received
        // this allows the sender and receiver to sync up
        radio.writeBlocking(&data, sizeof(data), RADIO_SEND_TIMEOUT);
        radio.txStandBy(RADIO_SEND_TIMEOUT);
        // send a square wave over the ultrasonic emitter
        for(int j = 0; j < NUM_CYCLES; j++){
            digitalWrite(emitterNum, LOW);
            delayMicroseconds(PERIOD/2);
            digitalWrite(emitterNum, HIGH);
            delayMicroseconds(PERIOD/2);
        }
        // delay between signals
        // tweak depending on performance
        delay(SAMPLE_DELAY);
    }
}
inline char receiveData(char nodeNum){
    struct timespec startTime;
    char data;
    // listen on the specified reading pipe
    radio.openReadingPipe(1,readPipe[nodeNum]);
    radio.startListening();
    clock_gettime(CLOCK_REALTIME, &startTime);
    while(!radio.available()){
        // after the timeout period give up and set the
        // packet to the max distance
//        if(timeDifference(startTime) >= RADIO_RECEIVE_TIMEOUT){
//            radio.stopListening();
//            data = 0xFF;
//            return data;
//        }
    }
    // if we've made it here there is data to be read
    radio.read(&data, sizeof(data));
    radio.stopListening();
    radio.closeReadingPipe(1);
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
    pinMode(EMITTER_1, OUTPUT);
//    pinMode(EMITTER_2, OUTPUT);
//    pinMode(EMITTER_3, OUTPUT);
//    pinMode(EMITTER_4, OUTPUT);
//    pinMode(EMITTER_5, OUTPUT);
//    pinMode(START_PIN, INPUT);
//    pinMode(VOLUP_PIN, INPUT);
//    pinMode(VOLDOWN_PIN, INPUT);
    // set initial volume level
//    setVolume();
    // setup radio    
    radio.begin();
//    while(1){
        char nodeData[NUM_NODES][NUM_EMITTERS][NUM_RECEIVERS_PER_NODE];
        int location[NUM_NODES][NUM_EMITTERS][NUM_RECEIVERS_PER_NODE];
        float distance[NUM_NODES][NUM_EMITTERS][NUM_RECEIVERS_PER_NODE];
        int closestNode;
        float closestDistance = 100;
        int closestEmitter;
        bool volUp, volDown;
        // wait til a button is pressed then continue
//        while(!digitalRead(START_PIN) && 
//             !(volUp = digitalRead(VOLUP_PIN)) &&
//             !(volDown = digitalRead(VOLDOWN_PIN))){}
//        // if the volume buttons were pressed do the appropriate operation
//        // and start again from the beginning of the loop
//        if(volUp){
//            if(vol <= 100){
//                vol += 5;
//                setVolume();
//            }
//             continue;
//        }
//        if(volDown){
//            if(vol >= 0){
//                vol -= 5;
//                setVolume();
//            }
//            continue;
//        }
        // iterate through each node and each emitter
        // pinging each receiver NUM_SAMPLES times each
        for(int i = 0; i < NUM_NODES; i++){
            radio.openWritingPipe(writePipe[i]);
            for(int j = 0; j < NUM_EMITTERS; j++){
                for(int k = 0; k < NUM_RECEIVERS_PER_NODE; k++){
                    // send ping signal
                    ping(emitters[j]);
                    // wait to receive radio signal
                    nodeData[i][j][k] = receiveData(i);
                    //cout << bitset<8>(nodeData[i][j][k]) << endl;
                }
            }
        }
        // take the data received and interpret it
        for(int i = 0; i < NUM_NODES; i++){
            for(int j = 0; j < NUM_EMITTERS; j++){
                // take bits 7-5 of each byte as the node ID
                // take bits 4-1 of each byte as the integer distance
                // take bit 0 of each byte as the flag for a half increment
                for(int k = 0; k < NUM_RECEIVERS_PER_NODE; k++){
                    location[i][j][k] = 
                        (int)(nodeData[i][j][k] >> 5);
                    distance[i][j][k] = 
                        (float)((nodeData[i][j][k] >> 1) & 0xF);
                    if((nodeData[i][j][k] & 1)){
                        distance[i][j][k] += 0.5;
                    }
                }
            }
        }
        // iterate through all the data and determine the shortest distance
        // record the distance, node ID, and emitter number of
        // the closest distance
        for(int i = 0; i < NUM_NODES; i++){
            for(int j = 0; j < NUM_EMITTERS; j++){
                for(int k = 0; k < NUM_RECEIVERS_PER_NODE; k++){
                    cout << distance[i][j][k] << endl;
                    if(distance[i][j][k] < closestDistance){
                        closestDistance = distance[i][j][k];
                        closestNode = location[i][j][k];
                        closestEmitter = j + 1;
                    }
                }
            }
        }
        // remove when testing audio feedback
        return 0;
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
            // what do we do here?
        }
//    }
    return (EXIT_SUCCESS);
}