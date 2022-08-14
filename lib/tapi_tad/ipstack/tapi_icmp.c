/** @file
 * @brief Test API for TAD. Wrappers for IPv4/IPv6 ICMP stack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI ICMP"

#include "te_config.h"

#include "tapi_icmp4.h"
#include "tapi_icmp6.h"

te_errno
tapi_ipproto_ip_icmp_ip_eth_csap_create(const char    *ta_name,
                                int                    sid,
                                const char            *eth_dev,
                                unsigned int           receive_mode,
                                const uint8_t         *loc_eth,
                                const uint8_t         *rem_eth,
                                const struct sockaddr *loc_saddr,
                                const struct sockaddr *rem_saddr,
                                const struct sockaddr *msg_loc_saddr,
                                const struct sockaddr *msg_rem_saddr,
                                int                    af,
                                int                    ip_proto,
                                csap_handle_t         *ip_proto_csap)
{
    in_addr_t           laddr4 = 0;
    in_addr_t           raddr4 = 0;
    in_addr_t           msg_laddr4 = 0;
    in_addr_t           msg_raddr4 = 0;
    int                 msg_lport4 = -1;
    int                 msg_rport4 = -1;

    struct in6_addr     laddr6 = {.s6_addr = {0}};
    struct in6_addr     raddr6 = {.s6_addr = {0}};
    struct sockaddr_in6 msg_lsaddr6 = {.sin6_addr.s6_addr = {0},
                                       .sin6_port = -1};
    struct sockaddr_in6 msg_rsaddr6 = {.sin6_addr.s6_addr = {0},
                                       .sin6_port = -1};

    switch (af)
    {
        case AF_INET:
            if (loc_saddr != NULL)
                laddr4 = CONST_SIN(loc_saddr)->sin_addr.s_addr;

            if (msg_loc_saddr != NULL)
            {
                msg_laddr4 = CONST_SIN(msg_loc_saddr)->sin_addr.s_addr;
                msg_lport4 = CONST_SIN(msg_loc_saddr)->sin_port;
            }

            if (rem_saddr != NULL)
                raddr4 = CONST_SIN(rem_saddr)->sin_addr.s_addr;

            if (msg_rem_saddr != NULL)
            {
                msg_raddr4 = CONST_SIN(msg_rem_saddr)->sin_addr.s_addr;
                msg_rport4 = CONST_SIN(msg_rem_saddr)->sin_port;
            }

            return tapi_ipproto_ip4_icmp_ip4_eth_csap_create(
                                    ta_name, sid, eth_dev,
                                    receive_mode,
                                    loc_eth, rem_eth,
                                    laddr4, raddr4,
                                    msg_laddr4, msg_raddr4,
                                    msg_lport4, msg_rport4,
                                    ip_proto, ip_proto_csap);
        case AF_INET6:
            if (loc_saddr != NULL)
                laddr6 = CONST_SIN6(loc_saddr)->sin6_addr;

            if (msg_loc_saddr != NULL)
            {
                msg_lsaddr6.sin6_addr = CONST_SIN6(msg_loc_saddr)->sin6_addr;
                msg_lsaddr6.sin6_port = CONST_SIN6(msg_loc_saddr)->sin6_port;
            }

            if (rem_saddr != NULL)
                raddr6 = CONST_SIN6(rem_saddr)->sin6_addr;

            if (msg_rem_saddr != NULL)
            {
                msg_rsaddr6.sin6_addr = CONST_SIN6(msg_rem_saddr)->sin6_addr;
                msg_rsaddr6.sin6_port = CONST_SIN6(msg_rem_saddr)->sin6_port;
            }

            return tapi_ipproto_ip6_icmp_ip6_eth_csap_create(
                                    ta_name, sid, eth_dev,
                                    receive_mode,
                                    loc_eth, rem_eth,
                                    (uint8_t *)&laddr6, (uint8_t *)&raddr6,
                                    &msg_lsaddr6, &msg_rsaddr6,
                                    ip_proto, ip_proto_csap);
        default:
            ERROR("Invalid ip address family");
            return TE_EINVAL;
    }
}

/* See description in tapi_icmp.h */
te_errno
tapi_udp_ip_icmp_ip_eth_csap_create(const char    *ta_name,
                            int                    sid,
                            const char            *eth_dev,
                            unsigned int           receive_mode,
                            const uint8_t         *loc_eth,
                            const uint8_t         *rem_eth,
                            const struct sockaddr *loc_saddr,
                            const struct sockaddr *rem_saddr,
                            const struct sockaddr *msg_loc_saddr,
                            const struct sockaddr *msg_rem_saddr,
                            int                    af,
                            csap_handle_t         *udp_csap)
{
    return tapi_ipproto_ip_icmp_ip_eth_csap_create(ta_name, sid, eth_dev,
                                        receive_mode, loc_eth, rem_eth,
                                        loc_saddr, rem_saddr,
                                        msg_loc_saddr, msg_rem_saddr,
                                        af, IPPROTO_UDP, udp_csap);
}

/* See description in tapi_icmp.h */
te_errno
tapi_tcp_ip_icmp_ip_eth_csap_create(const char    *ta_name,
                            int                    sid,
                            const char            *eth_dev,
                            unsigned int           receive_mode,
                            const uint8_t         *loc_eth,
                            const uint8_t         *rem_eth,
                            const struct sockaddr *loc_saddr,
                            const struct sockaddr *rem_saddr,
                            const struct sockaddr *msg_loc_saddr,
                            const struct sockaddr *msg_rem_saddr,
                            int                    af,
                            csap_handle_t         *tcp_csap)
{
    return tapi_ipproto_ip_icmp_ip_eth_csap_create(ta_name, sid, eth_dev,
                                        receive_mode, loc_eth, rem_eth,
                                        loc_saddr, rem_saddr,
                                        msg_loc_saddr, msg_rem_saddr,
                                        af, IPPROTO_TCP, tcp_csap);
}
