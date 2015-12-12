/* 
 * File:   sounds.h
 * Author: Justin
 *
 * This file contains the locations of all the sound files for the system,
 * other than the location sound files which are dependent on the location.
 * This file will not be modified once the LGS is deployed.
 */

#ifndef SOUNDS_H
#define SOUNDS_H

// sounds for distance measurement
#define SOUND_00 "/root/soundfiles/05.wav"
#define SOUND_05 "/root/soundfiles/05.wav"
#define SOUND_10 "/root/soundfiles/10.wav"
#define SOUND_15 "/root/soundfiles/15.wav"
#define SOUND_20 "/root/soundfiles/20.wav"
#define SOUND_25 "/root/soundfiles/25.wav"
#define SOUND_30 "/root/soundfiles/30.wav"
#define SOUND_35 "/root/soundfiles/35.wav"
#define SOUND_40 "/root/soundfiles/40.wav"
#define SOUND_45 "/root/soundfiles/45.wav"
#define SOUND_50 "/root/soundfiles/50.wav"
// "is"
#define IS_SOUND "/root/soundfiles/is.wav"
// "right"
#define RIGHT_SOUND "/root/soundfiles/right.wav"
// "left"
#define LEFT_SOUND "/root/soundfiles/left.wav"
// "ahead"
#define AHEAD_SOUND "/root/soundfiles/ahead.wav"
 // "behind"
#define BEHIND_SOUND "/root/soundfiles/behind.wav"
// volume up and down indicator
#define VOLUME_SOUND "/root/volume.wav"

#define AHEAD 0
#define RIGHT 1
#define BEHIND 2
#define LEFT 3
    
// each beacon will be given an ID which corresponds to its location
// in the map string array
const char intSounds[6][50] = {SOUND_00, SOUND_10, SOUND_20, SOUND_30, 
     SOUND_40, SOUND_50};
const char decSounds[5][50] = {SOUND_05, SOUND_15, SOUND_25, SOUND_35, 
     SOUND_45};
const char direction[4][50] = {AHEAD_SOUND, RIGHT_SOUND, BEHIND_SOUND, LEFT_SOUND};

#endif  /* SOUNDS_H */