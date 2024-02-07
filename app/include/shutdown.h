// Shutdown Module
// This module is responsible for synchronizing the shutdown of the program across all
// modules and threads. Main will be waiting in a function in this module until another
// thread from another module unlocks it (initiates shutdown)

#ifndef _SHUTDOWN_H_
#define _SHUTDOWN_H_

#include <stdbool.h>

void Shutdown_init(void);
void Shutdown_cleanup(void);

void Shutdown_signalShutdown(void);
bool Shutdown_isShutdown(void);
void Shutdown_waitForShutdown(void);

#endif