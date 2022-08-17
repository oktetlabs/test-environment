/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2005-2022 OKTET Labs Ltd. All rights reserved. */

#include "te_config.h"

#include <stdio.h>

#include "test_types.h"




char buffer [1000];

#define COMPON_TEST 1

#define BUF_TO_READ 100
char buf_to_read[BUF_TO_READ];

int
main (void)
{
    asn_value *seq_val   = asn_init_value(&at_plain_seq1);
    asn_value *ch_val   = asn_init_value(&at_plain_choice1);

    asn_value *subval;

    int a = 1981;
    char str[] = "uura..";
    int r;
#if 0
    int read_len = BUF_TO_READ;
    char str2[] = "sss.... lslsls";
    int b = 1234;
#endif

    r = asn_write_value_field(seq_val, &a, sizeof(a), "number");
    if (r) { printf("write numbver error: %d\n", r); return 1; }


    r = asn_get_child_value(seq_val, (const asn_value **)&subval,
                            PRIVATE, SEQ_NUMBER_TAG);
    printf("rc getting subval by tag 'number': %d\n", r);


    r = asn_get_child_value(seq_val, (const asn_value **)&subval,
                            PRIVATE, SEQ_STRING_TAG);
    printf("rc getting subval by tag 'string': %d\n", r);

    r = asn_write_value_field(seq_val, str, sizeof(str), "string");
    if (r) { printf("error code returned: %d\n", r); return r; }

    r = asn_sprint_value(seq_val, buffer, 1000, 0);
    printf("seq_val after write values: \n\"%s\"\n", buffer);

    r = asn_get_child_value(seq_val, (const asn_value **)&subval,
                            PRIVATE, SEQ_NUMBER_TAG);
    printf("rc getting subval by tag 'number': %d\n", r);

    r = asn_get_child_value(seq_val, (const asn_value **)&subval,
                            PRIVATE, SEQ_INT_ARRAY_TAG);
    printf("rc getting subval by tag 'ingeter_array': %d\n", r);

    r = asn_write_value_field(ch_val, &a, sizeof(a), "#number");
    if (r) { printf("write number to choice error: %d\n", r); return 1; }

    subval = NULL;
    r = asn_get_choice_value(ch_val, &subval, NULL, NULL);
    printf("rc getting choice subval: %d\n", r);

    if (r == 0 && subval != NULL)
        printf("got subval syntax: %d\n", subval->syntax);

    return 0;
}
