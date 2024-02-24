//
// Created by alperen on 18.02.2024.
//

#ifndef CS342_PROJECT1_MSG_ITEM_H
#define CS342_PROJECT1_MSG_ITEM_H
#define MAX_PIPE_NAME 50

#include "message.h"

struct msg_item{
    struct message *mq_data;
};

#endif //CS342_PROJECT1_MSG_ITEM_H
