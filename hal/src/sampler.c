#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "hal/sampler.h"
#include "hal/file.h"

static void *collectionLoop(void *arg);
static void calculateDips(void);
static void sleepForMs(long long delayInMs);
static long long getTimeInMs(void);

static pthread_t sampleThread;
bool isRunning;

struct sampler_history {
    double samples[1000];
    int size;
    int dips;
};

struct sampler_info {
    double samples[1000];
    double average;
    int currentSize;
    int totalSize;
    struct sampler_history samplerHistory;
};

#define VOLTAGE_DIRECTORY "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
static struct sampler_info samplerInfo = {
    .samples = {0},
    .average = 0,
    .currentSize = 0,
    .totalSize = 0,
    .samplerHistory = {
        .samples = {0},
        .size = 0,
        .dips = 0
    }
};

void Sampler_init(void) {
    isRunning = true;
    pthread_create(&sampleThread, NULL, collectionLoop, NULL);
}

void Sampler_cleanup(void) {
    isRunning = false;
    pthread_join(sampleThread, NULL);
}

static void *collectionLoop(void *arg) {
    char buffer[16];
    while(isRunning) {
        long long currentTime = getTimeInMs();
        while(getTimeInMs() - currentTime < 1000) {
            File_readFromFile(VOLTAGE_DIRECTORY, buffer);
            double readValue = strtod(buffer, NULL) / 4095 * 1.8;
            samplerInfo.average = samplerInfo.average == 0 ? readValue : samplerInfo.average*0.999 + readValue*0.001;
            samplerInfo.samples[samplerInfo.currentSize] = readValue;
            samplerInfo.currentSize++;
            sleepForMs(1);
        }

        samplerInfo.samplerHistory.dips = 0;
        calculateDips();
        Sampler_moveCurrentDataToHistory();
    }
    return arg;
}

void Sampler_moveCurrentDataToHistory(void) {
    memcpy(samplerInfo.samplerHistory.samples, samplerInfo.samples, sizeof(double)*samplerInfo.currentSize);
    samplerInfo.samplerHistory.size = samplerInfo.currentSize;
    samplerInfo.totalSize += samplerInfo.currentSize;

    samplerInfo.currentSize = 0;
    memset(samplerInfo.samples, 0, sizeof(double)*samplerInfo.currentSize);
}

int Sampler_getHistorySize(void) {
    return samplerInfo.samplerHistory.size;
}

double* Sampler_getHistory(int *size) {
    *size = samplerInfo.samplerHistory.size;

    double* history = (double*)malloc(sizeof(double) * *size);
    memcpy(history, samplerInfo.samplerHistory.samples, sizeof(double) * *size);

    return history;
}

double Sampler_getAverageReading(void) {
    return samplerInfo.average;
}


long long Sampler_getNumSamplesTaken(void) {
    return samplerInfo.totalSize;
}

int Sampler_getDips(void) {
    return samplerInfo.samplerHistory.dips;
}

void calculateDips(void) {
    bool dipped = false;
    double* history = samplerInfo.samplerHistory.samples;
    for(int i=0; i<samplerInfo.samplerHistory.size; i++) {
        if(!dipped && (history[i] < samplerInfo.average - 0.1) | (history[i] > samplerInfo.average + 0.1)) {
            dipped = true;
            samplerInfo.samplerHistory.dips++;
        }
        if(dipped && (history[i] > samplerInfo.average - 0.07) && (history[i] < samplerInfo.average + 0.07)) {
            dipped = false;
        }
    }
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