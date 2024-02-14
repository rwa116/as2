#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>

#include "statistics.h"
#include "hal/sampler.h"
#include "hal/led.h"

static void *printingLoop(void *arg);
static void printStatistics(void);
static void sleepForMs(long long delayInMs);

static bool isRunning;
static pthread_t printingThread;

void Statistics_init(void) {
    isRunning = true;
    pthread_create(&printingThread, NULL, printingLoop, NULL);
}

void Statistics_cleanup(void) {
    isRunning = false;
    pthread_join(printingThread, NULL);
}

static void *printingLoop(void *arg) {
    (void)arg;
    while(isRunning) {
        sleepForMs(1000);
        printStatistics();
    }
    return NULL;
}

static void printStatistics(void) {
    int samples = Sampler_getHistorySize();
    double avgSample = Sampler_getAverageReading();
    int dips = Sampler_getDips();
    int historySize = 0;
    int potValue = Led_getPOTValue();
    double* history = Sampler_getHistory(&historySize);
    Period_statistics_t* stats = Sampler_getHistoryStats();
    printf("#Smpl/s = %d \tPOT @ %d => %dHz \tavg = %1.3fV \tdips = %d\t Smpl ms[ %1.3f, %1.3f] avg %1.3f/%d\n",
        samples, potValue, potValue/40, avgSample, dips, 
        stats->minPeriodInMs, stats->maxPeriodInMs, stats->avgPeriodInMs, stats->numSamples);

    int currentSample = 0;
    int increment = historySize / 20 == 0 ? 1 : historySize / 20;
    while(currentSample < historySize) {
        printf("  %d:%1.3f  ", currentSample, history[currentSample]);
        currentSample += increment;
    }
    free(history);
    printf("\n");
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