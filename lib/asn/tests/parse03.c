#include <stdio.h>

#include "ndn.h"

char buffer[1000];

extern int asn_count_txt_len (asn_value_p, int);

void test_string_parse(const char * str, const asn_type * type)
{
    int rc;
    asn_value_p new_val;
    int s_parsed;

    rc = asn_parse_value_text(str, type, &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);
    if (rc) return rc; 
    rc = asn_sprint_value(new_val, buffer, sizeof(buffer), 0);
    printf ("\nparsed value: \n--\n%s\n--\nused syms: %d\n", buffer, rc); 

    rc = asn_count_txt_len(new_val, 0);
    printf ("count txt syms: %d\n", rc);
    asn_free_value(new_val);
}

int 
main (void)
{ 
    int rc;
    int s_parsed;
    asn_value_p new_val; 

    char filename[1000];
    int fn_len = 1000;
    asn_value_p gen;

#if 0
    rc = asn_parse_value_text("8", &at_our_names, &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);
    if (rc) return rc;

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 


    rc = asn_parse_value_text("9", &at_our_names, &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);
    if (rc) return rc;

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 


    rc = asn_parse_value_text("thor", &at_our_names, &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);
    if (rc) return rc;

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
#endif
#if 0
    rc = asn_parse_value_text("{1 3 6 1 2 1 }", asn_base_objid, &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);
    if (rc) return rc;

    rc = asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\nused syms: %d\n", buffer, rc); 

    rc = asn_count_txt_len(new_val, 0);
    printf ("count txt syms: %d\n", rc);


    test_string_parse (" { snmp:{ version plain:1 } }", ndn_csap_spec);


    test_string_parse ("{ number 1, string \"aa\" }", &at_plain_seq1);
    test_string_parse ("{ number 1 }", &at_plain_seq1);
#endif
    test_string_parse ("snmp:{ version plain:1 }", ndn_generic_csap_level);
    test_string_parse ("{ version plain:1 }", ndn_snmp_csap);
#if 1

    test_string_parse (
          "{pdus {snmp:{ type get-next, variable-bindings {{ \
          name plain:{1 3 6 1 2 1 }}}}}}", 
            ndn_traffic_template);

    return 0;
#endif
}

#if 0 
"{pdus {snmp:{ type get-next, variable-bindings { name plain:{1 3 6 1 2 1 }}}}}"
#endif
