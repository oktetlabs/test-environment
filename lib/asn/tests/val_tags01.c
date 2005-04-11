#include <stdio.h>

#include "test_types.h"




char buffer [1000];

#define COMPON_TEST 1

#define BUF_TO_READ 100
char buf_to_read[BUF_TO_READ];

int 
main (void)
{ 
    asn_value_p seq_val   = asn_init_value(&at_plain_seq1);
    asn_value_p ch_val   = asn_init_value(&at_plain_choice1);

    const asn_value **subval;

    int a = 1981, b = 1234; 
    char str[] = "uura..";
    char str2[] = "sss.... lslsls";
    int r; 
    int read_len = BUF_TO_READ;

    r = asn_write_value_field(seq_val, &a, sizeof(a), "number");
    if (r) { printf("write numbver error: 0x%X\n", r); return 1; }


    r = asn_get_child_value(seq_val, &subval, PRIVATE, SEQ_NUMBER_TAG);
    printf("rc getting subval by tag 'number': 0x%X\n", r); 


    r = asn_get_child_value(seq_val, &subval, PRIVATE, SEQ_STRING_TAG);
    printf("rc getting subval by tag 'string': 0x%X\n", r); 

    r = asn_write_value_field(seq_val, str, sizeof(str), "string");
    if (r) { printf("error code returned: %d\n", r); return r; }

    r = asn_sprint_value(seq_val, buffer, 1000, 0);
    printf("seq_val after write values: \n\"%s\"\n", buffer); 

    r = asn_get_child_value(seq_val, &subval, PRIVATE, SEQ_NUMBER_TAG);
    printf("rc getting subval by tag 'number': 0x%X\n", r); 

    r = asn_get_child_value(seq_val, &subval, PRIVATE, SEQ_INT_ARRAY_TAG);
    printf("rc getting subval by tag 'ingeter_array': 0x%X\n", r); 

    r = asn_write_value_field(ch_val, &a, sizeof(a), "#number");
    if (r) { printf("write number to choice error: 0x%X\n", r); return 1; } 

    subval = NULL;
    r = asn_get_choice_value(ch_val, &subval, NULL, NULL);
    printf("rc getting choice subval: 0x%X\n", r); 

    if (r == 0 && subval != NULL)
        printf("got subval syntax: %d\n", (*subval)->syntax);

    return 0;
}
