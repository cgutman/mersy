// Contains the definitions for this implemntation
#include "mersy-impl.h"

#ifndef __MERSY_H
#define __MERSY_H

// Version
#define __MERSY_MAJOR_VER 2
#define __MERSY_MINOR_VER 0

// Message tags
#define MSG_ALL     0
#define MSG_VERBOSE 1
#define MSG_INFO    2
#define MSG_WARNING 3
#define MSG_ERROR   4
#define MSG_FATAL   5
#define MSG_NONE    6

// Mersy.c function definitions
void FindPrimes(unsigned int ThreadCount, unsigned int StartingPValue);

#endif
