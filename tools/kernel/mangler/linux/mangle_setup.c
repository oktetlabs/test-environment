#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "if_mangle.h"

int
main(int argc, char *argv[])
{
    int sock;
    int rc = 0;
    struct ifreq req;

    if (argc != 4)
    {
        fputs("Usage: mangle_setup (attach|detach) mangler-name interface-name\n",
              stderr);
        return EXIT_FAILURE;
    }
    sock = socket(PF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
        return EXIT_FAILURE;
    strncpy(req.ifr_name, argv[2], sizeof(req.ifr_name));
    strncpy(req.ifr_slave, argv[3], sizeof(req.ifr_name));
    if (strcmp(argv[1], "attach") == 0)
    {
        if (ioctl(sock, MANGLE_ENSLAVE, &req) != 0)
        {
            perror("can't attach interface");
            rc = EXIT_FAILURE;
        }
    }
    else if (strcmp(argv[1], "detach") == 0)
    {
        if (ioctl(sock, MANGLE_EMANCIPATE, &req) != 0)
        {
            perror("can't detach interface");
            rc = EXIT_FAILURE;
        }
    }
    else
    {
        rc = EXIT_FAILURE;
        fprintf(stderr, "Unknown command '%s'", argv[1]);
    }
    close(sock);
    return rc;
}
