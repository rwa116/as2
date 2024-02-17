// Sampler module
// Module to sample light levels in the background (uses a thread).
//
// It continuously samples the light level, and stores it internally.
// It provides access to the samples it recorded during the _previous_
// complete second.
//
// The application will do a number of actions each second which must
// be synchronized (such as computing dips and printing to the screen).
// To make easy to work with the data, the app must call
// Sampler_moveCurrentDataToHistory() each second to trigger this
// module to move the current samples into the history.

#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include "hal/periodTimer.h"

// Begin/end the background thread which samples light levels.
void Sampler_init(void);
void Sampler_cleanup(void);

// Gets history stats from period timer for current history
// i.e., average time between samples, min, max, num samples
Period_statistics_t Sampler_getHistoryStats(void);

// Get the number of samples collected during the previous complete second.
int Sampler_getHistorySize(void);

// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `size` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: It provides both data and size to ensure consistency.
double* Sampler_getHistory(int *size);

// Get the average light level (not tied to the history).
double Sampler_getAverageReading(void);

// Get the total number of light level samples taken so far.
long long Sampler_getNumSamplesTaken(void);

// Gets the number of dips from 1s history
int Sampler_getDips(void);

#endif