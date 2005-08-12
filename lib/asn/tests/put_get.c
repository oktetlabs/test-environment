/**
 * ASN.1 library
 *
 * self test, for put and get methods 
 *
 */
#include <stdio.h>

#include "asn_usr.h"
#include "test_types.h"


int result = 0;



char buffer [1000];

#define COMPON_TEST 1

#define BUF_TO_READ 100
char buf_to_read[BUF_TO_READ];

int 
main (void)
{ 
    asn_value_p seq_val = asn_init_value(&at_plain_seq1);
    asn_value_p int_val; 
    asn_value_p str_val; 
    asn_value_p child_val; 
    asn_value_p choice_val; 
    size_t syms; 
    int rc;

    asn_parse_value_text("15", asn_base_integer, &int_val, &syms);

    rc = asn_put_child_value(seq_val, int_val, PRIVATE, SEQ_NUMBER_TAG);
    if (rc) 
    {
        result = 1;
        fprintf(stderr, "put int child value failed 0x%X\n", rc);
    }

    asn_parse_value_text("\"uajajaja\"", asn_base_charstring,
                         &str_val, &syms);

    rc = asn_put_child_value(seq_val, str_val, PRIVATE, SEQ_STRING_TAG);
    if (rc) 
    {
        result = 1;
        fprintf(stderr, "put string child value failed 0x%X\n", rc);
    }

    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf("composed value: \n%s\n", buffer);

    asn_parse_value_text("2520", asn_base_integer, &int_val, &syms);
    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf("composed value: \n%s\n", buffer);


    rc = asn_get_child_value(seq_val, &child_val, PRIVATE, SEQ_STRING_TAG);
    if (rc) 
    {
        result = 1;
        fprintf(stderr, "get child value failed 0x%X\n", rc);
    }
    asn_sprint_value(child_val, buffer, 1000, 0);
    printf("got child value: \n%s\n", buffer);

    rc = asn_free_child_value(seq_val,  PRIVATE, SEQ_STRING_TAG);
    if (rc) 
    {
        result = 1;
        fprintf(stderr, "free child value failed 0x%X\n", rc);
    }
    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf("after free value: \n%s\n", buffer);

    rc = asn_parse_value_text("string:\"uuulalal\"", &at_plain_choice1,
                              &choice_val, &syms);
    if (rc)
    {
        result = 1;
        fprintf(stderr, "parse choice failed 0x%X, syms %d\n", rc, syms);
    }
    else
    {
        uint16_t tag = asn_get_tag(choice_val);
        printf("got tag: %d\n", tag);
    }

    return result;
}
