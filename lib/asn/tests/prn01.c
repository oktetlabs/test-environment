#include <stdio.h>

#include "asn_impl.h"

#define FIRST_TEST 0

extern int asn_sprint_integer(asn_value_p value, char *buffer, int buf_len);

asn_type base_integer = 
{
    "INTEGER",
    {UNIVERSAL, 2},
    INTEGER,
    1, /* len */
    {NULL}  /* sp  */
};

asn_type base_char_string = 
{
    "UniversalString",
    {UNIVERSAL, 28},
    CHAR_STRING,
    1, /* len */
    {NULL}  /* sp  */
};

asn_named_entry_t my_entry_array [] = {
    { "number", &base_integer },
    { "string", &base_char_string }
};

asn_type my_sequence = {
    "MySequence",
    {APPLICATION, 1},
    SEQUENCE,
    2,
    {&my_entry_array}
};


asn_type my_choice = {
    "MyChoice",
    {APPLICATION, 2},
    CHOICE,
    2,
    {&my_entry_array}
};


char buffer [1000];

int 
main (void)
{
    int r;
    int a;
    char str[] = "ooooooooo my";
#if FIRST_TEST
    asn_value test_int_value = 
    {
        &base_integer, 
        {UNIVERSAL, 2},
        INTEGER,
        NULL,
        1, 
        {10 /* value itself */}
    }; 
    asn_value test_str = 
    {
        &base_char_string, 
        {UNIVERSAL, 28},
        CHAR_STRING,
        NULL,
        4, 
        {"test"/* value itself */}
    };
    asn_value_p val_arr[] = {&test_int_value, &test_str};

    asn_value seq_val = 
    {
        &my_sequence, 
        {APPLICATION, 1},
        SEQUENCE,
        "seq-value",
        2,
        {&val_arr}
    };
#endif
    asn_value_p ch_val;

#if FIRST_TEST
    seq_val.data.array[0]->name = my_sequence.sp.named_entries[0].name;
    seq_val.data.array[1]->name = my_sequence.sp.named_entries[1].name;

#if 0
    r = asn_sprint_value(&test_int_value, buffer, 1000, 0);
    printf ("printed int value: \"%s\", ret val = %d\n", buffer, r);
    r = asn_sprint_value(&test_str, buffer, 1000, 0);
    printf ("printed string value: \"%s\", ret val = %d\n", buffer, r);
#endif
    r = asn_sprint_value(&seq_val, buffer, 1000, 0);
    printf ("printed string value: \"%s\", ret val = %d\n", buffer, r);
#endif

    a = 22;
    ch_val = asn_init_value(&my_choice);
    r = asn_write_value_field(ch_val, &a, sizeof(a), "number");

    asn_sprint_value(ch_val, buffer, 1000, 0);
    printf ("printed choice value: \"%s\", ret val of write_value = %8x\n", 
             buffer, r);

    r = asn_write_value_field(ch_val, &str, sizeof(str), "string");
    asn_sprint_value(ch_val, buffer, 1000, 0);
    printf ("printed choice value: \"%s\", ret val of write_value = %8x\n", 
             buffer, r);

    a = 33;
    r = asn_write_value_field(ch_val, &a, sizeof(a), "number");
    asn_sprint_value(ch_val, buffer, 1000, 0);
    printf ("printed choice value: \"%s\", ret val of write_value = %8x\n", 
             buffer, r);

    return 0;
}
