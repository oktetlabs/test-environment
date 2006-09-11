/** @file
 * @brief Testing Results Comparator
 *
 * Parser/dumper of expected results data base (XML format).
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

#include "te_config.h"
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

#include "logger_api.h"

#include "trc_tag.h"
#include "trc_db.h"
#include "trc_xml.h"
#include "logic_expr.h"


/** Testing results comparison database */
trc_database trc_db;

/** XML document with expected testing results database */
static xmlDocPtr trc_db_doc = NULL;


static int get_tests(xmlNodePtr *node, test_runs *tests,
                     const test_iter *parent_iter);


/**
 * Get text content of the node.
 *
 * @param node      Location of the XML node pointer
 * @param name      Expected name of the XML node
 * @param content   Location for the result
 *
 * @return Status code
 */
int
get_text_content(xmlNodePtr node, const char *name, char **content)
{
    if (node->children == NULL)
    {
        return 0;
    }
    if (node->children != node->last)
    {
        ERROR("Too many children in the node '%s' with text content",
              name);
        return EINVAL;
    }
    if (xmlStrcmp(node->children->name, CONST_CHAR2XML("text")) != 0)
    {
        ERROR("Unexpected element '%s' in the node '%s' with text "
              "content", node->children->name, name);
        return EINVAL;
    }
    if (node->children->content == NULL)
    {
        ERROR("Empty content of the node '%s'", name);
        return EINVAL;
    }

    *content = XML2CHAR_DUP(node->children->content);
    if (*content == NULL)
    {
        ERROR("String duplication failed");
        return ENOMEM;
    }

    return 0;
}

/**
 * Get node with text content.
 *
 * @param node      Location of the XML node pointer
 * @param name      Expected name of the XML node
 * @param content   Location for the result
 *
 * @return Status code
 */
static int
get_node_with_text_content(xmlNodePtr *node, const char *name,
                           char **content)
{
    int rc;

    if (xmlStrcmp((*node)->name, CONST_CHAR2XML(name)) != 0)
        return ENOENT;

    rc = get_text_content(*node, name, content);
    if (rc == 0)
        *node = xmlNodeNext(*node);

    return rc;
}

/**
 * Allocate and get test iteration argument.
 *
 * @param node      XML node
 * @param args      List of arguments to be filled in
 *
 * @return Status code.
 */
static int
alloc_and_get_test_arg(xmlNodePtr node, test_args *args)
{
    int         rc;
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

    rc = get_text_content(node, "arg", &p->value);
    if (rc != 0)
    {
        ERROR("Failed to get value of the argument '%s'", p->name);
    }

    return rc;
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
get_test_args(xmlNodePtr *node, test_args *args)
{
    int rc = 0;

    assert(node != NULL);
    assert(args != NULL);

    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("arg")) == 0 &&
           (rc = alloc_and_get_test_arg(*node, args)) == 0)
    {
        *node = xmlNodeNext(*node);
    }

    return rc;
}

/**
 * Get the result.
 *
 * @param node      XML node
 * @param name      Name of XML property to get.
 * @param result    Location for got result
 *
 * @return Status code.
 */
static int
get_result(xmlNodePtr node, const char *name, trc_test_result *result)
{
    char *tmp;

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML(name)));
    if (tmp == NULL)
        return ENOENT;
    INFO("Expected result is '%s'", tmp);
    if (strcmp(tmp, "PASSED") == 0)
        *result = TRC_TEST_PASSED;
    else if (strcmp(tmp, "FAILED") == 0)
        *result = TRC_TEST_FAILED;
    else if (strcmp(tmp, "SKIPPED") == 0)
        *result = TRC_TEST_SKIPPED;
    else if (strcmp(tmp, "UNSPEC") == 0)
        *result = TRC_TEST_UNSPEC;
    else
    {
        ERROR("Unknown result '%s' of the test iteration", tmp);
        free(tmp);
        return EINVAL;
    }
    free(tmp);

    return 0;
}

/**
 * Get expected result in accordance with list of tags.
 *
 * @param iter_node XML node of the parent iteration
 * @param node      Location of the first non-arg node of the test 
 *                  iteration (IN), of the first non-result node of the
 *                  test iteration (OUT)
 * @param tags      List of tags
 * @param result    Location for the expected result data
 *
 * @return Status code.
 */
static int
get_expected_result(xmlNodePtr iter_node, xmlNodePtr *node,
                    trc_tags *tags, trc_exp_result *result)
{
    int         rc = 0;
    xmlNodePtr  result_node, p;
    int         tagged_result_prio;

    struct trc_tagged_result {
        LIST_ENTRY(trc_tagged_result)   links;
        
        xmlNodePtr          node;
        char               *tags_expr_str;
        logic_expr         *tags_expr;
        trc_test_result     value;
    } *tagged_result, *tmp;

    LIST_HEAD(trc_tagged_results, trc_tagged_result)    tagged_results;


    assert(node != NULL);
    assert(tags != NULL);
    assert(result != NULL);

    /* Get all tagged results and remember corresponding XML node */
    LIST_INIT(&tagged_results);
    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("results")) == 0)
    {
        p = xmlNodeChildren(*node);
        if (xmlStrcmp(p->name, CONST_CHAR2XML("result")) != 0)
        {
            ERROR("Unexpected node '%s' in results", XML2CHAR(p->name));
            rc = EINVAL;
            goto exit;
        }
            
        if (tags->tqh_first != NULL)
        {
            tagged_result = calloc(1, sizeof(*tagged_result));
            if (tagged_result == NULL)
            {
                rc = ENOMEM;
                goto exit;
            }
            LIST_INSERT_HEAD(&tagged_results, tagged_result, links);
            tagged_result->node = *node;
            tagged_result->tags_expr_str =
                XML2CHAR(xmlGetProp(*node, CONST_CHAR2XML("tags")));
            if (logic_expr_parse(tagged_result->tags_expr_str,
                                 &tagged_result->tags_expr) != 0)
            {
                rc = EINVAL;
                goto exit;
            }
            rc = get_result(p, "value", &tagged_result->value);
            if (rc != 0)
                goto exit;
            INFO("Tagged result %#08x: tag='%s' value=%d",
                 (unsigned)tagged_result, tagged_result->tags_expr_str,
                 tagged_result->value);
        }
        *node = xmlNodeNext(*node); 
    }

    /* Do we have a tag with expected SKIPPED result? */
    for (tagged_result = NULL, tagged_result_prio = 0,
             tmp = tagged_results.lh_first;
         tmp != NULL;
         tmp = tmp->links.le_next)
    {
        int res = logic_expr_match(tmp->tags_expr, tags);

        INFO("Tagged result %#08x: tag='%s' value=%u match=%u",
             (unsigned)tmp, tmp->tags_expr_str, tmp->value, res);
        if (res != 0)
        {
            if (tmp->value == TRC_TEST_SKIPPED)
            {
                /* Skipped results have top priority in any case */
                tagged_result = tmp;
                tagged_result_prio = res;
                INFO("Stop on SKIPPED tagged result %#08x: tag='%s'",
                     (unsigned)tmp, tmp->tags_expr_str);
                break;
            }
            if (tagged_result == NULL || res < tagged_result_prio)
            {
                tagged_result = tmp;
                tagged_result_prio = res;
                INFO("Intermediate tagged result %#08x: tag='%s' prio=%d",
                     (unsigned)tmp, tmp->tags_expr_str, res);
            }
        }
    }

    /* We have found matching tagged result */
    if (tagged_result != NULL)
    {
        xmlNodePtr  p;
        char       *s;
        tqe_string *v;

        result->value = tagged_result->value;
        result_node = tagged_result->node;
        p = xmlNodeChildren(xmlNodeChildren(result_node));
        while (p != NULL)
        {
            rc = get_node_with_text_content(&p, "verdict", &s);
            if (rc == ENOENT)
            {
                ERROR("Unexpected node '%s' in the tagged result", p->name);
                rc = EINVAL;
                goto exit;
            }
            v = calloc(1, sizeof(*v));
            if (v == NULL)
            {
                ERROR("Memory allocation failure");
                free(s);
                rc = ENOMEM;
                goto exit;
            }
            v->v = s;
            TAILQ_INSERT_TAIL(&result->verdicts, v, links);
        }
    }
    else
    {
        rc = get_result(iter_node, "result", &result->value);
        if (rc != 0)
        {
            ERROR("Failed to get default result of the test iteration");
            goto exit;
        }
        result_node = iter_node;
    }
    result->key = XML2CHAR(xmlGetProp(result_node,
                                      CONST_CHAR2XML("key")));
    result->notes = XML2CHAR(xmlGetProp(result_node,
                                        CONST_CHAR2XML("notes")));

exit:
    /* Free allocated resources in any case */
    while ((tagged_result = tagged_results.lh_first) != NULL)
    {
        LIST_REMOVE(tagged_result, links);
        free(tagged_result->tags_expr_str);
        logic_expr_free(tagged_result->tags_expr);
        free(tagged_result);
    }

    return rc;
}

/**
 * Allocate and get test iteration.
 *
 * @param node          XML node
 * @param test_name     Name of the test
 * @param test_type     Type of the test
 * @param iters         List of iterations to be filled in
 * @param parent_iter   Parent test iteration
 *
 * @return Status code.
 */
static int
alloc_and_get_test_iter(xmlNodePtr node, const char *test_name, 
                        trc_test_type test_type, test_iters *iters,
                        const test_iter *parent_iter)
{
    int             rc;
    test_iter      *p;
    char           *tmp;
    xmlNodePtr      results;
    trc_tags_entry *tags_entry;
    unsigned int    i;

    UNUSED(test_name);

    INFO("New iteration of the test %s", test_name);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
        return errno;
    }
    p->node = p->tests.node = node;
    if (test_type == TRC_TEST_SCRIPT)
        p->stats.not_run = 1;
    TAILQ_INIT(&p->args.head);
    TAILQ_INIT(&p->tests.head);
    TAILQ_INIT(&p->exp_result.verdicts);
    for (i = 0; i < TRC_DIFF_IDS; ++i)
        TAILQ_INIT(&p->diff_exp[i].verdicts);
    p->got_result = TRC_TEST_UNSPEC;
    TAILQ_INIT(&p->got_verdicts);
    TAILQ_INSERT_TAIL(&iters->head, p, links);
    
    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("n")));
    if (tmp != NULL)
    {
        ERROR("Number of iterations is not supported yet");
        free(tmp);
        return EINVAL;
    }

    p->args.node = node;

    node = xmlNodeChildren(node);

    /* Get arguments of the iteration */
    rc = get_test_args(&node, &p->args);
    if (rc != 0)
        return rc;

    /* Get notes */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("notes")) == 0)
    {
        rc = get_node_with_text_content(&node, "notes", &p->notes);
        if ((rc != 0) && (rc != ENOENT))
        {
            ERROR("Failed to get notes for the test iteration");
            return rc;
        }
    }

    /* Remember estimated position of nodes with tagged results */
    results = node;

    /* Get expected result */
    rc = get_expected_result(p->args.node, &node, &tags, &p->exp_result);
    if (rc != 0)
    {
        ERROR("Expected result of the test iteration is missing/invalid");
        return rc;
    }
    if ((parent_iter != NULL) && 
        (parent_iter->exp_result.value == TRC_TEST_SKIPPED) &&
        (p->exp_result.value != TRC_TEST_SKIPPED))
    {
        INFO("Package iteration expects skipped result,\n"
             "but its item '%s' iteration doesn't - "
             "force to expect skipped.\n", test_name);
        p->exp_result.value = TRC_TEST_SKIPPED;
    }

    /* Get expected result in accordance with sets to diff */
    for (tags_entry = tags_diff.tqh_first;
         tags_entry != NULL;
         tags_entry = tags_entry->links.tqe_next)
    {
        node = results;
        rc = get_expected_result(p->args.node, &node, &tags_entry->tags,
                                 &p->diff_exp[tags_entry->id]);
        if (rc != 0)
        {
            ERROR("Expected result of the test iteration is "
                  "missing/invalid");
            return rc;
        }
        if ((parent_iter != NULL) && 
            (parent_iter->diff_exp[tags_entry->id].value ==
                 TRC_TEST_SKIPPED) &&
            (p->diff_exp[tags_entry->id].value != TRC_TEST_SKIPPED))
        {
            INFO("Package iteration expects skipped result,\n"
                 "but its item '%s' iteration doesn't - "
                 "force to expect skipped.\n", test_name);
            p->diff_exp[tags_entry->id].value = TRC_TEST_SKIPPED;
        }
    }

    /* Get sub-tests */
    rc = get_tests(&node, &p->tests, p);
    if (rc != 0)
        return rc;

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in test iteration", node->name);
        return EINVAL;
    }

    return 0;
}

/**
 * Get test iterations.
 *
 * @param node          XML node
 * @param test_name     Name of the test
 * @param test_type     Type of the test
 * @param iters         List of iterations to be filled in
 * @param parent_iter   Parent test iteration
 *
 * @return Status code
 */
static int
get_test_iters(xmlNodePtr *node, const char *test_name, 
               trc_test_type test_type, test_iters *iters,
               const test_iter *parent_iter)
{
    int rc = 0;

    assert(node != NULL);
    assert(iters != NULL);

    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("iter")) == 0 &&
           (rc = alloc_and_get_test_iter(*node, test_name, test_type, 
                                         iters, parent_iter)) == 0)
    {
        *node = xmlNodeNext(*node);
    }

    return rc;
}

/**
 * Get expected result of the test.
 *
 * @param node          XML node
 * @param tests         List of tests to add the new test
 * @param parent_iter   Parent test iteration
 *
 * @return Status code.
 */
static int
alloc_and_get_test(xmlNodePtr node, test_runs *tests,
                   const test_iter *parent_iter)
{
    int         rc;
    test_run   *p;
    char       *tmp;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
        return errno;
    }
    p->node = p->iters.node = node;
    TAILQ_INIT(&p->iters.head);
    TAILQ_INSERT_TAIL(&tests->head, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name of the test is missing");
        return EINVAL;
    }

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("type")));
    if (tmp == NULL)
    {
        p->type = TRC_TEST_SCRIPT;
    }
    else if (strcmp(tmp, "package") == 0)
    {
        p->type = TRC_TEST_PACKAGE;
    }
    else if (strcmp(tmp, "session") == 0)
    {
        p->type = TRC_TEST_SESSION;
    }
    else if (strcmp(tmp, "script") == 0)
    {
        p->type = TRC_TEST_SCRIPT;
    }
    else
    {
        ERROR("Invalid type '%s' of the test '%s'", tmp, p->name);
        free(tmp);
        return EINVAL;
    }
    free(tmp);

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("auxiliary")));
    if (tmp == NULL ||
        strcmp(tmp, "false") == 0)
    {
        p->aux = FALSE;
    }
    else if (strcmp(tmp, "true") == 0)
    {
        p->aux = TRUE;
    }
    else
    {
        ERROR("Invalid auxiliary property value '%s' of the test '%s'",
              tmp, p->name);
        free(tmp);
        return EINVAL;
    }
    free(tmp);

    INFO("Parsing test '%s' type=%d aux=%d", p->name, p->type, p->aux);

    node = xmlNodeChildren(node);

    p->obj_node = node;
    rc = get_node_with_text_content(&node, "objective", &p->objective);
    if (rc != 0)
    {
        ERROR("Failed to get objective of the test '%s'", p->name);
        return rc;
    }

    if (xmlStrcmp(node->name, CONST_CHAR2XML("notes")) == 0)
    {
        rc = get_node_with_text_content(&node, "notes", &p->notes);
        if ((rc != 0) && (rc != ENOENT))
        {
            ERROR("Failed to get objective of the test '%s'", p->name);
            return rc;
        }
    }

    rc = get_test_iters(&node, p->name, p->type, &p->iters, parent_iter);
    if (rc != 0)
    {
        ERROR("Failed to get iterations of the test '%s'", p->name);
        return rc;
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in test entry", node->name);
        return EINVAL;
    }

    return 0;
}

/**
 * Get tests.
 *
 * @param node          XML node
 * @param tests         List of tests to be filled in
 * @param parent_iter   Parent test iteration
 *
 * @return Status code
 */
static int
get_tests(xmlNodePtr *node, test_runs *tests, const test_iter *parent_iter)
{
    int rc = 0;

    assert(node != NULL);
    assert(tests != NULL);

    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("test")) == 0 &&
           (rc = alloc_and_get_test(*node, tests, parent_iter)) == 0)
    {
        *node = xmlNodeNext(*node);
    }
    if (*node != NULL)
    {
        ERROR("Unexpected element '%s'", (*node)->name);
        rc = EINVAL;
    }

    return rc;
}


/* See description in trc_db.h */
int
trc_parse_db(const char *filename)
{
    xmlParserCtxtPtr    parser;
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
    if ((trc_db_doc = xmlCtxtReadFile(parser, filename, NULL,
                                      XML_PARSE_NOBLANKS |
                                      XML_PARSE_XINCLUDE |
                                      XML_PARSE_NONET)) == NULL)
    {
#if HAVE_XMLERROR
        err = xmlCtxtGetLastError(parser);
        ERROR("Error occured during parsing configuration file:\n"
              "    %s:%d\n    %s", filename, err->line, err->message);
#else
        ERROR("Error occured during parsing configuration file:\n"
              "%s", filename);
#endif
        xmlFreeParserCtxt(parser);
        return EINVAL;
    }

    node = xmlDocGetRootElement(trc_db_doc);

    if (node == NULL)
    {
        ERROR("Empty XML document of the DB with expected testing "
              "results");
        rc = EINVAL;
    }
    else if (xmlStrcmp(node->name, CONST_CHAR2XML("trc_db")) != 0)
    {
        ERROR("Unexpected root element of the DB XML file");
        rc = EINVAL;
    }
    else
    {
        trc_db.version = XML2CHAR(xmlGetProp(node,
                                             CONST_CHAR2XML("version")));
        if (trc_db.version == NULL)
        {
            ERROR("Version of the TRC DB is missing");
            return EINVAL;
        }

        node = xmlNodeChildren(node);
        rc = get_tests(&node, &trc_db.tests, NULL);
        if (rc != 0)
        {
            ERROR("Preprocessing of DB with expected testing results in "
                  "file '%s' failed", filename);
        }
        else
        {
            INFO("DB with expected testing results in file '%s' "
                 "parsed successfully", filename);
        }
    }

    xmlFreeParserCtxt(parser);
    xmlCleanupParser();

    return rc;
}

static int trc_update_tests(test_runs *tests);

static int
trc_update_iters(test_iters *iters)
{
    int         rc;
    test_iter  *p;

    for (p = iters->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->node == NULL)
        {
            test_arg   *a;
            xmlNodePtr  node;

            INFO("Add node for iteration %p node=%p", iters, iters->node);
            p->tests.node = xmlNewChild(iters->node, NULL,
                                        BAD_CAST "iter", NULL);
            if (p->tests.node == NULL)
            {
                ERROR("xmlNewChild() failed");
                return ENOMEM;
            }
            xmlNewProp(p->tests.node, BAD_CAST "result",
                       BAD_CAST "PASSED");
            for (a = p->args.head.tqh_first;
                 a != NULL;
                 a = a->links.tqe_next)
            {
                xmlNodePtr arg = xmlNewChild(p->tests.node, NULL,
                                            BAD_CAST "arg",
                                            BAD_CAST a->value);
                if (arg == NULL)
                {
                    ERROR("xmlNewChild() failed for 'arg'");
                    return ENOMEM;
                }
                xmlNewProp(arg, BAD_CAST "name", BAD_CAST a->name);
            }
            node = xmlNewChild(p->tests.node, NULL,
                               BAD_CAST "notes", NULL);
            if (node == NULL)
            {
                ERROR("xmlNewChild() failed for 'notes'");
                return ENOMEM;
            }
        }
        rc = trc_update_tests(&p->tests);
        if (rc != 0)
            return rc;
    }
    return 0;
}

static const char *
trc_test_type_to_str(trc_test_type type)
{
    switch (type)
    {
        case TRC_TEST_SCRIPT:   return "script";
        case TRC_TEST_PACKAGE:  return "package";
        case TRC_TEST_SESSION:  return "session";
        default: return "OOps";
    }
}

static int
trc_update_tests(test_runs *tests)
{
    int         rc;
    test_run   *p;
    xmlNodePtr  node;

    for (p = tests->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->node == NULL)
        {
            INFO("Add node for '%s'", p->name);
            p->iters.node = xmlNewChild(tests->node, NULL,
                                        BAD_CAST "test",
                                        NULL);
            if (p->iters.node == NULL)
            {
                ERROR("xmlNewChild() failed for 'test'");
                return ENOMEM;
            }
            xmlNewProp(p->iters.node, BAD_CAST "name",
                       BAD_CAST p->name);
            xmlNewProp(p->iters.node, BAD_CAST "type",
                       BAD_CAST trc_test_type_to_str(p->type));
            if (xmlNewChild(p->iters.node, NULL,
                            BAD_CAST "objective",
                            BAD_CAST p->objective) == NULL)
            {
                ERROR("xmlNewChild() failed for 'objective'");
                return ENOMEM;
            }
            node = xmlNewChild(p->iters.node, NULL,
                               BAD_CAST "notes", NULL);
            if (node == NULL)
            {
                ERROR("xmlNewChild() failed for 'notes'");
                return ENOMEM;
            }
        }
        if (p->obj_update)
        {
            xmlNodeSetContent(p->obj_node, BAD_CAST p->objective);
        }
        rc = trc_update_iters(&p->iters);
        if (rc != 0)
            return rc;
    }
    return 0;
}


/* See description in trc_db.h */
int
trc_dump_db(const char *filename, te_bool init)
{
    int rc;

    if (init)
    {
        xmlNodePtr node;

        trc_db_doc = xmlNewDoc(BAD_CAST "1.0");
        if (trc_db_doc == NULL)
        {
            ERROR("xmlNewDoc() failed");
            return ENOMEM;
        }
        node = xmlNewNode(NULL, BAD_CAST "trc_db");
        if (node == NULL)
        {
            ERROR("xmlNewNode() failed");
            return ENOMEM;
        }
        xmlDocSetRootElement(trc_db_doc, node);
        trc_db.tests.node = node;
    }
    if ((rc = trc_update_tests(&trc_db.tests)) != 0)
    {
        ERROR("Failed to update DB XML document");
        return rc;
    }
    if (xmlSaveFormatFileEnc(filename, trc_db_doc, "UTF-8", 1) == -1)
    {
        ERROR("xmlSaveFormatFileEnc(%s) failed", filename);
        return EINVAL;
    }
    else
    {
        printf("DB with expected testing results has been updated:\n%s\n\n",
               filename);
    }

    return 0;
}


static void trc_free_test_runs(test_runs *tests);

/**
 * Free resources allocated for the list of test arguments.
 *
 * @param args      List of test arguments to be freed
 */
static void
trc_free_test_args(test_args *args)
{
    test_arg   *p;

    while ((p = args->head.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&args->head, p, links);
        free(p->name);
        free(p->value);
        free(p);
    }
}

/**
 * Free resources allocated for the list of test iterations.
 *
 * @param iters     List of test iterations to be freed
 */
static void
trc_free_test_iters(test_iters *iters)
{
    test_iter  *p;
    unsigned    i;

    while ((p = iters->head.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&iters->head, p, links);
        trc_free_test_args(&p->args);
        free(p->notes);
        free(p->exp_result.key);
        free(p->exp_result.notes);
        tq_strings_free(&p->exp_result.verdicts, free);
        for (i = 0; i < TRC_DIFF_IDS; ++i)
        {
            free(p->diff_exp[i].key);
            free(p->diff_exp[i].notes);
            tq_strings_free(&p->diff_exp[i].verdicts, free);
        }
        trc_free_test_runs(&p->tests);
        tq_strings_free(&p->got_verdicts, free);
        free(p);
    }
}

/**
 * Free resources allocated for the list of tests.
 *
 * @param tests     List of tests to be freed
 */
static void
trc_free_test_runs(test_runs *tests)
{
    test_run   *p;

    while ((p = tests->head.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&tests->head, p, links);
        free(p->name);
        free(p->notes);
        free(p->objective);
        free(p->test_path);
        trc_free_test_iters(&p->iters);
        free(p);
    }
}

/* See description in trc_db.h */
void
trc_free_db(trc_database *db)
{
    free(db->version);
    trc_free_test_runs(&db->tests);
    xmlFreeDoc(trc_db_doc);
}
