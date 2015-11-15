/* 
 * File:   map.h
 * Author: Justin
 *
 * This file contains the mapping of each beacon ID to a locaion and audio file.
 * It will change depending on the location the LGS is installed (as part of
 * the install process this will have to be mapped out)
 */

#ifndef MAP_H
#define	MAP_H

#ifdef	__cplusplus
extern "C" {
#endif

#define NUM_LOCATIONS 3
#define LOCATION_0 "/root/test.wav"
#define LOCATION_1 "/root/volume.wav"
#define LOCATION_2 "/root/volume.wav"
    
    // each beacon will be given an ID which corresponds to its location
    // in the map string array
    const char map[NUM_LOCATIONS][50] = {LOCATION_0, LOCATION_1, LOCATION_2};

#ifdef	__cplusplus
}
#endif

#endif	/* MAP_H */
