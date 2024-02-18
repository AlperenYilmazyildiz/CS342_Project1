//
// Created by alperen on 18.02.2024.
//

#ifndef CS342_PROJECT1_MSG_BUFFER_H
#define CS342_PROJECT1_MSG_BUFFER_H
#define MAX_PIPE_NAME 50

struct msg_buffer {
    long msg_type;
    char mq_data[MAX_PIPE_NAME];
};

#endif //CS342_PROJECT1_MSG_BUFFER_H
