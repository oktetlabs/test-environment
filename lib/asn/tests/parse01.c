#include <stdio.h>

#include "asn_usr.h"
#include "ndn.h"


extern int asn_impl_parse_text(const char*text, asn_type_p, 
                        asn_value_p *parsed, int *parsed_syms);

char buffer[1000];

int 
main (void)
{ 
    int rc;
    int s_parsed;
    asn_value_p new_val; 

#if 0
    rc = asn_impl_parse_text("14", &asn_base_integer, &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 

    rc = asn_impl_parse_text("\"berberber\"", &asn_base_charstring, &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    
    rc = asn_impl_parse_text("\"berbe\"rber\"", &asn_base_charstring, 
                                &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    asn_free_value (new_val);

    rc = asn_impl_parse_text("{ number 16, string \"lalala\" }",
                            &at_plain_seq1, &new_val, &s_parsed);
    printf ("ret code from parse sequence: %6x, syms: %d\n", rc, s_parsed);
    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    asn_free_value (new_val);


    rc = asn_impl_parse_text("{ name \"uuu\" , array {1, 2, 35  , 55 } }",
                            &at_named_int_array, &new_val, &s_parsed);
    printf ("ret code from parse sequence: %6x, syms: %d\n", rc, s_parsed);
    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    asn_free_value (new_val); 


    rc = asn_impl_parse_text("number:222",
                            &at_plain_choice1, &new_val, &s_parsed);
    printf ("ret code from parse sequence: %6x, syms: %d\n", rc, s_parsed);
    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    asn_free_value (new_val); 

#endif
    rc = asn_parse_value_text("{ arg-sets { simple-for:{begin 1}}, "
                              "  pdus     { eth:{}               } } }",
                              ndn_traffic_template,
                              &new_val, &s_parsed);

    printf ("rc: %x; syms: %d\n", rc, s_parsed);

    rc = asn_parse_value_text(
"{ pdus {  }, arg-sets {simple-for:{ begin 1, end 10 } }, payload function:\"eth_udp_payload64\" }",
                              ndn_traffic_template,
                              &new_val, &s_parsed);

    printf ("rc: %x; syms: %d\n", rc, s_parsed);

{
    char buf[1000];
    asn_sprint_value(new_val, buf, sizeof(buf), 0);
    printf ("parsed value: <%s>\n", buf);
}

    return 0;
}
