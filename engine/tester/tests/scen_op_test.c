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

#ifdef ERROR
#undef ERROR
#endif
#define ERROR(args...) DBG_MSG("ERROR: " args);

#ifdef RING
#undef RING
#endif
#define RING(args...) DBG_MSG("RING: " args);

#ifdef VERB
#undef VERB
#endif
#if 0
#define VERB(args...) DBG_MSG("VERB: " args);
#else
#define VERB(args...)
#endif

typedef enum {
    /* operation codes */
    TESTING_ACT_OR,
    TESTING_ACT_SUBTRACT,
    TESTING_ACT_EXCLUDE,
} testing_act_op;

static const char *op_names[] = {"OR", "SUB", "EX"};

static char *flag_names[FLAG_NAMES_TABLE_SIZE];
static int   known_flags = 0;

extern te_errno testing_scenarios_op(testing_scenario *h0,
                                     testing_scenario *h1,
                                     testing_scenario *h_rslt_p,
                                     testing_act_op op_code);

/*
 * Parse flags in string representation (like "flag1+flag2+flag3...")
 * 
 */
unsigned int
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

int
sprint_flags(char *buf, int size, unsigned int flags)
{
    char *ptr = buf;
    int   flag_no;

    for (flag_no = 0; flag_no < known_flags; flag_no++)
    {
        if ((flags & 0x1) != 0)
        ptr += snprintf(ptr, size - (ptr - buf), "%s", flag_names[flag_no]);
        if ((flags >>= 1) != 0)
            ptr += snprintf(ptr, size - (ptr - buf), "+");
    }

    return ptr - buf;
}

#define FLAGS_TO_STR_BUF_SIZE   1024

const char *
flags_to_str(unsigned int flags)
{
    static char flags_str_buf[FLAGS_TO_STR_BUF_SIZE];
    
    sprint_flags(flags_str_buf, FLAGS_TO_STR_BUF_SIZE, flags);

    return flags_str_buf;
}

int
sprint_testing_act(char *buf, int size, const testing_act *act)
{
    char *ptr = buf;

    ptr += snprintf(ptr, size - (ptr - buf), "[%d,%d,", act->first, act->last);

    ptr += sprint_flags(ptr, size - (ptr - buf), act->flags);

    ptr += snprintf(ptr, size - (ptr - buf), "]");

    return (ptr - buf);
}

#define SCENARIO_ACT_TO_STR_BUF_SIZE    1024

const char *
testing_act_to_str(const testing_act *act)
{
    static char scenario_act_buf[SCENARIO_ACT_TO_STR_BUF_SIZE];

    sprint_testing_act(scenario_act_buf, SCENARIO_ACT_TO_STR_BUF_SIZE, act);

    return scenario_act_buf;
}

int
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

#define SCENARIO_TO_STR_BUF_SIZE   4096

const char *
testing_scenario_to_str(const testing_scenario *scenario)
{
    static char scenario_str_buf[SCENARIO_TO_STR_BUF_SIZE];

    sprint_testing_scenario(scenario_str_buf,
                            SCENARIO_TO_STR_BUF_SIZE, scenario);

    return scenario_str_buf;
}



char *
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


char *
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

int
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

int
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

    RING("First scenario: %s", scenario_to_str(&ts1));
    RING("Second scenario: %s", scenario_to_str(&ts2));
    RING("Operation: %s", op_names[op]);
    RING("Expected result: %s", scenario_to_str(&ts_expected));

    if ((err = testing_scenarios_op(&ts1, &ts2, &ts_result, op)) != 0)
    {
        ERROR("testing_scenarios_op() failed, errno=%d(0x%x)", err, err);
        return -1;
    }

    RING("Calculated result: %s", scenario_to_str(&ts_result));

    if (compare_scenarios(&ts_expected, &ts_result) != 0)
    {
        ERROR("Operation results differs from the expected one");
        return -1;
    }

    return 0;
}
