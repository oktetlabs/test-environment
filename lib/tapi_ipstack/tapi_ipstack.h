
#ifndef TAPI_IPSTACK_H__
#define TAPI_IPSTACK_H__

#include <assert.h>
#include <netinet/in.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"


/** Structure of UDP/IPv4 datagram */
typedef struct udp4_datagram {
    struct in_addr      src_addr;
    struct in_addr      dst_addr;
    uint16_t            src_port;
    uint16_t            dst_port;
    uint16_t            payload_len;
    uint8_t            *payload;
} udp4_datagram;

/** 
 * Callback function for the tapi_snmp_walk() routine, it is called
 * for each variable in a walk subtree.
 *
 * @param pkt           received UDP datagram. After return from this callback
 *                      memory block under this datagram will be freed. 
 * @param userdata      Parameter, provided by the caller of tapi_snmp_walk().
 */
typedef void (*udp4_callback)(const udp4_datagram *pkt, void *userdata);



/**
 * Convert UDP.IPv4 datagram ASN.1 value to plain C structure.
 *
 * @param pkt           ASN.1 value of type Raw-Packet.
 * @param udp_dgram     converted structure (OUT).
 *
 * @return zero on success or error code.
 */
extern int ndn_udp4_dgram_to_plain(asn_value_p pkt, 
                        struct udp4_datagram **udp_dgram);



/**
 * Creates usual 'data.udp.ip4' CSAP on specified Test Agent and got its handle.
 * 
 * @param ta            Test Agent name.
 * @param sid           RCF SID
 * @param loc_addr_str  character string with local IP address (may be NULL).
 * @param rem_addr_str  character string with remote IP address (may be NULL).
 * @param loc_port      local UDP port (may be zero).
 * @param rem_port      remote UDP port (may be zero). 
 * @param csap_id       identifier of an SNMP CSAP (OUT).
 * 
 * @return zero on success or error code.
 */
extern int tapi_udp4_csap_create(const char *ta_name, int sid,
             const char *loc_addr_str, const char *rem_addr_str,
             uint16_t loc_port, uint16_t rem_port, csap_handle_t *udp_csap);


/**
 * Send UDP datagram via 'data.udp.ip4' CSAP.
 * 
 * @param ta            Test Agent name.
 * @param sid           RCF SID
 * @param csap          identifier of an SNMP CSAP (OUT).
 * @param udp_dgram     UDP datagram to be sent.
 * 
 * @return zero on success or error code.
 */
extern int tapi_udp4_dgram_send(const char *ta_name, int sid,
            csap_handle_t csap, const udp4_datagram *udp_dgram);



/**
 * Start receiving of UDP datagrams via 'data.udp.ip4' CSAP, non-block method.
 * 
 * @param ta            Test Agent name.
 * @param sid           RCF SID
 * @param csap          identifier of an SNMP CSAP (OUT).
 * @param udp_dgram     UDP datagram with pattern for filter. 
 * @param callback      callback function, which will be call for each 
 *                      received packet
 * @param userdata      opaque data to be passed into the callback function.
 * 
 * @return zero on success or error code.
 */
extern int tapi_udp4_dgram_start_recv(const char *ta_name,  int sid,
            csap_handle_t csap, const  udp4_datagram *udp_dgram, 
            udp4_callback callback, void *user_data);

/**
 * Receive some number of UDP datagrams via 'data.udp.ip4' CSAP, block method.
 * 
 * @param ta            Test Agent name.
 * @param sid           RCF SID
 * @param csap          identifier of an SNMP CSAP (OUT).
 * @param number        number of dgrams to be received.
 * @param timeout       total timeout for receive, in seconds.
 * @param udp_dgram     UDP datagram with pattern for filter. 
 * @param callback      callback function, which will be call for each 
 *                      received packet
 * @param userdata      opaque data to be passed into the callback function.
 * 
 * @return zero on success or error code.
 */
extern int tapi_udp4_dgram_recv(const char *ta_name,  int sid,
            csap_handle_t csap, int number, int timeout,
            const  udp4_datagram *udp_dgram, 
            udp4_callback callback, void *user_data);

/**
 * Send UDP datagram via 'data.udp.ip4' CSAP and receive response to it
 * (that is first received UDP datagram with reverse to the sent 
 * source/destination addresses and ports).
 * 
 * @param ta            Test Agent name.
 * @param sid           RCF SID
 * @param csap          identifier of an SNMP CSAP (OUT).
 * @param timeout       timeout for receive, in seconds.
 * @param dgram_sent    UDP datagram to be sent.
 * @param dgram_recv    location for received UDP datagram; memory for payload 
 *                      will be allocated by TAPI and should be freed by user.
 * 
 * @return zero on success or error code.
 */
extern int tapi_udp4_dgram_send_recv( const char *ta_name, int sid, 
            csap_handle_t csap, unsigned int timeout,
            const udp4_datagram *dgram_send, udp4_datagram *dgram_recv);



#endif
