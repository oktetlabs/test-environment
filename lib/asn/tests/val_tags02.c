#include "te_config.h"

#include <stdio.h>

#include "asn_usr.h"
#include "test_types.h"

#include "ndn.h"
#include "ndn_ipstack.h"




char buffer [1000];

char str_csap_nds_pdu[] = 
"{ local-port plain:27103, remote-port plain:27104 }";

char str_pattern[] = 
"{ { pdus { tcp:{ }, ip4:{ }, eth:{ } } } }";

#define COMPON_TEST 1

#define BUF_TO_READ 100
char buf_to_read[BUF_TO_READ];

int 
main (void)
{ 
    asn_value *csap_nds_pdu;
    asn_value *pattern;
    asn_value *tcp_header_v = asn_init_value(ndn_tcp_header);
    const asn_value *du_field;
    const asn_value *du_val;
    asn_tag_class     t_class;
    asn_tag_value     t_val1 = NDN_DU_UNDEF;
    asn_tag_value     t_val2 = NDN_DU_UNDEF;

    te_errno rc;


    int        s_parsed;

    rc = asn_parse_value_text(str_csap_nds_pdu, ndn_tcp_csap,
                              &csap_nds_pdu, &s_parsed);
    if (rc != 0)
    {
        fprintf(stderr, "parse 1 failed rc %x, syms: %d\n", rc, s_parsed);
        return 1;
    } 
    rc = asn_parse_value_text(str_pattern, ndn_traffic_pattern,
                              &pattern, &s_parsed);
    if (rc != 0)
    {
        fprintf(stderr, "parse 2 failed rc %x, syms: %d\n", rc, s_parsed);
        return 1;
    } 
    rc = asn_get_child_value(csap_nds_pdu, &du_field, PRIVATE,
                             NDN_TAG_TCP_LOCAL_PORT);
    if (rc != 0)
    {
        fprintf(stderr, "get child failed rc %x\n", rc);
        return 1;
    } 

    rc = asn_write_component_value(tcp_header_v, du_field, "dst-port");
    if (rc != 0)
    {
        fprintf(stderr, "write comp failed rc %x\n", rc);
        return 1;
    } 
    asn_get_choice_value(du_field, (asn_value **)&du_val, 
                         &t_class, &t_val1);
    /* printf("tag cl: %d, val %d\n", (int)t_class, (int)t_val1); */

    rc = asn_get_child_value(tcp_header_v, &du_field, PRIVATE, 
                             NDN_TAG_TCP_DST_PORT);
        
    asn_get_choice_value(du_field, (asn_value **)&du_val, 
                         &t_class, &t_val2);
    /* printf("tag val %d\n", (int)t_val2); */
    if (t_val1 != t_val2)
    {
        fprintf(stderr, "diff tag values: src %d, after assign %d\n", 
                (int)t_val1, (int)t_val2);
        return 2;
    }


    return 0;
}
