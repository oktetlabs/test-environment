/** @file
 * @brief Remote Procedure Call
 *
 * Definition of RPC structures and functions
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */


/** Types of RPC operations */
enum tarpc_op {
    TARPC_CALL,       /**< Call non-blocking RPC (if supported) */
    TARPC_WAIT,       /**< Wait until non-blocking RPC is finished */
    TARPC_CALL_WAIT   /**< Call blocking RPC */                        
};

/**
 * Input arguments common for all RPC calls.
 *
 * @attention It should be the first field of all routine-specific 
 *            parameters, since implementation casts routine-specific
 *            parameters to this structure.
 */
struct tarpc_in_arg {
    char          name[16];   /**< Server name */
    enum tarpc_op op;         /**< RPC operation */
    unsigned long start_high; /**< Start time (in milliseconds) */
    unsigned long start_low;
    int           tid;        /**< Thread identifier (for checking and 
                                   waiting) */
};

/**
 * Output arguments common for all RPC calls.
 *
 * @attention It should be the first field of all routine-specific 
 *            parameters, since implementation casts routine-specific
 *            parameters to this structure.
 */
struct tarpc_out_arg {
    unsigned long _errno;   /**< @e errno of the operation from
                                 te_errno.h or rcf_rpc_defs.h */
    unsigned long duration; /**< Duration of the called routine
                                 execution (in microseconds) */
    int           tid;      /**< Identifier of the thread which 
                                 performs possibly blocking operation,
                                 but caller does not want to block.
                                 It should be passed as input when
                                 RPC client want to check status or
                                 wait for finish of the initiated
                                 operation. */
    unsigned long win_error;/**< Windows error as is */                                 
};


/** Generic address */
struct tarpc_sa {
    int           sa_family; /**< TA-independent domain */
    unsigned char sa_data<>;
};

/** struct timeval */
struct tarpc_timeval {
    long tv_sec;
    long tv_usec;
};


/** struct ifreq */
struct tarpc_ifreq {
#define IFNAMESIZE 16

    char rpc_ifr_name[IFNAMESIZE]; /**< Interface name */
    struct tarpc_sa rpc_ifr_addr;  /**< Different interface addresses */
    short int       rpc_ifr_flags; /**< Interface flags */
    int             rpc_ifr_mtu;   /**< Interface MTU */
};

/** struct ifconf */
struct tarpc_ifconf {
    tarpc_ifreq rpc_ifc_req<>;  
                         /**< Interface list returned by the ioctl */
    int         buflen;  /**< Length of the buffer to be passed to ioctl */
};

/** struct timespec */
struct tarpc_timespec {
    long tv_sec;
    long tv_nsec;
};
/** RPC size_t analog */
typedef unsigned int tarpc_size_t;
/** RPC pid_t analog */
typedef unsigned int tarpc_pid_t;
/** RPC ssize_t analog */
typedef int tarpc_ssize_t;
/** RPC socklen_t analog */
typedef unsigned int tarpc_socklen_t;
/** Handle of the 'sigset_t' or 0 */
typedef unsigned int tarpc_sigset_t;
/** Handle of the 'fd_set' or 0 */
typedef unsigned int tarpc_fd_set;
/** RPC off_t analog */
typedef long int tarpc_off_t;

/** Handle of the 'WSAEvent' or 0 */
typedef unsigned int tarpc_wsaevent;
/** Handle of the window */
typedef unsigned int tarpc_hwnd;
/** WSAOVERLAPPED structure */
typedef unsigned int tarpc_overlapped;

/** Function outputs nothing */
struct tarpc_void_out {
    struct tarpc_out_arg    common;
};

/** Function outputs 'int' return value only */
struct tarpc_int_retval_out {
    struct tarpc_out_arg    common;

    int                     retval;
};




/** Function outputs 'ssize_t' return value only */
struct tarpc_ssize_t_retval_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t           retval;
};


/*
 * RPC arguments
 */

/* setlibname() */

struct tarpc_setlibname_in {
    struct tarpc_in_arg common;
    
    char libname<>;
};

typedef struct tarpc_int_retval_out tarpc_setlibname_out;


/* socket() */

struct tarpc_socket_in {
    struct tarpc_in_arg common;
    
    int domain;  /**< TA-independent domain */
    int type;    /**< TA-independent socket type */
    int proto;   /**< TA-independent socket protocol */
    char info<>; /**< Protocol Info (for winsock2 only) */
    int flags;   /**< If 1, the socket is overlapped (for winsock2 only) */
};

struct tarpc_socket_out {
    struct tarpc_out_arg common;

    int fd;     /**< TA-local socket */
};

/* WSADuplicateSocket() */
struct tarpc_duplicate_socket_in {
    struct tarpc_in_arg common;
    
    int  s;             /**< Old socket */
    int  pid;           /**< Destination process PID */
    char info<>;        /**< Location for protocol info */
};
   
struct tarpc_duplicate_socket_out {
    struct tarpc_out_arg common;

    int  retval;
    char info<>;
};

/* dup() */

struct tarpc_dup_in {
    struct tarpc_in_arg common;

    int oldfd;
};

typedef struct tarpc_socket_out tarpc_dup_out;


/* dup2() */

struct tarpc_dup2_in {
    struct tarpc_in_arg common;

    int oldfd;
    int newfd;
};

typedef struct tarpc_socket_out tarpc_dup2_out;


/* close() */

struct tarpc_close_in {
    struct tarpc_in_arg common;

    int fd;     /**< TA-local socket */
};

typedef struct tarpc_int_retval_out tarpc_close_out;


/* shutdown() */

struct tarpc_shutdown_in {
    struct tarpc_in_arg common;

    int fd;     /**< TA-local socket */
    int how;    /**< rpc_shut_how */
};

typedef struct tarpc_int_retval_out tarpc_shutdown_out;




/* read() / write() */

struct tarpc_read_in {
    struct tarpc_in_arg common;

    int                 fd;
    unsigned char       buf<>;
    tarpc_size_t        len;
};

struct tarpc_read_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t           retval;
    
    unsigned char           buf<>;
};


typedef struct tarpc_read_in tarpc_write_in;

typedef struct tarpc_ssize_t_retval_out tarpc_write_out;



/* readv() / writev() */

struct tarpc_iovec {
    unsigned char   iov_base<>;
    tarpc_size_t    iov_len;
};

struct tarpc_readv_in {
    struct tarpc_in_arg common;

    int                 fd;
    struct tarpc_iovec  vector<>;
    int                 count;
};

struct tarpc_readv_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t           retval;

    struct tarpc_iovec      vector<>;
};


typedef struct tarpc_readv_in tarpc_writev_in;

typedef struct tarpc_ssize_t_retval_out tarpc_writev_out;



/* send() */

struct tarpc_send_in {
    struct tarpc_in_arg common;

    int                 fd;
    unsigned char       buf<>;
    tarpc_size_t        len;
    int                 flags;
};

typedef struct tarpc_ssize_t_retval_out tarpc_send_out;


/* recv() */

struct tarpc_recv_in {
    struct tarpc_in_arg common;

    int                 fd;
    unsigned char       buf<>;
    tarpc_size_t        len;
    int                 flags;
};

struct tarpc_recv_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t           retval;

    unsigned char           buf<>;
};

/* WSARecvEx() */

struct tarpc_wsarecv_ex_in {
    struct tarpc_in_arg common;

    int                 fd;       /**< TA-local socket */
    unsigned char       buf<>;    /**< Buffer for received data */
    tarpc_size_t        len;      /**< Maximum length of expected data */
    int                 flags;    /**< TA-independent flags */
};

struct tarpc_wsarecv_ex_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t           retval;  /**< Returned length */

    unsigned char           buf<>;   /**< Returned buffer with received 
                                          data */
    int                     flags;   /**< TA-independent flags */
};

/* sendto() */

struct tarpc_sendto_in {
    struct tarpc_in_arg common;

    int                 fd;
    unsigned char       buf<>;
    tarpc_size_t        len;
    int                 flags;
    struct tarpc_sa     to;
    tarpc_socklen_t     tolen;
};

typedef struct tarpc_ssize_t_retval_out tarpc_sendto_out;


/* recvfrom() */

struct tarpc_recvfrom_in {
    struct tarpc_in_arg common;

    int                 fd;         /**< TA-local socket */
    unsigned char       buf<>;      /**< Buffer for received data */
    tarpc_size_t        len;        /**< Maximum length of expected data */
    int                 flags;      /**< TA-independent flags */
    struct tarpc_sa     from;       /**< Address to be passed as location
                                         for data source address */
    tarpc_socklen_t     fromlen<>;  /**< Maximum expected length of the
                                         address */
};

struct tarpc_recvfrom_out {
    struct tarpc_out_arg  common;

    tarpc_ssize_t   retval;     /**< Returned length */

    unsigned char   buf<>;      /**< Returned buffer with received 
                                     data */
    struct tarpc_sa from;       /**< Source address returned by the 
                                     function */
    tarpc_socklen_t fromlen<>;  /**< Length of the source address
                                     returned by the function */
};



/* sendmsg() / recvmsg() */

struct tarpc_msghdr {
    struct tarpc_sa     msg_name;
    tarpc_socklen_t     msg_namelen;
    struct tarpc_iovec  msg_iov<>;
    tarpc_size_t        msg_iovlen;
    unsigned char       msg_control<>;
    tarpc_socklen_t     msg_controllen;
    int                 msg_flags;
};

struct tarpc_sendmsg_in {
    struct tarpc_in_arg common;

    int                 s;
    struct tarpc_msghdr msg<>;
    int                 flags;
};

typedef struct tarpc_int_retval_out tarpc_sendmsg_out;

struct tarpc_recvmsg_in {
    struct tarpc_in_arg common;

    int                 s;
    struct tarpc_msghdr msg<>;
    int                 flags;
};                

struct tarpc_recvmsg_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t           retval;
    
    struct tarpc_msghdr     msg<>;
};



/* bind() */

struct tarpc_bind_in {
    struct tarpc_in_arg common;

    int                 fd;     /**< TA-local socket */
    struct tarpc_sa     addr;   /**< Socket name */
    tarpc_socklen_t     len;    /**< Length to be passed to bind() */
};

typedef struct tarpc_int_retval_out tarpc_bind_out;


/* connect() */

struct tarpc_connect_in {
    struct tarpc_in_arg   common;

    int                 fd;     /**< TA-local socket */
    struct tarpc_sa     addr;   /**< Remote address */
    tarpc_socklen_t     len;    /**< Length to be passed to connect() */
};

typedef struct tarpc_int_retval_out tarpc_connect_out;

/* ConnectEx() */
struct tarpc_connect_ex_in {
    struct tarpc_in_arg     common;
    
    int                     fd;       /**< TA-local socket */
    struct tarpc_sa         addr;     /**< Remote address */
    tarpc_socklen_t         len;      /**< Length to be passed to connectEx() */
    char                    buf<>;    /**< Buffer for data to be sent */
    tarpc_size_t            len_buf;  /**< Size of data passed to connectEx() */
    tarpc_overlapped        overlapped; /**< WSAOVERLAPPED structure */
    tarpc_size_t            len_sent<>; /**< Returned by the function
                                             size of sent data from buf */
};                                               

struct tarpc_connect_ex_out {
    struct tarpc_out_arg  common;

    tarpc_ssize_t  retval;     /**< Returned value (1 success, 0 failure) */
    tarpc_size_t   len_sent<>; /**< Returned by the function
                                    size of sent data from buf */
};

/* DisconnectEx */
struct tarpc_disconnect_ex_in {
    struct tarpc_in_arg    common;
    
    int               fd;         /**< TA-local socket */   
    tarpc_overlapped  overlapped; /**< WSAOVERLAPPED structure */
    int               flags;      /**< Function call processing flag */
};

typedef struct tarpc_int_retval_out tarpc_disconnect_ex_out;     

/* listen() */

struct tarpc_listen_in {
    struct tarpc_in_arg     common;

    int                     fd;         /**< TA-local socket */
    int                     backlog;    /**< Length of the backlog */
};

typedef struct tarpc_int_retval_out tarpc_listen_out;


/* accept() */

struct tarpc_accept_in {
    struct tarpc_in_arg common;

    int                 fd;     /**< TA-local socket */
    struct tarpc_sa     addr;   /**< Location for peer name */
    tarpc_socklen_t     len<>;  /**< Length of the location 
                                     for peer name */
};

struct tarpc_accept_out {
    struct tarpc_out_arg    common;

    int                     retval;

    struct tarpc_sa         addr;   /**< Location for peer name */
    tarpc_socklen_t         len<>;  /**< Length of the location 
                                         for peer name */
};

/* WSAAccept() */

enum tarpc_accept_verdict {
    TARPC_CF_REJECT,  
    TARPC_CF_ACCEPT,
    TARPC_CF_DEFER 
};

struct tarpc_accept_cond {
    unsigned short       port;       /**< Caller port             */
    tarpc_accept_verdict verdict;    /**< Verdict for this caller */
};

struct tarpc_wsa_accept_in {
    struct tarpc_in_arg common;

    int                      fd;      /**< TA-local socket */
    struct tarpc_sa          addr;    /**< Location for peer name */
    tarpc_socklen_t          len<>;   /**< Length of the peer name location */
    struct tarpc_accept_cond cond<>;  /**< Data for the callback function */
};

struct tarpc_wsa_accept_out {
    struct tarpc_out_arg common;

    int                  retval;

    struct tarpc_sa      addr;   /**< Location for peer name */
    tarpc_socklen_t      len<>;  /**< Length of the peer name location */
};

/* AcceptEx() */
/* Also arguments for call of GetAcceptExSockAddrs function are specified */

struct tarpc_accept_ex_in {
    struct tarpc_in_arg common;
                                                                               
    int                     fd;           /**< TA-local socket */
    int                     fd_a;         /**< TA-local socket to wich the
                                               connection will be actually
                                               made */
    char                    buf<>;        /**< Buffer to receive first block 
                                               of data */
    tarpc_size_t            len;          /**< Length of data to be received */
 
    tarpc_overlapped        overlapped;   /**< WSAOVERLAPPED structure */
    tarpc_size_t            count<>;      /**< Location for
                                               count of received bytes */ 
    struct tarpc_sa         laddr;        /**< Pointer to the sockaddr structure
                                               that receives the local address of
                                               the connection 
                                               returned by 
                                               GetAcceptExSockAddrs function */
    tarpc_socklen_t         laddr_len<>;  /**< Size fo the local address
                                               in bytes */
    struct tarpc_sa         raddr;        /**< Pointer to the sockaddr structure
                                               that receives the remote address
                                               of the connection 
                                               returned by 
                                               GetAcceptExSockAddrs function */
    tarpc_socklen_t         raddr_len<>;  /**< Size fo the remote address
                                               in bytes */
};

struct tarpc_accept_ex_out {
    struct tarpc_out_arg    common;

    int                     retval;

    tarpc_size_t            count<>;      /**< Location for
                                               count of received bytes */ 
    char                    buf<>;        /**< Buffer to receive first block 
                                               of data */
    struct tarpc_sa         laddr;        /**< Pointer to the sockaddr structure
                                               that receives the local address of
                                               the connection 
                                               returned by 
                                               GetAcceptExSockAddrs function */
    tarpc_socklen_t         laddr_len<>;  /**< Size fo the local address
                                               in bytes */
    struct tarpc_sa         raddr;        /**< Pointer to the sockaddr structure
                                               that receives the remote address
                                               of the connection 
                                               returned by 
                                               GetAcceptExSockAddrs function */
    tarpc_socklen_t         raddr_len<>;  /**< Size fo the remote address
                                               in bytes */
};

/* TransmitFile() */

struct tarpc_transmit_file_in {
    struct tarpc_in_arg common;
                                                                               
    int                     fd;           /**< TA-local socket */
    char                    file<>;       /**< Handle to the open file to be
                                               transmitted */
    tarpc_size_t            len;          /**< Number of file bytes to
                                               transmite */
    tarpc_size_t            len_per_send; /**< Number of bytes of each block of
                                               data sent in each send operarion */
    tarpc_size_t            offset;       /**< Offset to be passed
                                               to OVERLAPPED. File position
                                               at wich to start to transfer */
    tarpc_size_t            offset_high;  /**< OffsetHigh to be passed
                                               to OVERLAPPED. High-oreder
                                               word of the file position
                                               at wich to start to transfer */                                               
    tarpc_overlapped        overlapped;   /**< WSAOVERLAPPED structure */
    char                    head<>;       /**< Buffer to be transmitted before
                                               the file data is transmitted */
    tarpc_size_t            head_len;     /**< Length of head in bytes */
    char                    tail<>;       /**< Buffer to be transmitted after
                                               the file data is transmitted */
    tarpc_size_t            tail_len;     /**< Length of tail in bytes */
    tarpc_size_t            flags;        /**< Parameter of TransmitFile() */
};

typedef struct tarpc_int_retval_out tarpc_transmit_file_out;

/* getsockname() */

typedef struct tarpc_accept_in tarpc_getsockname_in;

typedef struct tarpc_accept_out tarpc_getsockname_out;


/* getpeername() */

typedef struct tarpc_accept_in tarpc_getpeername_in;

typedef struct tarpc_accept_out tarpc_getpeername_out;



/*
 * fd_set operations
 */

/* fd_set_new() */

struct tarpc_fd_set_new_in {
    struct tarpc_in_arg common;
};

struct tarpc_fd_set_new_out {
    struct tarpc_out_arg    common;

    tarpc_fd_set            retval;
};


/* fd_set_delete() */

struct tarpc_fd_set_delete_in {
    struct tarpc_in_arg common;

    tarpc_fd_set        set;
};

typedef struct tarpc_void_out tarpc_fd_set_delete_out;


/* FD_ZERO() */

struct tarpc_do_fd_zero_in {
    struct tarpc_in_arg common;

    tarpc_fd_set        set;
};

typedef struct tarpc_void_out tarpc_do_fd_zero_out;


/* WSACreateEvent() */
struct tarpc_create_event_in {
    struct tarpc_in_arg common;
};

struct tarpc_create_event_out {
    struct tarpc_out_arg common;
    tarpc_wsaevent       retval;
};

/* WSACloseEvent() */
struct tarpc_close_event_in {
    struct tarpc_in_arg common;
    tarpc_wsaevent hevent;
};
typedef struct tarpc_int_retval_out tarpc_close_event_out;


/* WSAResetEvent() */
struct tarpc_reset_event_in {
    struct tarpc_in_arg common;
    tarpc_wsaevent hevent;
};

typedef struct tarpc_int_retval_out tarpc_reset_event_out;

/* Create/delete WSAOVERLAPPED structure */
struct tarpc_create_overlapped_in {
    struct tarpc_in_arg common;
    tarpc_wsaevent      hevent;
    unsigned int        offset;
    unsigned int        offset_high;
};

struct tarpc_create_overlapped_out {
    struct tarpc_out_arg common;
    tarpc_overlapped     retval;
};

struct tarpc_delete_overlapped_in {
    struct tarpc_in_arg common;
    tarpc_overlapped     overlapped;
};
typedef struct tarpc_void_out tarpc_delete_overlapped_out;


/* WSAEventSelect() */

struct tarpc_event_select_in {
    struct tarpc_in_arg common;

    int             fd;           /**< TA-local socket */ 
    tarpc_wsaevent  event_object; /**< Event object to be associated
                                       with set of network events */   
    unsigned long   event;        /**< Bitmask that specifies the set
                                         of network events */      
};

typedef struct tarpc_int_retval_out tarpc_event_select_out;

/* WSAEnumNetworkEvents() */

struct tarpc_enum_network_events_in {
    struct tarpc_in_arg common;

    int             fd;           /**< TA-local socket */ 
    tarpc_wsaevent  event_object; /**< Event object to be reset */   
    unsigned long   event<>;      /**< Bitmask that specifies the set
                                       of network events occurred */      
};

struct tarpc_enum_network_events_out {
    struct tarpc_out_arg common;

    int                  retval;
    unsigned long        event<>;    /**< Bitmask that specifies the set
                                          of network events occurred */      
};

/* Create window */
struct tarpc_create_window_in {
    struct tarpc_in_arg common;
};

struct tarpc_create_window_out {
    struct tarpc_in_arg common;
    tarpc_hwnd          hwnd;
};

/* DestroyWindow */
struct tarpc_destroy_window_in {
    struct tarpc_in_arg common;
    tarpc_hwnd          hwnd;
};

typedef struct tarpc_void_out tarpc_destroy_window_out;

/* Get completion callback result */
struct tarpc_completion_callback_in {
    struct tarpc_in_arg common;
};

struct tarpc_completion_callback_out {
    struct tarpc_out_arg common;
    int                  called;        /**< 1 if callback was called    */
    int                  error;         /**< Error code (as is for now)  */
    int                  bytes;         /**< Number of bytes transferred */
    tarpc_overlapped     overlapped;    /**< Overlapped passed to callback */
};


/* WSAAsyncSelect */
struct tarpc_wsa_async_select_in {
    struct tarpc_in_arg common;
    int                 sock;   /**< Socket for WSAAsyncSelect() */
    tarpc_hwnd          hwnd;   /**< Window for messages receiving */
    unsigned int        event;  /**< Network event to be listened */
};

typedef struct tarpc_int_retval_out tarpc_wsa_async_select_out;

/* PeekMessage */
struct tarpc_peek_message_in {
    struct tarpc_in_arg common;
    tarpc_hwnd          hwnd;   /**< Window to be checked */
};

struct tarpc_peek_message_out {
    struct tarpc_out_arg common;
    int                  retval; /**< 0 if there are no messages */
    int                  sock;   /**< Socket about which the message is 
                                      received */
    unsigned int         event;  /**< Event about which the message is 
                                      received */
};

/* FD_SET() */

struct tarpc_do_fd_set_in {
    struct tarpc_in_arg common;

    int                 fd;
    tarpc_fd_set        set;
};

typedef struct tarpc_void_out tarpc_do_fd_set_out;


/* FD_CLR() */

struct tarpc_do_fd_clr_in {
    struct tarpc_in_arg common;

    int                 fd;
    tarpc_fd_set        set;
};

typedef struct tarpc_void_out tarpc_do_fd_clr_out;


/* FD_ISSET() */

struct tarpc_do_fd_isset_in {
    struct tarpc_in_arg common;

    int                 fd;
    tarpc_fd_set        set;
};

typedef struct tarpc_int_retval_out tarpc_do_fd_isset_out;



/*
 * I/O multiplexing
 */

/* select() */

struct tarpc_select_in {
    struct tarpc_in_arg     common;

    int                     n;
    tarpc_fd_set            readfds;
    tarpc_fd_set            writefds;
    tarpc_fd_set            exceptfds;
    struct tarpc_timeval    timeout<>;
};

struct tarpc_select_out {
    struct tarpc_out_arg    common;

    int                     retval;

    struct tarpc_timeval    timeout<>;
};
  

/* pselect() */

struct tarpc_pselect_in {
    struct tarpc_in_arg     common;

    int                     n;
    tarpc_fd_set            readfds;
    tarpc_fd_set            writefds;
    tarpc_fd_set            exceptfds;
    struct tarpc_timespec   timeout<>;
    tarpc_sigset_t          sigmask;
};

typedef struct tarpc_int_retval_out tarpc_pselect_out;


/* poll() */

struct tarpc_pollfd {
    int     fd;
    short   events;
    short   revents;
};

struct tarpc_poll_in {
    struct tarpc_in_arg common;

    struct tarpc_pollfd ufds<>;
    unsigned int        nfds;
    int                 timeout;
};

struct tarpc_poll_out {
    struct tarpc_out_arg    common;

    int                     retval;

    struct tarpc_pollfd     ufds<>;
};


/*
 * Socket options
 */
 
enum option_type {
    OPT_INT      = 1,
    OPT_LINGER   = 2,
    OPT_TIMEVAL  = 3,
    OPT_MREQN    = 4,
    OPT_IPADDR   = 5,
    OPT_STRING   = 6,
    OPT_TCP_INFO = 7
};

struct option_value_linger {
    int l_onoff; 
    int l_linger;
};

struct option_value_mreqn {
    char imr_multiaddr[4]; /* IP multicast group address */                  
    char imr_address[4];   /* IP address of local interface */
    int  imr_ifindex;      /* Interface index */
};

struct option_value_tcp_info {
    unsigned char tcpi_state;
    unsigned char tcpi_ca_state;
    unsigned char tcpi_retransmits;
    unsigned char tcpi_probes;
    unsigned char tcpi_backoff;
    unsigned char tcpi_options;
    unsigned char tcpi_snd_wscale;
    unsigned char tcpi_rcv_wscale;

    unsigned int tcpi_rto;
    unsigned int tcpi_ato;
    unsigned int tcpi_snd_mss;
    unsigned int tcpi_rcv_mss;

    unsigned int tcpi_unacked;
    unsigned int tcpi_sacked;
    unsigned int tcpi_lost;
    unsigned int tcpi_retrans;
    unsigned int tcpi_fackets;

    /* Times. */
    unsigned int tcpi_last_data_sent;
    unsigned int tcpi_last_ack_sent;
    unsigned int tcpi_last_data_recv;
    unsigned int tcpi_last_ack_recv;

    /* Metrics. */
    unsigned int tcpi_pmtu;
    unsigned int tcpi_rcv_ssthresh;
    unsigned int tcpi_rtt;
    unsigned int tcpi_rttvar;
    unsigned int tcpi_snd_ssthresh;
    unsigned int tcpi_snd_cwnd;
    unsigned int tcpi_advmss;
    unsigned int tcpi_reordering;
};

union option_value switch (option_type opttype) {
    case OPT_INT:     int opt_int;
    case OPT_LINGER:  struct option_value_linger opt_linger;
    case OPT_TIMEVAL: struct tarpc_timeval opt_timeval;
    case OPT_MREQN:   struct option_value_mreqn  opt_mreqn;
    case OPT_IPADDR:  char opt_ipaddr[4];
    case OPT_STRING:  char opt_string<>;
    case OPT_TCP_INFO: struct option_value_tcp_info opt_tcp_info;
};

/* setsockopt() */

struct tarpc_setsockopt_in {
    struct tarpc_in_arg  common;

    int                  s; 
    int                  level;
    int                  optname;
    option_value         optval<>;
    int                  optlen;
};

typedef struct tarpc_int_retval_out tarpc_setsockopt_out;

/* getsockopt() */

struct tarpc_getsockopt_in {
    struct tarpc_in_arg  common;

    int                  s;
    int                  level; 
    int                  optname;
    option_value         optval<>;
    int                  optlen<>;
};

struct tarpc_getsockopt_out {
    struct tarpc_out_arg  common;

    int                   retval;
    option_value          optval<>;
    int                   optlen<>;
};




/* ioctl() */

enum ioctl_type {
    IOCTL_INT = 1,
    IOCTL_TIMEVAL = 2,
    IOCTL_IFREQ = 3,
    IOCTL_IFCONF = 4
};

/* Access type of ioctl requests */
enum ioctl_access {
    IOCTL_RD = 1,
    IOCTL_WR = 2,
    IOCTL_RW = 3
};    

union ioctl_request switch (ioctl_type type) {
    case IOCTL_INT:     int           req_int;
    case IOCTL_TIMEVAL: tarpc_timeval req_timeval;
    case IOCTL_IFREQ:   tarpc_ifreq   req_ifreq; 
    case IOCTL_IFCONF:  tarpc_ifconf  req_ifconf; 
};

struct tarpc_ioctl_in {
    struct tarpc_in_arg common;
    
    int                 s;
    int                 code;
    ioctl_access        access;
    ioctl_request       req<>;
};

struct tarpc_ioctl_out {
    struct tarpc_out_arg common;
    int                  retval;
    ioctl_request        req<>;
};

/*
 * Interface name/index
 */

/* if_nametoindex() */

struct tarpc_if_nametoindex_in {
    struct tarpc_in_arg common;

    char                ifname<>;
};

struct tarpc_if_nametoindex_out {
    struct tarpc_out_arg    common;

    unsigned int            ifindex;
};


/* if_indextoname() */

struct tarpc_if_indextoname_in {
    struct tarpc_in_arg common;

    unsigned int        ifindex;
    char                ifname<>;
};

struct tarpc_if_indextoname_out {
    struct tarpc_out_arg    common;

    char                    ifname<>;
};


/* if_nameindex() */

struct tarpc_if_nameindex {
    unsigned int    ifindex;
    char            ifname<>;
};

struct tarpc_if_nameindex_in {
    struct tarpc_in_arg common;
};

struct tarpc_if_nameindex_out {
    struct tarpc_out_arg        common;

    struct tarpc_if_nameindex   ptr<>;
    unsigned int                mem_ptr;
};


/* if_freenameindex() */

struct tarpc_if_freenameindex_in {
    struct tarpc_in_arg         common;

    unsigned int                mem_ptr;
};

typedef struct tarpc_void_out tarpc_if_freenameindex_out;

/*
 * Signal handlers are registered by name of the funciton
 * in RPC server context.
 */

/* signal() */

struct tarpc_signal_in {
    struct tarpc_in_arg common;

    int                 signum;
    char                handler<>;  /**< Name of the signal handler
                                         function */
};

struct tarpc_signal_out {
    struct tarpc_out_arg    common;

    char                    handler<>;  /**< Name of the old signal
                                             handler function */
};


/*
 * Is kill() RPC required? It seems RCF is sufficient.
 * It might be usefull, if IUT has it's own implementation,
 * but it look unreal.
 */

/* kill() */

struct tarpc_kill_in {
    struct tarpc_in_arg common;

    tarpc_pid_t         pid;
    int                 signum;
};

typedef struct tarpc_int_retval_out tarpc_kill_out;



/*
 * Signal set are allocated/destroyed and manipulated in server context.
 * Two additional functions are defined to allocate/destroy signal set.
 * All standard signal set related functions get pointer to sigset_t.
 * Zero (0) handle corresponds to NULL pointer.
 */

/* sigset_new() */

struct tarpc_sigset_new_in {
    struct tarpc_in_arg common;
};

struct tarpc_sigset_new_out {
    struct tarpc_out_arg    common;

    tarpc_sigset_t          set;    /**< Handle (pointer in server 
                                         context) of the allocated set */
};


/* sigset_delete() */

struct tarpc_sigset_delete_in {
    struct tarpc_in_arg common;

    tarpc_sigset_t      set;    /**< Handle of the set to be freed */
                                  
};

typedef struct tarpc_void_out tarpc_sigset_delete_out;


/* sigprocmask() */

struct tarpc_sigprocmask_in {
    struct tarpc_in_arg common;

    int                 how;
    tarpc_sigset_t      set;    /**< Handle of the signal set */
    tarpc_sigset_t      oldset; /**< Handle of the old signal set */
};

typedef struct tarpc_int_retval_out tarpc_sigprocmask_out;

/* sigemptyset() */

struct tarpc_sigemptyset_in {
    struct tarpc_in_arg common;

    tarpc_sigset_t      set;    /**< Handle of the signal set */
};

typedef struct tarpc_int_retval_out tarpc_sigemptyset_out;


/* sigfillset() */

typedef struct tarpc_sigemptyset_in tarpc_sigfillset_in;

typedef struct tarpc_sigemptyset_out tarpc_sigfillset_out;


/* sigaddset() */

struct tarpc_sigaddset_in {
    struct tarpc_in_arg common;

    tarpc_sigset_t      set;    /**< Handle of the signal set */
    int                 signum;
};

typedef struct tarpc_sigemptyset_out tarpc_sigaddset_out;


/* sigdelset() */

typedef struct tarpc_sigaddset_in tarpc_sigdelset_in;

typedef struct tarpc_sigemptyset_out tarpc_sigdelset_out;
 

/* sigismember() */

typedef struct tarpc_sigaddset_in tarpc_sigismember_in;

typedef struct tarpc_sigemptyset_out tarpc_sigismember_out;


/* sigpending() */

typedef struct tarpc_sigemptyset_in tarpc_sigpending_in;

typedef struct tarpc_int_retval_out tarpc_sigpending_out;


/* sigsuspend() */

typedef struct tarpc_sigemptyset_in tarpc_sigsuspend_in;

typedef struct tarpc_int_retval_out tarpc_sigsuspend_out;


/* sigreceived() */

typedef struct tarpc_sigset_new_in tarpc_sigreceived_in;

typedef struct tarpc_sigset_new_out tarpc_sigreceived_out;



/* gethostbyname */

struct tarpc_h_addr {
    char val<>;
};

struct tarpc_h_alias {
    char name<>;
};

struct tarpc_hostent {
    char          h_name<>;         /**< Host name */
    tarpc_h_alias h_aliases<>;      /**< List of aliases */
    int           h_addrtype;       /**< RPC domain */
    int           h_length;         /**< Address length */
    tarpc_h_addr  h_addr_list<>;    /**< List of addresses */
};    

struct tarpc_gethostbyname_in {
    struct tarpc_in_arg common;
    char                name<>;
};    

struct tarpc_gethostbyname_out {
    struct tarpc_out_arg common;
    tarpc_hostent        res<>;
};

/* gethostbyaddr */

struct tarpc_gethostbyaddr_in {
    struct tarpc_in_arg common;
    tarpc_h_addr        addr; /**< Addresses */
    int                 type; /**< RPC domain */
};    

struct tarpc_gethostbyaddr_out {
    struct tarpc_out_arg common;
    tarpc_hostent        res<>;
};

struct tarpc_ai {
    int           flags;
    int           family;
    int           socktype;
    int           protocol;
    tarpc_size_t  addrlen;
    tarpc_sa      addr;
    char          canonname<>;
};

struct tarpc_getaddrinfo_in {
    struct tarpc_in_arg common;
    char                node<>;
    char                service<>;
    tarpc_ai            hints<>;
};

struct tarpc_getaddrinfo_out {
    struct tarpc_out_arg common;
    int                  retval;
    int                  mem_ptr;
    tarpc_ai             res<>;
};

struct tarpc_freeaddrinfo_in {
    struct tarpc_in_arg common;
    int                 mem_ptr;
};

typedef struct tarpc_void_out tarpc_freeaddrinfo_out;

/* pipe() */

struct tarpc_pipe_in {
    struct tarpc_in_arg common;
};

struct tarpc_pipe_out {
    struct tarpc_out_arg common;
    int retval;
    int filedes[2];
};

/* socketpair() */

struct tarpc_socketpair_in {
    struct tarpc_in_arg common;
    
    int domain; /**< TA-independent domain */
    int type;   /**< TA-independent socket type */
    int proto;  /**< TA-independent socket protocol */
};

struct tarpc_socketpair_out {
    struct tarpc_out_arg common;
    
    int retval; /**< Returned value */
    int sv[2];  /**< Socket pair */
};

/* fopen() */
struct tarpc_fopen_in {
    struct tarpc_in_arg common;
    
    char path<>;
    char mode<>;
};

struct tarpc_fopen_out {
    struct tarpc_out_arg common;
    
    unsigned int mem_ptr;
};

/* fileno() */
struct tarpc_fileno_in {
    struct tarpc_in_arg common;
    
    unsigned int mem_ptr;
};

struct tarpc_fileno_out {
    struct tarpc_out_arg common;
    
    int fd;
};

/* getuid() */
struct tarpc_getuid_in {
    struct tarpc_in_arg common;
};

struct tarpc_getuid_out {
    struct tarpc_out_arg common;
    
    unsigned int uid;
};

/* geteuid */
typedef struct tarpc_getuid_in tarpc_geteuid_in;
typedef struct tarpc_getuid_out tarpc_geteuid_out;

/* setuid() */
struct tarpc_setuid_in {
    struct tarpc_in_arg common;
    
    unsigned int uid;
};

typedef struct tarpc_int_retval_out tarpc_setuid_out;


/* geteuid */
typedef struct tarpc_setuid_in tarpc_seteuid_in;
typedef struct tarpc_setuid_out tarpc_seteuid_out;


/* getpid() */
struct tarpc_getpid_in {
    struct tarpc_in_arg common;
};

typedef struct tarpc_int_retval_out tarpc_getpid_out;


struct tarpc_simple_sender_in {
    struct tarpc_in_arg common;
    
    int s;               /**< Socket to be used */
    int size_min;        /**< Minimum size of the message */
    int size_max;        /**< Maximum size of the message */
    int size_rnd_once;   /**< If true, random size should be calculated
                              only once and used for all messages;
                              if false, random size is calculated for
                              each message */
    int delay_min;       /**< Minimum delay between messages in microseconds */
    int delay_max;       /**< Maximum delay between messages in microseconds */
    int delay_rnd_once;  /**< If true, random delay should be calculated
                              only once and used for all messages;
                              if false, random delay is calculated for
                              each message */
    int time2run;        /**< How long run (in seconds) */
};

struct tarpc_simple_sender_out {
    struct tarpc_out_arg common;
    
    /** Number of sent bytes */
    unsigned int bytes_high;
    unsigned int bytes_low;
    
    int      retval;  /**< 0 (success) or -1 (failure) */
};

struct tarpc_simple_receiver_in {
    struct tarpc_in_arg common;
    
    int s;               /**< Socket to be used */
};

struct tarpc_simple_receiver_out {
    struct tarpc_out_arg common;
    
    /** Number of received bytes */
    unsigned int bytes_high;
    unsigned int bytes_low;
    
    int      retval;  /**< 0 (success) or -1 (failure) */
};

/*
 * IOMUX functions
 */
enum iomux_func {
    FUNC_SELECT = 1,
    FUNC_PSELECT = 2,
    FUNC_POLL = 3
};


struct tarpc_flooder_in {
    struct tarpc_in_arg common;

    /** Input */
    int         sndrs<>;
    int         rcvrs<>;
    int         bulkszs;
    int         time2run;        /**< How long run (in seconds) */
    iomux_func  iomux;
    int         rx_nonblock;

    unsigned long   tx_stat<>;
    unsigned long   rx_stat<>;
};

struct tarpc_flooder_out {
    struct tarpc_out_arg common;

    int             retval;  /**< 0 (success) or -1 (failure) */
    unsigned long   tx_stat<>;
    unsigned long   rx_stat<>;
};


struct tarpc_echoer_in {
    struct tarpc_in_arg common;

    int         sockets<>;
    int         time2run;        /**< How long run (in seconds) */
    iomux_func  iomux;

    unsigned long   tx_stat<>;
    unsigned long   rx_stat<>;
};

struct tarpc_echoer_out {
    struct tarpc_out_arg common;

    int             retval;  /**< 0 (success) or -1 (failure) */
    unsigned long   tx_stat<>;
    unsigned long   rx_stat<>;
};


struct tarpc_aio_read_test_in {
    struct tarpc_in_arg common;
    
    int  s;      /**< Socket to be used */
    int  signum; /**< Signal to be used */
    int  t;      /**< Timeout for select() */
    int  buflen; /**< Buffer length to be passed to the read */
    char buf<>;  /**< Read data */
    char diag<>; /**< Error message */
};

struct tarpc_aio_read_test_out {
    struct tarpc_out_arg common;
    
    int  retval; /**< Status returned by the test routine */
    char buf<>;  /**< Data buffer */
    char diag<>; /**< Error message */
};

struct tarpc_aio_error_test_in {
    struct tarpc_in_arg common;
    char diag<>; /**< Error message */
};

struct tarpc_aio_error_test_out {
    struct tarpc_out_arg common;
    
    int  retval; /**< Status returned by the test routine */
    char diag<>; /**< Error message */
};

struct tarpc_aio_write_test_in {
    struct tarpc_in_arg common;
    
    int s;       /**< Socket to be used */
    int signum;  /**< Signal to be used */
    char buf<>;  /**< Data to be sent*/
    char diag<>; /**< Error message */
};

struct tarpc_aio_write_test_out {
    struct tarpc_out_arg common;
    
    int  retval; /**< Status returned by the test routine */
    char diag<>; /**< Error message */
};

struct tarpc_aio_suspend_test_in {
    struct tarpc_in_arg common;
    
    int s;       /**< Socket to be used */
    int s_aux;   /**< Additional dummy socket */
    int signum;  /**< Signal to be used */
    int t;       /**< Timeout for suspend */
    char buf<>;  /**< Data buffer */
    char diag<>; /**< Error message */
};

struct tarpc_aio_suspend_test_out {
    struct tarpc_out_arg common;
    
    int  retval; /**< Status returned by the test routine */
    char diag<>; /**< Error message */
    char buf<>;  /**< Read data */
};

struct tarpc_fork_in {
    struct tarpc_in_arg common;
    
    char name[16];
};

struct tarpc_fork_out {
    struct tarpc_out_arg common;
    
    int pid;
};

struct tarpc_pthread_create_in {
    struct tarpc_in_arg common;
    
    char name[16];
};

struct tarpc_pthread_create_out {
    struct tarpc_out_arg common;
    
    unsigned int tid;
    int          retval;
};

struct tarpc_pthread_cancel_in {
    struct tarpc_in_arg common;
    
    unsigned int tid;
};

struct tarpc_pthread_cancel_out {
    struct tarpc_out_arg common;
    
    int retval;
};

struct tarpc_execve_in {
    struct tarpc_in_arg common;
};

struct tarpc_execve_out {
    struct tarpc_out_arg common;
};


/* sendfile() */
struct tarpc_sendfile_in {
    struct tarpc_in_arg common;

    int                 out_fd;
    int                 in_fd;
    tarpc_off_t         offset<>;
    tarpc_size_t        count;
};

struct tarpc_sendfile_out {
    struct tarpc_out_arg  common;

    tarpc_ssize_t   retval;     /**< Returned length */
    tarpc_off_t     offset<>;   /**< new offset returned by the function */
};


/* socket_to_file() */
struct tarpc_socket_to_file_in {
    struct tarpc_in_arg common;

    int                 sock;
    char                path<>;
    long                timeout;
};

typedef struct tarpc_ssize_t_retval_out tarpc_socket_to_file_out;

/* WSASend */
struct tarpc_wsa_send_in {
    struct tarpc_in_arg common;

    int                 s;              /**< Socket */
    struct tarpc_iovec  vector<>;       /**< Buffers */
    int                 count;          /**< Number of buffers */
    int                 bytes_sent<>;   /**< Location for sent bytes num */
    int                 flags;          /**< Flags */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    int                 callback;       /**< If 1, completion callback should be 
                                             specified */
};

struct tarpc_wsa_send_out {
    struct tarpc_out_arg common;
    int                  retval;
    int                  bytes_sent<>;
};

/* WSARecv */
struct tarpc_wsa_recv_in {
    struct tarpc_in_arg common;

    int                 s;              /**< Socket */
    struct tarpc_iovec  vector<>;       /**< Buffers */
    int                 count;          /**< Number of buffers */
    int                 bytes_received<>;   
                                        /**< Location for received bytes num */
    int                 flags<>;        /**< Flags */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    int                 callback;       /**< If 1, completion callback should be 
                                             specified */
};

struct tarpc_wsa_recv_out {
    struct tarpc_out_arg common;
    int                  retval;
    struct tarpc_iovec   vector<>;       
    int                  bytes_received<>;   
    int                  flags<>;        
};


/* WSAGetOverlappedResult */
struct tarpc_get_overlapped_result_in {
    struct tarpc_in_arg common;

    int                 s;              /**< Socket    */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure */
    int                 wait;           /**< Wait flag */
    int                 bytes<>;        /**< Transferred bytes location */
    int                 flags<>;        /**< Flags location */
};    

struct tarpc_get_overlapped_result_out {
    struct tarpc_out_arg common;

    int                  retval;
    int                  bytes<>;        /**< Transferred bytes */
    int                  flags<>;        /**< Flags */
};    


program tarpc
{
    version ver0
    {
changequote([,])
define([cnt], 1)
define([counter], [cnt[]define([cnt],incr(cnt))])
define([RPC_DEF], [tarpc_$1_out _$1(tarpc_$1_in *) = counter;])

        RPC_DEF(setlibname)

        RPC_DEF(fork)
        RPC_DEF(pthread_create)
        RPC_DEF(pthread_cancel)
        RPC_DEF(execve)
        RPC_DEF(getpid)
        
        RPC_DEF(socket)
        RPC_DEF(duplicate_socket)
        RPC_DEF(dup)
        RPC_DEF(dup2)
        RPC_DEF(close)
        RPC_DEF(shutdown)

        RPC_DEF(read)
        RPC_DEF(write)

        RPC_DEF(readv)
        RPC_DEF(writev)

        RPC_DEF(send)
        RPC_DEF(recv)

        RPC_DEF(sendto)
        RPC_DEF(recvfrom)

        RPC_DEF(sendmsg)
        RPC_DEF(recvmsg)

        RPC_DEF(bind)
        RPC_DEF(connect)
        RPC_DEF(listen)
        RPC_DEF(accept)

        RPC_DEF(fd_set_new)
        RPC_DEF(fd_set_delete)
        RPC_DEF(do_fd_set)
        RPC_DEF(do_fd_zero)
        RPC_DEF(do_fd_clr)
        RPC_DEF(do_fd_isset)

        RPC_DEF(select)
        RPC_DEF(pselect)
        RPC_DEF(poll)

        RPC_DEF(getsockopt)
        RPC_DEF(setsockopt)

        RPC_DEF(ioctl) 

        RPC_DEF(getsockname)
        RPC_DEF(getpeername)

        RPC_DEF(if_nametoindex)
        RPC_DEF(if_indextoname)
        RPC_DEF(if_nameindex)
        RPC_DEF(if_freenameindex)

        RPC_DEF(signal)
        RPC_DEF(kill)

        RPC_DEF(sigset_new)
        RPC_DEF(sigset_delete)
        RPC_DEF(sigprocmask)
        RPC_DEF(sigemptyset)
        RPC_DEF(sigfillset)
        RPC_DEF(sigaddset)
        RPC_DEF(sigdelset)
        RPC_DEF(sigismember)
        RPC_DEF(sigpending)
        RPC_DEF(sigsuspend)
        RPC_DEF(sigreceived) 

        RPC_DEF(gethostbyname)
        RPC_DEF(gethostbyaddr)
        
        RPC_DEF(getaddrinfo)
        RPC_DEF(freeaddrinfo) 
        
        RPC_DEF(pipe)
        RPC_DEF(socketpair)
        RPC_DEF(fopen)
        RPC_DEF(fileno)
        RPC_DEF(getuid)
        RPC_DEF(geteuid)
        RPC_DEF(setuid)
        RPC_DEF(seteuid)

        RPC_DEF(simple_sender)
        RPC_DEF(simple_receiver)
        RPC_DEF(flooder)
        RPC_DEF(echoer)
        
        RPC_DEF(aio_read_test)
        RPC_DEF(aio_error_test)
        RPC_DEF(aio_write_test)
        RPC_DEF(aio_suspend_test)
        
        RPC_DEF(sendfile)
        RPC_DEF(socket_to_file)

        RPC_DEF(create_event)
        RPC_DEF(close_event)
        RPC_DEF(create_overlapped)
        RPC_DEF(delete_overlapped)

        RPC_DEF(wsarecv_ex)
        RPC_DEF(connect_ex)
        RPC_DEF(wsa_accept)
        RPC_DEF(accept_ex)
        RPC_DEF(disconnect_ex)
        RPC_DEF(reset_event)     
        RPC_DEF(event_select)
        RPC_DEF(enum_network_events)
        RPC_DEF(transmit_file)
        RPC_DEF(completion_callback)

        RPC_DEF(wsa_send)
        RPC_DEF(wsa_recv)
        RPC_DEF(get_overlapped_result)
        
        RPC_DEF(create_window)
        RPC_DEF(destroy_window)
        RPC_DEF(wsa_async_select)
        RPC_DEF(peek_message)

    } = 1;
} = 1;
