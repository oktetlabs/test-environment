#include <stdio.h>

#include "ndn.h"



int 
main (int argc, char *argv[])
{ 
    int level;
    int rc, s_parsed;
    int n;
#if 1
    const asn_type *a_type = ndn_traffic_template;
#else
    const asn_type *a_type = ndn_csap_spec;
#endif
    asn_value_p packet;
    asn_value *pdus;

    rc = asn_parse_dvalue_in_file(argv[1], a_type, &packet, &s_parsed);
    
    if (rc)
    {
        printf ("rc from parse: %x, syms: %d\n", rc, s_parsed);
        return 1;
    }

    rc = asn_get_subvalue(packet, &pdus, "pdus"); 
    if (rc)
    {
        printf ("rc from parse: %x, syms: %d\n", rc, s_parsed);
        return 1;
    } 

    n = asn_get_length(packet, "pdus");

    for (level = 0; (rc == 0) && (level < n); level ++)
    { 
        char label_pdu_choice[20] = "#eth";
        asn_value * level_pdu;
        uint16_t eth_type;


        /* TODO: rewrite with more fast ASN method, that methods should 
         * be prepared and tested first */
        level_pdu = asn_read_indexed(pdus, level, ""); 

        if (level_pdu == NULL) 
            return ETADWRONGNDS; 

        rc = asn_write_value_field(level_pdu, &eth_type, sizeof(eth_type), 
                                    "eth-type.#plain");

        if (rc == 0)
        {
            rc = asn_write_indexed(pdus, level_pdu, level, "");
            printf("TAD_SEND " "asn_write_val_field rc: %x", rc);
        }
        asn_free_value(level_pdu);
        if (rc) 
        {
            /* "PDU does not confirm" */ 
            printf("TAD_SEND template does not confirm to CSAP; "
                       "rc: 0x%x,  level: %d\n", rc, level);
            break;
        }
    }

    asn_save_to_file(pdus, "pdus-after-confirm.asn");

    return 0;
}

