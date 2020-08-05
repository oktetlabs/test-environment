/** @file
 * @brief TE project. Logger subsystem.
 *
 * Logger configuration file parser.
 * No schema checking is carried out.
 *
 * Copyright (C) 2003-2020 OKTET Labs. All rights reserved.
 *
 * @author Igor B. Vasiliev <Igor.Vasiiev@oktetlabs.ru>
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#define TE_LGR_USER "Cfg file"

#include "logger_internal.h"

#if HAVE_STDDEF_H
#include <stddef.h>
#endif

#define TE_EXPAND_XML 1
#include "te_expand.h"

#include "rcf_common.h"
#include <pthread.h>
#include <semaphore.h>
#include "te_alloc.h"
#include "te_str.h"
#include "te_kernel_log.h"

#include <yaml.h>

/* TA single linked list */
extern ta_inst_list ta_list;

/* Cpature logs polling settings */
extern snif_polling_sets_t snifp_sets;

/* Path to the directory for logs */
extern const char *te_log_dir;

static te_bool   thread_parsed = FALSE;

/** TA configuration rule */
typedef struct ta_rule {
    char    *name;    /**< Agent name, type regex check is used */
    regex_t  type;    /**< Regex for agent type */
    int      polling; /**< Polling setting for this agent */
} ta_rule;

/** A set of TA configuration rules */
struct ta_cfg {
    ta_rule *rules;           /**< Array of rules */
    int      rules_num;       /**< Size of the rules array */
    int      polling_default; /**< Default polling setting */
} ta_cfg;

/*
 * Context that will be passed to the threads
 * specified in the config file.
 */
typedef struct thread_context {
    int         argc;
    char       *argv[RCF_MAX_PARAMS + 1];
    const char *thread_name;
    sem_t       args_processed;
} thread_context;

/** Remove the currently accumulated config rules */
static void
config_clear()
{
    ta_rule *rule;

    for (rule = ta_cfg.rules; rule != ta_cfg.rules + ta_cfg.rules_num; rule++)
    {
        if (rule->name != NULL)
            free(rule->name);
        else
            regfree(&rule->type);
    }
    ta_cfg.rules_num = 0;
}

/**
 * Initialize the rule for a specific agent.
 *
 * @param rule      configuration rule
 * @param name      agent name
 * @param polling   polling setting for this agent
 *
 * @return  Status information
 *
 * @retval   0          Success.
 * @retval   Negative   Failure.
 */
static int
rule_init_agent(ta_rule *rule, const char *name, int polling)
{
    rule->name = strdup(name);
    if (rule->name == NULL)
    {
        ERROR("Failed to allocate memory for TA name");
        return -1;
    }
    rule->polling = polling;

    return 0;
}

/**
 * Initialize the rule for agents of specific type.
 *
 * @param rule      configuration rule
 * @param type      agent type regex
 * @param polling   polling setting for this agent
 *
 * @return  Status information
 *
 * @retval   0          Success.
 * @retval   Negative   Failure.
 */
static int
rule_init_type(ta_rule *rule, const char *type, int polling)
{
    rule->name = NULL;
    if (regcomp(&rule->type, type, REG_EXTENDED) != 0)
    {
        ERROR("Polling: RegExpr compilation failure\n");
        return -1;
    }
    rule->polling = polling;

    return 0;
}

/**
 * Logger thread wrapper.
 *
 * @param arg   thread context
 *
 * @return NULL
 */
static void *
logger_thread_wrapper(void *arg)
{
    int             rc;
    thread_context *ctx = (thread_context *)arg;

    if (strcmp(ctx->thread_name, "log_serial") == 0)
    {
        rc = log_serial(&ctx->args_processed, ctx->argc, ctx->argv);
    }
    else
    {
        ERROR("Unknown thread %s", ctx->thread_name);
        sem_post(&ctx->args_processed);
        return NULL;
    }

    if (rc != 0)
    {
        ERROR("%s() failed", ctx->thread_name);
        sem_post(&ctx->args_processed);
    }

    return NULL;
}

/*************************************************************************/
/*           XML                                                         */
/*************************************************************************/

static ta_rule *
allocate_rule()
{
    ta_rule *old = ta_cfg.rules;

    ta_cfg.rules = realloc(ta_cfg.rules,
                           (ta_cfg.rules_num + 1) * sizeof(ta_rule));
    if (ta_cfg.rules == NULL)
    {
        ta_cfg.rules = old;
        ERROR("Failed to allocate memory for a new TA configuration rule");
        return NULL;
    }

    return &ta_cfg.rules[ta_cfg.rules_num];
}

/**
 * User call back called when an opening tag has been processed.
 *
 * @param thread_ctx  XML parser context (thread_context in this case)
 * @param xml_name    Element name
 * @param xml_atts    Array with element attributes
 */
static void
startElementLGR(void           *thread_ctx,
                const xmlChar  *xml_name,
                const xmlChar **xml_atts)
{
    int             ret;
    int             value;
    te_errno        rc;
    const char     *name = (const char *)xml_name;
    const char    **atts = (const char **)xml_atts;
    thread_context *ctx = thread_ctx;
    ta_rule        *new;

    if ((atts == NULL) || (atts[0] == NULL) || (atts[1] == NULL))
        return;

    if (!strcmp(name, "polling") &&
        (atts != NULL) &&
        (atts[0] != NULL) &&
        (atts[1] != NULL))
    {
        /* Get default polling value and assign it to the all TA */
        if (strcmp(atts[0], "default") == 0)
        {
            rc = te_strtoi(atts[1], 0, &ta_cfg.polling_default);
            if (rc != 0)
                ERROR("Invalid polling value %s: %r", atts[1], rc);
            config_clear();
        }
    }
    else if (!strcmp(name, "type") &&
             (atts != NULL) &&
             (atts[0] != NULL) &&
             (atts[1] != NULL) &&
             (atts[2] != NULL) &&
             (atts[3] != NULL))
    {
        /*
         * Get polling value for separate TA type
         * and assign it to the appropriate TA
         */
        if (!strcmp(atts[0], "type") &&
            !strcmp(atts[2], "value"))
        {
            rc = te_strtoi(atts[3], 0, &value);
            if (rc != 0)
            {
                ERROR("Invalid polling value %s: %r", atts[3], rc);
                return;
            }

            new = allocate_rule();
            if (new == NULL)
                return;
            ret = rule_init_type(new, atts[1], value);
            if (ret == 0)
                ta_cfg.rules_num++;
        }
    }
    else if (!strcmp(name, "agent") &&
             (atts != NULL) &&
             (atts[0] != NULL) &&
             (atts[1] != NULL) &&
             (atts[2] != NULL) &&
             (atts[3] != NULL))
    {
        /* Get polling value for separate TA and assign it */
        if (!strcmp(atts[0], "agent") &&
            !strcmp(atts[2], "value"))
        {
            rc = te_strtoi(atts[3], 0, &value);
            if (rc != 0)
            {
                ERROR("Invalid polling value %s: %r", atts[3], rc);
                return;
            }

            new = allocate_rule();
            if (new == NULL)
                return;
            ret = rule_init_agent(new, atts[1], value);
            if (ret == 0)
                ta_cfg.rules_num++;
        }
    }
    else if (!strcmp(name, "snif_fname"))
    {
        te_strlcpy(snifp_sets.name, atts[1], RCF_MAX_PATH);
    }
    else if (!strcmp(name, "snif_path"))
    {
        te_strlcpy(snifp_sets.dir, atts[1], RCF_MAX_PATH);
    }
    else if (!strcmp(name, "snif_max_fsize"))
    {
        rc = te_strtoui(atts[1], 0, &snifp_sets.fsize);
        if (rc != 0)
            ERROR("Invalid value for \"snif_max_fsize\": %s", atts[1]);
        /* The value is in megabytes, convert to bytes */
        snifp_sets.fsize <<= 20;
    }
    else if (!strcmp(name, "snif_space"))
    {
        rc = te_strtoui(atts[1], 0, &snifp_sets.sn_space);
        if (rc != 0)
            ERROR("Invalid value for \"snif_space\": %s", atts[1]);
        /* The value is in megabytes, convert to bytes */
        snifp_sets.sn_space <<= 20;
    }
    else if (!strcmp(name, "snif_rotation"))
    {
        rc = te_strtoui(atts[1], 0, &snifp_sets.rotation);
        if (rc != 0)
            ERROR("Invalid value for \"snif_rotation\": %s", atts[1]);
    }
    else if (!strcmp(name, "snif_overall_size"))
    {
        rc = te_strtoui(atts[1], 0, &snifp_sets.osize);
        if (rc != 0)
            ERROR("Invalid value for \"snif_overall_size\": %s", atts[1]);
        /* The value is in megabytes, convert to bytes */
        snifp_sets.osize <<= 20;
    }
    else if (!strcmp(name, "snif_ovefill_meth"))
    {
        rc = te_strtoi(atts[1], 0, &value);
        if (rc != 0)
            ERROR("Invalid value for \"snif_overfill_meth\": %s", atts[1]);
        snifp_sets.ofill = value == 0 ?
                           ROTATION : TAIL_DROP;
    }
    else if (!strcmp(name, "snif_period"))
    {
        rc = te_strtoui(atts[1], 0, &snifp_sets.period);
        if (rc != 0)
            ERROR("Invalid value for \"snif_period\": %s", atts[1]);
    }
    else if (!strcmp(name, "thread"))
    {
        char *result = NULL;
        int   when_id = 0;
        int   name_id = 0;

        if (!strcmp(atts[0], "name"))
        {
            when_id = 2;
            name_id = 0;
        }
        else
        {
            when_id = 0;
            name_id = 2;
        }

        if (atts[when_id] == NULL || atts[name_id] == NULL)
        {
            ERROR("Not enough attributes is specified for a thread");
            return;
        }

        if (strcmp(atts[when_id], "when"))
        {
            ERROR("Failed to find 'when' attribute in <thread>");
            return;
        }
        if (te_expand_env_vars(atts[when_id + 1], NULL, &result) != 0)
        {
            ERROR("Failed to expand '%s'", atts[when_id + 1]);
            return;
        }
        if (strlen(result) == 0)
        {
            free(result);
            return;
        }
        free(result);

        if (strcmp(atts[name_id], "name"))
        {
            ERROR("Failed to find 'name' attribute in <thread>");
            return;
        }
        if (te_expand_env_vars(atts[name_id + 1], NULL, &result) != 0)
        {
            ERROR("Failed to expand '%s'", atts[name_id + 1]);
            return;
        }

        if (strlen(result) == 0)
        {
            free(result);
            return;
        }

        ctx->argc = 0;
        ctx->thread_name = result;
        thread_parsed = TRUE;
    }
    else if (!strcmp(name, "arg"))
    {
        if (strcmp(atts[0], "value"))
        {
            ERROR("Failed to find 'value' attribute in <arg>");
            return;
        }
        if (ctx->argc >= RCF_MAX_PARAMS)
        {
            ERROR("Too many <arg> elements");
            return;
        }

        if (te_expand_env_vars(atts[1], NULL, &ctx->argv[ctx->argc]) != 0)
        {
            ERROR("Failed to expand argument value '%s'",
                  atts[1]);
            return;
        }
        ctx->argc++;
    }
}

/**
 * User call back called when a closing tag has been processed.
 *
 * @param thread_ctx  XML parser context (thread_context in this case)
 * @param xml_name    Element name
 */
static void
endElementLGR(void           *thread_ctx,
              const xmlChar  *xml_name)
{
    thread_context *ctx = thread_ctx;

    if (strcmp((char *)xml_name, "thread") == 0 && thread_parsed)
    {
        pthread_t thread_id;

        ctx->argv[ctx->argc] = NULL;
        sem_init(&ctx->args_processed, 0, 0);
        pthread_create(&thread_id, NULL, logger_thread_wrapper,
                       ctx);
        sem_wait(&ctx->args_processed);
        sem_destroy(&ctx->args_processed);
        thread_parsed = FALSE;
    }

    if (strcmp((char *)xml_name, "thread") == 0)
    {
        int i;

        for (i = 0; i < ctx->argc; i++)
            free(ctx->argv[i]);
        ctx->argc = 0;
    }
}

xmlSAXHandler loggerSAXHandlerStruct = {
    NULL, /*internalSubsetLGR*/
    NULL, /*isStandaloneLGR*/
    NULL, /*hasInternalSubsetLGR*/
    NULL, /*hasExternalSubsetLGR*/
    NULL, /*resolveEntityLGR*/
    NULL, /*getEntityLGR*/
    NULL, /*entityDeclLGR*/
    NULL, /*notationDeclLGR*/
    NULL, /*attributeDeclLGR*/
    NULL, /*elementDeclLGR*/
    NULL, /*unparsedEntityDeclLGR*/
    NULL, /***setDocumentLocatorLGR*/
    NULL, /***startDocumentLGR*/
    NULL, /***endDocumentLGR*/
    startElementLGR,
    endElementLGR,
    NULL, /*referenceLGR*/
    NULL, /***charactersLGR*/
    NULL, /*ignorableWhitespaceLGR*/
    NULL, /*processingInstructionLGR*/
    NULL, /*commentLGR*/
    NULL, /*warningLGR*/
    NULL, /*errorDebugLGR*/
    NULL, /*fatalErrorDebugLGR*/
    NULL, /*getParameterEntityLGR*/
    NULL, /*cdataBlockLGR*/
    NULL, /*externalSubsetLGR*/
    1,    /*initialized*/
    /* v2 only extensions */
    NULL, /*_private*/
    NULL, /*startElementNs*/
    NULL, /*endElementNs*/
    NULL  /*serror*/
};

xmlSAXHandlerPtr loggerSAXHandler = &loggerSAXHandlerStruct;

/*************************************************************************/
/*           YAML                                                        */
/*************************************************************************/


/** Extract the value if it's a scalar node */
static const char *
get_scalar_value(const yaml_node_t *node)
{
    if (node != NULL && node->type == YAML_SCALAR_NODE)
        return (const char *)node->data.scalar.value;
    return NULL;
}

/**
 * Parse the "polling" section of the config file.
 *
 * @param d         YAML document
 * @param section   YAML node for this section
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
static int
handle_polling(yaml_document_t *d, yaml_node_t *section)
{
    int               ret;
    te_errno          rc;
    yaml_node_item_t *item;
    size_t            items;
    int               rule_index;
    int               value;

    if (section->type != YAML_SEQUENCE_NODE)
    {
        ERROR("%s: Expected a sequence, got something else", __FUNCTION__);
        return -1;
    }

    items = section->data.sequence.items.top -
            section->data.sequence.items.start;

    if (ta_cfg.rules != NULL)
        free(ta_cfg.rules);

    ta_cfg.rules = TE_ALLOC(items * sizeof(ta_rule));
    ta_cfg.rules_num = 0;
    ta_cfg.polling_default = 0;
    rule_index = 0;

    for (item = section->data.sequence.items.start;
         item < section->data.sequence.items.top; item++)
    {
        yaml_node_t *rule = yaml_document_get_node(d, *item);

        if (rule->type != YAML_MAPPING_NODE)
        {
            ERROR("%s: Expected a mapping, got something else", __FUNCTION__);
            return -1;
        }

        yaml_node_pair_t *pair = rule->data.mapping.pairs.start;
        yaml_node_t *k = yaml_document_get_node(d, pair->key);
        yaml_node_t *v = yaml_document_get_node(d, pair->value);

        const char *rule_key = get_scalar_value(k);
        const char *rule_value = get_scalar_value(v);

        if (rule_key == NULL)
        {
            ERROR("%s: Non-scalar key in mapping", __FUNCTION__);
            return -1;
        }

        if (strcmp(rule_key, "default") == 0)
        {
            /* Get default polling value and assign it to the all TA */
            rc = te_strtoi(rule_value, 0, &ta_cfg.polling_default);
            if (rc != 0)
            {
                ERROR("%s: Invalid polling value %s: %r",
                      __FUNCTION__, rule_value, rc);
                return -1;
            }
            config_clear();
            rule_index = 0;
        }
        else if (strcmp(rule_key, "type") == 0)
        {
            /*
             * Get polling value for separate TA type
             * and assign it to the appropriate TA
             */
            const char *val_str = NULL;

            while (++pair < rule->data.mapping.pairs.top)
            {
                yaml_node_t *k = yaml_document_get_node(d, pair->key);
                yaml_node_t *v = yaml_document_get_node(d, pair->value);
                const char  *key = get_scalar_value(k);

                if (key == NULL)
                {
                    ERROR("%s: Non-scalar key in mapping", __FUNCTION__);
                    return -1;
                }

                if (strcmp(key, "value") == 0)
                    val_str = get_scalar_value(v);
            }

            if (val_str == NULL)
            {
                ERROR("%s: Missing value in rule for type %s",
                      __FUNCTION__, rule_value);
                return -1;
            }

            rc = te_strtoi(val_str, 0, &value);
            if (rc != 0)
            {
                ERROR("%s: Invalid polling value %s: %r",
                      __FUNCTION__, rule_value, rc);
                return -1;
            }

            ret = rule_init_type(&ta_cfg.rules[rule_index], rule_value, value);
            if (ret != 0)
                return ret;
            rule_index++;
        }
        else if (strcmp(rule_key, "agent") == 0)
        {
            /* Get polling value for separate TA and assign it */
            const char *val_str = NULL;

            while (++pair < rule->data.mapping.pairs.top)
            {
                yaml_node_t *k = yaml_document_get_node(d, pair->key);
                yaml_node_t *v = yaml_document_get_node(d, pair->value);
                const char  *key = get_scalar_value(k);

                if (key == NULL)
                {
                    ERROR("%s: Non-scalar key in mapping", __FUNCTION__);
                    return -1;
                }

                if (strcmp(key, "value") == 0)
                    val_str = get_scalar_value(v);
            }

            if (val_str == NULL)
            {
                ERROR("%s: Missing value in rule for agent %s",
                      __FUNCTION__, rule_value);
                return -1;
            }

            rc = te_strtoi(val_str, 0, &value);
            if (rc != 0)
            {
                ERROR("%s: Invalid polling value %s: %r",
                      __FUNCTION__, rule_value, rc);
                return -1;
            }

            ret = rule_init_agent(&ta_cfg.rules[rule_index], rule_value, value);
            if (ret != 0)
                return ret;
            rule_index++;
        }
    }

    ta_cfg.rules_num = rule_index;

    return 0;
}

/**
 * Parse the "sniffers" section of the config file.
 *
 * @param d         YAML document
 * @param section   YAML node for this section
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
static int
handle_sniffers(yaml_document_t *d, yaml_node_t *section)
{
    te_errno          rc;
    unsigned int      tmp;
    yaml_node_pair_t *pair;

    if (section->type != YAML_MAPPING_NODE)
    {
        ERROR("%s: Expected a mapping, got something else", __FUNCTION__);
        return -1;
    }

    for (pair = section->data.mapping.pairs.start;
         pair < section->data.mapping.pairs.top; pair++)
    {
        yaml_node_t *k = yaml_document_get_node(d, pair->key);
        yaml_node_t *v = yaml_document_get_node(d, pair->value);
        const char  *key   = get_scalar_value(k);
        const char  *value = get_scalar_value(v);

        if (key == NULL)
        {
            ERROR("%s: Non-scalar key in mapping", __FUNCTION__);
            return -1;
        }

        if (value == NULL)
        {
            ERROR("%s: Non-scalar value for key %s", __FUNCTION__, key);
            return -1;
        }

        if (strcmp(key, "fname") == 0)
        {
            te_strlcpy(snifp_sets.name, value, RCF_MAX_PATH);
        }
        else if (strcmp(key, "path") == 0)
        {
            te_strlcpy(snifp_sets.dir, value, RCF_MAX_PATH);
        }
        else if (strcmp(key, "max_fsize") == 0)
        {
            rc = te_strtoui(value, 0, &snifp_sets.fsize);
            if (rc != 0)
            {
                ERROR("%s: Invalid value for \"max_fsize\": %s",
                      __FUNCTION__, value);
                return -1;
            }
            /* The value is in megabytes, convert to bytes */
            snifp_sets.fsize <<= 20;
        }
        else if (strcmp(key, "space") == 0)
        {
            rc = te_strtoui(value, 0, &snifp_sets.sn_space);
            if (rc != 0)
            {
                ERROR("%s: Invalid value for \"space\": %s",
                      __FUNCTION__, value);
                return -1;
            }
            /* The value is in megabytes, convert to bytes */
            snifp_sets.sn_space <<= 20;
        }
        else if (strcmp(key, "rotation") == 0)
        {
            rc = te_strtoui(value, 0, &snifp_sets.rotation);
            if (rc != 0)
            {
                ERROR("%s: Invalid value for \"rotation\": %s",
                      __FUNCTION__, value);
                return -1;
            }
        }
        else if (strcmp(key, "overall_size") == 0)
        {
            rc = te_strtoui(value, 0, &snifp_sets.osize);
            if (rc != 0)
            {
                ERROR("%s: Invalid value for \"overall_size\": %s",
                      __FUNCTION__, value);
                return -1;
            }
            /* The value is in megabytes, convert to bytes */
            snifp_sets.osize <<= 20;
        }
        else if (strcmp(key, "ovefill_meth") == 0)
        {
            rc = te_strtoui(value, 0, &tmp);
            if (rc != 0)
            {
                ERROR("%s: Invalid value for \"overfill_meth\": %s",
                      __FUNCTION__, value);
                return -1;
            }
            snifp_sets.ofill = tmp == 0 ? ROTATION : TAIL_DROP;
        }
        else if (strcmp(key, "period") == 0)
        {
            rc = te_strtoui(value, 0, &snifp_sets.period);
            if (rc != 0)
            {
                ERROR("%s: Invalid value for \"period\": %s",
                      __FUNCTION__, value);
                return -1;
            }
        }
    }

    return 0;
}

/**
 * Extract thread configuration and start the thread.
 *
 * @param d         YAML document
 * @param cfg       YAML node for this thread
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
static int
run_thread(yaml_document_t *d, yaml_node_t *cfg)
{
    int            i;
    pthread_t      thread_id;
    thread_context ctx;
    const char    *enabled_str;
    char          *enabled_exp;

    yaml_node_pair_t *pair;
    yaml_node_item_t *item;
    yaml_node_t      *name = NULL;
    yaml_node_t      *enabled = NULL;
    yaml_node_t      *args = NULL;

    ptrdiff_t args_num = 0;

    if (cfg->type != YAML_MAPPING_NODE)
    {
        ERROR("%s: Expected a mapping, got something else", __FUNCTION__);
        return -1;
    }

    memset(&ctx, 0, sizeof(ctx));

    for (pair = cfg->data.mapping.pairs.start;
         pair < cfg->data.mapping.pairs.top; pair++)
    {
        yaml_node_t *k = yaml_document_get_node(d, pair->key);
        yaml_node_t *v = yaml_document_get_node(d, pair->value);
        const char  *key   = get_scalar_value(k);

        if (key == NULL)
        {
            ERROR("%s: Non-scalar key in mapping", __FUNCTION__);
            return -1;
        }

        if (strcmp(key, "name") == 0)
            name = v;
        else if (strcmp(key, "enabled") == 0)
            enabled = v;
        else if (strcmp(key, "args") == 0)
            args = v;
    }

    if (name == NULL)
    {
        ERROR("%s: Encountered a thread with no name", __FUNCTION__);
        return -1;
    }

    if (name->type != YAML_SCALAR_NODE)
    {
        ERROR("%s: Name must be a scalar value", __FUNCTION__);
        return -1;
    }

    ctx.thread_name = get_scalar_value(name);

    if (enabled == NULL)
    {
        ERROR("%s(%s): Enabled not specified",
              __FUNCTION__, ctx.thread_name);
        return -1;
    }

    if (enabled->type != YAML_SCALAR_NODE)
    {
        ERROR("%s(%s): Enabled must be a scalar",
              __FUNCTION__, ctx.thread_name);
        return -1;
    }

    enabled_str = get_scalar_value(enabled);
    if (te_expand_env_vars(enabled_str, NULL, &enabled_exp) != 0)
    {
        ERROR("%s(%s): Failed to expand '%s'",
              __FUNCTION__, ctx.thread_name, enabled_str);
        return -1;
    }

    if (strlen(enabled_exp) == 0 ||
        strcasecmp(enabled_exp, "no") == 0 ||
        strcasecmp(enabled_exp, "false") == 0)
    {
        free(enabled_exp);
        return 0;
    }
    free(enabled_exp);

    if (args != NULL)
    {
        if (args->type != YAML_SEQUENCE_NODE)
        {
            ERROR("%s(%s): args can only be a sequence",
                  __FUNCTION__, ctx.thread_name);
            return -1;
        }

        args_num = args->data.sequence.items.top - args->data.sequence.items.start;
        if (args_num > RCF_MAX_PARAMS)
        {
            ERROR("%s(%s): Too many arguments (%d while only %d are allowed)",
                  __FUNCTION__, ctx.thread_name, args_num, RCF_MAX_PARAMS);
            return -1;
        }

        for (item = args->data.sequence.items.start;
             item < args->data.sequence.items.top; item++)
        {
            yaml_node_t *arg   = yaml_document_get_node(d, *item);
            const char  *value = get_scalar_value(arg);

            if (value == NULL)
            {
                ERROR("%s(%s): argument must be a scalar",
                      __FUNCTION__, ctx.thread_name);
                break;
            }
            if (te_expand_env_vars(value, NULL, &ctx.argv[ctx.argc]) != 0)
            {
                ERROR("%s(%s): Failed to expand argument value '%s'",
                      __FUNCTION__, ctx.thread_name, value);
                break;
            }
            ctx.argc++;
        }
    }

    if (ctx.argc == args_num)
    {
        ctx.argv[ctx.argc] = NULL;
        sem_init(&ctx.args_processed, 0, 0);
        pthread_create(&thread_id, NULL, logger_thread_wrapper, &ctx);
        pthread_detach(thread_id);
        sem_wait(&ctx.args_processed);
        sem_destroy(&ctx.args_processed);
    }

    for (i = 0; i < ctx.argc; i++)
        free(ctx.argv[i]);

    return ctx.argc == args_num ? 0 : -1;
}

/**
 * Parse the "threads" section of the config file.
 *
 * @param d         YAML document
 * @param section   YAML node for this section
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
static int
handle_threads(yaml_document_t *d, yaml_node_t *section)
{
    int               res;
    yaml_node_item_t *item;

    if (section->type != YAML_SEQUENCE_NODE)
    {
        ERROR("%s: Expected a sequence, got something else", __FUNCTION__);
        return -1;
    }

    for (item = section->data.sequence.items.start;
         item < section->data.sequence.items.top; item++)
    {
        yaml_node_t *v = yaml_document_get_node(d, *item);

        if (v->type != YAML_MAPPING_NODE)
        {
            ERROR("%s: A thread must be a mapping", __FUNCTION__);
            return -1;
        }

        res = run_thread(d, v);
        if (res != 0)
            return -1;
    }

    return 0;
}

/**
 * Parse a YAML-formatted config file.
 *
 * @param filename      path to the config file
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
static int
config_parser_yaml(const char *filename)
{
    int               res;
    FILE             *input;
    yaml_parser_t     parser;
    yaml_document_t   document;
    yaml_node_t      *root             = NULL;
    yaml_node_t      *polling          = NULL;
    yaml_node_t      *sniffers_default = NULL;
    yaml_node_t      *sniffers         = NULL;
    yaml_node_t      *threads          = NULL;
    yaml_node_pair_t *pair;

    RING("Opening config file: %s", filename);

    input = fopen(filename, "r");
    if (input == NULL)
    {
        ERROR("Failed to open YAML config file %s: %s",
              filename, strerror(errno));
        return -1;
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, input);
    if (yaml_parser_load(&parser, &document) == 0)
    {
        ERROR("Failed to load YAML document");
        yaml_parser_delete(&parser);
        fclose(input);
        return -1;
    }
    yaml_parser_delete(&parser);
    fclose(input);

    root = yaml_document_get_root_node(&document);
    if (root == NULL)
    {
        RING("Failed to get root YAML node, assuming empty config");
        yaml_document_delete(&document);
        return 0;
    }

    if (root->type == YAML_SCALAR_NODE && root->data.scalar.length == 0)
        return 0;

    if (root->type != YAML_MAPPING_NODE)
    {
        ERROR("Root YAML node is not a mapping");
        yaml_document_delete(&document);
        return -1;
    }

    res = 0;
    for (pair = root->data.mapping.pairs.start;
         pair < root->data.mapping.pairs.top; pair++)
    {
        yaml_node_t *k = yaml_document_get_node(&document, pair->key);
        yaml_node_t *v = yaml_document_get_node(&document, pair->value);
        const char  *key = get_scalar_value(k);

        if (key == NULL)
        {
            ERROR("%s: Non-scalar key in mapping", __FUNCTION__);
            res = -1;
            break;
        }

        if (strcmp(key, "polling") == 0)
            polling = v;
        else if (strcmp(key, "sniffers_default") == 0)
            sniffers_default = v;
        else if (strcmp(key, "sniffers") == 0)
            sniffers = v;
        else if (strcmp(key, "threads") == 0)
            threads = v;
        else
            WARN("Unknown config section: %s", key);
    }

    if (res == 0 && polling != NULL)
        res = handle_polling(&document, polling);

    if (res == 0 && sniffers_default != NULL)
        res = handle_sniffers(&document, sniffers_default);

    if (res == 0 && sniffers != NULL)
        res = handle_sniffers(&document, sniffers);

    if (res == 0 && threads != NULL)
        res = handle_threads(&document, threads);

    yaml_document_delete(&document);

    return res;
}

/**
 * Parse an XML-formatted config file.
 *
 * @param filename      path to the config file
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
static int
config_parser_xml(const char *filename)
{
    int res = 0;
    thread_context ctx;

    if (filename == NULL)
        return 0;

    memset(&ctx, 0, sizeof(ctx));

    res = xmlSAXUserParseFile(loggerSAXHandler, &ctx, filename);
    xmlCleanupParser();
    xmlMemoryDump();
    return res;
}

/* See description in logger_internal.h */
int
config_parser(const char *filename)
{
#define PREREAD_SIZE 8
    static const char xml_head[]  = "<?xml";
    static const char yaml_head[] = "---";

    int      res = 0;
    char     buf[PREREAD_SIZE + 1];
    FILE    *fp;
    ta_inst *tmp_el;

    if (filename == NULL)
        return 0;

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        ERROR("Couldn't open config file %s: %s", filename, strerror(errno));
        return -1;
    }

    res = fread(buf, 1, PREREAD_SIZE, fp);
    if (res == 0 && feof(fp))
    {
        /* Config file is empty, nothing to do */
        fclose(fp);
        return 0;
    }
    fclose(fp);
#undef PREREAD_SIZE

    buf[res] = '\0';
    if (strncmp(buf, yaml_head, sizeof(yaml_head) - 1) == 0)
    {
        res = config_parser_yaml(filename);
    }
    else if (strncmp(buf, xml_head, sizeof(xml_head) - 1) == 0)
    {
        res = config_parser_xml(filename);
    }
    else
    {
        ERROR("Unknown config file format");
        res = -1;
    }

    SLIST_FOREACH(tmp_el, &ta_list, links)
    {
        VERB("Agent %s Type %s Polling %u",
             tmp_el->agent, tmp_el->type, tmp_el->polling);
    }

    VERB("snifp_sets.name     %s\n", snifp_sets.name);
    VERB("snifp_sets.dir      %s\n", snifp_sets.dir);
    VERB("snifp_sets.fsize    %u\n", snifp_sets.fsize);
    VERB("snifp_sets.sn_space %u\n", snifp_sets.sn_space);
    VERB("snifp_sets.rotation %u\n", snifp_sets.rotation);
    VERB("snifp_sets.osize    %u\n", snifp_sets.osize);
    VERB("snifp_sets.ofill    %u\n", snifp_sets.ofill);
    VERB("snifp_sets.period   %u\n", snifp_sets.period);

    return res;
}

/* See description in logger_internal.h */
void
config_ta(ta_inst *ta)
{
    const ta_rule *rule = ta_cfg.rules;
    const ta_rule *last = ta_cfg.rules + ta_cfg.rules_num;
    regmatch_t     pmatch[1];

    ta->polling = ta_cfg.polling_default;
    for (; rule < last; rule++)
    {
        if (rule->name)
        {
            if (strcmp(ta->agent, rule->name) == 0)
                ta->polling = rule->polling;
        }
        else
        {
            memset(pmatch, 0, sizeof(*pmatch));
            if (regexec(&rule->type, ta->type, 1, pmatch, 0) == 0 &&
                (size_t)(pmatch->rm_eo - pmatch->rm_so) == strlen(ta->type))
                ta->polling = rule->polling;
        }
    }
}
