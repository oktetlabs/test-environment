#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "thread_communication.h"

int
recv_request(int s, iscsi_communication_event *event,
             struct timeval *timeout)
{
    fd_set         readfds;
    int            rc;
    uint8_t        buf;

    FD_ZERO(&readfds);
    FD_SET(s, &readfds);

    rc = select(s + 1, &readfds, NULL, NULL, timeout);
    if (rc == 0)
    {
        printf("Failed to receive the request via the sync pipe\n");
        return 1;
    }

    recv(s, &buf, 1, 0);

    *event = buf;

    return 0;
}

int
send_request(int s, iscsi_communication_event event)
{
    int rc;
    
    rc = send(s, &event, 1, 0);
    if (rc != 1)
    {
        printf("Failed to send the request via the sync pipe\n");
        return 1;
    }
    
    return 0;
}

int 
send_request_with_answer(int s, iscsi_communication_event *event,
                         struct timeval *timeout)
{
    send_request(s, *event);
    recv_request(s, event, timeout);
    return 0;
}
