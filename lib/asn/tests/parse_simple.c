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
    asn_value *new_val; 
    int i, slen;

    rc = asn_parse_value_text(string, type, &new_val, &s_parsed);
    if (rc != 0)
    {
        printf ("parse of '%s', type %s: \n  rc %6x, syms: %d\n",
                string, type->name, rc, s_parsed);
        result = 1;
        return;
    }
    slen = rc = asn_count_txt_len(new_val, 0);
    printf("\ncount len: %d, ", rc);
    rc = asn_sprint_value(new_val, buffer, 1000, 0);
    printf("printed len: %d\n", rc);
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
    for (i = 0; i < slen; i++)
    {
        rc = asn_sprint_value(new_val, buffer, i + 1, 0);
        if (rc != slen)
        {
            printf("++++ rc = %d, from underlen sprint(%d) wrong, "
                   "should be %d\n     printed <%s>", 
                   rc, i, slen, buffer);
            break;
        }
        if ((rc = strlen(buffer)) != i)
        {
            printf("++++ strlen = %d after underlen sprint wrong, "
                   "should be %d\n     printed <%s>", 
                   rc, i, buffer);
            break;
        }
    }
    printf("partial print ended on %d length\n\n", i);
} 

int 
main (void)
{ 

    test_string_parse("\"berb\\\"erber\"", asn_base_charstring);
    test_string_parse("\"Somethins long string with ''' oo   \n aaa\"",
                      asn_base_charstring); 
    test_string_parse("'00 01 03 05 23 5F 8A 5B CC 00 00 0 0 'H",
                      asn_base_octstring); 

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
#if 1
    test_string_parse(
"{ { pdus { eth:{"
"        src-addr plain:'00 0E A6 41 D5 2E 'H,"
"        dst-addr plain:'FF FF FF FF FF FF 'H,"
"        eth-type plain:2054"
"      } },"
"      payload mask:{"
"      v '00 01 08 00 06 04 00 01 00 0E A6 41 D5 2E 00 00 00 00 00 00 00 00 0 0 00 0A 12 0A 03 'H,"
"      m 'FF FF FF FF FF FF FF FF FF FF FF FF FF FF 00 00 00 00 00 00 00 00 0 0 00 FF FF FF FF 'H,"
"      exact-len FALSE"
"    },"
"    actions {"
"      function:\"tad_eth_arp_reply:01:02:03:04:05:06\""
"} } }",
                      ndn_traffic_pattern); 
    test_string_parse(
"{\
  received {\
    seconds 1140892564,\
    micro-seconds 426784\
  },\
  pdus {\
    tcp:{\
      src-port plain:20587,\
      dst-port plain:20586,\
      seqn plain:-281709452,\
      ackn plain:1284566196,\
      hlen plain:6,\
      flags plain:18,\
      win-size plain:5840,\
      checksum plain:7001,\
      urg-p plain:0\
    },\
    ip4:{\
      version plain:4,\
      header-len plain:5,\
      type-of-service plain:0,\
      ip-len plain:44,\
      ip-ident plain:0,\
      flags plain:2,\
      ip-offset plain:0,\
      time-to-live plain:64,\
      protocol plain:6,\
      h-checksum plain:4772,\
      src-addr plain:'0A 12 0A 02 'H,\
      dst-addr plain:'0A 12 0A 03 'H\
    },\
    eth:{\
      src-addr plain:'00 0E A6 41 D5 2E 'H,\
      dst-addr plain:'01 02 03 04 05 06 'H,\
      eth-type plain:2048\
    }\
  },\
  payload bytes:''H\
}",
                      ndn_raw_packet); 
#endif
    return result;
}
