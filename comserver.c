#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <mqueue.h>
#include "msg_item.h"
#include "message.h"

#define MAX_CLIENTS 10
#define MAX_PIPE_NAME 50
#define MAXFILENAME 64
#define MAX_COMMAND_LENGTH 100
#define MAX_BUFFER_SIZE 1024

char *bufferp;
int bufferlen;
struct message *messagep;

// Function prototypes
void serve_client(const char *cs_pipe_name, const char *sc_pipe_name, int wsize, unsigned int messageType);
void handle_client_request(int cs_fd, int sc_fd, int wsize);
unsigned int little_endian_convert(unsigned char data[4]);

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s MQNAME\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char MQNAME[MAXFILENAME];
    snprintf(MQNAME, sizeof(MQNAME), "%s", argv[1]);

    // TODO: Implement server logic
    // Initialize message queue
    mqd_t mq;
    struct mq_attr mqAttr;

    mq = mq_open(MQNAME, O_CREAT, 0666, NULL);

    if(mq == -1){
        perror("Message queue couldn't be created\n");
        exit(1);
    }
    printf("mq created, mq id = %d\n", (int) mq);
    mq_getattr(mq, &mqAttr);
    printf("mq maximum msgsize = %d\n", (int) mqAttr.mq_msgsize);

    bufferlen = mqAttr.mq_msgsize;
    bufferp = (char*) malloc(bufferlen);

    // Wait for incoming client connections
    while (1){
        int n = mq_receive(mq, bufferp, bufferlen, NULL);
        if(n == -1){
            perror("mq receive failed \n");
            exit(1);
        }
        printf("mq receive success, message size = %d\n", n);

        messagep = (struct message*) bufferp;

        unsigned char length[4];
        unsigned int messageType;
        char *data;

        length[0] = messagep->length[0];
        length[1] = messagep->length[1];
        length[2] = messagep->length[2];
        length[3] = messagep->length[3];

        messageType = messagep->type[0];

        data = messagep->data;

        printf("message data %s\n", data);

        char *scName;
        char *csName;
        int clientId;
        int wSize;

        scName = strtok(data, " ");
        csName = strtok(NULL, " ");
        clientId = atoi(strtok(NULL, " "));
        wSize = atoi(strtok(NULL, " "));

        serve_client(csName, scName, wSize, messageType);

    }

    free(bufferp);
    mq_close(mq);
    // Accept incoming client connections

    // Fork child processes to serve clients

    return 0;
}

// Function to serve a connected client
void serve_client(const char *cs_pipe_name, const char *sc_pipe_name, int wsize, unsigned int messageType) {
    // TODO: Implement client-serving logic
    if(messageType == 1){
        printf("CONREQUEST received");
        printf("cs name %s\n", cs_pipe_name);
        printf("sc name %s\n", sc_pipe_name);
        printf("wsize %d\n", wsize);
        printf("message type %d\n", messageType);
    }
    else if(messageType == 2){
        printf("CONRESULT received");
    }
    else if(messageType == 3){
        printf("COMLINE received");
    }
    else if(messageType == 4){
        printf("COMRESULT received");
    }
    else if(messageType == 5){
        printf("QUITREQ received");
    }
    else if(messageType == 6){
        printf("QUITREPLY received");
    }
    else if(messageType == 7){
        printf("QUITALL received");
    }
    // Open named pipes for communication
    // Call handle_client_request() to handle client commands
}

// Function to handle client commands
void handle_client_request(int cs_fd, int sc_fd, int wsize) {
    // TODO: Implement command handling logic
    // Read command from cs_fd
    // Execute command and write result to sc_fd
}

// Function to convert little endian format
unsigned int little_endian_convert(unsigned char data[4]){
    unsigned int x = (data[3] << 24) + (data[2] << 16) + (data[1] << 8) + (data[0]);
    return x;
}