/** @file
 * @brief TE project. Logger subsystem.
 *
 * Logger configuration file XML parser.
 * No grammar validity checking is carried out.
 * This code relies on external XML grammar validator.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Igor B. Vasiliev <Igor.Vasiiev@oktetlabs.ru>
 */

#include "logger_internal.h"

#define TE_EXPAND_XML 1
#include "te_expand.h"

#include "rcf_common.h"
#include <pthread.h>
#include <semaphore.h>
#include "te_str.h"
#include "te_kernel_log.h"

/* TA single linked list */
extern ta_inst *ta_list;

/* Cpature logs polling settings */
extern snif_polling_sets_t snifp_sets;

/* Path to the directory for logs */
extern const char *te_log_dir;

static te_bool   thread_parsed = FALSE;


/*
 * Context that will be passed to the threads
 * specified in the config file.
 */
typedef struct thread_context {
    int         argc;
    char       *argv[RCF_MAX_PARAMS];
    const char *thread_name;
    sem_t       args_processed;
} thread_context;

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

/**
 * Set polling interval for all agents.
 *
 * @param interval  polling interval
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
static void
polling_set_default(uint32_t interval)
{
    ta_inst *tmp_el;

    tmp_el = ta_list;
    while (tmp_el != NULL)
    {
        tmp_el->polling = interval;
        tmp_el = tmp_el->next;
    }
}

/**
 * Set polling interval for all agents of a specific type.
 *
 * @param type      TA type regex
 * @param interval  polling interval
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
static int
polling_set_type(const char *type, uint32_t interval)
{
    ta_inst    *tmp_el;
    regmatch_t  pmatch[1];
    regex_t     cmp_buffer;
    uint32_t    str_len;

    memset(pmatch, 0, sizeof(regmatch_t));

    if (regcomp(&cmp_buffer, type, REG_EXTENDED) != 0)
    {
        ERROR("Polling: RegExpr compilation failure\n");
        return -1;
    }

    tmp_el = ta_list;
    while (tmp_el != NULL)
    {
        if (regexec(&cmp_buffer, tmp_el->type, 1, pmatch, 0) == 0)
        {
            str_len = strlen(tmp_el->type);
            if ((str_len - (pmatch->rm_eo - pmatch->rm_so) == 0))
                tmp_el->polling = interval;

        }
        tmp_el = tmp_el->next;
    }
    regfree(&cmp_buffer);

    return 0;
}

/**
 * Set polling interval for a specific agent.
 *
 * @param agent     TA name
 * @param interval  polling interval
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
static void
polling_set_agent(const char *agent, uint32_t interval)
{
    ta_inst *tmp_el;

    tmp_el = ta_list;
    while (tmp_el != NULL)
    {
        if (strcmp(agent, tmp_el->agent) == 0)
            tmp_el->polling = interval;

        tmp_el = tmp_el->next;
    }
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
    const char     *name = (const char *)xml_name;
    const char    **atts = (const char **)xml_atts;
    thread_context *ctx = thread_ctx;

    if ((atts == NULL) || (atts[0] == NULL) || (atts[1] == NULL))
        return;

    if (!strcmp(name, "polling") &&
        (atts != NULL) &&
        (atts[0] != NULL) &&
        (atts[1] != NULL))
    {
        /* Get default polling value and assign it to the all TA */
        if (strcmp(atts[0], "default") == 0)
            polling_set_default(strtoul(atts[1], NULL, 0));
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
            polling_set_type(atts[1], strtoul(atts[3], NULL, 0));
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
            polling_set_agent(atts[1], strtoul(atts[3], NULL, 0));
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
        snifp_sets.fsize = (unsigned)strtoul(atts[1], NULL, 0) << 20;
    }
    else if (!strcmp(name, "snif_space"))
    {
        snifp_sets.sn_space = (unsigned)strtoul(atts[1], NULL, 0) << 20;
    }
    else if (!strcmp(name, "snif_rotation"))
    {
        snifp_sets.rotation = (unsigned)strtoul(atts[1], NULL, 0);
    }
    else if (!strcmp(name, "snif_overall_size"))
    {
        snifp_sets.osize = (unsigned)strtoul(atts[1], NULL, 0) << 20;
    }
    else if (!strcmp(name, "snif_ovefill_meth"))
    {
        snifp_sets.ofill = (unsigned)strtoul(atts[1], NULL, 0) == 0 ?
                           ROTATION : TAIL_DROP;
    }
    else if (!strcmp(name, "snif_period"))
    {
        snifp_sets.period = (unsigned)strtoul(atts[1], NULL, 0);
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
        if (ctx->argc >= RCF_MAX_PARAMS - 1)
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

/**
 * Parse logger configuration file.
 *
 * @param file_name     - XML configuration file full name
 *
 * @return Status information
 *
 * @retval 0            Success.
 * @retval Negative     Failure.
 */
int
configParser(const char *file_name)
{
    int res = 0;
    thread_context ctx;

    if (file_name == NULL)
        return 0;

    memset(&ctx, 0, sizeof(ctx));

    res = xmlSAXUserParseFile(loggerSAXHandler, &ctx, file_name);
    xmlCleanupParser();
    xmlMemoryDump();
    return res;
}
