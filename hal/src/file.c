#include "hal/file.h"

#include <stdio.h>
#include <stdlib.h>

void File_writeToFile(char* filename, char* content) {
    FILE *file = fopen(filename, "w");
    if(file == NULL) {
        printf("ERROR OPENING %s", filename);
        exit(1);
    }

    int written = fprintf(file, content);
    if (written <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }

    fclose(file);
}

void File_readFromFile(char* filename, char* buff) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("ERROR: Unable to open file (%s) for read\n", filename);
        exit(-1);
    }

    const int MAX_LENGTH = 1024;
    fgets(buff, MAX_LENGTH, file);

    fclose(file);
}