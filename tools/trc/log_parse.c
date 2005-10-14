/** @file
 * @brief Testing Results Comparator
 *
 * Parser of TE log in XML format.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "trc_log.h"
#include "trc_db.h"
#include "trc_xml.h"


static int get_logs(xmlNodePtr node, test_runs *tests);


/**
 * Find element of the list by name.
 *
 * @param tests     List of tests
 * @param name      Name of the test
 *
 * @return Pointer to found element or NULL.
 */
static test_run *
trc_db_find_by_name(test_runs *tests, const char *name)
{
    test_run *p;

    for (p = tests->head.tqh_first;
         (p != NULL) && (strcmp(p->name, name) != 0);
         p = p->links.tqe_next);

    return p;
}

static te_bool
test_args_1in2(const test_args *args1, const test_args *args2)
{
    test_arg *p, *q;

    for (p = args1->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        for (q = args2->head.tqh_first; q != NULL; q = q->links.tqe_next)
        {
            if (strcmp(p->name, q->name) == 0 &&
                strcmp(p->value, q->value) == 0)
                break;
        }
        if (q == NULL)
            break;
    }
    return (p == NULL);
}

static te_bool
test_args_equal(const test_args *args1, const test_args *args2)
{
    return test_args_1in2(args1, args2) && test_args_1in2(args2, args1);
}

static test_iter *
trc_db_find_by_args(test_iters *iters, const test_args *args)
{
    test_iter *p;

    for (p = iters->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (!p->used && test_args_equal(&p->args, args))
            break;
    }

    return p;
}


/**
 * Updated iteration statistics by expected and got results.
 *
 * @param iter      Test iteration
 */
static void
iter_stats_update_by_result(test_iter *iter)
{
    if (iter->got_result == TRC_TEST_UNSPEC)
    {
        ERROR("Unexpected got result value");
        return;
    }
    if (iter->got_result == TRC_TEST_FAKED)
    {
        return;
    }
    iter->stats.not_run--;

    switch (iter->exp_result.value)
    {
        case TRC_TEST_UNSPEC:
            switch (iter->got_result)
            {
                case TRC_TEST_SKIPPED:
                    iter->stats.new_not_run++;
                    break;
                default:
                    iter->stats.new_run++;
            }
            break;

        case TRC_TEST_PASSED:
            switch (iter->got_result)
            {
                case TRC_TEST_PASSED:
                    if (tq_strings_equal(&iter->got_verdicts,
                                         &iter->exp_result.verdicts))
                    {
                        iter->stats.pass_exp++;
                        iter->got_as_expect = TRUE;
                    }
                    else
                        iter->stats.pass_une++;
                    break;
                case TRC_TEST_FAILED:
                    iter->stats.fail_une++;
                    break;
                case TRC_TEST_SKIPPED:
                    iter->stats.skip_une++;
                    break;
                default:
                    iter->stats.aborted++;
            }
            break;

        case TRC_TEST_FAILED:
            switch (iter->got_result)
            {
                case TRC_TEST_PASSED:
                    iter->stats.pass_une++;
                    break;
                case TRC_TEST_FAILED:
                    if (tq_strings_equal(&iter->got_verdicts,
                                         &iter->exp_result.verdicts))
                    {
                        iter->stats.fail_exp++;
                        iter->got_as_expect = TRUE;
                    }
                    else
                        iter->stats.fail_une++;
                    break;
                case TRC_TEST_SKIPPED:
                    iter->stats.skip_une++;
                    break;
                default:
                    iter->stats.aborted++;
            }
            break;

        case TRC_TEST_SKIPPED:
            switch (iter->got_result)
            {
                case TRC_TEST_PASSED:
                    iter->stats.pass_une++;
                    break;
                case TRC_TEST_FAILED:
                    iter->stats.fail_une++;
                    break;
                case TRC_TEST_SKIPPED:
                    iter->stats.skip_exp++;
                    iter->got_as_expect = TRUE;
                    break;
                default:
                    iter->stats.aborted++;
            }
            break;

        default:
            ERROR("Invalid expected testing result %u",
                  iter->exp_result.value);
    }
}


/**
 * Get test result.
 *
 * @param node      Node with boolean property
 * @param value     Location for value
 *
 * @return Status code.
 * @retval ENOENT   Property does not exists. Value is not modified.
 */
static int
get_result(xmlNodePtr node, trc_test_result *value)
{
    xmlChar *s = xmlGetProp(node, CONST_CHAR2XML("result"));

    if (s == NULL)
        return ENOENT;
    if (xmlStrcmp(s, CONST_CHAR2XML("PASSED")) == 0)
        *value = TRC_TEST_PASSED;
    else if (xmlStrcmp(s, CONST_CHAR2XML("FAILED")) == 0)
        *value = TRC_TEST_FAILED;
    else if (xmlStrcmp(s, CONST_CHAR2XML("SKIPPED")) == 0)
        *value = TRC_TEST_SKIPPED;
    else if (xmlStrcmp(s, CONST_CHAR2XML("KILLED")) == 0)
        *value = TRC_TEST_KILLED;
    else if (xmlStrcmp(s, CONST_CHAR2XML("CORED")) == 0)
        *value = TRC_TEST_CORED;
    else if (xmlStrcmp(s, CONST_CHAR2XML("FAKED")) == 0)
        *value = TRC_TEST_FAKED;
    else
    {
        ERROR("Invalid value '%s' of the result", XML2CHAR(s));
        xmlFree(s);
        return EINVAL;
    }
    xmlFree(s);

    return 0;
}

/**
 * Allocate and get test iteration parameters.
 *
 * @param node      XML node
 * @param args      List of arguments to be filled in
 *
 * @return Status code.
 */
static int
alloc_and_get_test_param(xmlNodePtr node, test_args *args)
{
    test_arg   *p;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
        return errno;
    }
    p->node = node;
    TAILQ_INSERT_TAIL(&args->head, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name of the argument is missing");
        return EINVAL;
    }
    p->value = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("value")));
    if (p->value == NULL)
    {
        ERROR("Value of the argument is missing");
        return EINVAL;
    }

    return 0;
}

/**
 * Get test iteration arguments.
 *
 * @param node      XML node
 * @param args      List of arguments to be filled in
 *
 * @return Status code
 */
static int
get_test_params(xmlNodePtr node, test_args *args)
{
    int rc = 0;

    assert(node != NULL);
    assert(args != NULL);

    while (node != NULL)
    {
        if (xmlStrcmp(node->name, CONST_CHAR2XML("param")) != 0)
        {
            ERROR("Unexpected element '%s' in 'params'", node->name);
            return EINVAL;
        }
        rc = alloc_and_get_test_param(node, args);
        if (rc != 0)
        {
            ERROR("Failed to allcate/get parameter");
            break;
        }
        node = xmlNodeNext(node);
    }

    return rc;
}

/**
 * Get test iteration verdicts.
 *
 * @param node      XML node
 * @param verdicts  List of verdicts to be filled in
 *
 * @return Status code
 */
static int
get_verdicts(xmlNodePtr node, tqh_string *verdicts)
{
    int         rc = 0;
    tqe_string *p;

    assert(node != NULL);
    assert(verdicts != NULL);

    while (node != NULL)
    {
        if (xmlStrcmp(node->name, CONST_CHAR2XML("verdict")) != 0)
        {
            ERROR("Unexpected element '%s' in 'params'", node->name);
            return EINVAL;
        }
        p = calloc(1, sizeof(*p));
        if (p == NULL)
            return EINVAL;
        TAILQ_INSERT_TAIL(verdicts, p, links);
        rc = get_text_content(node, "verdict", &p->str);
        if (rc != 0)
            break;
        node = xmlNodeNext(node);
    }

    return rc;
}

static int
get_meta(xmlNodePtr node, char **objective, test_args *args,
         tqh_string *verdicts)
{
    int rc;

    if ((node == NULL) ||
        (xmlStrcmp(node->name, CONST_CHAR2XML("meta")) != 0))
    {
        ERROR("'meta' not found");
        return EINVAL;
    }

    node = xmlNodeChildren(node);

    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("objective")) != 0 &&
           xmlStrcmp(node->name, CONST_CHAR2XML("verdicts")) != 0 &&
           xmlStrcmp(node->name, CONST_CHAR2XML("params")) != 0)
    {
        node = xmlNodeNext(node);
    }
    if (node == NULL)
    {
        return 0;
    }

    if (xmlStrcmp(node->name, CONST_CHAR2XML("objective")) == 0)
    {
        if ((rc = get_text_content(node, "objective", objective)) != 0)
        {
            ERROR("'objective' content is not value");
            return rc;
        }
        node = xmlNodeNext(node);
    }

    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("verdicts")) != 0 &&
           xmlStrcmp(node->name, CONST_CHAR2XML("params")) != 0)
    {
        node = xmlNodeNext(node);
    }
    if (node == NULL)
    {
        return 0;
    }

    if (xmlStrcmp(node->name, CONST_CHAR2XML("verdicts")) == 0)
    {
        rc = get_verdicts(xmlNodeChildren(node), verdicts);
        if (rc != 0)
        {
            ERROR("Failed to parse parameters");
            return rc;
        }
        node = xmlNodeNext(node);
    }

    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("params")) != 0)
    {
        node = xmlNodeNext(node);
    }
    if (node != NULL)
    {
        rc = get_test_params(xmlNodeChildren(node), args);
        if (rc != 0)
        {
            ERROR("Failed to parse parameters");
            return rc;
        }
        node = xmlNodeNext(node);
    }

    return 0;
}


static int
get_test_result(xmlNodePtr root, trc_test_type type, test_runs *tests)
{
    int                 rc = 0;
    xmlNodePtr          node = NULL;
    char               *name;
    test_run           *test;
    test_iter          *iter;
    te_bool             new_test = FALSE;
    char               *objective = NULL;
    test_args           args;
    tqh_string          verdicts;


    name = xmlGetProp(root, CONST_CHAR2XML("name"));
    if ((name == NULL) && (type != TRC_TEST_SESSION))
    {
        ERROR("Missing name of the test package/script");
        return EINVAL;
    }
    if (name != NULL)
    {
        test = trc_db_find_by_name(tests, name);
        if (test == NULL)
        {
            new_test = TRUE;
            test = calloc(1, sizeof(*test));
            if (test == NULL)
            {
                free(name);
                ERROR("calloc() failed");
                return errno;
            }
            test->type = type;
            test->name = name;
            TAILQ_INIT(&test->iters.head);
            TAILQ_INSERT_TAIL(&tests->head, test, links);
        }
        else
        {
            free(name);
            name = NULL;
        }

        node = xmlNodeChildren(root);

        memset(&args, 0, sizeof(args));
        TAILQ_INIT(&args.head);
        TAILQ_INIT(&verdicts);

        rc = get_meta(node, &objective, &args, &verdicts);
        if (rc != 0)
        {
            ERROR("Failed to get meta data");
            return rc;
        }
        node = xmlNodeNext(node);

        if (new_test)
        {
            test->objective = objective;
            iter = NULL;
        }
        else
        {
            if ((test->objective == NULL) && (objective != NULL) &&
                (strlen(objective) > 0))
            {
                test->obj_update = TRUE;
                test->objective = objective;
            }
            else
            {
                free(objective);
            }
            iter = trc_db_find_by_args(&test->iters, &args);
        }

        if (iter == NULL)
        {
            new_test = TRUE;
            iter = calloc(1, sizeof(*iter));
            if (iter == NULL)
            {
                ERROR("calloc() failed");
                return errno;
            }
            iter->args = args;
            iter->exp_result.value = TRC_TEST_UNSPEC;
            if (type == TRC_TEST_SCRIPT)
                iter->stats.not_run = 1;
            TAILQ_INIT(&iter->tests.head);
            TAILQ_INSERT_TAIL(&test->iters.head, iter, links);
        }
        else
        {
            test_arg   *arg;

            while ((arg = args.head.tqh_first) != NULL)
            {
                TAILQ_REMOVE(&args.head, arg, links);
                free(arg->name);
                free(arg->value);
                free(arg);
            }
        }
        iter->used = TRUE;

        iter->got_verdicts = verdicts;
        rc = get_result(root, &iter->got_result);
        if (rc != 0)
        {
            ERROR("Failed to get the result");
            return rc;
        }

        if (test->type == TRC_TEST_SCRIPT)
            iter_stats_update_by_result(iter);
        else if (iter->got_result == iter->exp_result.value)
            iter->got_as_expect = TRUE;

        tests = &iter->tests;
    }
    else
    {
        node = xmlNodeChildren(root);

        while (node != NULL &&
               xmlStrcmp(node->name, CONST_CHAR2XML("branch")) != 0)
        {
            node = xmlNodeNext(node);
        }
    }

    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("branch")) == 0)
    {
        rc = get_logs(xmlNodeChildren(node), tests);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("logs")) == 0 &&
        (node = xmlNodeNext(node)) != NULL)
    {
        ERROR("Unexpected node '%s'", node->name);
        return EINVAL;
    }

    return rc;
}

/**
 */
static int
get_logs(xmlNodePtr node, test_runs *tests)
{
    int rc = 0;

    while (node != NULL)
    {
        if (xmlStrcmp(node->name, CONST_CHAR2XML("pkg")) == 0)
        {
            rc = get_test_result(node, TRC_TEST_PACKAGE, tests);
            if (rc != 0)
                break;
        }
        else if (xmlStrcmp(node->name, CONST_CHAR2XML("session")) == 0)
        {
            rc = get_test_result(node, TRC_TEST_SESSION, tests);
            if (rc != 0)
                break;
        }
        else if (xmlStrcmp(node->name, CONST_CHAR2XML("test")) == 0)
        {
            rc = get_test_result(node, TRC_TEST_SCRIPT, tests);
            if (rc != 0)
                break;
        }
        node = xmlNodeNext(node);
    }
    if (rc != 0)
    {
        ERROR("Failed to process '%s' node", node->name);
    }
    return rc; 
}


/* See description in trc_db.h */
int
trc_parse_log(const char *filename)
{
    xmlParserCtxtPtr    parser;
    xmlDocPtr           doc;
    xmlNodePtr          node;
#if HAVE_XMLERROR
    xmlError           *err;
#endif
    int                 rc;

    if (filename == NULL)
    {
        ERROR("Invalid file name");
        return EINVAL;
    }
    parser = xmlNewParserCtxt();
    if (parser == NULL)
    {
        ERROR("xmlNewParserCtxt() failed");
        return ENOMEM;
    }
    if ((doc = xmlCtxtReadFile(parser, filename, NULL,
                               XML_PARSE_NOBLANKS |
                               XML_PARSE_XINCLUDE |
                               XML_PARSE_NONET)) == NULL)
    {
#if HAVE_XMLERROR
        err = xmlCtxtGetLastError(parser);
        ERROR("Error occured during parsing XML log file:\n"
              "    %s:%d\n    %s", filename, err->line, err->message);
#else
        ERROR("Error occured during parsing XML log file:\n"
              "%s", filename);
#endif
        xmlFreeParserCtxt(parser);
        return EINVAL;
    }

    node = xmlDocGetRootElement(doc);
    if (node == NULL)
    {
        ERROR("Empty XML log file");
        rc = ENOENT;
    }
    else if (xmlStrcmp(node->name, CONST_CHAR2XML("log_report")) != 0)
    {
        ERROR("Unexpected root element of the XML log file");
        rc = EINVAL;
    }
    else
    {
        rc = get_logs(xmlNodeChildren(node), &trc_db.tests);
    }
    
    xmlFreeDoc(doc);
    xmlFreeParserCtxt(parser);
    xmlCleanupParser();

    return rc;  
}

