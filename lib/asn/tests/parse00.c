/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved. */

#include "te_config.h"

#include <stdio.h>

#include "asn_usr.h"
#include "ndn.h"
#include "logger_api.h"

char buffer [10000];

int
main (int argc, char *argv[])
{
    int rc;
    int oid_vals[100];
    size_t oid_len = sizeof(oid_vals)/sizeof(oid_vals[0]);
    int s_parsed;
    asn_value *new_val;

    te_log_init("parse00", te_log_message_file);

    if (argc < 2)
        return 0;

    rc = asn_parse_dvalue_in_file(argv[1],
                            ndn_traffic_template, &new_val, &s_parsed);
    printf ("ret code from parse sequence: %6x, syms: %d\n", rc, s_parsed);
    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer);


    rc = asn_read_value_field(new_val, oid_vals, &oid_len,
                              "pdus.0.#snmp.variable-bindings.0.name.#plain");

    printf("read name rc 0x%X, oid_len %d\n", rc, oid_len);

    if (rc == 0)
    {
        int i;
        printf ("OID: ");
        for (i = 0; i < oid_len; i++)
            printf(".%d", oid_vals[i]);
        printf ("\n");
    }

    asn_free_value (new_val);

    return 0;
}
