/** @file
 *
 * Catches changes of IP on network intefaces from netlink socket.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include <linux/rtnetlink.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <time.h>
#include <poll.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PROGRAM_NAME "test_netlink"

#define ASSERT(_cond)                                     \
    if (_cond) {                                          \
        fprintf(stderr, "assert at %s() line %d: " #_cond \
                , __FUNCTION__, __LINE__);                \
        return;                                           \
    }

#define DEFINE_TO_STR(str_) #str_

#define ERROR(...) fprintf(stderr, "error: " __VA_ARGS__ )

#define LOG(...) fprintf(stdout,  __VA_ARGS__ )

#define BUFSIZE 1024

#define VERBOSE(...)                          \
    do {                                      \
        if (verbose) {                        \
                fprintf(stderr, __VA_ARGS__); \
        }                                     \
    } while (0)                               \

static volatile bool verbose = false;

static inline int
print_nl_msg_type(uint16_t type)
{
    char *str_type = NULL;
    int rc = 0;
    switch (type)
    {
        case RTM_NEWADDR:
            str_type = DEFINE_TO_STR(RTM_NEWADDR);
            break;
        case RTM_DELADDR:
            str_type = DEFINE_TO_STR(RTM_DELADDR);
            break;
        case RTM_GETADDR:
            str_type = DEFINE_TO_STR(RTM_GETADDR);
            break;
        default:
            str_type = "(unsupported by the utility)";
            rc = -1;
    }
    LOG("NLMSG type: %s\n", str_type);
    return rc;
}

static int
print_ifaddr_msg_fields(struct ifaddrmsg *msg)
{
    int rc = 0;
    char *family;
    switch (msg->ifa_family) {
        case AF_INET:
            family = "IPv4";
            break;
        case AF_INET6:
            family = "IPv6";
            break;
        default:
            family = "Unknown";
            rc = -1;
    }

    LOG("IFADDRMSG fields:\n");
    LOG("\tFAMILY: %s\n", family);
    LOG("\tPREFIXLEN: %d\n", msg->ifa_prefixlen);

    return rc;
}

/**
 * Check if netlink message header is one of RTM_*ADDR types.
 *
 * @param hdr       Netlink message header.
 */
static inline bool
check_nlmsg_type(struct nlmsghdr *hdr)
{
    switch (hdr->nlmsg_type)
    {
        case RTM_NEWADDR:
        case RTM_DELADDR:
        case RTM_GETADDR:
            break;
        default:
            return false;
    }
    return true;
}

static inline void
handle_rta(struct ifaddrmsg *msg_addr, struct nlmsghdr *nl_hdr)
{
    struct rtattr *rta     = (struct rtattr *) IFA_RTA(msg_addr);
    int            rta_len = IFA_PAYLOAD(nl_hdr);

    LOG("RTATTR fields:\n");
    for ( ; RTA_OK(rta, rta_len); rta = RTA_NEXT(rta, rta_len))
    {

        const void *data = RTA_DATA(rta);
        int  af = msg_addr->ifa_family;

        char buf[BUFSIZE];
        const size_t b_len = sizeof(buf);

        char *msg = "";

        memset(buf, 0, sizeof(buf));

#define IFA_CASE(type_, handle_)             \
    case type_: {                            \
        msg = "\t"DEFINE_TO_STR(type_)": ";  \
        handle_;                             \
        break;                               \
    }
        switch (rta->rta_type)
        {
            IFA_CASE(IFA_ADDRESS,   inet_ntop(af, data, buf, b_len));
            IFA_CASE(IFA_LOCAL,     inet_ntop(af, data, buf, b_len));
            IFA_CASE(IFA_BROADCAST, inet_ntop(af, data, buf, b_len));
            IFA_CASE(IFA_ANYCAST,   inet_ntop(af, data, buf, b_len));
            IFA_CASE(IFA_LABEL,     strncpy(buf, data, b_len)      );
            default:
                VERBOSE(PROGRAM_NAME": error: `rta_type' (%d) isn't "
                        "handled by the util.\n", rta->rta_type);
                continue;
        }
#undef IFA_CASE
        LOG("%s%s\n", msg, buf);
    }
}


/**
 * Read and handle message from kernel.
 *
 * @param sock  Socket to read from.
 */
void
handle_netlink_connection(int sock)
{

    uint8_t buf[BUFSIZE] = { 0 };
    struct nlmsghdr *nl_hdr;

    ssize_t len = recv(sock, buf, sizeof(buf), 0);
    if (len < 0)
    {
        perror("recv(): couldn't read a message");
        return;
    }

    nl_hdr = (struct nlmsghdr *)buf;

    ASSERT(nl_hdr->nlmsg_type == NLMSG_DONE);
    ASSERT(nl_hdr->nlmsg_type == NLMSG_ERROR);

    for ( ; NLMSG_OK(nl_hdr, len);  nl_hdr = NLMSG_NEXT(nl_hdr, len))
    {
        int rc;
        struct ifaddrmsg * msg_addr;
        if (!check_nlmsg_type(nl_hdr))
        {
            fprintf(stderr,
                    "error: `nlmsg_type' (%d) isn't an address type.\n",
                    nl_hdr->nlmsg_type);
            continue;
        }

        rc =  print_nl_msg_type(nl_hdr->nlmsg_type);
        if (rc < 0)
            continue;

        msg_addr = (struct ifaddrmsg *) NLMSG_DATA(nl_hdr);

        rc = print_ifaddr_msg_fields(msg_addr);
        if (rc < 0)
            continue;

        handle_rta(msg_addr, nl_hdr);
    }
}

static void
print_current_time(void)
{
    struct timeval t_val;
    const size_t b_len = 28;  /* %Y-%m-%d %H:%M:%S %lu ms\n */
    char buf[b_len];

    int rc = gettimeofday(&t_val, NULL);
    if (rc < 0)
    {
        ERROR("can't get current time.\n");
        return;
    }

    struct tm *tm = localtime(&t_val.tv_sec);
    if (tm == NULL)
    {
        ERROR("can't get local time.\n");
        return;
    }

    size_t d_len = strftime(buf, b_len, "%F %H:%M:%S", tm);
    if (d_len == 0)
    {
        ERROR("can't convert time to string.\n");
        return;
    }

    snprintf(buf + d_len, b_len - d_len, " %lu ms",t_val.tv_usec / 1000);

    LOG("%s\n", buf);
}


/**
 * The main process of socket listening.
 *
 * @param sock      Socket to listen.
 */
void
listen_netlink_socket(int sock)
{
    struct pollfd fds = (struct pollfd) {
        .fd     = sock,
        .events = POLLIN, /* Only read event. */
        .revents = 0
    };

    VERBOSE(PROGRAM_NAME ": run polling...\n");
    while (poll(&fds, 1, -1))
    {
        if (fds.revents & POLLIN)
        {
            VERBOSE(PROGRAM_NAME": socket is ready for reading.\n");

            print_current_time();

            handle_netlink_connection(sock);
        }
    }
}

/**
 * Initialize netlink socket and bind it.
 *
 * @param sock_out      Created socket (OUT).
 *
 * @return Status code.
 *
 * @se Create and bind socket.
 */
static int
init_netlink_socket(int *sock_out)
{
    int sock;
    struct sockaddr_nl nl_addr = (struct sockaddr_nl) {
        .nl_family  = AF_NETLINK,
        .nl_pad     = 0,
        .nl_pid     = 0,
        .nl_groups  = RTMGRP_IPV4_IFADDR  /* Need track IPv6? */
    };

    sock = socket(AF_NETLINK, SOCK_RAW | SOCK_NONBLOCK, NETLINK_ROUTE);
    if (sock < 0)
    {
        perror("error: socket()");
        return -1;
    }

    VERBOSE(PROGRAM_NAME": create socket (%d).\n", sock);

    int rc = bind(sock, (struct sockaddr*)&nl_addr, sizeof(nl_addr));
    if (rc < 0)
    {
        perror("error: bind()");
        close(sock);
        return -1;
    }

    VERBOSE(PROGRAM_NAME": bind socket to netlink.\n");

    *sock_out = sock;
    return 0;
}

void
usage(int exit_code)
{
    fputs("usage: " PROGRAM_NAME " [OPTION]\n"\
          " -h, --help      Print help message.\n"
          " -v, --verbose   Enable verbose mode.\n", stderr);
    exit(exit_code);
}

/**
 * Check and parse command line options.
 */
static void
check_option(int argc, char *argv[])
{
    int c;
    int long_opt_index = 0;
    static struct option long_opt[] = {
        { "help",    no_argument, 0, 'h' },
        { "verbose", no_argument, 0, 'v' }
    };

    while ((c = getopt_long(argc, argv, "hv", long_opt, &long_opt_index)) > 0)
    {
        switch (c)
        {
            case 'h':
            {
                usage(EXIT_SUCCESS);
                break;
            }
            case 'v':
                verbose = true;
                break;
            default:
                usage(EXIT_FAILURE);
        }
    }
}

int
main(int argc, char *argv[])
{
    int sock;
    int rc = 0;

    /* If `help' option is called, it's an endpoint. */
    check_option(argc, argv);

    rc = init_netlink_socket(&sock);
    if (rc < 0)
    {
        ERROR(PROGRAM_NAME": can't initilize netlink socket.\n");
        return -1;
    }

    listen_netlink_socket(sock);
    /* Unreachable. */
    close(sock);
    return rc;
}
