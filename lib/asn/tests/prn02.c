#include <stdio.h>
#include <string.h>

#include "asn_impl.h"
#include "te_stdint.h"

#include "ndn.h"

#define FIRST_TEST 0

char print_buffer[1000];


extern int asn_impl_parse_text(const char*text, asn_type_p, 
                               asn_value_p *parsed, int *parsed_syms);


main (void)
{
        int rc, syms = 4;
        uint8_t payload [2000];
        int p_len = 100; /* for test */
        asn_value *template;
        asn_value *asn_pdus;
        asn_value *asn_pdu;
        asn_value *asn_bpdu;
        asn_value *asn_eth_hdr;
        char eth_device[] = "eth0"; 

        uint8_t own_addr[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
        uint8_t out_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff}; 

        ndn_stp_bpdu_t plain_bpdu;

        memset (&plain_bpdu, 0, sizeof (plain_bpdu));

        plain_bpdu.cfg.root_path_cost = 1;

        asn_bpdu = ndn_bpdu_plain_to_asn(&plain_bpdu);

        template = asn_init_value(ndn_traffic_template);
        asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
        asn_pdu = asn_init_value(ndn_generic_pdu); 
        asn_eth_hdr = asn_init_value(ndn_eth_header); 
        
        rc = asn_write_component_value(asn_pdu, asn_bpdu, "#bridge");
        if (rc == 0)
            rc = asn_insert_indexed(asn_pdus, asn_pdu, 0, "");
 
        asn_free_value(asn_pdu);
        asn_pdu = asn_init_value(ndn_generic_pdu); 
        if (rc == 0)
            rc = asn_write_component_value(asn_pdu, asn_eth_hdr, "#eth");
        if (rc == 0)
            rc = asn_insert_indexed(asn_pdus, asn_pdu, 1, "");

        if (rc == 0)
            rc = asn_write_component_value(template, asn_pdus, "pdus");

        syms = asn_sprint_value(template, print_buffer, sizeof(print_buffer), 0);

        printf ("printed value: <%s>\n", print_buffer);

    return 0;
}
