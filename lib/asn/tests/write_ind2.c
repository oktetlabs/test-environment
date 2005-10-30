#include <stdio.h>
#include "test_types.h"

#include "ndn.h"


int
main(void)
{
    asn_value *nds, *gen_pdu;
    int rc, syms;

    rc = asn_parse_value_text("{{pdus {}}}", ndn_traffic_pattern, 
                              &nds, &syms);
    if (rc)
    {
        printf("parse pattern failed: %x, sym %d\n", rc, syms);
        return 2;
    }

    rc = asn_parse_value_text("eth:{}", ndn_generic_pdu, 
                              &gen_pdu, &syms);
    if (rc)
    {
        printf("parse gen pdu failed: %x, sym %d\n", rc, syms);
        return 2;
    }

    rc = asn_write_component_value(nds, gen_pdu,  "0.pdus.0");
    if (rc)
    {
        printf("write pdu to pattern failed: %x\n", rc);
        return 3;
    } 

    return 0;
}
