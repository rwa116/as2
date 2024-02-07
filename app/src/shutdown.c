#include <pthread.h>

#include "shutdown.h"

static pthread_mutex_t lock;
static bool timeToShutdown = false;

void Shutdown_init(void) {
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_lock(&lock);
}

void Shutdown_cleanup(void) {
    pthread_mutex_destroy(&lock);
}

void Shutdown_signalShutdown(void) {
    pthread_mutex_unlock(&lock);
    timeToShutdown = true;
}

bool Shutdown_isShutdown(void) {
    return timeToShutdown;
}

void Shutdown_waitForShutdown(void) {
    pthread_mutex_lock(&lock);
    pthread_mutex_unlock(&lock);
}