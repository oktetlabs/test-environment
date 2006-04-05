
/**
 * Test for ASN library.
 *
 * Parse plain syntax values. 
 *
 */
#include <stdio.h>

#include "asn_usr.h"
#include "ndn.h"
#include "ndn_eth.h"

#include "test_types.h"


char buffer[1000];

int result = 0;

char packet_asn_string[] = 
"{\
  received {\
    seconds 1140892564,\
    micro-seconds 426784\
  },\
  pdus {\
    tcp:{\
      src-port plain:20587,\
      dst-port plain:20586,\
      seqn plain:-281709452,\
      ackn plain:1284566196,\
    },\
    ip4:{\
      version plain:4,\
      header-len plain:5,\
      src-addr plain:'0A 12 0A 02 'H,\
      dst-addr plain:'0A 12 0A 03 'H\
    }\
  },\
  payload bytes:''H\
}";


char buf[10000];

int 
main(void)
{
    asn_value *val;
    asn_value *sub_val;
    te_errno   rc = 0;
    int        s_parsed;

    rc = asn_parse_value_text(packet_asn_string, ndn_raw_packet,
                              &val, &s_parsed);
    if (rc != 0)
    {
        printf("parse failed rc %x, syms: %d\n", rc, s_parsed);
        return 1;
    } 

    sub_val = asn_retrieve_descendant(val, &rc, "pdus.%d.src-addr", 1);

    if (rc != 0)
        printf("status %x\n", rc); 

    if (rc != TE_EASNWRONGLABEL)
    {
        printf("wrong status, should be 'wrong label' = %x, "
               "there was a choice\n", TE_EASNWRONGLABEL);
        return 1;
    }

    rc = 0;
    sub_val = asn_retrieve_descendant(val, &rc, "pdus.0.#tcp"); 
    printf("2: return %p, status %x\n", sub_val, rc);

    rc = 0;
    sub_val = asn_retrieve_descendant(val, &rc, "pdus.0.#tcp.checksum.#plain"); 
    printf("2: return %p, status %x\n", sub_val, rc);

#if 1
    rc = 0;
    sub_val = asn_retrieve_descendant(val, &rc, "pdus.2.#eth.length-type.#plain"); 
    printf("2: return %p, status %x\n", sub_val, rc);
#endif

    asn_sprint_value(val, buf, sizeof(buf), 0);
    printf("after: <%s>\n", buf);

    return 0;
}
