// Main program to build the application

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "hal/sampler.h"
#include "network.h"
#include "shutdown.h"
#include "statistics.h"
#include "hal/led.h"
#include "hal/segDisplay.h"

int main() {

    Shutdown_init();
    Sampler_init();
    Led_init();
    Seg_init();
    Statistics_init();
    Network_init();

    Shutdown_waitForShutdown();
    
    Network_cleanup();
    Statistics_cleanup();
    Seg_cleanup();
    Led_cleanup();
    Sampler_cleanup();
    Shutdown_cleanup();

}