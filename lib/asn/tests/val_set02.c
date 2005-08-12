#include <stdio.h>

#include "test_types.h"

extern asn_type asn_base_integer_s;
extern asn_type asn_base_charstring_s;


#if 0
asn_named_entry_t my_entry_array [] = {
    { "number", &asn_base_integer_s },
    { "string", &asn_base_charstring_s }
};

asn_type my_sequence = {
    "MySequence",
    {APPLICATION, 1},
    SEQUENCE,
    2,
    {&my_entry_array}
};
#endif


char buffer [1000];

#define FIRST_TEST 1
#define TAG_TEST 0

#define BUF_TO_READ 100
char buf_to_read[BUF_TO_READ];

int 
main (void)
{ 
    asn_value_p seq_val = asn_init_value(&at_plain_seq1);
    int a = 1981; 
    char str[] = "test string";
    int r; 
    int read_len = BUF_TO_READ;

#if FIRST_TEST
    r = asn_write_value_field(seq_val, &a, sizeof(a), "number");
    if (r) { printf ("error code returned: %d\n", r); return r; }

    r = asn_write_value_field(seq_val, str, sizeof(str), "string");
    if (r) { printf ("error code returned: %d\n", r); return r; }

#if 0
    r = asn_sprint_value(seq_val, buffer, 1000, 0);
    printf ("seq_val after write values: \n\"%s\"\n", buffer); 

    r = asn_read_value_field(seq_val, buf_to_read, &read_len, "string");
    printf ("read 'string' leaf, ret: %6x, str: '%s'; len: %d\n", 
            r, buf_to_read, read_len);

    r = asn_free_subvalue(seq_val, "string");
    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf ("free 'string' subvalue (ret = 0x%6x): \n\"%s\"\n", r, buffer); 

    r = asn_read_value_field(seq_val, buf_to_read, &read_len, "string");
    printf ("read 'string' leaf after free: ret: %6x, str: '%s'; len: %d\n", 
            r, buf_to_read, read_len);
#endif

    {
        asn_value_p cmpl = asn_init_value(&my_complex);
        r = asn_write_value_field(cmpl, &a, sizeof(a), "subseq.number");
        if (r) 
        { 
            printf("set number error code returned: 0x%X\n", r); 
            return r; 
        }

        r = asn_write_value_field(cmpl, str, sizeof(str), "choice.#string");
        if (r) 
        { 
            printf("set string error code returned: 0x%X\n", r); 
            return r; 
        }

        r = asn_sprint_value(cmpl, buffer, 1000, 0);
        printf ("cmpl after write values: \n\"%s\"\n", buffer); 

        r = asn_free_subvalue(cmpl, "choice.#string");
        printf ("rc of free_subval: 0x%0X\n", r);
        asn_sprint_value(cmpl, buffer, 1000, 0);
        printf ("free 'choice' subvalue (ret = 0x%6x): \n\"%s\"\n", r, buffer); 

        r = asn_free_subvalue(cmpl, "choice");
        printf ("rc of free_subval: 0x%0X\n", r);
        asn_sprint_value(cmpl, buffer, 1000, 0);
        printf ("free 'choice' subvalue (ret = 0x%6x): \n\"%s\"\n", r, buffer); 

    }
#endif

#if TAG_TEST
    printf("Lets test Tagged:\n");
    seq_val = asn_init_value(&my_tagged);
    a = 4;
    r = asn_write_value_field(seq_val, &a, sizeof(a), "number");
    if (r) { printf ("error code returned: %6x\n", r); return r; }

    r = asn_sprint_value(seq_val, buffer, 1000, 0);
    printf ("seq_val after write values: \n\"%s\"\n", buffer); 

#endif
    asn_free_value(seq_val);
    printf ("value freed!\n");

    return 0;
}
