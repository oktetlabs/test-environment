/** @file
 * @brief TAPI Job options testing
 *
 * TAPI Job options testing
 *
 * Copyright (C) 2022 OKTET Labs Ltd., St.-Petersburg, Russia
 */

/** @page jobopt-sample A sample of all types of TAPI Job option types
 *
 * @objective Sanity check that options of all types are constructed correctly
 *
 * @par Scenario:
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 */

#define TE_TEST_NAME "jobopt/sample"

#include "te_config.h"
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "te_enum.h"
#include "tapi_job_opt.h"
#include "tapi_test.h"


int
main(int argc, char **argv)
{
    enum sample_enum {
        E1,
        E2,
        E3,
    };
    typedef struct {
        unsigned int uint;
        tapi_job_opt_uint_t opt_uint1;
        tapi_job_opt_uint_t opt_uint2;
        tapi_job_opt_uint_t opt_uint3;
        tapi_job_opt_double_t dbl1;
        tapi_job_opt_double_t dbl2;
        const char *str;
        const char *str2;
        te_bool flag1;
        te_bool flag2;
        size_t n_array;
        const char *array[8];
#ifdef HAVE_NETINET_IN_H
        const struct sockaddr *addr;
#endif
        enum sample_enum e1;
        te_bool e2;
        te_bool e3;
    } data_sample;

    static const te_enum_map enum_mapping[] = {
        {.name = "E1", .value = E1},
        {.name = "E2", .value = E2},
        {.name = "E3", .value = E3},
        TE_ENUM_MAP_END
    };
    static const te_enum_map yesno_mapping[] = {
        {.name = "yes", .value = TRUE},
        {.name = "no", .value = FALSE},
        TE_ENUM_MAP_END
    };

    const tapi_job_opt_bind option_descs[] =
        TAPI_JOB_OPT_SET(
            TAPI_JOB_OPT_UINT("--uint", FALSE, NULL, data_sample, uint),
            TAPI_JOB_OPT_UINT_T("--uint=", TRUE, NULL, data_sample, opt_uint1),
            TAPI_JOB_OPT_UINT_T("--uint=", TRUE, NULL, data_sample, opt_uint2),
            TAPI_JOB_OPT_UINT_T_HEX("-h", FALSE, "h", data_sample, opt_uint3),
            TAPI_JOB_OPT_DOUBLE("-d", FALSE, NULL, data_sample, dbl1),
            TAPI_JOB_OPT_DOUBLE("-d", FALSE, NULL, data_sample, dbl2),
            TAPI_JOB_OPT_STRING("-s", FALSE, data_sample, str),
            TAPI_JOB_OPT_STRING("-s", FALSE, data_sample, str2),
            TAPI_JOB_OPT_BOOL("--flag1", data_sample, flag1),
            TAPI_JOB_OPT_BOOL("--flag2", data_sample, flag2),
            TAPI_JOB_OPT_ARRAY(data_sample, n_array, array,
                               TAPI_JOB_OPT_STRING("--item=", TRUE,
                                                   data_sample, array)),
            TAPI_JOB_OPT_DUMMY("--dummy"),
#ifdef HAVE_NETINET_IN_H
            TAPI_JOB_OPT_SOCKADDR_PTR("--ip", FALSE, data_sample, addr),
            TAPI_JOB_OPT_ADDR_PORT_PTR("--port", FALSE, data_sample, addr),
#endif
            TAPI_JOB_OPT_ENUM("--enum", FALSE, data_sample, e1, enum_mapping),
            TAPI_JOB_OPT_ENUM_BOOL("--", TRUE, data_sample, e2, yesno_mapping),
            TAPI_JOB_OPT_ENUM_BOOL("--", TRUE, data_sample, e3, yesno_mapping)
            );
#ifdef HAVE_NETINET_IN_H
    struct sockaddr_in sample_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8888),
        .sin_addr = {htonl(INADDR_LOOPBACK)}
    };
#endif
    data_sample option_data = {
        .uint = 1,
        .opt_uint1 = TAPI_JOB_OPT_UINT_VAL(2),
        .opt_uint2 = TAPI_JOB_OPT_UINT_UNDEF,
        .opt_uint3 = TAPI_JOB_OPT_UINT_VAL(0xdeadbeef),
        .dbl1 = TAPI_JOB_OPT_DOUBLE_VAL(1.0),
        .dbl2 = TAPI_JOB_OPT_DOUBLE_UNDEF,
        .str = "string",
        .str2 = NULL,
        .flag1 = TRUE,
        .flag2 = FALSE,
        .n_array = 3,
        .array = {
            "value1",
            "value2",
            "value3",
        },
#ifdef HAVE_NETINET_IN_H
        .addr = (const struct sockaddr *)&sample_addr,
#endif
        .e1 = E2,
        .e2 = TRUE,
        .e3 = FALSE
    };
    te_vec result_args = TE_VEC_INIT(char *);
    static const char *expected_strs[] = {
        "sample",
        "--uint", "1",
        "--uint=2",
        "-h", "0xdeadbeefh",
        "-d", "1.000000",
        "-s", "string",
        "--flag1",
        "--item=value1", "--item=value2", "--item=value3",
        "--dummy",
#ifdef HAVE_NETINET_IN_H
        "--ip", "127.0.0.1",
        "--port", "127.0.0.1:8888",
#endif
        "--enum", "E2",
        "--yes", "--no"
    };
    unsigned i;

    TEST_START;

    TEST_STEP("Building the argument vector");
    CHECK_RC(tapi_job_opt_build_args("sample",
                                     option_descs, &option_data,
                                     &result_args));
    TEST_STEP("Checking the vector contains expected items");

    if (te_vec_size(&result_args) != TE_ARRAY_LEN(expected_strs) + 1)
    {
        for (i = 0; i < te_vec_size(&result_args); i++)
            RING("---- %s", TE_VEC_GET(const char *, &result_args, i));

        TEST_VERDICT("Number of constructed arguments is not as expected: "
                     "got %zu, expected %zu",
                     te_vec_size(&result_args),
                     TE_ARRAY_LEN(expected_strs) + 1);
    }

    for (i = 0; i < TE_ARRAY_LEN(expected_strs); i++)
    {
        const char *val = TE_VEC_GET(const char *, &result_args, i);

        if (val == NULL)
            TEST_VERDICT("%u'th argument is NULL", i);

        if (strcmp(val, expected_strs[i]) != 0)
        {
            TEST_VERDICT("%u'th argument differs: got '%s', expected '%s'",
                         i, val, expected_strs[i]);
        }
    }
    if (TE_VEC_GET(const char *, &result_args, i) != NULL)
        TEST_VERDICT("The arguments vector is not properly terminated");

    TEST_SUCCESS;

cleanup:
    te_vec_deep_free(&result_args);

    TEST_END;
}
