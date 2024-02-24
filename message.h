//
// Created by alperen on 21.02.2024.
//

#ifndef CS342_PROJECT1_MESSAGE_H
#define CS342_PROJECT1_MESSAGE_H
#include <stdint.h>


// Define the message structure
struct message {
    unsigned char length[4];    // Total message size (4 bytes)
    unsigned int type[1];       // Type of the message (1 byte)
    unsigned int padding[3]; // Padding (3 bytes)
    unsigned int *data;      // Pointer to message data (0 or more bytes)
};

#endif //CS342_PROJECT1_MESSAGE_H
