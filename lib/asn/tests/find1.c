
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
      hlen plain:6,\
      flags plain:18,\
      win-size plain:5840,\
      checksum plain:7001,\
      urg-p plain:0\
    },\
    ip4:{\
      version plain:4,\
      h-length plain:5,\
      type-of-service plain:0,\
      total-length plain:44,\
      ip-ident plain:0,\
      dont-frag plain:1,\
      frag-offset plain:0,\
      time-to-live plain:64,\
      protocol plain:6,\
      h-checksum plain:4772,\
      src-addr plain:'0A 12 0A 02 'H,\
      dst-addr plain:'0A 12 0A 03 'H\
    },\
    eth:{\
      src-addr plain:'00 0E A6 41 D5 2E 'H,\
      dst-addr plain:'01 02 03 04 05 06 'H,\
      length-type plain:2048\
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
        printf("parse failed rc %x, syms: %d\n",
               rc, s_parsed);
        return 1;
    } 

    sub_val = asn_find_descendant(val, &rc, "pdus.%d.src-addr", 1);

    if (rc != 0)
        printf("status %x\n", rc); 

    if (sub_val == NULL)
    {
        printf("return NULL\n");
        rc = TE_EWRONGPTR;
    }

    if (sub_val != NULL && rc == 0)
    {
        asn_sprint_value(sub_val, buf, sizeof(buf), 0);
        printf("got value: <%s>\n", buf);
    }

    return rc;
}
