/** @file
 * @brief TCP states API library
 *
 * @defgroup tapi_tcp_states_def TAPI definitions for testing TCP states
 * @ingroup tapi_tcp_states
 * @{
 *
 * TCP states API library - public declarations of API used to achieve
 * a requested TCP socket state following specified sequence of TCP
 * states.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TAPI_TCP_STATES_H__
#define __TAPI_TCP_STATES_H__

#include "te_config.h"

#include "te_defs.h"
#include "rcf_rpc.h"
#include <netinet/tcp.h>
#include <net/if.h>
#include "tapi_tcp.h"

/**
 * Value returned when function stopped transition
 * in specified state, before reaching end state in sequence.
 */
#define TSA_ESTOP -2

/**
 * Get tsa_tst_type parameter determining whether we use socket or CSAP
 * emulation of TCP socket on the TESTER.
 */
#define TEST_GET_TSA_TST_TYPE_PARAM(var_) \
    TEST_GET_ENUM_PARAM(var_, {"TSA_TST_SOCKET", TSA_TST_SOCKET}, \
                        {"TSA_TST_CSAP", TSA_TST_CSAP},           \
                        {"TSA_TST_GW_CSAP", TSA_TST_GW_CSAP})

/** What should be used on tester side: */
typedef enum tsa_tst_type {
    TSA_TST_SOCKET = 1, /** socket */
    TSA_TST_CSAP,       /** CSAP */
    TSA_TST_GW_CSAP     /** CSAP with gateway */
} tsa_tst_type;

/** Flags used by various functions in library  */
typedef enum tsa_flags {
    TSA_TST_USE_REUSEADDR = 0x1,      /**< Set SO_REUSEADDR option on
                                           TESTER socket */
    TSA_NO_CONNECTIVITY_CHANGE = 0x2, /**< Do not use breaking/repairing
                                           connection to control
                                           transmission of TCP packets */
    TSA_ACT_TIMEOUT = 0x4,            /**< Use time waiting to move from
                                           one TCP state to another if
                                           possible */
    TSA_ACT_RST = 0x8,                /**< Use sending RST to move from
                                           one TCP state to another if
                                           possible */
    TSA_MOVE_IGNORE_ERR = 0x10,       /**< Ignore errors of achieving not
                                           expected TCP state when
                                           performing TCP states transition
                                           (it is useful when we don't use
                                           beaking/repairing connection in
                                           TSA_TST_SOCKET mode and do not
                                           see states like TCP_LAST_ACK) */
    TSA_MOVE_IGNORE_START_ERR = 0x20, /**< It has the same effect as
                                           TSA_MOVE_IGNORE_ERR but
                                           only in tsa_do_moves_str()
                                           when it's required to
                                           move to some starting
                                           state before performing
                                           transition specified. */
    TSA_ESTABLISH_PASSIVE = 0x40,     /**< Use passive opening of TCP
                                           connection to obtain
                                           TCP_ESTABLISHED state */
    TSA_NO_CFG_WAIT_CHANGES = 0x80,   /**< Do not Wait for changes
                                           in connectivity */
} tsa_flags;

/** Environment configuration for current working session with TSA */
typedef struct tsa_config {

    /**< RPC servers */
    rcf_rpc_server *pco_iut;            /**< RPC server on the IUT side */
    rcf_rpc_server *pco_tst;            /**< RPC server on the TST side */
    rcf_rpc_server *pco_gw;             /**< RPC server on a gateway */

    /**< Addresses */
    const struct sockaddr  *iut_addr;        /**< Network address on
                                                  the IUT side */
    const struct sockaddr  *tst_addr;        /**< Network address on
                                                  the TST side */
    const struct sockaddr  *gw_iut_addr;     /**< Gateway IUT address */
    const struct sockaddr  *gw_tst_addr;     /**< Gateway TST address */
    const void             *alien_link_addr; /**< Invalid ethernet
                                                  address */
    const void             *iut_link_addr;   /**< IUT ethernet
                                                  address */
    const void             *gw_tst_link_addr; /**< Gateway tester interface
                                                   ethernet address */

    /**< Interfaces */
    const struct if_nameindex  *iut_if; /**< Network interface on
                                             the IUT side */
    const struct if_nameindex  *tst_if; /**< Network interface on
                                             the TST side */
    const struct if_nameindex  *gw_iut_if; /**< Network interface on
                                                the gateway IUT side */
    const struct if_nameindex  *gw_tst_if; /**< Network interface on
                                                the gateway TST side */

    /**< Socket options */
    uint32_t            flags;          /**< Flags described in tsa_flags
                                             enum */

    te_bool             gw_preconf;     /**< If @c TRUE, gateway is
                                             preconfigured, this library
                                             should not touch it. */
} tsa_config;

/** Structure describing variables used in @c TSA_TST_SOCKET mode */
typedef struct tsa_state_sock {
    /** Sockets */
    int                 tst_s;          /**< Socket on the TST side */
    int                 tst_s_aux;      /**< Socket in listening
                                             state stored after
                                             accept on @p pco_tst */

    /**< Routing */
    te_bool     route_dst_added;        /**< @c TRUE if route from
                                             @p pco_iut to @p
                                             pco_tst was added */
    te_bool     route_src_added;        /**< @c TRUE if route from @p
                                             pco_tst to @p pco_iut
                                             was added */
    te_bool     ipv4_fw_enabled;        /**< @c TRUE if IPv4 forwarding is
                                             enabled */
    te_bool     ipv4_fw;                /**< Was IPv4 forwarding configured
                                             previously? */
    te_bool     ipv6_fw_enabled;        /**< @c TRUE if IPv6 forwarding is
                                             enabled */
    te_bool     ipv6_fw;                /**< Was IPv6 forwarding configured
                                             previously? */

    /**< Sending RST via CSAP */
    tapi_tcp_reset_hack_t rst_hack_c;   /**< TCP reset hack context */
    int                   sid;          /**< RCF session id */
} tsa_state_sock;

/** Structure describing variables used in TSA_TST_CSAP mode */
typedef struct tsa_state_csap {
    /** Sockets */
    tapi_tcp_handler_t  csap_tst_s;     /**< Handler of CSAP implementation
                                             of TCP connection */
} tsa_state_csap;

struct tsa_session;

/**
 * Type for handler to be called when moving
 * from one TCP state to another one.
 */
typedef te_errno (*tsa_handler)(struct tsa_session *ss);

/**
 * Structure containing handlers to be used for
 * moving between TCP states.
 */
typedef struct tsa_handlers {
    tsa_handler iut_syn;        /**< Send SYN from IUT. */
    tsa_handler tst_syn;        /**< Send SYN from Tester. */
    tsa_handler iut_syn_ack;    /**< Send SYN-ACK from IUT. */
    tsa_handler tst_syn_ack;    /**< Send SYN-ACK from Tester. */
    tsa_handler iut_ack;        /**< Send ACK from IUT. */
    tsa_handler tst_ack;        /**< Send ACK from Tester. */
    tsa_handler iut_fin;        /**< Send FIN from IUT. */
    tsa_handler tst_fin;        /**< Send FIN from Tester. */
    tsa_handler tst_fin_ack;    /**< Send FIN-ACK from Tester. */
    tsa_handler tst_rst;        /**< Send RST from Tester. */
    tsa_handler iut_listen;     /**< Move IUT socket to listening
                                     state. */
} tsa_handlers;

/**< TSA session state structure */
typedef struct tsa_state {
    tsa_tst_type    tst_type;           /**< What should be used on the
                                             tester - socket or CSAP? */
    rpc_tcp_state   state_cur;          /**< Current TCP state of IUT
                                             socket */

    /**< State variables specific to different working modes */
    union {
        tsa_state_sock  sock;           /**< State variables for
                                             @c TSA_TST_SOCKET mode */
        tsa_state_csap  csap;           /**< State variables for
                                             @c TSA_TST_CSAP mode */
    };

    tsa_handlers        move_handlers;  /**< Functions used to move
                                             between TCP states. */

    int                 iut_s;          /**< Socket on the IUT side */
    int                 iut_s_aux;      /**< Socket in listening
                                             state stored after
                                             accept on @p pco_iut */

    /**< Characterics of path in the space of TCP states */
    rpc_tcp_state       state_from;     /**< TCP state before change */
    rpc_tcp_state       state_to;       /**< TCP state after change */
    const char         *rem_path;       /**< Part of TCP state sequence
                                             remaining when error occured */
    te_bool             timeout_used;   /**< TRUE if the last TCP state
                                             change was achieved through
                                             timeout */
    int                 elapsed_time;   /**< Time waited for TCP state
                                             change lastly in milliseconds
                                             */

    /** Functions called with RCF_RPC_CALL */
    te_bool         iut_wait_connect;   /**< @c TRUE if @b rpc_connect with
                                             @c RCF_RPC_CALL was called on
                                             @p pco_iut */
    te_bool         tst_wait_connect;   /**< @c TRUE if @b rpc_connect with
                                             @c RCF_RPC_CALL was called on
                                             @p pco_tst */
    te_bool         close_listener;     /**< Close listener socket just
                                             after accepting connection */

    te_bool     iut_alien_arp_added;    /**< @c TRUE if ARP for alien MAC
                                             was added to break connection
                                             from @p pco_tst to
                                             @p pco_iut */
    te_bool     tst_alien_arp_added;    /**< @c TRUE if ARP for alien MAC
                                             was added to break connection
                                             from @p pco_iut to
                                             @p pco_tst */
} tsa_state;

/**< TSA session variables */
typedef struct tsa_session {
    tsa_config config;   /**< Configuration parameters */
    tsa_state  state;    /**< State parameters */
} tsa_session;

/**
 * Initializer for tsa_session structure.
 */
#define TSA_SESSION_INITIALIZER { .config = { .pco_iut = NULL } }

/** Actions must be performed to move from one TCP state to another */
typedef struct tcp_move_action {
    rpc_tcp_state state_from;        /** Current TCP state */
    rpc_tcp_state state_to;          /** Next TCP state */
    tsa_handler   tst_act;           /** Action on the TST side */
    tsa_handler   iut_act;           /** Action on the IUT side */
} tcp_move_action;

/**
 * Context to count packets in processing in the packet handler function.
 */
typedef struct tsa_packets_counter {
    int count;          /**< Total packets counter */
    int ack;            /**< ACK packets number */
    int syn;            /**< SYN packets number */
    int syn_ack;        /**< SYN-ACK packets number */
    int fin_ack;        /**< FIN-ACK packets number */
    int push_ack;       /**< PSH-ACK packets number */
    int push_fin_ack;   /**< PSH-FIN-ACK packets number */
    int rst_ack;        /**< RST-ACK packets number */
    int rst;            /**< RST packets number */
    int other;          /**< Other packets number */
} tsa_packets_counter;

/**
 * Accessor for IUT socket.
 *
 * @param ss    Pointer to TSA session structure of current working session
 *
 * @return
 *      IUT socket ID
 */
static inline int
tsa_iut_sock(tsa_session *ss)
{
    return ss->state.iut_s;
}

/**
 * Accessor for TESTER TCP socket (or its CSAP emulator handler if
 * TSA_TST_CSAP mode is used).
 *
 * @param ss    Pointer to TSA session structure of current working session
 *
 * @return
 *      Socket (CSAP handler) or -1
 */
static inline int
tsa_tst_sock(tsa_session *ss)
{
    return (ss->state.tst_type == TSA_TST_CSAP) ?
                                    ss->state.csap.csap_tst_s :
                                    ss->state.sock.tst_s;
}

/**
 * Accessor for current TCP state of IUT socket.
 *
 * @param ss    Pointer to TSA session structure of current working session
 *
 * @return
 *      Current TCP state of IUT socket
 */
static inline rpc_tcp_state
tsa_state_cur(tsa_session *ss)
{
    return ss->state.state_cur;
}

/**
 * Setter for current TCP state of IUT socket.
 *
 * @param ss    Pointer to TSA session structure of current working session
 */
static inline void
tsa_state_cur_set(tsa_session *ss, rpc_tcp_state state)
{
    ss->state.state_cur = state;
}

/**
 * Accessor for state_to field of tsa_state structure.
 *
 * @param ss    Pointer to TSA session structure of current working session
 *
 * @return
 *      Value of state_to field of tsa_state stucture
 */
static inline rpc_tcp_state
tsa_state_to(tsa_session *ss)
{
    return ss->state.state_to;
}

/**
 * Accessor for state_from field of tsa_state structure.
 *
 * @param ss    Pointer to TSA session structure of current working session
 *
 * @return
 *      Value of state_from field of tsa_state structure.
 */
static inline rpc_tcp_state
tsa_state_from(tsa_session *ss)
{
    return ss->state.state_from;
}

/**
 * Accessor for rem_path field of tsa_state structure.
 *
 * @param ss    Pointer to TSA session structure of current working session
 *
 * @return
 *      Value of rem_path field of tsa_state structure
 */
static inline const char *
tsa_rem_path(tsa_session *ss)
{
    return ss->state.rem_path;
}

/**
 * Accessor for timeout_used field of tsa_state structure.
 *
 * @param ss    Pointer to TSA session structure of current working session
 *
 * @return
 *      Value of timeout_used field of tsa_state structure
 */
static inline te_bool
tsa_timeout_used(tsa_session *ss)
{
    return ss->state.timeout_used;
}

/**
 * Accessor for elapsed_time field of tsa_state structure.
 *
 * @param ss    Pointer to TSA session structure of current working session
 *
 * @return
 *      Value of elapsed_time field of tsa_state structure
 */
static inline int
tsa_elapsed_time(tsa_session *ss)
{
    return ss->state.elapsed_time;
}

/**
 * Initialize tsa_session structure.
 *
 * @param ss                    Pointer to TSA session structure
 *                              to be initialized
 * @param tst_type              What should be used on the tester
 *                              (socket, CSAP)
 *
 * @return Status code.
 */
extern te_errno tsa_state_init(tsa_session *ss, tsa_tst_type tst_type);


/**
 * Set tsa_session structure fields related to RPC server on the IUT side.
 *
 * @param ss                    Pointer to TSA session structure
 * @param pco_iut               RPC server on the IUT side
 * @param iut_if                Network interface on the IUT side
 * @param iut_addr              Network address on the IUT side
 *
 * @return Status code.
 */
extern te_errno tsa_iut_set(tsa_session *ss, rcf_rpc_server *pco_iut,
                            const struct if_nameindex *iut_if,
                            const struct sockaddr *iut_addr);

/**
 * Set tsa_session structure fields related to RPC server on the TST side.
 * Add entries in ARP and routing tables for fake addresses if we
 * use CSAP emulation on the TESTER (CFG_WAIT_CHANGES should be used
 * after call of this function in such a case).
 *
 * @param ss                    Pointer to TSA session structure
 * @param pco_tst               RPC server on the TST side
 * @param tst_if                Network interface on the TST side
 * @param tst_addr              Network address on the TST side
 * @param fake_link_addr        Fake ethernet address to be
 *                              used by CSAP (if we use CSAP
 *                              implementation of TCP socket)
 *
 * @return Status code.
 */
extern te_errno tsa_tst_set(tsa_session *ss, rcf_rpc_server *pco_tst,
                            const struct if_nameindex *tst_if,
                            const struct sockaddr *tst_addr,
                            const void *fake_link_addr);

/**
 * Set tsa_session structure fields related to gateway;
 * configure routes via gateway.
 * CFG_WAIT_CHANGES should be used after calling this function.
 *
 * @param ss                    Pointer to TSA session structure
 * @param pco_gw                RPC server on a gateway
 * @param gw_iut_addr           Gateway IUT address
 * @param gw_tst_addr           Gateway TST address
 * @param gw_iut_if             Gateway IUT interface.
 * @param gw_tst_if             Gateway TST interface.
 * @param alien_link_addr       Invalid ethernet address
 *
 * @return Status code.
 */
extern te_errno tsa_gw_set(tsa_session *ss, rcf_rpc_server *pco_gw,
                           const struct sockaddr *gw_iut_addr,
                           const struct sockaddr *gw_tst_addr,
                           const struct if_nameindex *gw_iut_if,
                           const struct if_nameindex *gw_tst_if,
                           const void *alien_link_addr);

/**
 * Specify whether gateway is already configured. It makes sense to call
 * this function only before @b tsa_gw_set().
 *
 * @param ss                    Pointer to TSA session structure
 * @param preconfigured         If @c TRUE, gateway is already configured,
 *                              @b tsa_gw_set() should not try to configure
 *                              routes or forwarding.
 */
extern void tsa_gw_preconf(tsa_session *ss, te_bool preconfigured);

/**
 * Break network connection from @p pco_tst to @p pco_iut by
 * adding ARP table entry associating IUT IP address with
 * alien MAC address.
 * CFG_WAIT_CHANGES should be used after calling this function.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
extern te_errno tsa_break_tst_iut_conn(tsa_session *ss);

/**
 * Break network connection from @p pco_iut to @p pco_tst by
 * adding ARP table entry associating Tester IP address with
 * alien MAC address.
 * CFG_WAIT_CHANGES should be used after calling this function.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
extern te_errno tsa_break_iut_tst_conn(tsa_session *ss);

/**
 * Repair network connection from @p pco_tst to @p pco_iut
 * by removing ARP table entry for alien MAC address.
 * CFG_WAIT_CHANGES should be used after calling this function.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
extern te_errno tsa_repair_tst_iut_conn(tsa_session *ss);

/**
 * Repair network connection from @p pco_iut to @p pco_tst
 * by removing ARP table entry for alien MAC address.
 * CFG_WAIT_CHANGES should be used after calling this function.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
extern te_errno tsa_repair_iut_tst_conn(tsa_session *ss);

/**
 * Move from one TCP state to another one.
 *
 * @param ss            Pointer to TSA session structure
 * @param state_from    Initial TCP state
 * @param state_to      TCP state to move to
 * @param flags         Flags defined in tsa_flags enum
 *
 * @return Status code.
 */
extern te_errno tsa_do_tcp_move(tsa_session *ss, rpc_tcp_state state_from,
                                rpc_tcp_state state_to, uint32_t flags);

/**
 * Perform a given sequence of TCP state transitions.
 *
 * @param ss            Pointer to TSA session structure
 * @param stop_state    TCP state where to stop (it's usefull when we want
 *                      to perform some actions in some interjacent state
 *                      and then resume transition)
 * @param flags         Flags defined in tsa_flags enum
 * @param ...           List of TCP states in transition sequence
 *                      (it should be terminated by RPC_TCP_UNKNOWN)
 *
 * @return Non-negative status code or TSA_ESTOP if function stopped in
 *         specificed state.
 */
extern te_errno tsa_do_moves(tsa_session *ss, rpc_tcp_state stop_state,
                             uint32_t flags, ...);

/**
 * Perform a given sequence of TCP state transitions (like @b tsa_do_moves
 * but the sequence is given as a string).
 *
 * @param ss            Pointer to TSA session structure
 * @param init_state    TCP state to be considered as initial in search
 *                      for the first transition actions (it is useful
 *                      when we work with TCP_LISTEN -> TCP_SYN_RECV ->
 *                      TCP_ESTABLISHED because TCP_SYN_RECV is not
 *                      observable in this case)
 * @param stop_state    TCP state where to stop (it's usefull when we want
 *                      to perform some actions in some interjacent state
 *                      and then resume transition)
 * @param flags         Flags defined in tsa_flags enum
 * @param s             String containing the sequence of TCP states.
 *                      For example, "TCP_CLOSE -> TCP_SYN_SENT -> timeout
 *                      -> TCP_CLOSE". TCP states can be separated by
 *                      spaces, '-', '>', ';', ':', ',' (including
 *                      any combinations of these symbols).
 *
 * @return Non-negative status code or TSA_ESTOP if function stopped in
 *         specificed state.
 */
extern te_errno tsa_do_moves_str(tsa_session *ss,
                                 rpc_tcp_state init_state,
                                 rpc_tcp_state stop_state,
                                 uint32_t flags, const char *s);

/**
 * Create IUT socket and TESTER socket or CSAP emulation (according to
 * working mode determined by @p tst_type during initialization),
 * set socket options if needed, bind() socket(s). Cleanup should
 * be done by @b tsa_destroy_session() call.
 *
 * @param ss    Pointer to TSA session structure
 * @param flags Flags (described above in tsa_flags
 *              enum)
 *
 * @return Status code.
 */
extern te_errno tsa_create_session(tsa_session *ss, uint32_t flags);

/**
 * Perform cleanup on TSA library context.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
extern te_errno tsa_destroy_session(tsa_session *ss);

/**
 * Move from TCP_CLOSE state to another state using correspoding
 * pre-defined path. Pre-defined path is just sequence of
 * TCP states from TCP_CLOSE to a given state which is defined
 * in tcp_state.c and is used by default. There is only two
 * pre-defined paths for any TCP state - path using active opening
 * of TCP connection and path using passive one (for some paths these
 * make no sense and two paths are defined for simplicity of related
 * functions).
 *
 * @param ss            Pointer to TSA session structure
 * @param state         TCP state to be achieved
 * @param stop_state    TCP state where to stop (it's usefull when we want
 *                      to perform some actions in some interjacent state
 *                      and then resume transition)
 * @param flags         Flags defined in tsa_flags enum
 *
 * @return Status code.
 */
extern te_errno tsa_set_start_tcp_state(
                                  tsa_session *ss, rpc_tcp_state state,
                                  rpc_tcp_state stop_state,
                                  uint32_t flags);

/**
 * Update current TCP state of IUT socket.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
extern te_errno tsa_update_cur_state(tsa_session *ss);

/**
 * Send RST from TESTER to IUT.
 *
 * @param ss    Pointer to TSA session structure
 *
 * @return Status code.
 */
extern te_errno tsa_tst_send_rst(tsa_session *ss);

/**
 * Packet handler callback function.
 *
 * @param packet      Packet handler
 * @param user_param  Packet context as opaque argument
 */
extern void tsa_packet_handler(const char *packet, void *user_param);

/**
 * Print captured packets stats.
 *
 * @param ctx   Packets stats struct
 */
extern void tsa_print_packet_stats(tsa_packets_counter *ctx);

/**@} <!-- END tapi_tcp_states_def --> */

#endif /* !__TAPI_TCP_STATES_H__ */
