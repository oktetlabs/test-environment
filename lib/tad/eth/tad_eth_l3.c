/** @file
 * @brief Test Environment: 
 *
 * User methods for fill in eth frame payload. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#define TE_LGR_USER "TAD ETH L3"

#include <string.h>
#include "tad_eth_impl.h"
#include "te_defs.h"

#include "logger_api.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Following variables can be set by means of rcf methods */
char             mi_src_addr[20];
/* for correct rcf_pch processing */
char             *mi_src_addr_human = mi_src_addr;

char             mi_dst_addr[20];
/* for correct rcf_pch processing */
char             *mi_dst_addr_human = mi_dst_addr;

unsigned short   mi_src_port;
unsigned short   mi_dst_port;
int              mi_payload_length;

unsigned int userdata_to_udp(unsigned char *raw_pkt);

/**
 * Type for reference to user function for generating MAC Control frame data 
 * to be sent.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl          ASN value with template. 
 *                      function should replace that field (which it should
 *                      generate) with #plain (in headers) or #bytes (in payload)
 *                      choice (IN/OUT)
 *
 * @return zero on success or error code.
 */
int 
eth_mac_ctrl_payload(int csap_id, int layer, asn_value *tmpl)
{
    static unsigned char  buffer[20000];
    unsigned int          length = 4;
    int rc = 0;

    rc = asn_free_subvalue (tmpl, "payload");
    if (rc)
        return rc;
    
    buffer[0] = 0x00;
    buffer[1] = 0x01;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    
    rc = asn_write_value_field (tmpl, buffer, length, "payload.#bytes");
    UNUSED(csap_id);
    UNUSED(layer);
    return rc;  
}

/**
 * Type for reference to user function for generating MAC Control 
 * frame data (with unsupported OpCode) to be sent.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl          ASN value with template. 
 *                      function should replace that field (which it should
 *                      generate) with #plain (in headers) or #bytes (in payload)
 *                      choice (IN/OUT)
 *
 * @return zero on success or error code.
 */
int
eth_mac_ctrl_unsupp_payload(int csap_id, int layer, asn_value *tmpl)
{
    static unsigned char  buffer[20000];
    unsigned int          length = 4;
    int rc = 0;

    rc = asn_free_subvalue (tmpl, "payload");
    if (rc)
        return rc;
    
    buffer[0] = 0xFF;
    buffer[1] = 0xFF;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    
    rc = asn_write_value_field (tmpl, buffer, length, "payload.#bytes");
    UNUSED(csap_id);
    UNUSED(layer);
    return rc;  
}



/**
 * Type for reference to user function for generating data to be sent.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl          ASN value with template. 
 *                      function should replace that field (which it should
 *                      generate) with #plain (in headers) or #bytes (in payload)
 *                      choice (IN/OUT)
 *
 * @return zero on success or error code.
 */
int 
eth_udp_payload(int csap_id, int layer, asn_value *tmpl)
{
    static unsigned char  buffer[20000];
    unsigned int          length;
    int rc = 0;

    rc = asn_free_subvalue (tmpl, "payload");
    if (rc)
        return rc;

    length = userdata_to_udp(buffer);

    rc = asn_write_value_field (tmpl, buffer, length, "payload.#bytes");
    UNUSED(csap_id);
    UNUSED(layer);
    return rc;
}


/**
 * Calculates the sum of array as a 16 bit integers
 * 
 * @param arr    data array pointer
 * @param length data array length
 * @param sum    returned sum
 */
#define SUM_16(_arr, _tmp_len, _sum) \
    do {                                                     \
           unsigned char *_data = (unsigned char *)(_arr);   \
           int _length = _tmp_len;                           \
           for (; (_length) > 1; _data += 2, (_length) -= 2) \
               (_sum) += (_data[0] << 8) + _data[1];         \
           if ((_length) > 0)                                \
               (_sum) += (_data[0] << 8);                    \
       } while (0)

/** UDP header fields access offsets */
#define UDPHDR_SRCPORT   offsetof(t_udp_header, src_port)
#define UDPHDR_DSTPORT   offsetof(t_udp_header, dst_port)
#define UDPHDR_LENGTH    offsetof(t_udp_header, length)
#define UDPHDR_CHECKSUM  offsetof(t_udp_header, checksum)

/* UDP header */
typedef struct t_udp_header
{
    unsigned char  src_port[2];  /* source port */
    unsigned char  dst_port[2];  /* destination port */
    unsigned char  length[2];    /* message length */
    unsigned char  checksum[2];  /* checksum   */
}t_udp_header;

#define UDPHEADER_LEN sizeof(struct t_udp_header)

/* UDP pseudo header for checksum calculation */
typedef struct t_udp_pseudo
{
    unsigned char src_ip[4];  /* source IP address */
    unsigned char dst_ip[4];  /* destination IP address */
    unsigned char null;       /* all ways 0 */
    unsigned char protocol;   /* (17) */
    unsigned char length[2];  /* UDP packet length as in UDP header */
}t_udp_pseudo;

#define DEFAULT_TTL 64
/** IP v4 header */
typedef struct ip_header
{
    unsigned char version_length;     /* IP version & header length */
    unsigned char service_type;       /* throughput, reliability ... */
    unsigned char total[2];           /* total packet length (in octets) */
    unsigned char identifier[2];      /* datagram id */
    unsigned char fragment_offset[2]; /* controls packet fragmentation */
    unsigned char time_to_live;       /* to avoid undeliverable packets */
    unsigned char protocol;           /* IP protocol incapsulated */
    unsigned char checksum[2];        /* header checksum */
    unsigned char src_addr[4];        /* source IP address */
    unsigned char dst_addr[4];        /* destination IP address */
}t_ip_header;

#define IPHEADER_LEN sizeof(struct ip_header)

/**
 *  Calculate checksum for data array.
 *
 *  @param  p_dpu        data array pointer
 *  @param  length       data array length
 *  
 *  @return checksum for data array
 */
static unsigned int 
ip_checksum (unsigned char *p_pdu, unsigned int length)
{
    unsigned long sum = 0;
    
    SUM_16(p_pdu, length, sum);
    
    sum = (sum >> 16) + (sum & 0xffff);
    sum = (sum >> 16) + (sum & 0xffff);
    return (unsigned int)~sum;
}

/**
 *  Adds IP header to the data array.
 *
 *  @param  p_pdu        data array with memory allocated for both 
 *                       IP and UDP headers
 *  @param  udata_len    data array length without taking into account the both
 *                       IP and UDP headers
 *  @param  src_addr     source IP address (network order)
 *  @param  dst_addr     destination IP address (network order)
 *  @param  protocol     protocol to be stored in the packet
 */
static void
add_ip_header(unsigned char *p_pdu, unsigned int udata_len,
              unsigned long src_addr, unsigned long dst_addr,
              unsigned char protocol)
{
    t_ip_header   *p_ip_hdr;
    unsigned int  hdr_len = IPHEADER_LEN;
    unsigned int  total;
    unsigned int  checksum;
   
    p_ip_hdr = (t_ip_header *)p_pdu; 
    
    /* IP header length in 32 bit words */
    p_ip_hdr->version_length = (4 << 4) | (hdr_len >> 2);
    
    total = udata_len + IPHEADER_LEN + UDPHEADER_LEN;
    
    p_ip_hdr->total[0] = (unsigned char)(total >> 8);
    p_ip_hdr->total[1] = (unsigned char)(total & 0xff);
    
    p_ip_hdr->time_to_live = DEFAULT_TTL;
    p_ip_hdr->protocol = protocol;
    
    *((unsigned long *)p_ip_hdr->src_addr) = src_addr;
    *((unsigned long *)p_ip_hdr->dst_addr) = dst_addr;

    checksum = ip_checksum((unsigned char *)p_ip_hdr, hdr_len);
    
    p_ip_hdr->checksum[0] = (unsigned char)(checksum >> 8);
    p_ip_hdr->checksum[1] = (unsigned char)(checksum & 0xff);
}

/**
 * Adds UDP header to the user data
 * 
 * @param  p_pdu      data array with memory allocated for IP and UDP headers
 * @param  udata_len  data array length without taking into account the both
 *                    IP and UDP headers
 * @param  src_port   source port (host order)
 * @param  dst_port   destination port (host order)
 * @param  src_addr   source IP address (network order)
 * @param  dst_addr   destination IP address (network order)
 * 
 * @return 0 on success
 */
static void
add_udp_header(unsigned char *p_pdu, unsigned int udata_len,
               unsigned short src_port, unsigned short dst_port, 
               unsigned long src_addr, unsigned long dst_addr)
{
        t_udp_header   *udp_hdr;
        t_udp_pseudo    udp_pseudo;
        unsigned char  *udata;
        unsigned short  length;
        unsigned long   sum_pseudo = 0, sum_data = 0, sum_hdr = 0;
        unsigned short  checksum = 0;

        
        memset(&udp_pseudo, 0, sizeof(t_udp_pseudo));
            
        length = udata_len + UDPHEADER_LEN;

        /* Pseudo header sum calculation */
        udp_pseudo.length[0] = (unsigned char)(length >> 8);
        udp_pseudo.length[1] = (unsigned char)(length &0xff);
        *((unsigned long *)udp_pseudo.dst_ip) = dst_addr;
        *((unsigned long *)udp_pseudo.src_ip) = src_addr;
        udp_pseudo.protocol = 17;
        SUM_16(&udp_pseudo, sizeof(t_udp_pseudo), sum_pseudo);
        
        /* User data sum calculation */
        udata = p_pdu + IPHEADER_LEN + UDPHEADER_LEN;
        SUM_16(udata, udata_len, sum_data);
        
        udp_hdr = (t_udp_header *)(p_pdu + IPHEADER_LEN);
        udp_hdr->src_port[0] = (unsigned char)(src_port >> 8);
        udp_hdr->src_port[1] = (unsigned char)(src_port &  0xff);
        udp_hdr->dst_port[0] = (unsigned char)(dst_port >> 8);
        udp_hdr->dst_port[1] = (unsigned char)(dst_port &  0xff);
        udp_hdr->length[0] =   (unsigned char)(length   >> 8);
        udp_hdr->length[1] =   (unsigned char)(length   &  0xff);
        
        /* UDP header sum calculation */
        SUM_16(udp_hdr, UDPHEADER_LEN, sum_hdr);

        /* Calculate udp packet checksum */
        sum_data += sum_pseudo + sum_hdr;
        sum_data = (sum_data >> 16) + (sum_data & 0xffff);
        sum_data = (sum_data >> 16) + (sum_data & 0xffff);
        checksum = (unsigned short)~sum_data;
        udp_hdr->checksum[0] = (unsigned char)(checksum >> 8);
        udp_hdr->checksum[1] = (unsigned char)checksum;
}


unsigned int
userdata_to_udp( unsigned char *raw_pkt)
{
    unsigned char *udata;
    unsigned int   udata_len;
    unsigned int   src_addr, dst_addr;
    
    unsigned int  user_data, frame_counter = 1;
    
    udata_len = mi_payload_length - IPHEADER_LEN - UDPHEADER_LEN;
    
    
    /* Allocate memory for IP packet incapsulating UDP datagram */
    memset(raw_pkt, 0, mi_payload_length);

    udata = raw_pkt + IPHEADER_LEN + UDPHEADER_LEN;
    
    user_data = htonl(frame_counter);
    memcpy(udata, (unsigned char *)&user_data, sizeof(int));
    src_addr = inet_addr(mi_src_addr_human);
    dst_addr = inet_addr(mi_dst_addr_human);
    
#if 0   
    src_addr = inet_addr("192.168.200.10");
    dst_addr = inet_addr("192.168.220.10");
#endif   
    
    add_udp_header(raw_pkt, udata_len, mi_src_port, mi_dst_port, 
                   src_addr, dst_addr);
    add_ip_header(raw_pkt, udata_len, src_addr, dst_addr, 17);

    return mi_payload_length;
}


/**
 * Convert standard string presentatino of Ethernet MAC address
 * to binary. 
 *
 * @param mac_str       string with address
 * @param mac           location (6 bytes) for binary address (OUT)
 *
 * @return status code
 */
static int
mac_str2addr(const char *mac_str, uint8_t *mac)
{
    char *endptr = NULL;
    int i;

    if (mac_str == NULL || mac == NULL)
        return TE_EWRONGPTR;

    for (i = 0; i < ETH_ALEN; i++, mac_str = endptr)
    {
        if (i > 0)
        {
            if (*mac_str == ':')
                mac_str++;
            else
            {
                ERROR("not colon, <%c>", (int)*mac_str);
                return TE_EINVAL;
            }
        }
        mac[i] = strtol(mac_str, &endptr, 16);
        if (endptr == NULL)
            break;
    }

    if (i < ETH_ALEN) /* parse error */ 
    {
        ERROR("too small index %d, NULL endptr", i);
        return TE_EINVAL;
    }

    return 0;
}

/**
 * Function for make arp reply by arp request, catched by 'eth' raw CSAP.
 * Prototype made according with 'tad_processing_pkt_method' function type.
 * This method uses write_cb callback of passed 'eth' CSAP for send reply.
 *
 * @param csap_descr  CSAP descriptor structure.
 * @param usr_param   String passed by user.
 * @param pkt         Packet binary data, as it was caught from net.
 * @param pkt_len     Length of pkt data.
 *
 * @return zero on success or error code.
 */
int 
tad_eth_arp_reply(csap_p csap_descr, const char *usr_param, 
                  const uint8_t *pkt, size_t pkt_len)
{
    uint8_t  my_mac[ETH_ALEN];
    uint8_t *arp_reply_frame;
    uint8_t *p;

    int rc;

    if (csap_descr == NULL || usr_param == NULL ||
        pkt == NULL || pkt_len == 0)
        return TE_EWRONGPTR;

    if (csap_descr->prepare_send_cb != NULL && 
        (rc = csap_descr->prepare_send_cb(csap_descr)) != 0)
    {
        ERROR("%s(): prepare for recv failed %r", rc);
        return rc;
    }

    if (strlen(usr_param) < (6 * 2 + 5)) 
    {
        ERROR("%s(): too small param <%s>, should be string with MAC",
              __FUNCTION__, usr_param);
        return TE_ETADLESSDATA;
    }

    if ((rc = mac_str2addr(usr_param, my_mac)) != 0)
    {
        ERROR("%s(): MAC parse error, <%s> ,rc %r",
              __FUNCTION__, usr_param, rc);
        return TE_EINVAL;
    } 

    VERB("%s(): got user param %s; parsed MAC: %x:%x:%x:%x:%x:%x;",
         __FUNCTION__, usr_param, 
         (int)my_mac[0], (int)my_mac[1], (int)my_mac[2], 
         (int)my_mac[3], (int)my_mac[4], (int)my_mac[5]); 

    if ((p = arp_reply_frame = calloc(1, pkt_len)) == NULL)
    {
        ERROR("%s(): no memory!", __FUNCTION__);
        return TE_ENOMEM;
    }
    /* fill eth header */
    memcpy(p, pkt + 6, 6);
    memcpy(p + 6, my_mac, 6);
    memcpy(p + 12, pkt + 12, 2);

    p += 14;
    pkt += 14; 

    memcpy(p, pkt, 6);
    p += 7;
    *p = 2; /* ARP reply */
    p++;

    pkt += 8;
    memcpy(p, my_mac, 6);
    memcpy(p + 6, pkt + 6 + 4 + 6, 4);



    memcpy(p + 6 + 4, pkt, 6 + 4); 

    rc = csap_descr->write_cb(csap_descr, arp_reply_frame, pkt_len);
    RING("%s(): sent %d bytes", __FUNCTION__, rc);
    if (rc < 0)
    {
        ERROR("%s() write error", __FUNCTION__);
        return csap_descr->last_errno;
    }

    free(arp_reply_frame);
    return 0;
}


