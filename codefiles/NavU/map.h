/* 
 * File:   map.h
 * Author: Justin
 *
 * This file contains the mapping of each beacon ID to a locaion and audio file.
 * It will change depending on the location the LGS is installed (as part of
 * the install process this will have to be mapped out)
 */

#ifndef MAP_H
#define MAP_H

#define NUM_LOCATIONS 6
#define LOCATION_0 "/root/soundfiles/room1.wav"
#define LOCATION_1 "/root/soundfiles/room2.wav"
#define LOCATION_2 "/root/soundfiles/room3.wav"
#define LOCATION_3 "/root/soundfiles/room4.wav"
#define LOCATION_4 "/root/soundfiles/room5.wav"
#define LOCATION_5 "/root/soundfiles/room6.wav"
    
// each beacon will be given an ID which corresponds to its location
// in the map string array
const char map[NUM_LOCATIONS][50] = {LOCATION_0, LOCATION_1, LOCATION_2, LOCATION_3, LOCATION_4, LOCATION_5};

#endif  /* MAP_H */