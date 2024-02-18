#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <mqueue.h>
#include "msg_buffer.h"

#define MAX_PIPE_NAME 50
#define MAXFILENAME 64
#define MAX_COMMAND_LENGTH 100
#define MAX_BUFFER_SIZE 1024

char *bufferp;
int bufferlen;
struct msg_buffer * messagep;

// Function prototypes
void interactive_mode(const char *mq_name);
void batch_mode(const char *mq_name, const char *com_file);
int create_named_pipe(const char *pipe_name);

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc < 2 || argc > 5) {
        fprintf(stderr, "Usage: %s MQNAME [-b COMFILE] [-s WSIZE]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char MQNAME[MAXFILENAME];
    snprintf(MQNAME, sizeof(MQNAME), "%s", argv[1]);

    char COMFILE[MAXFILENAME];
    snprintf(COMFILE, sizeof(COMFILE), "%s", argv[2]);

    int WSIZE = atoi((argv[3]));

    printf("%s %s %d", MQNAME, COMFILE, WSIZE);

    // TODO send connection request message to server with message queue
    mqd_t mq;
    struct mq_attr mqAttr;
    int n;
    int l;

    //mq = mq_open()

    // TODO: Parse command-line arguments and call appropriate mode
    // Interactive mode or batch mode

    return 0;
}

// Function to operate in interactive mode
void interactive_mode(const char *mq_name) {
    // TODO: Implement interactive mode logic
    // Read commands from user and send them to server
}

// Function to operate in batch mode
void batch_mode(const char *mq_name, const char *com_file) {
    // TODO: Implement batch mode logic
    // Read commands from com_file and send them to server
}

// Function to create a named pipe
int create_named_pipe(const char *pipe_name) {
    // TODO: Implement named pipe creation logic
    // Use mkfifo() system call to create named pipe
    // Return file descriptor of the opened named pipe
}
