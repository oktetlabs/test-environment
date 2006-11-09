/** @file
 * @brief Tester Engine scenario operaions test
 *
 * Test Example
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author  Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 */
#include "te_config.h"

#include "logger_api.h"
#include "tester_run.h"

DEFINE_LGR_ENTITY("scen_op_test");

#define SCEN_OP_BUF_SIZE        4096
#define FLAGS_BUF_SIZE          256
#define FLAG_NAMES_TABLE_SIZE   32

#define DBG_MSG(args...) \
    do {                        \
        fprintf(stdout, args);  \
        fprintf(stdout, "\n");  \
    } while (0)

#define DBG_ERR(args...) \
    do {                        \
        fprintf(stderr, args);  \
        fprintf(stderr, "\n");  \
    } while (0)

#ifdef ERROR
#undef ERROR
#endif
#define ERROR(args...) DBG_ERR("  ERROR: " args);

#ifdef WARN
#undef WARN
#endif
#define WARN(args...) DBG_ERR("  WARN: " args);

#ifdef RING
#undef RING
#endif
#define RING(args...) DBG_MSG("  RING: " args);

#ifdef VERB
#undef VERB
#endif
#if 0
#define VERB(args...) DBG_MSG("  VERB: " args);
#else
#define VERB(args...)
#endif

typedef enum {
    /* operation codes */
    TESTING_ACT_OR,
    TESTING_ACT_SUBTRACT,
    TESTING_ACT_EXCLUDE,
} testing_act_op;

/* String representation of supported operations */
static const char *op_names[] = {"OR", "SUB", "EX"};

/* Flags naming table */
static char *flag_names[FLAG_NAMES_TABLE_SIZE];

/* Amount of known flags */
static int   known_flags = 0;

/* see the description in scenario.c */
extern te_errno testing_scenarios_op(testing_scenario *h0,
                                     testing_scenario *h1,
                                     testing_scenario *h_rslt_p,
                                     testing_act_op op_code);


/**
 * Parse flags in string representation (like "flag1+flag2+flag3...")
 * and produce uint32_t result.
 *
 * If unknown flag is encounted, this flag is added to the known flags table
 *
 * @param buf           String buffer with flags to parse
 *
 * @return bit-field corresponding to the flags described in the buffer
 */
static unsigned int
parse_flags(const char *buf)
{
    char         *ptr = (char *)buf;
    char         *next;
    int           flag_no;
    unsigned int  flags = 0;
    int           len;

    while (ptr && (*ptr != '\0'))
    {
        VERB("Parse flags: \"%s\"", ptr);

        /* remove trailing spaces */
        while (*ptr == ' ')
            ptr++;

        next = strchr(ptr, '+');
        len = (next == NULL) ? (int)strlen(ptr) : (next - ptr);
        VERB("len=%d", len);

        if ((len == 1) && (*ptr == '0'))
        {
            ptr = (next == NULL) ? NULL : next + 1;
            continue;
        }
        
        for (flag_no = 0; flag_no < known_flags; flag_no++)
        {
            if (strncmp(flag_names[flag_no], ptr, len) == 0)
            {
                break;
            }
        }
        
        if (flag_no >= known_flags)
        {
            /* Found new flag */
            if (flag_no >= FLAG_NAMES_TABLE_SIZE)
            {
                ptr = (next == NULL) ? NULL : next + 1;
                continue;
            }

            /* Add flag to flags table */
            known_flags++;

            /* remove trailing spaces */
            while (ptr[len - 1] == ' ')
                len--;

            flag_names[flag_no] = (char *)malloc(len + 1);
            memcpy(flag_names[flag_no], ptr, len);
            flag_names[flag_no][len] = '\0';

            RING("New flag: '%s' = 0x%08x",
                 flag_names[flag_no], (1 << flag_no));
        }

        flags |= (1 << flag_no);
        ptr = (next == NULL) ? NULL : next + 1;
    }

    return flags;
}

/**
 * Convert flags bit-field to the string representation
 * (only known flags are printed)
 *
 * @param buf           Storage buffer to print to
 * @param size          Storage buffer size
 * @param flags         Flags bit-field to print
 *
 * @return amount of symbols printed to the storage buffer
 */
static int
sprint_flags(char *buf, int size, unsigned int flags)
{
    char *ptr = buf;
    int   flag_no;

    if (flags == 0)
    {
        ptr += snprintf(ptr, size - (ptr - buf), "0");
        return ptr - buf;
    }

    for (flag_no = 0; flag_no < known_flags; flag_no++)
    {
        if ((flags & 0x1) != 0)
        {
            ptr += snprintf(ptr, size - (ptr - buf), "%s",
                            flag_names[flag_no]);
            if ((flags >> 1) != 0)
                ptr += snprintf(ptr, size - (ptr - buf), "+");
        }
        flags >>= 1;
    }

    return ptr - buf;
}

/** Local storage buffer for printing flags values */
#define FLAGS_TO_STR_BUF_SIZE   1024

/**
 * sprint flags bit-field in the string representation to the local
 * buffer and return it
 *
 * @param flags         Flags bit-field to print
 *
 * @return local buffer with printed flags bit-field
 */
static const char *
flags_to_str(unsigned int flags)
{
    static char flags_str_buf[FLAGS_TO_STR_BUF_SIZE];
    
    sprint_flags(flags_str_buf, FLAGS_TO_STR_BUF_SIZE, flags);

    return flags_str_buf;
}

/**
 * Convert Testing Act structure to the string representation
 *
 * @param buf           Storage buffer to print to
 * @param size          Storage buffer size
 * @param act           Testing Act structure to print
 *
 * @return amount of symbols printed to the storage buffer
 */
static int
sprint_testing_act(char *buf, int size, const testing_act *act)
{
    char *ptr = buf;

    ptr += snprintf(ptr, size - (ptr - buf), "[%d,%d,", act->first, act->last);

    ptr += sprint_flags(ptr, size - (ptr - buf), act->flags);

    ptr += snprintf(ptr, size - (ptr - buf), "]");

    return (ptr - buf);
}

/** Local storage buffer for printing Testing Act structures */
#define SCENARIO_ACT_TO_STR_BUF_SIZE    1024

/**
 * sprint Testing Act structure in the string representation to the local
 * buffer and return it.
 *
 * @param act           Testing Act structure to print
 *
 * @return local buffer with printed flags bit-field
 */
static const char *
testing_act_to_str(const testing_act *act)
{
    static char scenario_act_buf[SCENARIO_ACT_TO_STR_BUF_SIZE];

    sprint_testing_act(scenario_act_buf, SCENARIO_ACT_TO_STR_BUF_SIZE, act);

    return scenario_act_buf;
}

/**
 * Convert Testing Scenario structure to the string representation
 *
 * @param buf           Storage buffer to print to
 * @param size          Storage buffer size
 * @param scenario      Testing Scenario to print
 *
 * @return amount of symbols printed to the storage buffer
 */
static int
sprint_testing_scenario(char *buf, int size, const testing_scenario *scenario)
{
    char *ptr = buf;
    const testing_act  *act;

    ptr += snprintf(ptr, size - (ptr - buf), "[");

    TAILQ_FOREACH(act, scenario, links)
    {
        ptr += sprint_testing_act(ptr, size - (ptr - buf), act);
        ptr += snprintf(ptr, size - (ptr - buf), ",");
    }

    /* remove last ',' */
    if (ptr - buf > 1)
        ptr--;

    ptr += snprintf(ptr, size - (ptr - buf), "]");

    return ptr - buf;
}

/** Local storage buffer for printing Testing Scenario list */
#define SCENARIO_TO_STR_BUF_SIZE   4096

/**
 * sprint Testing Act structure in the string representation to the local
 * buffer and return it.
 *
 * @param act           Testing Act structure to print
 *
 * @return local buffer with printed flags bit-field
 */
static const char *
testing_scenario_to_str(const testing_scenario *scenario)
{
    static char scenario_str_buf[SCENARIO_TO_STR_BUF_SIZE];

    sprint_testing_scenario(scenario_str_buf,
                            SCENARIO_TO_STR_BUF_SIZE, scenario);

    return scenario_str_buf;
}

/**
 * Parse Testing Act structure from the string representation
 *
 * @param buf   String buffer to parse
 * @param ts    Testing Scenario sequence to add parsed Testing Act to
 *
 * @return pointer to the first unparsed symbol
 *         (next to Testing Act description)
 */
static char *
parse_scenario_act(const char *buf, testing_scenario *ts)
{
    char *ptr = (char *)buf;
    char *first_ptr;
    char *last_ptr;
    char *flags_ptr;

    unsigned int    first;
    unsigned int    last;
    unsigned int    flags;
    int             flags_len;
    
    char  flags_buf[FLAGS_BUF_SIZE];

    VERB("Parse scenario act: \"%s\"", buf);

    if (*buf != '[')
    {
        ERROR("Invalid scenario syntax");
        return NULL;
    }

    first_ptr = ptr + 1;
    VERB("first_ptr=\"%s\"", first_ptr);
    last_ptr = strchr(first_ptr, ',');
    if (last_ptr == NULL)
        return NULL;
    last_ptr++;
    VERB("last_ptr=\"%s\"", last_ptr);
    flags_ptr = strchr(last_ptr, ',');
    if (flags_ptr == NULL)
        return NULL;
    flags_ptr++;
    VERB("flags_ptr=\"%s\"", flags_ptr);
    ptr = strchr(flags_ptr, ']');
    if (ptr == NULL)
        return NULL;

    flags_len = ptr - flags_ptr;
    if (flags_len > FLAGS_BUF_SIZE - 1)
        return NULL;

    ptr++;

    memcpy(flags_buf, flags_ptr, flags_len);
    flags_buf[flags_len] = '\0';

    if (sscanf(first_ptr, "%u", &first) != 1)
        return NULL;
        
    if (sscanf(last_ptr, "%u", &last) != 1)
        return NULL;

    flags = parse_flags(flags_buf);

    VERB("Parsed scenario act: first=%u, last=%u, flags=0x%x",
         first, last, flags);

    scenario_add_act(ts, first, last, flags);
    
    return ptr;    
}

/**
 * Parse Testing Scenario sequence from the string representation
 *
 * @param buf   String buffer to parse
 * @param ts    Uninitialised Testing Scenario sequence
 *              to fill with parsed data
 *
 * @return pointer to the first unparsed symbol
 *         (next to Testing Scenario sequence description)
 */
static char *
parse_scenario(const char *buf, testing_scenario *ts)
{
    char *ptr;

    TAILQ_INIT(ts);

    VERB("Parse scenario: \"%s\"", buf);

    if (*buf != '[')
    {
        ERROR("Invalid scenario syntax, '[' symbol expected");
        return NULL;
    }
    ptr = (char *)buf + 1;

    while (*ptr == '[')
    {
        char *prev = ptr;
        ptr = parse_scenario_act(ptr, ts);
        if (ptr == NULL)
        {
            ERROR("Failed to parse test scenario act, pos=%d: \"%s\"",
                  prev - buf, prev);
            return NULL;
        }
        if (*ptr == ',')
            ptr++;
    }
    if (*ptr != ']')
        return NULL;

    return ++ptr;
}

/**
 * Parse test input data: 2 testing scenario sequences, operation code and
 * expected testing scenario sequence produced by this operation.
 * Format of the input data:
 *   <scenario1> <op> <scenario2> = <expected_scenario>
 * Format of the scenario data:
 *   [<act1>,<act2>,...]
 * Format of the testing act data:
 *   [first,last,<flag1>+<flag2>+...]
 * If there are no flags, then use 0 instead of flags field ([fist,last,0])
 * Supported operations are: 
 *   'or'  - OR operation
 *   'sub' - Subtrate operation
 *   'ex'  - Exclude operation
 *
 * @param buf   String buffer with input data to parse
 * @param ts1   Parsed first Testing Scenario sequence involved in operation
 * @param ts2   Parsed second Testing Scenario sequence involved in operation
 * @param ts_r  Parsed Testing Scenario sequence that is expected
 *              as an operation result
 * @param op    Parsed operation code
 *
 * @return 0, on success, -1 on failure
 */
static int
parse_input(const char *buf,
            testing_scenario *ts1,
            testing_scenario *ts2,
            testing_scenario *ts_r,
            int *op)
{
    char *ptr = (char *)buf;
    char *ts1_ptr = (char *)buf;
    char *ts2_ptr = NULL;
    char *ts_r_ptr = NULL;
    char *op_ptr = NULL;
    
    if ((ptr = parse_scenario(ts1_ptr, ts1)) == NULL)
    {
        ERROR("Failed to parse the first scenario");
        return -1;
    }
    if ((op_ptr = strstr(ptr, "or")) != NULL)
    {
        *op = TESTING_ACT_OR;
    }
    else if ((op_ptr = strstr(ptr, "sub")) != NULL)
    {
        *op = TESTING_ACT_SUBTRACT;
    }
    else if ((op_ptr = strstr(ptr, "ex")) != NULL)
    {
        *op = TESTING_ACT_EXCLUDE;
    }
    else
    {
        ERROR("Failed to find the operation symbol");
        return -1;
    }
    
    if ((ts2_ptr = strchr(op_ptr, '[')) == NULL)
    {
        ERROR("Failed to find the second scenario");
        return -1;
    }

    if ((ptr = parse_scenario(ts2_ptr, ts2)) == NULL)
    {
        ERROR("Failed to parse the second scenario");
        return -1;
    }

    if ((ptr = strchr(ptr, '=')) == NULL)
    {
        ERROR("Failed to find the '=' symbol");
        return -1;
    }

    if ((ts_r_ptr = strchr(ptr, '[')) == NULL)
    {
        ERROR("Failed to find the expected result scenario");
        return -1;
    }

    if ((ptr = parse_scenario(ts_r_ptr, ts_r)) == NULL)
    {
        ERROR("Failed to parse the expected result scenario");
        return -1;
    }

    return 0;    
}

/**
 * Compare two testing scenario sequences, and print the difference
 *
 * @param ts1   First Testing Scenario sequence to compare
 * @param ts2   Sarsed Testing Scenario sequence to compare
 *
 * @return 0, if equal, -1 if not
 */
static int
compare_scenarios(const testing_scenario *ts1, const testing_scenario *ts2)
{
    int          act_no = 0;
    
    char         act1_str[SCENARIO_ACT_TO_STR_BUF_SIZE];
    char         act2_str[SCENARIO_ACT_TO_STR_BUF_SIZE];

    testing_act *act1;
    testing_act *act2;

    act1 = TAILQ_FIRST(ts1);
    act2 = TAILQ_FIRST(ts2);
    
    while ((act1 != NULL) && (act2 != NULL))
    {
        if ((act1->first != act2->first) ||
            (act1->last != act2->last) ||
            (act1->flags != act2->flags))
        {
            sprint_testing_act(act1_str, SCENARIO_ACT_TO_STR_BUF_SIZE, act1);
            sprint_testing_act(act2_str, SCENARIO_ACT_TO_STR_BUF_SIZE, act2);
            ERROR("Mismatching act #%d in scenarios: %s != %s",
                  act_no + 1, act1_str, act2_str);
            return -1;
        }

        act1 = TAILQ_NEXT(act1, links);
        act2 = TAILQ_NEXT(act2, links);

        act_no++;
    }
    if ((act1 != NULL) && (act2 == NULL))
    {
        sprint_testing_act(act1_str, SCENARIO_ACT_TO_STR_BUF_SIZE, act1);
        ERROR("Mismatching act #%d in scenarios: %s != (nil)",
              act_no + 1, act1_str);
        return -1;
    }
    if ((act1 == NULL) && (act2 != NULL))
    {
        sprint_testing_act(act2_str, SCENARIO_ACT_TO_STR_BUF_SIZE, act1);
        ERROR("Mismatching act #%d in scenarios: (nil) != %s",
              act_no + 1, act2_str);
        return -1;
    }
    
    return 0;
}

/**
 * Main test body.
 *
 * @return 0, if success, -1 if not
 */
int
main(void)
{
    char scen_buf[SCEN_OP_BUF_SIZE];

    testing_scenario ts1;
    testing_scenario ts2;
    testing_scenario ts_expected;
    testing_scenario ts_result;
    int              op;
    te_errno         err;

    fgets(scen_buf, SCEN_OP_BUF_SIZE, stdin);
    if (parse_input(scen_buf, &ts1, &ts2, &ts_expected, &op) < 0)
    {
        ERROR("Invalid syntax format of input scenario data");
        return -1;
    }

    RING("First scenario: %s", testing_scenario_to_str(&ts1));
    RING("Second scenario: %s", testing_scenario_to_str(&ts2));
    RING("Operation: %s", op_names[op]);
    RING("Expected result: %s", testing_scenario_to_str(&ts_expected));

    if ((err = testing_scenarios_op(&ts1, &ts2, &ts_result, op)) != 0)
    {
        ERROR("testing_scenarios_op() failed, errno=%d(0x%x)", err, err);
        return -1;
    }

    RING("Calculated result: %s", testing_scenario_to_str(&ts_result));

    if (compare_scenarios(&ts_expected, &ts_result) != 0)
    {
        ERROR("Operation results differs from the expected one");
        return -1;
    }

    return 0;
}
