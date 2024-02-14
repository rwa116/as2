// File module
// Part of the Hardware Abstraction Layer (HAL)
// Responsible for opening, writing/reading to/from, and closing files

#ifndef _FILE_H_
#define _FILE_H_

void File_writeToFile(char* filename, char* content);
void File_readFromFile(char* filename, char* buff);

#endif