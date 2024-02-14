#include "pthread.h"
#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"

#include "hal/file.h"
#include "hal/led.h"

static void runCommand(char* command);
static void *pwmMonitorUpdateLoop(void *arg);
static void sleepForMs(long long delayInMs);

static bool isRunning;
static int currentPOTValue;
static pthread_t flashingThread;

#define POT_VOLTAGE_DIRECTORY "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define PWM_DUTY_CYCLE_DIRECTORY "/dev/bone/pwm/0/b/duty_cycle"
#define PWM_PERIOD_DIRECTORY "/dev/bone/pwm/0/b/period"
#define PWM_ENABLE_DIRECTORY "/dev/bone/pwm/0/b/enable"
#define PWM_ENABLE_PIN_COMMAND "config-pin p9.21 pwm"

void Led_init() {
    isRunning = true;
    runCommand(PWM_ENABLE_PIN_COMMAND);
    pthread_create(&flashingThread, NULL, pwmMonitorUpdateLoop, NULL);
}

void Led_cleanup() {
    isRunning = false;
    File_writeToFile(PWM_ENABLE_DIRECTORY, "0");
    pthread_join(flashingThread, NULL);
}

int Led_getPOTValue(void) {
    return currentPOTValue;
}

#define SECOND_MULTIPLIER 1000000000
static void *pwmMonitorUpdateLoop(void *arg) {
    (void)arg;
    int currentHz = 0;
    while(isRunning) {
        char readFileBuffer[64];
        File_readFromFile(POT_VOLTAGE_DIRECTORY, readFileBuffer);
        currentPOTValue = atoi(readFileBuffer);
        int targetHz = currentPOTValue / 40;
        char targetPeriod[16];
        char targetDutyCycle[16];
        if(targetHz != currentHz) {
            if(targetHz == 0) {
                File_writeToFile(PWM_ENABLE_DIRECTORY, "0");
            }
            else {
                double flTargetPeriod = (float)SECOND_MULTIPLIER / (float)targetHz;
                double flTargetDutyCycle = 0.25 * flTargetPeriod;
                snprintf(targetPeriod, 16, "%d", (int)(flTargetPeriod));
                snprintf(targetDutyCycle, 16, "%d", (int)(flTargetDutyCycle));

                File_writeToFile(PWM_ENABLE_DIRECTORY, "1");
                File_writeToFile(PWM_PERIOD_DIRECTORY, targetPeriod);
                File_writeToFile(PWM_DUTY_CYCLE_DIRECTORY, targetDutyCycle);
            }
            currentHz = targetHz;
        }

        sleepForMs(100);
    }
    return NULL;
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

static void sleepForMs(long long delayInMs) {
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}