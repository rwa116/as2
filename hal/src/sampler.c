#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "hal/sampler.h"
#include "hal/file.h"
#include "hal/periodTimer.h"
#include "hal/segDisplay.h"

static void *collectionLoop(void *arg);
static void calculateDips(void);
static void moveCurrentDataToHistory(void);
static void sleepForMs(long long delayInMs);
static long long getTimeInMs(void);

static pthread_t sampleThread;
static _Atomic bool isRunning;

static pthread_mutex_t historyBufferLock;
static pthread_mutex_t historyStatsLock;

static double currentSamples[1000] = {0};
static int currentSize = 0;

static _Atomic double average = 0;
static _Atomic int totalSize = 0;

static double historySamples[1000];
static _Atomic int historySize;
static _Atomic int historyDips;
Period_statistics_t historyStats;

#define VOLTAGE_DIRECTORY "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"

void Sampler_init(void) {
    Period_init();
    isRunning = true;
    pthread_mutex_init(&historyBufferLock, NULL);
    pthread_mutex_init(&historyStatsLock, NULL);
    pthread_create(&sampleThread, NULL, collectionLoop, NULL);
}

void Sampler_cleanup(void) {
    isRunning = false;
    pthread_mutex_destroy(&historyStatsLock);
    pthread_mutex_destroy(&historyBufferLock);
    pthread_join(sampleThread, NULL);
    Period_cleanup();
}

static void *collectionLoop(void *arg) {
    (void)arg;
    char buffer[16];
    while(isRunning) {
        long long startTime = getTimeInMs();
        long long currentTime = getTimeInMs();
        while(currentTime - startTime < 1000) {
            File_readFromFile(VOLTAGE_DIRECTORY, buffer);
            double readValue = strtod(buffer, NULL) / 4095 * 1.8;
            average = average == 0 ? readValue : average*0.999 + readValue*0.001;
            currentSamples[currentSize] = readValue;
            currentSize++;
            sleepForMs(1);
            currentTime = getTimeInMs();
            Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        }

        calculateDips();
        pthread_mutex_lock(&historyStatsLock);
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &historyStats);
        pthread_mutex_unlock(&historyStatsLock);
        moveCurrentDataToHistory();
        Seg_updateDigitValues(historyDips);
    }
    return NULL;
}

Period_statistics_t Sampler_getHistoryStats(void) {
    Period_statistics_t historyStatsCopy;
    pthread_mutex_lock(&historyStatsLock);
    historyStatsCopy =  historyStats;
    pthread_mutex_unlock(&historyStatsLock);
    return historyStatsCopy;
}

int Sampler_getHistorySize(void) {
    return historySize;
}

double* Sampler_getHistory(int *size) {
    pthread_mutex_lock(&historyBufferLock);
    *size = historySize;

    double* history = (double*)malloc(sizeof(double) * *size);
    memcpy(history, historySamples, sizeof(double) * *size);
    pthread_mutex_unlock(&historyBufferLock);

    return history;
}

double Sampler_getAverageReading(void) {
    return average;
}

long long Sampler_getNumSamplesTaken(void) {
    return totalSize;
}

int Sampler_getDips(void) {
    return historyDips;
}

static void calculateDips(void) {
    historyDips = 0;
    bool dipped = false;
    double* history = historySamples;
    for(int i=0; i<historySize; i++) {
        if(!dipped && (history[i] < average - 0.1) | (history[i] > average + 0.1)) {
            dipped = true;
            historyDips++;
        }
        if(dipped && (history[i] > average - 0.07) && (history[i] < average + 0.07)) {
            dipped = false;
        }
    }
}

static void moveCurrentDataToHistory(void) {
    pthread_mutex_lock(&historyBufferLock);
    memset(historySamples, 0, sizeof(double)*historySize);
    memcpy(historySamples, currentSamples, sizeof(double)*currentSize);
    historySize = currentSize;
    pthread_mutex_unlock(&historyBufferLock);

    totalSize += currentSize;
    currentSize = 0;
    memset(currentSamples, 0, sizeof(double)*currentSize);
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

static long long getTimeInMs(void) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    long long milliSeconds = seconds * 1000
    + nanoSeconds / 1000000;
    return milliSeconds;
}