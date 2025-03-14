#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <mqueue.h>
#include <stdbool.h>
#include "msg_item.h"
#include "message.h"

#define MAX_PIPE_NAME 64
#define MAXFILENAME 64
#define MAX_COMMAND_LENGTH 256
#define MAX_BUFFER_SIZE 1024
#define MAX_COMMANDS 100 // Maximum number of commands to store

// Define constants for message types
#define CONREQUEST_TYPE 1
#define CONREPLY_TYPE    2
#define COMLINE_TYPE 3
#define COMRESULT_TYPE 4
#define QUITREQ_TYPE 5
#define QUITREPLY_TYPE 6
#define QUITALL_TYPE 7

char *bufferp;
int bufferlen;
struct message *messagep;

void create_named_pipe(const char *pipe_name);
void send_connection_request(const char *mq_name, const char *cs_pipe_name, const char *sc_pipe_name, int client_id, int WSIZE);
void send_command(const char *cs_pipe_name, char *command, int csfd);
bool receive_result(const char *sc_pipe_name, int scfd);
void interactive_mode(const char *cs_pipe_name, const char *sc_pipe_name, int cs_fd, int sc_fd);
void batch_mode(const char *mq_name, const char *com_file, int cs_fd, int sc_fd, const char *sc_pipe_name);

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

    // Named pipe names
    char cs_pipe_name[MAX_PIPE_NAME];
    char sc_pipe_name[MAX_PIPE_NAME];
    snprintf(cs_pipe_name, sizeof(cs_pipe_name), "%d_cs", client_id);
    snprintf(sc_pipe_name, sizeof(sc_pipe_name), "%d_sc", client_id);

    // Create named pipes
    create_named_pipe(cs_pipe_name);
    create_named_pipe(sc_pipe_name);

    send_connection_request(MQNAME, cs_pipe_name, sc_pipe_name, client_id, WSIZE);

    // open pipes
    int cs_fd = open(cs_pipe_name, O_WRONLY);
    if (cs_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int sc_fd = open(sc_pipe_name, O_RDONLY);
    if (sc_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    receive_result(sc_pipe_name, sc_fd);

    // Check for mode
    if (argc > 2 && strcmp(argv[2], "-b") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s MQNAME -b COMFILE\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        const char *com_file = argv[3];
        batch_mode(MQNAME, com_file, cs_fd, sc_fd, sc_pipe_name);
    } else {
        interactive_mode(cs_pipe_name, sc_pipe_name, cs_fd, sc_fd);
    }
    return 0;
}

// Send connection request message to server over message queue
// Include cs_pipe_name, sc_pipe_name, client_id, and WSIZE in the message
void send_connection_request(const char *mq_name, const char *cs_pipe_name, const char *sc_pipe_name, int client_id, int WSIZE) {
    mqd_t mq;
    struct mq_attr mqAttr;
    int n;

    mq = mq_open(mq_name, O_RDWR);
    if(mq == -1){
        perror("cannot open msg queue\n");
        exit(1);
    }

    mq_getattr(mq, &mqAttr);
    bufferlen = mqAttr.mq_msgsize;
    bufferp = (char *) malloc(bufferlen);

    messagep = (struct message *) bufferp;
    messagep->type[0] = CONREQUEST_TYPE;
    messagep->length[0] = 0;
    messagep->length[1] = 0;
    messagep->length[2] = 0;
    messagep->length[3] = 0;
    snprintf(messagep->data, sizeof(struct message), "%s %s %d %d", sc_pipe_name,  cs_pipe_name, client_id, WSIZE);
    n = mq_send(mq, bufferp, sizeof(struct message), 0);
    if (n == -1){
        perror("mq send failed \n");
        exit(1);
    }
}

bool receive_result(const char *sc_pipe_name, int scfd) {
    // Receive result from server through sc_pipe
    // TODO: Implement receiving result from server
    struct message *message;
    char *bufp = (char*) malloc(sizeof (struct message));
    message = (struct message*) bufp;
    ssize_t bytes_read = read(scfd, message, sizeof (struct message));

    if (bytes_read < 0) {
        perror("read");
        return false;
    } else if (bytes_read == 0) {
        return true;
    }

    // Process the received message based on its type
    switch (message->type[0]) {
        case CONREPLY_TYPE:
            printf("message: CONREPLY received ");
            break;
        case COMRESULT_TYPE:
            printf("message: COMRESULT received: ");
            printf("command result: \n");
            printf("%s \n", message->data);
            break;
        case QUITREPLY_TYPE:
            printf("Quit Reply: %s\n", message->data);
            break;
        default:
            printf("Unknown message type received.\n");
            break;
    }

    return true;
}

// Function for interactive mode
void interactive_mode(const char *cs_pipe_name, const char *sc_pipe_name, int cs_fd, int sc_fd) {
    printf("Interactive mode: Enter commands (type 'quit' to quit):\n");
    char command[MAX_COMMAND_LENGTH];
    while (1) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        // Remove newline
        command[strcspn(command, "\n")] = '\0';

        // Check if the user wants to exit
        if (strcmp(command, "quit") == 0) {
            printf("Exiting interactive mode.\n");
            struct message *msgp;
            char *bfrp = (char *) malloc(sizeof (struct message));

            msgp = (struct message *) bfrp;
            msgp->type[0] = QUITREQ_TYPE;
            snprintf(msgp->data, sizeof("quit"), "%s", "quit");
            write(cs_fd, msgp, sizeof(struct message));
            free(bfrp);
            break;
        }

        // Send command
        struct message *msgp;
        char *bfrp = (char *) malloc(sizeof (struct message));

        msgp = (struct message *) bfrp;
        msgp->type[0] = COMLINE_TYPE;
        msgp->length[0] = (char) sizeof (command);
        msgp->length[1] = 0;
        msgp->length[2] = 0;
        msgp->length[3] = 0;
        snprintf(msgp->data, sizeof(struct message), "%s", command);
        write(cs_fd, msgp, sizeof(struct message));
        // Receive result
        bool result_received = receive_result(sc_pipe_name, sc_fd);
    }
}

void batch_mode(const char *mq_name, const char *com_file, int cs_fd, int sc_fd, const char *sc_pipe_name) {
    FILE *file = fopen(com_file, "r");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char command[MAX_COMMAND_LENGTH];
    while (fgets(command, sizeof(command), file) != NULL) {
        // Remove newline
        command[strcspn(command, "\n")] = '\0';

        // Check if the user wants to exit
        if (strcmp(command, "quit") == 0) {
            struct message *msgp;
            char *bfrp = (char *) malloc(sizeof (struct message));

            msgp = (struct message *) bfrp;
            msgp->type[0] = QUITREQ_TYPE;
            snprintf(msgp->data, sizeof("quit"), "%s", "quit");
            write(cs_fd, msgp, sizeof(struct message));
            free(bfrp);
            break;
        }

        // Send command to server
        struct message *msgp;
        char *bfrp = (char *) malloc(sizeof (struct message));

        msgp = (struct message *) bfrp;
        msgp->type[0] = COMLINE_TYPE;
        msgp->length[0] = (char) sizeof (command);
        msgp->length[1] = 0;
        msgp->length[2] = 0;
        msgp->length[3] = 0;
        snprintf(msgp->data, sizeof(struct message), "%s", command);
        write(cs_fd, msgp, sizeof(struct message));

        // Send command
        if (write(cs_fd, command, sizeof(command)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        // Check if end of file has been reached
        if (feof(file)) {
            receive_result(sc_pipe_name, sc_fd);
            break;
        }
    }
    fclose(file);
}

// Function to create a named pipe
void create_named_pipe(const char *pipe_name) {
    if (access(pipe_name, F_OK) != -1) {
        // Named pipe already exists
        printf("Named pipe %s already exists.\n", pipe_name);
    } else {
        // Named pipe doesn't exist, create it
        if (mkfifo(pipe_name, 0666) == -1) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
}
