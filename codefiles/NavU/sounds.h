/* 
 * File:   sounds.h
 * Author: Justin
 *
 * This file contains the locations of all the sound files for the system,
 * other than the location sound files which are dependent on the location.
 * This file will not be modified once the LGS is deployed.
 */

#ifndef SOUNDS_H
#define	SOUNDS_H

// sounds for distance measurement
#define NUM_SOUNDS 6
// "zero"
#define SOUND_0 "/root/volume.wav"
// "one"
#define SOUND_1 "/root/volume.wav"
// "two"
#define SOUND_2 "/root/volume.wav"
// "three"
#define SOUND_3 "/root/volume.wav"
// "four"
#define SOUND_4 "/root/volume.wav"
// "five
#define SOUND_5 "/root/volume.wav"
// "point 5
#define SOUND_05 "/root/volume.wav"
// "is"
#define IS_SOUND "/root/volume.wav"
// "meters to your"
#define TOYOUR_SOUND "/root/volume.wav"
// "right"
#define RIGHT_SOUND "/root/volume.wav"
// "left"
#define LEFT_SOUND "/root/volume.wav"
// volume up and down indicator
#define VOLUME_SOUND "/root/volume.wav"
    
    // each beacon will be given an ID which corresponds to its location
    // in the map string array
    const char sounds[NUM_SOUNDS][] = {SOUND_0, SOUND_1, SOUND_2, SOUND_3, 
	SOUND_4, SOUND_5};
    const char lr[2][] = {LEFT_SOUND, RIGHT_SOUND};

#endif	/* SOUNDS_H */

