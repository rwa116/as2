// LED module
// Part of the Hardware Abstraction Layer (HAL)
// Responsible for controlling LED based off of POT input using PWN

#ifndef _LED_H_
#define _LED_H_

void Led_init(void);
void Led_cleanup(void);

int Led_getPOTValue(void);

#endif