/** @file
 * @brief Windows Test Agent
 *
 * Dummy definitions for Windows agent
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id: win32_rpc.h 3672 2004-07-20 00:00:04Z helen $
 */
 
#ifndef __WIN32_DUMMY_H__
#define __WIN32_DUMMY_H__

#undef ERROR

#define ERESTART        1000
#define ESTRPIPE        1001
#define EUCLEAN         1002
#define ENOTNAM         1003
#define ENAVAIL         1004
#define EISNAM          1005
#define EREMOTEIO       1006
#define EMEDIUMTYPE     1007

#define AF_PACKET       17

#define PF_PACKET       AF_PACKET

#define MSG_DONTWAIT    -201
#define MSG_WAITALL     -202
#define MSG_TRUNC       -203
#define MSG_CTRUNC      -204
#define MSG_ERRQUEUE    -205

#define SO_BINDTODEVICE -301
#define SO_PRIORITY     -302

#define IP_PKTINFO      -401
#define IP_RECVERR      -402
#define IP_RECVOPTS     -403
#define IP_RECVTOS      -404
#define IP_RECVTTL      -405
#define IP_RECVPTS      -406


#define SIOCGSTAMP      -501
#define SIOCSPGRP       -502
#define SIOCGPGRP       -503
#define SIOCSIFFLAGS    -504
#define SIOCSIFADDR     -505
#define SIOCSIFNETMASK  -506
#define SIOCSIFBRDADDR  -507
#define SIOCGIFDSTADDR  -508
#define SIOCSIFDSTADDR  -509
#define SIOCSIFMTU      -510

#define SIGUNUSED       33

#define AI_PASSIVE      -601
#define AI_CANONNAME    -602
#define AI_NUMERICHOST  -603

#define EAI_BADFLAGS    -701
#define EAI_NONAME      -702
#define EAI_AGAIN       -703
#define EAI_FAIL        -704
#define EAI_NODATA      -705
#define EAI_FAMILY      -706
#define EAI_SOCKTYPE    -707
#define EAI_SERVICE     -708
#define EAI_ADDRFAMILY  -709
#define EAI_MEMORY      -710
#define EAI_SYSTEM      -711

#define IFF_DEBUG       -801
#define IFF_POINTOPOINT -802
#define IFF_NOARP       -803
#define IFF_ALLMULTI    -804
#define IFF_MASTER      -805
#define IFF_SLAVE       -806
#define IFF_PORTSEL     -807

#define IP_RETOPTS      -901

/* Portable IPv6/IPv4 version of sockaddr.
   Uses padding to force 8 byte alignment
   and maximum size of 128 bytes */
struct sockaddr_storage {
    short ss_family;
    char __ss_pad1[6];    /* pad to 8 */
    __int64 __ss_align; /* force alignment */
    char __ss_pad2[112];  /* pad to 128 */
};

/* Request struct for multicast socket ops */

struct ip_mreqn {
	struct in_addr	imr_multiaddr;		/* IP multicast address of group */
	struct in_addr	imr_address;		/* local IP address of interface */
	int		imr_ifindex;		/* Interface index */
};

struct in_pktinfo {
	int		ipi_ifindex;
	struct in_addr	ipi_spec_dst;
	struct in_addr	ipi_addr;
};

/* Size of object which can be written atomically.

   This macro has different values in different kernel versions.  The
   latest versions of ther kernel use 1024 and this is good choice.  Since
   the C library implementation of readv/writev is able to emulate the
   functionality even if the currently running kernel does not support
   this large value the readv/writev call will not fail because of this.  */
#define UIO_MAXIOV	1024


/* Structure for scatter/gather I/O.  */
struct iovec {
        void *iov_base;	/* Pointer to data.  */
        size_t iov_len;	/* Length of data.  */
};

/* Structure to contain information about address of a service provider.  */
struct addrinfo
{
        int ai_flags;                 /* Input flags.  */
        int ai_family;                /* Protocol family for socket.  */
        int ai_socktype;              /* Socket type.  */
        int ai_protocol;              /* Protocol for socket.  */
        socklen_t ai_addrlen;         /* Length of socket address.  */
        struct sockaddr *ai_addr;     /* Socket address for socket.  */
        char *ai_canonname;           /* Canonical name for service location.  */
        struct addrinfo *ai_next;     /* Pointer to next in list.  */
};

/* Length of interface name.  */
#define IF_NAMESIZE     16

struct if_nameindex {
        unsigned int if_index;      /* 1, 2, ... */
        char *if_name;              /* null terminated name: "eth0", ... */
};

static inline struct if_nameindex *if_nameindex (void) {
        return NULL;
}

static inline void if_freenameindex (struct if_nameindex *__ptr) {
        (void) 0;
}

static inline int getaddrinfo (__const char *__restrict __name,
                               __const char *__restrict __service,
                               __const struct addrinfo *__restrict __req,
                               struct addrinfo **__restrict __pai) {
        return -1;
}

static inline void freeaddrinfo (struct addrinfo *__ai) {
        (void) 0;
}

/* Asynchronous I/O control block.  */
struct aiocb
{
        int aio_fildes;               /* File desriptor.  */
        int aio_lio_opcode;           /* Operation to be performed.  */
        int aio_reqprio;              /* Request priority offset.  */
        volatile void *aio_buf;       /* Location of buffer.  */
        size_t aio_nbytes;            /* Length of transfer.  */
        struct sigevent aio_sigevent; /* Signal number and value.  */

        /* Internal members.  */
        struct aiocb *__next_prio;
        int __abs_prio;
        int __policy;
        int __error_code;
        _ssize_t __return_value;

#ifndef __USE_FILE_OFFSET64
        __off_t aio_offset;           /* File offset.  */
        char __pad[sizeof (_off64_t) - sizeof (_off_t)];
#else
        _off64_t aio_offset;         /* File offset.  */
#endif
        char __unused[32];
};

#endif /* __WIN32_DUMMY_H__ */
