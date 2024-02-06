// Main program to build the application

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "hal/sampler.h"
#include "network.h"

static long long getTimeInMs(void);
static void sleepForMs(long long delayInMs);

int main() {

    Sampler_init();
    Network_init();

    while(1) {

    }

    
    for(int i=0; i<20; i++) {
        getTimeInMs();
        sleepForMs(500);
        printf("Average = %f\n", Sampler_getAverageReading());
    }
    
    Network_cleanup();
    Sampler_cleanup();

}

static long long getTimeInMs(void) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    long long milliSeconds = seconds * 1000
    + nanoSeconds / 1000000;
    return milliSeconds;
}

static void sleepForMs(long long delayInMs) {
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}