#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <mqueue.h>
#include "msg_item.h"
#include "message.h"

#define MAX_PIPE_NAME 50
#define MAXFILENAME 64
#define MAX_COMMAND_LENGTH 100
#define MAX_BUFFER_SIZE 1024

// Define constants for message types
#define CONREQUEST_TYPE 1
#define CONRESULT_TYPE    2
#define COMLINE_TYPE 3
#define COMRESULT_TYPE 4
#define QUITREQ_TYPE 5
#define QUITREPLY_TYPE 6
#define QUITALL_TYPE 7

char *bufferp;
int bufferlen;
struct message *messagep;

// Function prototypes
void create_named_pipe(const char *pipe_name);
void send_connection_request(const char *mq_name, const char *cs_pipe_name, const char *sc_pipe_name, int client_id, int WSIZE);
void send_command(const char *cs_pipe_name, const char *command, int csfd);
void receive_result(const char *sc_pipe_name);
void interactive_mode(const char *cs_pipe_name, const char *sc_pipe_name, int csfd);
void batch_mode(const char *mq_name, const char *com_file);

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s MQNAME\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Client ID and maximum message size
    int client_id = getpid(); // Using process ID as client ID
    int WSIZE = atoi((argv[3])); // Example maximum message size

    char MQNAME[MAXFILENAME];
    snprintf(MQNAME, sizeof(MQNAME), "%s", argv[1]);

    char COMFILE[MAXFILENAME];
    snprintf(COMFILE, sizeof(COMFILE), "%s", argv[2]);

    printf("%s %s %d", MQNAME, COMFILE, WSIZE);

    // Named pipe names
    char cs_pipe_name[MAX_PIPE_NAME];
    char sc_pipe_name[MAX_PIPE_NAME];
    snprintf(cs_pipe_name, sizeof(cs_pipe_name), "%d_cs", client_id);
    snprintf(sc_pipe_name, sizeof(sc_pipe_name), "%d_sc", client_id);

    // Create named pipes
    create_named_pipe(cs_pipe_name);
    create_named_pipe(sc_pipe_name);

    // Send connection request to server
    send_connection_request(MQNAME, cs_pipe_name, sc_pipe_name, client_id, WSIZE);

    // open pipes
    int cs_fd = open(cs_pipe_name, O_WRONLY);
    if (cs_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    else{
        printf("cs pipe opened\n");
    }

    int sc_fd = open(sc_pipe_name, O_RDONLY);
    if (sc_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    else{
        printf("sc pipe opened\n");
    }

    // Wait for server response
    // TODO: Implement wait for server response
    char connectionResult[30];
    read(sc_fd, connectionResult, sizeof(connectionResult));
    printf("%s\n", connectionResult);

    // Check for batch mode
    if (argc > 2 && strcmp(argv[2], "-b") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s MQNAME -b COMFILE\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        // Batch modee
        const char *com_file = argv[3];
        batch_mode(MQNAME, com_file);
    } else {
        // Interactive mode
        interactive_mode(cs_pipe_name, sc_pipe_name, cs_fd);
    }

    // TODO: Parse command-line arguments and call appropriate mode
    // Interactive mode or batch mode

    return 0;
}

// Function to send connection request to server
void send_connection_request(const char *mq_name, const char *cs_pipe_name, const char *sc_pipe_name, int client_id, int WSIZE) {
    // Send connection request message to server over message queue
    // Include cs_pipe_name, sc_pipe_name, client_id, and WSIZE in the message
    // TODO: Implement sending connection request message

    // TODO send connection request message to server with message queue
    mqd_t mq;
    struct mq_attr mqAttr;
    int n;

    mq = mq_open(mq_name, O_RDWR);
    if(mq == -1){
        perror("cannot open msg queue\n");
        exit(1);
    }
    printf("mq opened, mq id = %d\n", (int)mq);

    mq_getattr(mq, &mqAttr);
    printf("mq maximum msgsize = %d\n", (int) mqAttr.mq_msgsize);
    bufferlen = mqAttr.mq_msgsize;
    bufferp = (char *) malloc(bufferlen);

    messagep = (struct message *) bufferp;
    messagep->type[0] = CONREQUEST_TYPE;
    messagep->length[0] = 0;
    messagep->length[1] = 0;
    messagep->length[2] = 0;
    messagep->length[3] = 0;
    snprintf(messagep->data, sizeof(struct message), "%s %s %d %d", sc_pipe_name,  cs_pipe_name, client_id, WSIZE);
    printf("comcli message data %s\n", messagep->data);
    n = mq_send(mq, bufferp, sizeof(struct message), 0);
    if (n == -1){
        perror("mq send failed \n");
        exit(1);
    }

    // TODO: close message queue
}

// Function to send command to server
void send_command(const char *cs_pipe_name, const char *command, int cs_fd) {
    // Send command to server child process through cs_pipe
    // TODO: Implement sending command to server
    write(cs_fd, command, sizeof(command));

}

// Function to receive result from server
void receive_result(const char *sc_pipe_name) {
    // Receive result from server through sc_pipe
    // TODO: Implement receiving result from server
}

// Function to operate in interactive mode
void interactive_mode(const char *cs_pipe_name, const char *sc_pipe_name, int cs_fd) {
    printf("Interactive mode: Enter commands (type 'exit' to quit):\n");
    char command[MAX_COMMAND_LENGTH];
    while (1) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        // Remove trailing newline character
        command[strcspn(command, "\n")] = '\0';

        // Check if the user wants to exit
        if (strcmp(command, "exit") == 0) {
            printf("Exiting interactive mode.\n");
            break;
        }

        // Send command to server
        printf("Sending command to server: %s\n", command);
        send_command(cs_pipe_name, command, cs_fd);

        // Receive result from server
        receive_result(sc_pipe_name);
    }
}
// Function to operate in batch mode
void batch_mode(const char *mq_name, const char *com_file) {
    printf("Batch mode: Reading commands from file '%s'\n", com_file);
    FILE *file = fopen(com_file, "r");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char command[100];
    while (fgets(command, sizeof(command), file) != NULL) {
        // Remove trailing newline character
        command[strcspn(command, "\n")] = '\0';

        // Send command to server
        printf("Sending command to server: %s\n", command);
        // TODO: Send command to server using message queue
    }

    fclose(file);
}

// Function to create a named pipe
void create_named_pipe(const char *pipe_name) {
    // Check if the named pipe exists
    if (access(pipe_name, F_OK) != -1) {
        // Named pipe already exists
        printf("Named pipe %s already exists.\n", pipe_name);
        // Handle the situation as needed, e.g., delete the existing file
        // unlink(pipe_name);
    } else {
        // Named pipe doesn't exist, create it
        if (mkfifo(pipe_name, 0666) == -1) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
        printf("Named pipe %s created successfully.\n", pipe_name);
    }

    // Open the named pipe for reading and writing
    /*int fd = open(pipe_name, O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    return fd;*/
}

