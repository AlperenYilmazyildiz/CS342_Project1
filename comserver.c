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
#include <sys/wait.h>
#include "msg_item.h"
#include "message.h"
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_CLIENTS 5
#define MAX_PIPE_NAME 64
#define MAXFILENAME 64
#define MAX_COMMAND_LENGTH 256
#define MAX_BUFFER_SIZE 1024

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
// Global variable for output file descriptor
int output_fd;

// Function prototypes
void serve_client(const char *cs_pipe_name, const char *sc_pipe_name, int wsize, unsigned int messageType, int *sc_fd, int *cs_fd, int pid);
unsigned int handle_client_request(const char* cs_fd_str, const char* sc_fd_str, int sc_fd, int cs_fd, int wsize);
unsigned int little_endian_convert(unsigned char data[4]);

int main(int argc, char *argv[]) {
    // Check command-line arguments
    int currentClientCount = 0;

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

    // Check if the output file already exists
    struct stat st;
    if (stat("output.txt", &st) == 0) {
        // Output file exists, remove it
        if (remove("output.txt") == -1) {
            perror("remove");
            exit(EXIT_FAILURE);
        }
        printf("Existing output file 'output.txt' removed.\n");
    }

    // Open output file in append mode
    output_fd = open("output.txt", O_CREAT | O_WRONLY | O_APPEND, 0666);
    if (output_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

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

        int sc, cs;
        serve_client(csName, scName, wSize, messageType, &sc, &cs, clientId);

        if(messageType == 1 && currentClientCount < MAX_CLIENTS){
            pid_t serverChildPid;
            currentClientCount++;
            serverChildPid = fork();

            if (serverChildPid < 0) {
                printf ("fork() failed\n");
                exit (1);
            }

            if (serverChildPid == 0){
                printf("server child in action \n");
                printf("after serving client sc %d cs %d\n", sc, cs);
                // TODO wait for quit command
                unsigned int flag = 0;
                while (flag != 5) {
                    flag = handle_client_request(csName, scName, sc, cs, wSize);
                    serve_client(csName, scName, wSize, flag, &sc, &cs, clientId);
                    printf("flag %d /n", flag);
                }
            }

        }
    }

    free(bufferp);
    mq_close(mq);
    // Accept incoming client connections

    // Fork child processes to serve clients

    return 0;
}

// Function to serve a connected client
void serve_client(const char *cs_pipe_name, const char *sc_pipe_name, int wsize, unsigned int messageType, int *sc_fd, int *cs_fd, int pid) {
    // TODO: Implement client-serving logic
    if(messageType == 1){
        printf("message: CONREQUEST received ");
        printf("cs name = %s ", cs_pipe_name);
        printf("sc name =  %s ", sc_pipe_name);
        printf("pid = %d ", pid);
        printf("wsize = %d\n", wsize);

        *cs_fd = open(cs_pipe_name, O_RDONLY);
        if (*cs_fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        else{
            printf("cs pipe opened\n");
        }

        *sc_fd = open(sc_pipe_name, O_WRONLY);
        if (*sc_fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        else{
            struct message *msg;
            char *bufp = (char*) malloc(sizeof (struct message));
            msg = (struct message*) bufp;
            msg->type[0] = CONREPLY_TYPE;
            snprintf(msg->data, sizeof("Connection to server is successful\n"), "%s", "Connection to server is successful\n");
            write(*sc_fd, msg, sizeof (struct message));
        }
    }
    else if(messageType == 3){
        struct message *msg;
        char *bufp = (char*) malloc(sizeof (struct message));
        msg = (struct message*) bufp;
        read(*cs_fd, msg, sizeof(struct message));

        printf("message: COMLINE received ");
        printf("len=%d, ", little_endian_convert(msg->length) + 8);
        printf("type=%d, ", msg->type[0]);
        printf("data=%s", msg->data);

        char *bufp2 = (char*) malloc(sizeof (struct message));
        msg = (struct message*) bufp2;
        msg->type[0] = COMRESULT_TYPE;
        write(*sc_fd, msg, sizeof (struct message));
    }
    else if(messageType == 5){
        printf("message: QUITREQ received\n");
        struct message *msg;
        char *bufp = (char*) malloc(sizeof (struct message));
        msg = (struct message*) bufp;
        msg->type[0] = QUITREPLY_TYPE;
        snprintf(msg->data, sizeof("Quiting from child server\n"), "%s", "Quiting from child server\n");
        write(*sc_fd, msg, sizeof (struct message));
    }
    else if(messageType == 7){
        printf("QUITALL received");

    }
    // Open named pipes for communication
    // Call handle_client_request() to handle client commands
    // TODO: close pipes
    // TODO: return int cs_fd and sc_fd
}

void execute_command(const char *command_line, int sc_fd) {
    bool isTwoCommand;

    char command_copy[MAX_COMMAND_LENGTH];
    strcpy(command_copy, command_line); // Make a copy since strtok modifies the string

    char *token = strtok(command_copy, "|");
    char *command1 = NULL;
    char *command2 = NULL;

    if (token != NULL) {
        isTwoCommand = true;

        // First command before pipe
        printf("First command: %s\n", token);
        command1 = strdup(token);
        // Second command after pipe
        token = strtok(NULL, "|");
        if (token != NULL) {
            printf("Second command: %s\n", token);
            command2 = strdup(token);
        } else {
            isTwoCommand = false;
        }
    } else {
        isTwoCommand = false;
        printf("Command: %s\n", command_line);
    }

    if (!isTwoCommand) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {  // Child process (runner child)
            // Redirect standard output to the output file
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);

            // Execute the command
            execl("/bin/sh", "sh", "-c", command_line, NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        } else { // Parent process (server child)
            // Wait for the runner child to finish
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }

            // Read the content of the output file and send it through sc_fd
            lseek(output_fd, 0, SEEK_SET); // Move file offset to beginning
            char buffer[MAX_BUFFER_SIZE];
            ssize_t bytes_read;
            while ((bytes_read = read(output_fd, buffer, sizeof(buffer))) > 0) {
                // Write the content to the sc pipe
                if (write(sc_fd, buffer, bytes_read) == -1) {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }  else {
        // Code for handling two commands

        // Create an unnamed pipe
        int pipe_fds[2];
        if (pipe(pipe_fds) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // Fork the first runner child process
        pid_t pid1 = fork();
        if (pid1 < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid1 == 0) { // First runner child process
            // Redirect standard output to the write end of the pipe
            close(pipe_fds[0]); // Close read end of the pipe
            dup2(pipe_fds[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
            //write(pipe_fds[1], )
            close(pipe_fds[1]); // Close write end of the pipe

            //dup2(output_fd, STDOUT_FILENO); // Redirect stdout to the output file

            // Execute the first command
            execl("/bin/sh", "sh", "-c", command1, NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        }

        // Fork the second runner child process
        pid_t pid2 = fork();
        if (pid2 < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid2 == 0) { // Second runner child process
            // Redirect standard input to the read end of the pipe
            close(pipe_fds[1]); // Close write end of the pipe
            //dup2(pipe_fds[0], STDIN_FILENO); // Redirect stdin to the read end of the pipe
            ssize_t bytesRead;
            char buffer[MAX_BUFFER_SIZE];
            while ((bytesRead = read(pipe_fds[0], buffer, sizeof(buffer))) > 0) {
                // Process or store the data as needed
                write(STDOUT_FILENO, buffer, bytesRead);
            }
            //printf("buffer %s \n", buffer);
            //ssize_t bytes_written =
            write(output_fd, buffer, strlen(buffer));

            close(pipe_fds[0]); // Close read end of the pipe

            // Redirect standard output to the output file
            dup2(output_fd, STDOUT_FILENO); // Redirect stdout to the output file
            close(output_fd); // Close output file descriptor

            // Execute the second command
            execl("/bin/sh", "sh", "-c", command2, NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        }

        // Close unused ends of the pipe
        close(pipe_fds[0]);
        close(pipe_fds[1]);

        // Wait for both runner child processes to finish
        int status;
        waitpid(pid1, &status, 0);
        waitpid(pid2, &status, 0);
    }
    if (command1 != NULL) {
        free(command1);
    }
    if (command2 != NULL) {
        free(command2);
    }
}

// Function to handle client commands
unsigned int handle_client_request(const char* cs_fd_str, const char* sc_fd_str, int sc_fd, int cs_fd, int wsize) {
    // Convert string parameters to file descriptors

    char output_file[] = "output.txt";
    int output_fd = open(output_file, O_RDONLY);
    if (output_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Read command from cs_fd
    struct message *msg;
    char *bufp = (char*) malloc(sizeof (struct message));
    msg = (struct message*) bufp;
    ssize_t bytes_read = read(cs_fd, msg, sizeof(struct message));
    //char command[MAX_COMMAND_LENGTH];
    char *command = msg->data;
    printf("command size: %lu\n", sizeof(command));
    //printf("command ls | ps aux size: %lu\n", sizeof("ls | ps aux"));
    //ssize_t bytes_read = read(cs_fd, command, sizeof(command));
    //printf("com: %s \n" , command);
    if (bytes_read == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    printf("command: %s\n", command);

    if(msg->type[0] == 3){
        /*struct message *msg;
        char *bufp = (char*) malloc(sizeof (struct message));
        msg = (struct message*) bufp;
        read(*cs_fd, msg, sizeof(struct message));*/

        printf("message: COMLINE received ");
        printf("len=%d, ", little_endian_convert(msg->length) + 8);
        printf("type=%d, ", msg->type[0]);
        printf("data=%s", command);

        msg->type[0] = COMRESULT_TYPE;
        write(sc_fd, msg, sizeof (struct message));
    }

    // Send the result back to the client through sc_fd
    while (strcmp(command, "quit") != 0 ){
        // Execute command and write result to sc_fd
        //printf("loop %s\n", command);
        execute_command(command, sc_fd);
        //while ((bytes_read = read(output_fd, command, sizeof(command))) > 0) {
        //int noOfChunks = (int) bytes_read / wsize;
        //printf("bytes read %d\n", (int) bytes_read);
        //printf("no of chunks %d\n", noOfChunks);
        //for (int i = 0; i < noOfChunks; ++i) {
        if (write(sc_fd, command, wsize) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        //}
        //}
        //printf("afa\n");
        struct message *msg2;
        char *bufp2 = (char*) malloc(sizeof (struct message));
        msg2 = (struct message*) bufp2;
        read(cs_fd, msg2, sizeof(struct message));
        //char command[MAX_COMMAND_LENGTH];
        command = msg2->data;
        printf("command size: %lu\n", sizeof(command));
        //read(cs_fd, command, sizeof(command));
        if(msg2->type[0] == 3){
            /*struct message *msg;
            char *bufp = (char*) malloc(sizeof (struct message));
            msg = (struct message*) bufp;
            read(*cs_fd, msg, sizeof(struct message));*/

            printf("message: COMLINE received ");
            printf("len=%d, ", little_endian_convert(msg->length) + 8);
            printf("type=%d, ", msg2->type[0]);
            printf("data=%s\n", command);

            msg2->type[0] = COMRESULT_TYPE;
            write(sc_fd, msg2, sizeof (struct message));
        }
    }
    if (strcmp(command, "quit") == 0){
        printf("buraya geldi\n");
        return 5;
    }
    else if (strcmp(command, "quitall") == 0){
        return 7;
    }
    // Close the output file.txt
    close(output_fd);
    return 0;
}

// Function to convert little endian format
unsigned int little_endian_convert(unsigned char data[4]){
    unsigned int x = (data[3] << 24) + (data[2] << 16) + (data[1] << 8) + (data[0]);
    return x;
}