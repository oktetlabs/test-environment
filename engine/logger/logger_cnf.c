/** @file
 * @brief TE project. Logger subsystem.
 *
 * Logger configuration file XML parser.
 * No grammar validity checking is carried out.
 * This code relies on external XML grammar validator.
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
 * @author Igor B. Vasiliev <Igor.Vasiiev@oktetlabs.ru>
 *
 * $Id$
 */

#include "logger_internal.h"

#ifndef UNUSED
#define UNUSED(_var)    (void)(_var)
#endif

/* TA single linked list */
extern ta_inst *ta_list;

/* TEN entities single linked list */
extern te_inst *te_list;


/**
 * Insert filter appropriate type for separate entity.
 *
 * @param entity   Entity name
 * @param rexp     Regular expression for filter
 * @param type     Inclusion/exclusion of logged messages
 *                 appropriate to the filter
 */
static void
lgr_insert_filter(const char *entity, const char* rexp, uint32_t type)
{
    ta_inst *ta_el;
    te_inst *te_el;
    regex_t *cmp_buffer;

    cmp_buffer = (regex_t *)malloc(sizeof(regex_t));
    memset(cmp_buffer, 0, sizeof(regex_t));

    if (regcomp(cmp_buffer, rexp, REG_EXTENDED) != 0)
    {
        ERROR("RegExpr compilation failure\n");
        return;
    }

    ta_el = ta_list;
    while(ta_el != NULL)
    {
        if (!strcmp(ta_el->agent, entity))
        {
            LGR_PUT_FLTR(&ta_el->filters, type, cmp_buffer);
            return;
        }
        ta_el = ta_el->next;
    }

    te_el = te_list;
    while(te_el != NULL)
    {
        if (!strcmp(te_el->entity, entity))
        {
            LGR_PUT_FLTR(&te_el->filters, type, cmp_buffer);
            return;
        }
        te_el = te_el->next;
    }

    te_el = (te_inst *)malloc(sizeof(struct te_inst));
    memset(te_el, 0, sizeof(struct te_inst));
    te_el->next = te_list;
    te_list = te_el;
    strcpy(te_el->entity, entity);
    te_el->filters.next = te_el->filters.last = &te_el->filters;
    LGR_PUT_FLTR(&te_el->filters, type, cmp_buffer);
}


/**
 * User call back called when an opening tag has been processed.
 *
 * @param  name  Element name
 * @param  ctxt  XML parser context
 */
static void
startElementLGR(void *ctx ATTRIBUTE_UNUSED,
                const xmlChar *name,
                const xmlChar **atts)
{
    ta_inst *tmp_el;

    UNUSED(ctx);

    if (!strcmp(name, "polling") &&
        (atts != NULL) &&
        (atts[0] != NULL) &&
        (atts[1] != NULL))
    {
        /* Get default polling value and assign it to the all TA */
        if (strcmp(atts[0], "default") == 0)
        {
            uint32_t dft;

            dft = (uint32_t)strtoul(atts[1], NULL, 0);

            tmp_el = ta_list;
            while (tmp_el != NULL)
            {
                tmp_el->polling = dft;
                tmp_el = tmp_el->next;
            }
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
            uint32_t    val;
            regmatch_t  pmatch[1];
            regex_t    *cmp_buffer;
            uint32_t   str_len;

            memset(pmatch,0, sizeof(regmatch_t));
            cmp_buffer = (regex_t *)malloc(sizeof(*cmp_buffer));
            memset(cmp_buffer, 0, sizeof(cmp_buffer));

            val = (uint32_t)strtoul(atts[3], NULL, 0);
            if (regcomp(cmp_buffer, atts[1], REG_EXTENDED) != 0 )
            {
                ERROR("RegExpr compilation failure\n");
                return;
            }

            tmp_el = ta_list;
            while (tmp_el != NULL)
            {
                regmatch_t pmatch[1];

                if (regexec(cmp_buffer, tmp_el->type, 1, pmatch, 0) == 0)
                {
                    str_len = strlen(tmp_el->type);
                    if ((str_len - (pmatch->rm_eo - pmatch->rm_so) == 0))
                        tmp_el->polling = val;

                }
                tmp_el = tmp_el->next;
            }
            regfree(cmp_buffer);
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

            uint32_t val = (uint32_t)strtoul(atts[3], NULL, 0);

            tmp_el = ta_list;
            while (tmp_el != NULL)
            {
                if (!strcmp(atts[1], tmp_el->agent))
                tmp_el->polling = val;

                tmp_el = tmp_el->next;
            }
        }
    }
    else if (!strcmp(name, "include") &&
             (atts != NULL) &&
             (atts[0] != NULL) &&
             (atts[1] != NULL) &&
             (atts[2] != NULL) &&
             (atts[3] != NULL))
    {
        lgr_insert_filter(atts[1], atts[3], LGR_INCLUDE);
    }
    else if (!strcmp(name, "exclude") &&
             (atts != NULL) &&
             (atts[0] != NULL) &&
             (atts[1] != NULL) &&
             (atts[2] != NULL) &&
             (atts[3] != NULL))
    {
        lgr_insert_filter(atts[1], atts[3], LGR_EXCLUDE);
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
    NULL, /***endElementLGR*/
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

    res = xmlSAXUserParseFile(loggerSAXHandler, NULL, file_name);
    xmlCleanupParser();
    xmlMemoryDump();
    return res;
}
