#ifndef MY_LOGIN_H
#define MY_LOGIN_H
#include "../common/iscsi_common.h"
#include "../common/text_param.h"

#define VERSION_MAX 0

extern void  iscsi_start_new_session_group(void);
extern void* iscsi_server_rx_thread(void *arg);

#endif
