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


char tmpl_asn_string[] = 
"{\
  pdus {\
    atm:{\
      vpi plain:10,\
      vci plain:21,\
      payload-type plain:0,\
      clp plain:0,\
      hec plain:0\
    },\
    socket:{\
    }\
  },\
  payload bytes:'45 00 03 E1 00 00 00 00 40 11 48 C2 0A 24 0D 01 0A 24 0D 02 62 D1 62 D2 03 CD B7 52 1A 99 09 72 22 F7 27 56 C6 43 6C 50 E0 8F 9F 35 75 E1 8A 44 'H\
}";


char packet_asn_string[] = 
#if 0
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
#else
"{\
  received {\
    seconds 1166433609,\
    micro-seconds 410702\
  },\
  pdus {\
    atm:{\
      vpi plain:10,\
      vci plain:21,\
      payload-type plain:0,\
      clp plain:0,\
      hec plain:0\
    },\
    socket:{\
    }\
  },\
  payload bytes:'45 00 03 E1 00 00 00 00 40 11 48 C2 0A 24 0D 01 0A 24 0D 02 62 D1 62 D2 03 CD B7 52 1A 99 09 72 22 F7 27 56 C6 43 6C 50 E0 8F 9F 35 75 E1 8A 44 'H\
}";
#endif


char buf[10000];

int 
main(void)
{
    asn_value *val;
    asn_value *sub_val;
    asn_value *tmpl = NULL;
    te_errno   rc = 0;
    int        s_parsed;

    rc = asn_parse_value_text(packet_asn_string, ndn_raw_packet,
                              &val, &s_parsed);
    if (rc != 0)
    {
        printf("parse failed rc %x, syms: %d\n", rc, s_parsed);
        return 1;
    } 
    sub_val = asn_find_descendant(val, &rc, "pdus.%d", 1);

    if (rc != 0)
    {
        fprintf(stderr, "status %x\n", rc); 
        return rc;
    }
    if (sub_val == NULL)
    {
        fprintf(stderr, "returned value NULL!\n"); 
        return 1;
    } 

    rc = asn_parse_value_text(tmpl_asn_string, ndn_traffic_template,
                              &tmpl, &s_parsed);
    if (rc != 0)
    {
        printf("parse tmpl failed rc %x, syms: %d\n", rc, s_parsed);
        return 1;
    } 

    if (1)
    {
        rc = asn_free_subvalue(tmpl, "pdus.1");
        asn_sprint_value(tmpl, buf, sizeof(buf), 0);
        printf("parsed template after free_subvalue (rc %x) : \n%s\n",
               rc, buf);
    }
    asn_free_value(tmpl);
    tmpl = NULL;

    if (1)
    {

        rc = ndn_packet_to_template(val, &tmpl);
        printf("generate rc %x\n", rc);
        rc = asn_free_subvalue(tmpl, "pdus.1");
        asn_sprint_value(tmpl, buf, sizeof(buf), 0);
        printf("generated template after free_subvalue (rc %x) : \n%s\n",
               rc, buf);
    }

    rc = asn_free_subvalue(val, "pdus.1");
    if (rc != 0)
    {
        fprintf(stderr, "free subvalue status %x\n", rc); 
        return rc;
    }
    sub_val = asn_find_descendant(val, &rc, "pdus.1");

    if (sub_val != NULL || rc != TE_EASNINCOMPLVAL)
    {
        fprintf(stderr,
                "unexpected result of find after free: ptr %p, status %x\n",
                sub_val, rc); 
        if (!rc)
            rc = 1;

#if 0
        asn_sprint_value(val, buf, sizeof(buf), 0);
        printf("value after free_subvalue: \n%s\n",
               buf);
#endif
    }

    return 0;
}
