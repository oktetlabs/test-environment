#include <stdio.h>

#include "ndn.h"

char te_lgr_entity[] = "test";

int log_message(void)
{ ; }



int 
main (int argc, char *argv[])
{ 
    int rc, s_parsed;
#if 1
    asn_type *a_type = ndn_raw_packet;
#else
    asn_type *a_type = ndn_traffic_template;
#endif
    asn_value_p packet, eth_header, snmp_message;

    const asn_value *pattern_unit;

    if (argc < 2)
    {
        printf("usage: %s <filename>\n", argv[0]);
        return 0;
    }

    rc = asn_parse_dvalue_in_file(argv[1], a_type, &packet, &s_parsed);

    printf ("parse file , rc = %x, symbol %d\n", rc, s_parsed); 

    if (0)
    {
        char eth_src_script[] = "expr:(0x010203040506 + $0)";
        asn_free_subvalue(packet, "0.pdus.0.#eth.dst-addr");
        rc = asn_write_value_field(packet, eth_src_script, 
                sizeof(eth_src_script), "0.pdus.0.#eth.dst-addr.#script");
        printf("tempalte write expr %x\n", rc);
    }


    asn_save_to_file(packet, "out.asn");

#if 1
    asn_free_value(packet);
#endif


#if 0
    rc = asn_read_component_value (packet, &snmp_message, "pdus.0.#snmp");
    printf ("read_comp, for snmp pdu; rc %d\n", rc);
    {
        int rc;
        int len; 
        unsigned i;

        len = sizeof (msg.type);
        rc = asn_read_value_field(snmp_message, &msg.type, &len, "type");
        printf ("read type rc %d\n", rc);
    }

        snmp_message = asn_read_indexed (packet, 0, "pdus");
    rc = tapi_snmp_packet_to_plain(snmp_message, &msg);
    printf("packet to plain rc %x\n", rc);
#endif



#if 0
    if (rc == 0)
    {
        ndn_eth_header_plain eh;

        eth_header = asn_read_indexed (packet, 0, "pdus");
        rc = ndn_eth_packet_to_plain (eth_header, &eh);
        if (rc)
            printf ("eth_packet to plain fail: %x\n", rc);
        else
        {
            int i; 
            printf ("dst - %2x", eh.dst_addr[0]);
            for (i = 1; i < 6; i++)
                printf (":%2x", eh.dst_addr[i]);

            printf ("\nsrc - %2x", eh.src_addr[0]);
            for (i = 1; i < 6; i++)
                printf (":%2x", eh.src_addr[i]);
            printf ("\ntype - %4x\n", eh.eth_type);
        }

    }

#endif
    return 0;
}

