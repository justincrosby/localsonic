/* 
 * File:   defs.h
 * Author: Justin
 *
 * This file contains the common definitions for both emitters and receivers
 */

#ifndef DEFS_H
#define DEFS_H

#define NANOSECONDS_PER_SECOND 1E9
#define NUM_SAMPLES 5
#define NUM_EMITTERS 1
#define NUM_RECEIVERS_PER_NODE 2
#define CODE 0xAC

inline double timeDifference(struct timespec startTime)
{
    struct timespec endTime;
    clock_gettime(CLOCK_REALTIME, &endTime);
    return (double)(endTime.tv_sec - startTime.tv_sec) * NANOSECONDS_PER_SECOND
         + (double)(endTime.tv_nsec - startTime.tv_nsec);
}

#endif  /* DEFS_H */