/* 
 * File:   main.c
 * Author: Justin
 *
 * Created on November 12, 2015, 4:47 PM
 * 
 * NOTE: Must run gcc with -lpthread -lwiringPi options
 */

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <pthread.h>

// corresponds to GPIO pin 11
#define OUTPUT_PIN 0
// 25 microseconds per cycle
#define NUM_CYCLES 10
// period in microseconds
#define PERIOD 25

int usOutput;

void *usGen(){
    int i;
    for(i = 0; i<NUM_CYCLES; i++){
    //while(1){
	digitalWrite(usOutput, LOW);
	delayMicroseconds(PERIOD/2);
	//delay(500);
	digitalWrite(usOutput, HIGH);
	delayMicroseconds(PERIOD/2);
	//delay(500);
    }
    return NULL;
}

int main(int argc, char** argv) {
    // initial setup of wiringPi
    wiringPiSetup();
    pinMode(OUTPUT_PIN, OUTPUT);
    
    pthread_t usGenThread;
    // select the pin to send the us signal on
    usOutput = OUTPUT_PIN;
    // run usGen() in a separate thread
    if(pthread_create(&usGenThread, NULL, usGen, NULL)){
	// ERROR
	return 1;
    }
    // do something in this thread
    int i = 0;
    while(1){
	printf("%d\n", i);
	i++;
    }
    // wait for the other thread to complete
    if(pthread_join(usGenThread, NULL)){
	// ERROR
	return 2;
    }
    
    return (EXIT_SUCCESS);
}


