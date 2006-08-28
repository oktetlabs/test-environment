#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "if_mangle.h"

int
main(int argc, char *argv[])
{
    int sock;
    int rc = 0;
    struct ifreq req;

    if (argc < 4)
    {
        fputs("Usage: mangle_setup (attach|detach) mangler-name interface-name\n",
              stderr);
        return EXIT_FAILURE;
    }
    sock = socket(PF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
        return EXIT_FAILURE;
    strncpy(req.ifr_name, argv[2], sizeof(req.ifr_name));
    if (strcmp(argv[1], "attach") == 0)
    {
        strncpy(req.ifr_slave, argv[3], sizeof(req.ifr_name));
        if (ioctl(sock, MANGLE_ENSLAVE, &req) != 0)
        {
            perror("can't attach interface");
            rc = EXIT_FAILURE;
        }
    }
    else if (strcmp(argv[1], "detach") == 0)
    {
        strncpy(req.ifr_slave, argv[3], sizeof(req.ifr_name));
        if (ioctl(sock, MANGLE_EMANCIPATE, &req) != 0)
        {
            perror("can't detach interface");
            rc = EXIT_FAILURE;
        }
    }
    else if (strcmp(argv[1], "update") == 0)
    {
        if (ioctl(sock, MANGLE_UPDATE_SLAVE, &req) != 0)
        {
            perror("can't update slave interface");
            rc = EXIT_FAILURE;
        }
    }
    else if (strcmp(argv[1], "configure") == 0)
    {
        mangle_configure_request conf;
        char *value = strchr(argv[3], '=');
        
        if (value == NULL)
        {
            fputs("No value given\n", stderr);
            rc = EXIT_FAILURE;
        }
        else
        {
            *value++ = '\0';
            conf.value = strtol(value, NULL, 0);
            if (strcmp(argv[3], "droprate") == 0)
            {
                conf.param = MANGLE_CONFIGURE_DROP_RATE;
            }
            else
            {
                fprintf(stderr, "Invalid config parameter %s\n", argv[3]);
                rc = EXIT_FAILURE;
            }
            if (rc == 0)
            {
                req.ifr_data = (char *)&conf;
                if (ioctl(sock, MANGLE_CONFIGURE, &req))
                {
                    perror("cannot configure");
                    rc = EXIT_FAILURE;
                }
            }
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
