/**
 * Test for ASN library.
 *
 * Parse plain syntax values. 
 *
 */
#include "te_config.h"

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
      h-length plain:5,\
      src-addr plain:'0A 12 0A 02 'H,\
      dst-addr plain:'0A 12 0A 03 'H\
    }\
  },\
  payload bytes:''H\
}";


char buf[10000];

static te_errno 
check_walk_p(asn_value *v, void *d)
{
    static int m = 2000;

    printf("%s for subval %s, syntax %d\n", 
           __FUNCTION__, asn_get_name(v), asn_get_syntax(v, ""));
    asn_put_mark(v, m);
    m++;
    return 0;
}
static te_errno 
check_walk_g(asn_value *v, void *d)
{
    int m;
    asn_get_mark(v, &m);
    printf("%s for subval %s, syntax %d,mark %d\n", 
           __FUNCTION__, asn_get_name(v), asn_get_syntax(v, ""), m);
    return 0;
}

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
#if 1
    sub_val = asn_retrieve_descendant(val, &rc, "pdus.%d.src-addr", 1);

    if (rc != 0)
        printf("status %x\n", rc); 

    if (rc != TE_EASNWRONGLABEL)
    {
        printf("wrong status, should be 'wrong label' = %x, "
               "there was a choice\n", TE_EASNWRONGLABEL);
        return 1;
    }
#endif
    rc = 0;
    sub_val = asn_retrieve_descendant(val, &rc, "pdus.0.#tcp"); 
    printf("2: return %p, status %x\n", sub_val, rc);

    rc = 0;
    sub_val = asn_retrieve_descendant(val, &rc, "pdus.0.#tcp.checksum.#plain"); 
    printf("3: return %p, status %x\n", sub_val, rc);

#if 1
    rc = 0;
    sub_val = asn_retrieve_descendant(val, &rc, "pdus.2.#eth.length-type.#plain"); 
    printf("4: return %p, status %x\n", sub_val, rc);
#endif

    asn_sprint_value(val, buf, sizeof(buf), 0);
    printf("after: <%s>\n", buf);

    if (0)
    {
        te_errno cb_r = 0;
        rc = asn_walk_depth(val, TRUE, &cb_r, check_walk_p, NULL);


        printf("rc = %x, status %x\n\n  another walk:\n", rc, cb_r);
        rc = asn_walk_depth(val, FALSE, &cb_r, check_walk_g, NULL);
        printf("rc = %x, status %x:\n", rc, cb_r);
    }


    return 0;
}
