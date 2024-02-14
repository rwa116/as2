// Seg Display module
// Part of the Hardware Abstraction Layer (HAL)
// Responsible for controlling the Zen Cape's 14-seg display
// Should have one external function other than init and cleanup that takes 
// in the current values to display to the two digits

#ifndef _SEG_DISPLAY_H_
#define _SEG_DISPLAY_H_

void Seg_init(void);
void Seg_cleanup(void);

void Seg_updateDigitValues(unsigned int newValue);

#endif