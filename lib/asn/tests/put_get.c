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
    size_t syms; 
    int rc;

    asn_parse_value_text("15", asn_base_integer, &int_val, &syms);

    rc = asn_put_child_value(seq_val, int_val, PRIVATE, SEQ_NUMBER_TAG);
    if (rc) 
    {
        result = 1;
        fprintf(stderr, "put int child value failed %X\n", rc);
    }

    asn_parse_value_text("\"uajajaja\"", asn_base_charstring,
                         &str_val, &syms);

    rc = asn_put_child_value(seq_val, str_val, PRIVATE, SEQ_STRING_TAG);
    if (rc) 
    {
        result = 1;
        fprintf(stderr, "put string child value failed %X\n", rc);
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
        fprintf(stderr, "get child value failed %X\n", rc);
    }
    asn_sprint_value(child_val, buffer, 1000, 0);
    printf("got child value: \n%s\n", buffer);

    return result;
}
