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
#include "msg_buffer.h"

#define MAX_CLIENTS 10
#define MAX_PIPE_NAME 50
#define MAXFILENAME 64
#define MAX_COMMAND_LENGTH 100
#define MAX_BUFFER_SIZE 1024

char *bufferp;
int bufferlen;
struct msg_buffer * messagep;

// Function prototypes
void serve_client(const char *cs_pipe_name, const char *sc_pipe_name, int wsize);
void handle_client_request(int cs_fd, int sc_fd, int wsize);

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

        messagep = (struct msg_buffer*) bufferp;

        printf("received item.id = %ld\n", messagep->msg_type);
        printf("received item.astr = %s\n", messagep->mq_data);
        printf("\n");
    }

    free(bufferp);
    mq_close(mq);
    // Accept incoming client connections

    // Fork child processes to serve clients

    return 0;
}

// Function to serve a connected client
void serve_client(const char *cs_pipe_name, const char *sc_pipe_name, int wsize) {
    // TODO: Implement client-serving logic
    // Open named pipes for communication
    // Call handle_client_request() to handle client commands
}

// Function to handle client commands
void handle_client_request(int cs_fd, int sc_fd, int wsize) {
    // TODO: Implement command handling logic
    // Read command from cs_fd
    // Execute command and write result to sc_fd
    //deneme
}