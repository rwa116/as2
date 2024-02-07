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

static pthread_t networkThread;
static bool isRunning;

static void *listenLoop(void *arg);
static void sendReply(char* messageRx, int socketDescriptor, struct sockaddr_in *sinRemote);

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

        sendReply(messageRx, socketDescriptor, &sinRemote);
    }

    close(socketDescriptor);

    return arg;
}

#define MAX_SAMPLE_SIZE 8
static void sendReply(char* messageRx, int socketDescriptor, struct sockaddr_in *sinRemote) {
        char messageTx[MAX_LEN];
        unsigned int sinLen;

        if(strcmp(messageRx, "count\n") == 0) {
            snprintf(messageTx, MAX_LEN, "# samples taken total: %lld\n", Sampler_getNumSamplesTaken());
        }
        else if(strcmp(messageRx, "length\n") == 0) {
            snprintf(messageTx, MAX_LEN, "# samples taken last second: %d\n", Sampler_getHistorySize());
        }
        else if(strcmp(messageRx, "dips\n") == 0) {
            snprintf(messageTx, MAX_LEN, "# Dips: %d\n", Sampler_getDips());
        }
        else if(strcmp(messageRx, "history\n") == 0) {
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
        else if(strcmp(messageRx, "stop\n") == 0) {
            Shutdown_signalShutdown();
            return;
        }
        else {
            snprintf(messageTx, MAX_LEN, 
                "Accepted command examples: \n"
                "count \t -- get the total number of samples taken. \n"
                "length \t -- get the number of samples taken in the previously completed second. \n"
                "dips \t -- get the number of dips in the previously completed second. \n"
                "history \t -- get all the samples in the previously completed second. \n"
                "stop \t -- cause the server program to end. \n"
                "<enter> \t -- repeat last command.\n"); 
        }
        sinLen = sizeof(*sinRemote);
        sendto(socketDescriptor, messageTx, strlen(messageTx), 0, 
            (struct sockaddr*) sinRemote, sinLen);
}

