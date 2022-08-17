/* SPDX-License-Identifier: Apache-2.0 */
/*
 * TE ASN.1 Library test suite
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page save_to_file1 Usage of save_to_file() function
 *
 * @objective Check that save_to_file() function correctly processes
 *            asn_value with test fields.
 *
 * @par Test sequence:
 * -# Create asn_value;
 * -# for a set of strings do the following:
 *    - write string to asn_value with asn_write_string() function;
 *    - save the result in file;
 *    - parse just created file;
 *    - read string from created asn_value;
 *    - check that obtained string equals to string we put in file.
 *
 */

#include "te_config.h"

#include <stdio.h>
#include <unistd.h>

#include "test_types.h"
#include "ndn.h"
#include "asn_usr.h"
#include "te_errno.h"

const char *test_strings[] = {
    "",
    "aaaa",
    "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"",
    "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\""
    "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\""
    "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"",
};

#define TEST_FAIL(fmt_...) \
    do {                                   \
        fprintf(stderr, fmt_ ); \
    } while (0)

int
main(void)
{
    unsigned int i;
    int          rc;
    int          syms_parsed;
    asn_value   *p;
    asn_value   *ret_p = NULL;

    if ((p = asn_init_value(ndn_data_unit_char_string)) == NULL)
    {
        TEST_FAIL("Cannot create \"char string\" DATA UNIT\n");
        return 1;
    }
    for (i = 0; i < sizeof(test_strings) / sizeof(test_strings[0]); i++)
    {
        char *new_str;

        if ((rc = asn_write_string(p, test_strings[i], "#plain")) != 0)
        {
            TEST_FAIL("Cannot write string into asn_value %s\n",
                      te_rc_err2str(rc));
            return 2;
        }

        if ((rc = asn_save_to_file(p, "save_to_file1.asn")) != 0)
        {
            TEST_FAIL("Cannot save asn value into file %s\n",
                      te_rc_err2str(rc));
            return 3;
        }

        /* Now let's parse file */
        rc = asn_parse_dvalue_in_file("save_to_file1.asn",
                                      ndn_data_unit_char_string,
                                      &ret_p, &syms_parsed);
        if (rc != 0)
        {
            TEST_FAIL("Iter %d, string %s; Cannot parse file %s\n",
                      i, test_strings[i], te_rc_err2str(rc));
            return 4;
        }

        /* Now we should compare new and original strings */
        if ((rc = asn_read_string(ret_p, &new_str, "#plain")) != 0)
        {
            TEST_FAIL("Cannot read string from asn_value %s\n",
                      te_rc_err2str(rc));
            return 5;
        }

        if (strcmp(test_strings[i], new_str) != 0)
        {
            TEST_FAIL("Original ('%s') and new ('%s') strings "
                      "are different\n", test_strings[i], new_str);
            return 6;
        }

        free(new_str);
    }

    if (unlink("save_to_file1.asn") != 0)
    {
        TEST_FAIL("Failed to unlink save_to_file1.asn: errno=%d\n", errno);
        return 7;
    }

    return 0;
}
