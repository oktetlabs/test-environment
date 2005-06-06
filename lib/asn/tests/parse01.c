/**
 * Test for ASN library.
 *
 * Parse plain syntax values. 
 *
 */
#include <stdio.h>

#include "asn_usr.h"
#include "ndn.h"

#include "test_types.h"


char buffer[1000];

int result = 0;

void
test_string_parse(const char *string, const asn_type *type)
{
    int rc;
    int s_parsed;
    asn_value_p new_val; 

    rc = asn_parse_value_text(string, type, &new_val, &s_parsed);
    if (rc != 0)
    {
        printf ("parse of '%s', type %s: \n  rc %6x, syms: %d\n",
                string, type->name, rc, s_parsed);
        result = 1;
        return;
    }
    asn_sprint_value(new_val, buffer, 1000, 0);
    printf("type %s, parsed value: \n--\n%s\n--\n", type->name, buffer);
    asn_free_value(new_val);
    rc = asn_parse_value_text(buffer, type, &new_val, &s_parsed);
    if (rc != 0)
    {
        printf ("parse of printed byffer type %s: \n"
                "  rc %6x, syms: %d\n",
                type->name, rc, s_parsed);
        result = 1;
        return;
    }
} 

int 
main (void)
{ 

    test_string_parse("\"berb\\\"erber\"", asn_base_charstring);
    test_string_parse("\"Somethins long string with ''' oo   \n aaa\"",
                      asn_base_charstring); 

    test_string_parse("0", asn_base_integer);
    test_string_parse("14", asn_base_integer);
    test_string_parse("-2000001", asn_base_integer); 
    test_string_parse("{1 3 6 1 2 1 }", asn_base_objid);
    test_string_parse("{ number 16, string \"lalala\" }", &at_plain_seq1);
    test_string_parse("{ name \"uuu\" , array {1, 2, 35  , 55 } }",
                            &at_named_int_array);
    test_string_parse("number:222", &at_plain_choice1);
    test_string_parse("{ arg-sets { simple-for:{begin 1}}, "
                      "  pdus     { eth:{}               } } }",
                      ndn_traffic_template);
    test_string_parse("{ pdus { }, "
                      "arg-sets {simple-for:{ begin 1, end 10 } },"
                      " payload function:\"eth_udp_payload64\" }",
                      ndn_traffic_template); 
    return result;
}
