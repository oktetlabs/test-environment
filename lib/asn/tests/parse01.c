#include <stdio.h>

#include "asn_usr.h"
#include "ndn.h"

#include "test_types.h"


char buffer[1000];

int 
main (void)
{ 
    int rc;
    int s_parsed;
    asn_value_p new_val; 
    const char *str;

    rc = asn_parse_value_text("\"berb\\\"erber\"", asn_base_charstring,
                             &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\1. nparsed value: \n--\n%s\n--\n", buffer); 

    rc = asn_parse_value_text(buffer, asn_base_charstring,
                             &new_val, &s_parsed);
    printf ("ret code from parse again printed: %6x, syms: %d\n",
            rc, s_parsed);

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\n2. parsed value: \n--\n%s\n--\n", buffer); 

    str = NULL;
    rc = asn_get_field_data(new_val, &str, NULL);

    printf ("\n3. directly got string, rc = %x: \n--\n%s\n--\n", rc, str); 

#if 0
    rc = asn_parse_value_text("14", asn_base_integer, &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 

    
    rc = asn_parse_value_text("\"berbe\"rber\"", asn_base_charstring, 
                                &new_val, &s_parsed);
    printf ("ret code from parse: %6x, syms: %d\n", rc, s_parsed);

    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    asn_free_value (new_val);

    rc = asn_parse_value_text("{ number 16, string \"lalala\" }",
                            at_plain_seq1, &new_val, &s_parsed);
    printf ("ret code from parse sequence: %6x, syms: %d\n", rc, s_parsed);
    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    asn_free_value (new_val);


    rc = asn_parse_value_text("{ name \"uuu\" , array {1, 2, 35  , 55 } }",
                            at_named_int_array, &new_val, &s_parsed);
    printf ("ret code from parse sequence: %6x, syms: %d\n", rc, s_parsed);
    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    asn_free_value (new_val); 


    rc = asn_parse_value_text("number:222",
                            at_plain_choice1, &new_val, &s_parsed);
    printf ("ret code from parse sequence: %6x, syms: %d\n", rc, s_parsed);
    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    asn_free_value (new_val); 

#endif
#if 0
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
#endif
    return 0;
}
