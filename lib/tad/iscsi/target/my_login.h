#ifndef MY_LOGIN_H
#define MY_LOGIN_H
#include "../common/iscsi_common.h"
#include "../common/text_param.h"

#define VERSION_MAX 0

#define WAIT_FOR_TARGET_READY \
    sleep(1)

#define WAIT_FOR_TARGET(thread_param_) \
    do {                                                                           \
        iscsi_communication_event event = TEST_READY;                              \
        struct timeval timeout;                                                    \
        timeout.tv_sec = 5;                                                        \
        timeout.tv_usec = 0;                                                       \
        rc = send_request_with_answer((thread_param_).TEST_SIDE, &event, &timeout);\
        if (rc != 0 || event != TARGET_READY)                                      \
        {                                                                          \
            printf("Communication error\n");                                       \
            goto cleanup;                                                          \
        }                                                                          \
    } while(0)

#define WAIT_FOR_TARGET_FOREVER(thread_param_) \
    do {                                                                   \
        iscsi_communication_event event = TEST_READY;                      \
        rc = send_request_with_answer((thread_param_).TEST_SIDE, &event,   \
                                      NULL);                               \
        if (rc != 0 || event != TARGET_READY)                              \
        {                                                                  \
            printf("Communication error\n");                               \
            goto cleanup;                                                  \
        }                                                                  \
    } while(0)

#define SEND_PROCESS_FINISHED(thread_param_) \
    do {                                                                      \
        rc = send_request((thread_param_).TEST_SIDE, TEST_PROCESS_FINISHED);  \
        if (rc != 0)                                                          \
        {                                                                     \
            printf("Communication error\n");                                  \
            goto cleanup;                                                     \
        }                                                                     \
    } while(0)

extern int iscsi_server_rx_thread(void *arg);
extern int create_socket_pair(int *pipe);

#endif
