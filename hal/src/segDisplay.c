#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "hal/segDisplay.h"

/**
 *   -----1b------
 *  |\     |     /|
 * 32b 16b 8b  4b 128a
 *  |   \  |  /   |
 *   --2b-- --8a--
 *  |   /  |  \   |
 *128b 1a  2a  4a 64a 
 *  |/     |     \|
 *   -----16a-----  -32a-    
*/

#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"

#define I2C_DEVICE_ADDRESS 0x20

#define REG_DIRA 0x02
#define REG_DIRB 0x03
#define REG_OUTA 0x00
#define REG_OUTB 0x01

struct DigitValues {
    unsigned int first;
    unsigned int second;
};

struct SegValues {
    unsigned int regAVal;
    unsigned int regBVal;
};

static int initI2cBus(char* bus, int address);
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);
static void *segDisplayLoop(void *arg);
static struct SegValues getSegValues(unsigned int digitValue);
static void sleepForMs(long long delayInMs);
static void runCommand(char* command);

static bool isRunning;
static pthread_t segThread;
static struct DigitValues digitValues;

void Seg_init(void) {
    isRunning = true;
    segThread = pthread_create(&segThread, NULL, segDisplayLoop, NULL);
}

void Seg_cleanup(void) {
    isRunning = false;
    pthread_join(segThread, NULL);
}

void Seg_updateDigitValues(unsigned int newValue) {
    if(newValue >= 99) {
        digitValues.first = 9;
        digitValues.second = 9;
    }
    else {
        digitValues.first = newValue / 10;
        digitValues.second = newValue % 10;
    }
}

#define CONFIG_PINS_OUT_COMMAND "echo out > /sys/class/gpio/gpio61/direction; echo out > /sys/class/gpio/gpio44/direction"
#define CONFIG_PINS_I2C_COMMAND "config-pin P9_17 i2c; config-pin P9_18 i2c"
#define ZEN_RED_I2C_OUTPUT_COMMAND "i2cset -y 1 0x20 0x02 0x00; i2cset -y 1 0x20 0x03 0x00"

#define FIRST_DIGIT_OFF "echo 0 > /sys/class/gpio/gpio61/value"
#define SECOND_DIGIT_OFF "echo 0 > /sys/class/gpio/gpio44/value"
#define FIRST_DIGIT_ON "echo 1 > /sys/class/gpio/gpio61/value"
#define SECOND_DIGIT_ON "echo 1 > /sys/class/gpio/gpio44/value"
static void *segDisplayLoop(void *arg) {
    (void)arg;
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    unsigned int currentFirstDigitValue = 0;
    unsigned int currentSecondDigitValue = 0;
    struct SegValues firstValues = {0, 0};
    struct SegValues secondValues = {0, 0};
    // Configure pins for I2C
    runCommand(CONFIG_PINS_OUT_COMMAND);
    runCommand(CONFIG_PINS_I2C_COMMAND);
    runCommand(ZEN_RED_I2C_OUTPUT_COMMAND);
    while(isRunning) {
        // Update seg value structs if we have an update to our digit values
        if((digitValues.first != currentFirstDigitValue)) {
            currentFirstDigitValue = digitValues.first;
            firstValues = getSegValues(currentFirstDigitValue);
        }
        if((digitValues.second != currentSecondDigitValue)) {
            currentSecondDigitValue = digitValues.second;
            secondValues = getSegValues(currentSecondDigitValue);
        }
        // Turn both digits off
        runCommand(FIRST_DIGIT_OFF);
        runCommand(SECOND_DIGIT_OFF);

        // Write first digit and turn digit on, then sleep 5ms
        writeI2cReg(i2cFileDesc, REG_OUTA, firstValues.regAVal);
        writeI2cReg(i2cFileDesc, REG_OUTB, firstValues.regBVal);
        runCommand(FIRST_DIGIT_ON);
        sleepForMs(5);

        // Turn both digits off
        runCommand(FIRST_DIGIT_OFF);
        runCommand(SECOND_DIGIT_OFF);

        // Write second digit and turn digit on, then sleep 5ms
        writeI2cReg(i2cFileDesc, REG_OUTA, secondValues.regAVal);
        writeI2cReg(i2cFileDesc, REG_OUTB, secondValues.regBVal);
        runCommand(SECOND_DIGIT_ON);
        sleepForMs(5);
    }
    runCommand(FIRST_DIGIT_OFF);
    runCommand(SECOND_DIGIT_OFF);
    close(i2cFileDesc);
    return NULL;
}

#define CONFIG_P9_17 "config-pin P9_17 i2c"
#define CONFIG_P9_18 "config-pin P9_18 i2c"
static int initI2cBus(char* bus, int address)
{   
    // Configure pins for i2c
    runCommand(CONFIG_P9_17);
    runCommand(CONFIG_P9_18);

    // Initialize i2c bus
	int i2cFileDesc = open(bus, O_RDWR);
	if (i2cFileDesc < 0) {
		printf("I2C DRV: Unable to open bus for read/write (%s)\n", bus);
		perror("Error is:");
		exit(-1);
	}

	int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
	if (result < 0) {
		perror("Unable to set I2C device to slave address.");
		exit(-1);
	}
	return i2cFileDesc;
}

static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
	unsigned char buff[2];
	buff[0] = regAddr;
	buff[1] = value;
	int res = write(i2cFileDesc, buff, 2);
	if (res != 2) {
		perror("Unable to write i2c register");
		exit(-1);
	}
}

static struct SegValues getSegValues(unsigned int digitValue) {
    struct SegValues segValues;
    switch(digitValue) {
        case 0:
            segValues.regAVal = 16 + 64 + 128;
            segValues.regBVal = 1 + 32 + 128;
            break;
        case 1:
            segValues.regAVal = 64 + 128;
            segValues.regBVal = 4;
            break;
        case 2:
            segValues.regAVal = 8 + 16 + 128;
            segValues.regBVal = 1 + 2 + 128;
            break;
        case 3:
            segValues.regAVal = 8 + 16 + 64 + 128;
            segValues.regBVal = 1;
            break;
        case 4:
            segValues.regAVal = 8 + 64 + 128;
            segValues.regBVal = 2 + 32;
            break;
        case 5:
            segValues.regAVal = 8 + 16 + 64;
            segValues.regBVal = 1 + 2 + 32;
            break;
        case 6:
            segValues.regAVal = 8 + 16 + 64;
            segValues.regBVal = 1 + 2 + 32 + 128;
            break;
        case 7:
            segValues.regAVal = 64 + 128;
            segValues.regBVal = 1;
            break;
        case 8:
            segValues.regAVal = 8 + 16 + 64 + 128;
            segValues.regBVal = 1 + 2 + 32 + 128;
            break;
        default:
            segValues.regAVal = 8 + 64 + 128;
            segValues.regBVal = 1 + 2 + 32;
            break;
    }
    return segValues;
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

static void runCommand(char* command) {
    FILE *pipe = popen(command, "r");
    char buffer[1024];

    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
        break;
    }

    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        printf("Unable to execute command: %s, exit code: %d\n", command, exitCode);
        exit(1);
    }
}