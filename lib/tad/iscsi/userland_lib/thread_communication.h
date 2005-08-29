#ifndef THREAD_COMMUNICATION_H
#define THREAD_COMMUNICATION_H

#include <sys/time.h>

typedef enum scsi_communication_event {
    EMPTY_REQUEST,
    TEST_READY,
    TARGET_READY,
    TEST_PROCESS_FINISHED,
} iscsi_communication_event;

extern int recv_request(int s, iscsi_communication_event *event,
                        struct timeval *timeout);

extern int send_request(int s, iscsi_communication_event event);

extern int send_request_with_answer(int s, iscsi_communication_event *event,
                                    struct timeval *timeout);

extern int create_socket_pair(int *pipe);

#endif
