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
#include "hal/sampler.h"

static pthread_t networkThread;
static bool isRunning;

static void *listenLoop(void *arg);
static void generateReply(char* buffer, char* input);

void Network_init(void) {
    isRunning = true;
    pthread_create(&networkThread, NULL, listenLoop, NULL);
}

void Network_cleanup(void) {
    isRunning = false;
    pthread_join(networkThread, NULL);
}

#define MAX_LEN 1024
#define PORT 12345
static void *listenLoop(void *arg) {
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
    while(isRunning) {
        struct sockaddr_in sinRemote;
        unsigned int sinLen = sizeof(sinRemote);
        char messageRx[MAX_LEN];

        int bytesRx = recvfrom(socketDescriptor, messageRx, MAX_LEN - 1, 0, 
            (struct sockaddr*) &sinRemote, &sinLen);

        messageRx[bytesRx] = 0;

        printf("Message received (%d bytes): '%s'\n", bytesRx, messageRx);
        char messageTx[MAX_LEN];
        generateReply(messageTx, messageRx);

        sinLen = sizeof(sinRemote);
        sendto(socketDescriptor, messageTx, strlen(messageTx), 0,
            (struct sockaddr*) &sinRemote, sinLen);
    }

    close(socketDescriptor);

    return arg;
}

static void generateReply(char* buffer, char* input) {
    if(strcmp(input, "count\n") == 0) {
        snprintf(buffer, MAX_LEN, "# samples taken total: %lld\n", Sampler_getNumSamplesTaken());
    }
    else if(strcmp(input, "length\n") == 0) {
        snprintf(buffer, MAX_LEN, "# samples taken last second: %d\n", Sampler_getHistorySize());
    }
    else {
        snprintf(buffer, MAX_LEN, 
            "Accepted command examples: \n"
            "count \t -- get the total number of samples taken. \n"
            "length \t -- get the number of samples taken in the previously completed second. \n"
            "dips \t -- get the number of dips in the previously completed second. \n"
            "history \t -- get all the samples in the previously completed second. \n"
            "stop \t -- cause the server program to end. \n"
            "<enter> \t -- repeat last command.\n"); 
    }
}

