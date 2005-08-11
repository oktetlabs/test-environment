#include <stdio.h>

#include "ndn.h"

#include "tapi_snmp.h"


int 
main (int argc, char *argv[])
{ 
    int rc, s_parsed;
    asn_value_p packet, eth_header, snmp_message;
    tapi_snmp_message_t msg;
    unsigned i;

    rc = asn_parse_dvalue_in_file(argv[1], ndn_raw_packet, &packet, &s_parsed);

    printf ("parse file , rc = %x, symbol %d\n", rc, s_parsed); 


    printf ("parse file OK!\n");

#if 1
    rc = asn_read_component_value (packet, &snmp_message, "pdus.0.#snmp");
    printf ("read_comp, for snmp pdu; rc %d\n", rc);
    {
        int rc;
        int len; 

        len = sizeof (msg.type);
        rc = asn_read_value_field(snmp_message, &msg.type, &len, "type");
        printf ("read type rc %d\n", rc);
    }

    if (rc)
        return rc;

    {
        tapi_snmp_varbind_t vb;
        int len;
        asn_value *var_bind = asn_read_indexed(snmp_message, 0, "variable-bindings");
#define CL_MAX 40
        char choice_label[CL_MAX];

        if (var_bind == NULL)
        {
            fprintf(stderr, "SNMP msg to C struct: var_bind = NULL\n");
            return TE_EASNGENERAL; 
        }

        len = vb.name.length = asn_get_length(var_bind, "name.#plain");
        if (len > sizeof(vb.name.id) / sizeof(vb.name.id[0]))
        {
            return TE_ESMALLBUF;
        }
        rc = asn_read_value_field (var_bind, &vb.name.id,
                                    &len, "name.#plain"); 
        printf ("rc from read_value from OID: %x\n", rc);
    }

#endif
    return 0;
}

