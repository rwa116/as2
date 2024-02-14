#include <pthread.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "network.h"
#include "shutdown.h"
#include "hal/sampler.h"

enum Command {
    COUNT,
    LENGTH,
    DIPS,
    HISTORY,
    STOP,
    HELP,
    ENTER,
    UNKNOWN
};

static pthread_t networkThread;
static bool isRunning;

static void *listenLoop(void *arg);
static enum Command checkCommand(char* input);
static void sendReply(enum Command command, int socketDescriptor, struct sockaddr_in *sinRemote);

void Network_init(void) {
    isRunning = true;
    pthread_create(&networkThread, NULL, listenLoop, NULL);
}

void Network_cleanup(void) {
    isRunning = false;
    pthread_join(networkThread, NULL);
}

#define MAX_LEN 1500
#define PORT 12345
static void *listenLoop(void *arg) {
    (void)arg;
    enum Command lastCommand = UNKNOWN;
    // Socket initialization
    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);

    int socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketDescriptor == -1) {
        perror("Failed to establish server socket");
        exit(1);
    }
    bind(socketDescriptor, (struct sockaddr*) &sin, sizeof(sin));

    // Message receive and reply loop
    while(!Shutdown_isShutdown()) {
        struct sockaddr_in sinRemote;
        unsigned int sinLen = sizeof(sinRemote);
        char messageRx[MAX_LEN];

        int bytesRx = recvfrom(socketDescriptor, messageRx, MAX_LEN - 1, 0, 
            (struct sockaddr*) &sinRemote, &sinLen);

        messageRx[bytesRx] = 0;
        enum Command sentCommand = checkCommand(messageRx);
        lastCommand = sentCommand == ENTER ? lastCommand : sentCommand;
        sendReply(lastCommand, socketDescriptor, &sinRemote);
    }

    close(socketDescriptor);

    return NULL;
}

static enum Command checkCommand(char* input) {
    if(strcmp(input, "count\n") == 0) {
        return COUNT;
    }
    else if(strcmp(input, "length\n") == 0) {
        return LENGTH;
    }
    else if(strcmp(input, "dips\n") == 0) {
        return DIPS;
    }
    else if(strcmp(input, "history\n") == 0) {
        return HISTORY;
    }
    else if(strcmp(input, "stop\n") == 0) {
        return STOP;
    }
    else if((strcmp(input, "help\n") == 0) || (strcmp(input, "?\n") == 0)) {
        return HELP;
    }
    else if(strcmp(input, "\n") == 0) {
        return ENTER;
    }
    else {
        return UNKNOWN;
    }
}

#define MAX_SAMPLE_SIZE 8
static void sendReply(enum Command command, int socketDescriptor, struct sockaddr_in *sinRemote) {
    char messageTx[MAX_LEN];
    unsigned int sinLen;

    switch(command) {
        case COUNT:
            snprintf(messageTx, MAX_LEN, "# samples taken total: %lld\n", Sampler_getNumSamplesTaken());
            break;
        case LENGTH:
            snprintf(messageTx, MAX_LEN, "# samples taken last second: %d\n", Sampler_getHistorySize());
            break;
        case DIPS:
            snprintf(messageTx, MAX_LEN, "# Dips: %d\n", Sampler_getDips());
            break;
        case HISTORY:
            {
                int length = 0;
                int offset = 0;
                double* history = Sampler_getHistory(&length);
                for(int i=0; i<length; i++) {
                    int written = snprintf(messageTx + offset, MAX_LEN - offset, "%.3f", history[i]);
                    offset += written;

                    if(i != length - 1) {
                        written = snprintf(messageTx + offset, MAX_LEN - offset, ", ");
                        offset += written;
                    }

                    if((i+1) % 10 == 0 || i == length - 1) {
                        written = snprintf(messageTx + offset, MAX_LEN - offset, "\n");
                        offset += written;
                    }
                    
                    if(MAX_LEN - offset < MAX_SAMPLE_SIZE) {
                        sinLen = sizeof(*sinRemote);
                        sendto(socketDescriptor, messageTx, strlen(messageTx), 0,
                            (struct sockaddr*) sinRemote, sinLen);

                        memset(messageTx, 0, MAX_LEN);
                        offset = 0;
                    }
                }
                free(history);
            }
            break;
        case STOP:
            Shutdown_signalShutdown();
            return;
        case HELP:
            snprintf(messageTx, MAX_LEN, 
                "Accepted command examples: \n"
                "count \t -- get the total number of samples taken. \n"
                "length \t -- get the number of samples taken in the previously completed second. \n"
                "dips \t -- get the number of dips in the previously completed second. \n"
                "history \t -- get all the samples in the previously completed second. \n"
                "stop \t -- cause the server program to end. \n"
                "<enter> \t -- repeat last command.\n"); 
            break;
        case UNKNOWN:
            snprintf(messageTx, MAX_LEN, "Unknown command.\n");
            break;
        default:
            perror("Invalid command");
            exit(1);
    }
    
    sinLen = sizeof(*sinRemote);
    sendto(socketDescriptor, messageTx, strlen(messageTx), 0, 
        (struct sockaddr*) sinRemote, sinLen);
}

