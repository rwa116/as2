// Network module
// Part of the Hardware Abstraction Layer (HAL)
// Acts as a server to serve UDP requests from the client regarding
// requests for details about the sampler (light intensity sensor)

#ifndef _NETWORK_H_
#define _NETWORK_H_

// Begin/end the background thread which samples light levels.
void Network_init(void);
void Network_cleanup(void);

#endif