#include <stdio.h>

#include "asn_impl.h"



asn_named_entry_t my_entry_array [] = {
    { "number", &asn_base_integer },
    { "string", &asn_base_charstring }
};

asn_type my_sequence = {
    "MySequence",
    {APPLICATION, 1},
    SEQUENCE,
    2,
    {&my_entry_array}
};


char buffer [1000];

int 
main (void)
{
    asn_value_p my_val = asn_init_value(&asn_base_integer);
    int a = 10;

    asn_write_value_field(my_val, &a, sizeof(a), "");

    asn_sprint_value(my_val, buffer, sizeof(buffer), 0);

    printf ("val: %s .\n", buffer);

    {
        int r;
        asn_value_p copy;
        char new_str[] = "My beautiful string for testing ... ";

        asn_value test_int_value = 
        {
            &asn_base_integer, 
            {UNIVERSAL, 2},
            INTEGER,
            NULL,
            1, 
            {10 /* value itself */}
        }; 
        asn_value test_str = 
        {
            &asn_base_charstring, 
            {UNIVERSAL, 28},
            CHAR_STRING,
            NULL,
            4, 
            {"test"/* value itself */}
        };
        asn_value_p val_arr[] = {&test_int_value, &test_str};

        asn_value seq_val = { &my_sequence, {APPLICATION, 1}, SEQUENCE,
        "seq-value", 2, {&val_arr} };

        seq_val.data.array[0]->name = my_sequence.sp.named_entries[0].name;
        seq_val.data.array[1]->name = my_sequence.sp.named_entries[1].name; 
        seq_val.txt_len = -1;

        copy = asn_copy_value (&seq_val);
        if (copy)
        {
            r = asn_sprint_value(copy, buffer, 1000, 0);
            printf ("copy : \n\"%s\", ret val = %d\n", buffer, r);

            r = asn_count_txt_len(copy, 0);
            printf ("count len: %d\n", r);

        }
        else 
            return 1; 

        a = 15;
        r = asn_write_value_field(copy, &a, sizeof(a), "number");
        if (r) { printf ("error code returned: %d\n", r); return r; }

        r = asn_write_value_field(copy, new_str, sizeof(new_str), "string");
        if (r) { printf ("error code returned: %d\n", r); return r; }

        r = asn_sprint_value(copy, buffer, 1000, 0);
        printf ("copy after write value to %d: \n\"%s\"\nlen: %d\n", 
                    a, buffer, r); 
        r = asn_count_txt_len(copy, 0);
        printf ("count len: %d\n", r);
    }

    return 0;
}
