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

#define START_PIN 7
#define VOLUP_PIN 21
#define VOLDOWN_PIN 22
// emitter 1 should be the front, 2 the right, 3 the back, etc
#define EMITTER_1 0
#define EMITTER_2 4
#define EMITTER_3 1
#define EMITTER_4 6
#define EMITTER_5 5
// number of periods to emit ultrasonic
#define NUM_CYCLES 150
// period in microseconds (25 is 40kHz)
#define PERIOD 25
// total beacons in the system
#define NUM_NODES 1
// delay between pings in milliseconds
#define SAMPLE_DELAY 40
// radio receive timeout in nanoseconds
#define RADIO_RECEIVE_TIMEOUT (5 * NANOSECONDS_PER_SECOND)
// radio send timeout in milliseconds
#define RADIO_SEND_TIMEOUT 500

// GLOBAL VARIABLES
// default volume is 80%
int vol = 80;
RF24 radio(22,0);
const uint8_t writePipe[2][6] = {"1Pipe", "2Pipe"};
const uint8_t readPipe[2][6] = {"3Pipe", "4Pipe"};
const int emitterPins[5] = {EMITTER_1, EMITTER_2, EMITTER_3, EMITTER_4, EMITTER_5};
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
inline uint16_t receiveData(int nodeNum){
    struct timespec startTime;
    uint16_t data;
    // listen on the specified reading pipe
    radio.openReadingPipe(1,readPipe[nodeNum]);
    radio.startListening();
    clock_gettime(CLOCK_REALTIME, &startTime);
    while(!radio.available()){
        // after the timeout period give up and set the
        // packet to the max distance
        if(timeDifference(startTime) >= RADIO_RECEIVE_TIMEOUT){
            radio.stopListening();
            data = 0xFFFF;
            return data;
        }
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
void userFeedback(int location, float distance, int orientation){
    // play a sequence of files that give the user their location
    // i.e. "<location> is <distance> to your <direction>
    // location is given as a unique integer value which maps to a
    // filename in sounds[]
    // distance is given in an integer number +/- 0.5m
    // direction is given as 0 or 1, 0 corresponding to left, 1 is right
    
    // do some calculations first - need to play the sounds quickly
    int intpart = (int)distance;
    float decpart = distance - intpart;
    if(orientation == 4){
        playAudio(AT_SOUND);
        // location name first
        if(location > 5){
            playAudio(map[0]);
        }
        else{
            playAudio(map[location]);
        }
    }
    else{
        // location name first
        if(location > 5){
            playAudio(map[0]);
        }
        else{
            playAudio(map[location]);
        }
        // connecting words
        playAudio(IS_SOUND);
        // play distance
        if(distance > 5){
            playAudio(intSounds[5]);
        }
        else{
            if(decpart != 0){
                playAudio(decSounds[intpart]);
            }
            else{
                playAudio(intSounds[intpart]);
            }
        }
        // direction: ahead, right, behind, left
        if(orientation > 3){
            playAudio(direction[0]);
        }
        else{
            playAudio(direction[orientation]);
        }
    }
}
int main(int argc, char** argv) {
    // initial wiringPi setup, run once
    wiringPiSetup();
    pinMode(EMITTER_1, OUTPUT);
    pinMode(EMITTER_2, OUTPUT);
    pinMode(EMITTER_3, OUTPUT);
    pinMode(EMITTER_4, OUTPUT);
    pinMode(EMITTER_5, OUTPUT);
    pinMode(START_PIN, INPUT);
    pinMode(VOLUP_PIN, INPUT);
    pinMode(VOLDOWN_PIN, INPUT);
    // set initial volume level
//    while(1){
//        digitalWrite(EMITTER_4, LOW);
//        delayMicroseconds(PERIOD/2);
//        digitalWrite(EMITTER_4, HIGH);
//        delayMicroseconds(PERIOD/2);
//    }

    setVolume();
    setVolume();
    playAudio(WELCOME_SOUND);
    
    radio.begin();
    while(1){
        uint16_t nodeData[NUM_NODES];
        int location[NUM_NODES];
        float distance[NUM_NODES];
        int emitter[NUM_NODES];
        int closestNode;
        float closestDistance = MAX_DISTANCE;
        int closestEmitter;
        int error = 0;
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
            }
            setVolume();
            continue;
        }
        if(volDown){
            if(vol >= 0){
               vol -= 5;
            }
            setVolume();
            continue;
        }
        playAudio(VOLUME_SOUND);
        // iterate through each node and each emitter
        // pinging each receiver NUM_SAMPLES times each
        for(int i = 0; i < NUM_NODES; i++){
            radio.openWritingPipe(writePipe[i]);
            for(int j = 0; j < NUM_RECEIVERS_PER_NODE; j++){
                for(int k = 0; k < NUM_EMITTERS; k++){
                    // send ping signal
                    ping(emitterPins[k]);
                }
            }
            // wait to receive radio signal
            nodeData[i] = receiveData(i);
        }
        // check for errors
        for(int i = 0; i < NUM_NODES; i++){
            if(nodeData[i] == 0xFFFF){
                error++;
            }
        }
        if(error == NUM_NODES){
            playAudio(ERROR_SOUND);
            continue;
        }
        // take the data received and interpret it
        for(int i = 0; i < NUM_NODES; i++){
            // take bits 10-8 of each byte as the node ID
            // take bits 7-5 as the emitter number
            // take bits 4-1 of each byte as the integer distance
            // take bit 0 of each byte as the flag for a half increment
            location[i] = (int)(nodeData[i] >> 8);
            emitter[i] = (int)((nodeData[i] >> 5) & 0x7);
            distance[i] = (float)((nodeData[i] >> 1) & 0xF);
            if((nodeData[i] & 1)){
                distance[i] += 0.5;
            }
        }
        // iterate through all the data and determine the shortest distance
        // record the distance, node ID, and emitter number of
        // the closest distance
        for(int i = 0; i < NUM_NODES; i++){
            if(distance[i] < closestDistance){
                closestDistance = distance[i];
                closestEmitter = emitter[i];
                closestNode = location[i];
            }
        }
        // output the closest node to the user via audio feedback
        userFeedback(closestNode, closestDistance, closestEmitter);
    }
    return (EXIT_SUCCESS);
}