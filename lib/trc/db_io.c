/** @file
 * @brief Testing Results Comparator
 *
 * Parser/dumper of expected results data base (XML format).
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "logger_api.h"
#include "te_alloc.h"
#include "te_test_result.h"
#include "logic_expr.h"

#include "te_trc.h"
#include "trc_db.h"


#define CONST_CHAR2XML  (const xmlChar *)
#define XML2CHAR(p)     ((char *)p)
#define XML2CHAR_DUP(p) XML2CHAR(xmlStrdup(p))


/** Widely used expected results */
static trc_exp_results  exp_defaults;
/** Are exp_defaults initialized? */
static te_bool          exp_defaults_inited = FALSE;


static te_errno get_tests(xmlNodePtr *node, trc_tests *tests,
                          trc_test_iter *parent);


/**
 * Free resourses allocated for widely used expected results.
 */
static void
exp_defaults_free(void)
{
    if (exp_defaults_inited)
    {
        trc_exp_result *p;

        while ((p = SLIST_FIRST(&exp_defaults)) != NULL)
        {
            SLIST_REMOVE(&exp_defaults, p, trc_exp_result, links);
            assert(TAILQ_FIRST(&p->results) != NULL);
            assert(TAILQ_NEXT(TAILQ_FIRST(&p->results), links) == NULL);
            assert(TAILQ_FIRST(
                       &TAILQ_FIRST(&p->results)->result.verdicts) == NULL);
            free(TAILQ_FIRST(&p->results));
            free(p);
        }
        exp_defaults_inited = FALSE;
    }
}

/**
 * Initialize set of widely used expected results.
 *
 * @return Status code.
 */
static void 
exp_defaults_init(void)
{
    if (!exp_defaults_inited)
    {
        SLIST_INIT(&exp_defaults);
        atexit(exp_defaults_free);
        exp_defaults_inited = TRUE;
    }
}

/**
 * Get expected results from set of widely used singleton results
 * without verdicts.
 *
 * @param status        Expected test status
 *
 * @return Pointer to expected result with signle entry with specified
 *         status and no verdicts.
 */
static const trc_exp_result *
exp_defaults_get(te_test_status status)
{
    trc_exp_result         *p;
    trc_exp_result_entry   *entry;

    exp_defaults_init();

    SLIST_FOREACH(p, &exp_defaults, links)
    {
        assert(TAILQ_FIRST(&p->results) != NULL);
        assert(TAILQ_NEXT(TAILQ_FIRST(&p->results), links) == NULL);
        assert(TAILQ_FIRST(
                   &TAILQ_FIRST(&p->results)->result.verdicts) == NULL);
        if (TAILQ_FIRST(&p->results)->result.status == status)
            return p;
    }

    p = TE_ALLOC(sizeof(*p));
    if (p == NULL)
        return NULL;
    TAILQ_INIT(&p->results);

    entry = TE_ALLOC(sizeof(*entry));
    if (entry == NULL)
    {
        free(p);
        return NULL;
    }
    TAILQ_INIT(&entry->result.verdicts);
    entry->result.status = status;
    TAILQ_INSERT_HEAD(&p->results, entry, links);
    
    SLIST_INSERT_HEAD(&exp_defaults, p, links);

    return p;
}


/**
 * Skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return Current, one of next or NULL.
 */
static inline xmlNodePtr
xmlNodeSkipComment(xmlNodePtr node)
{
    while ((node != NULL) &&
           (xmlStrcmp(node->name, CONST_CHAR2XML("comment")) == 0))
    {
        node = node->next;
    }
    return node;
}

/**
 * Go to the next XML node, skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return The next XML node.
 */
static inline xmlNodePtr
xmlNodeChildren(xmlNodePtr node)
{
    assert(node != NULL);
    return xmlNodeSkipComment(node->children);
}

/**
 * Go to the next XML node, skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return The next XML node.
 */
static inline xmlNodePtr
xmlNodeNext(xmlNodePtr node)
{
    assert(node != NULL);
    return xmlNodeSkipComment(node->next);
}


/**
 * Get text content of the node.
 *
 * @param node      Location of the XML node pointer
 * @param name      Expected name of the XML node
 * @param content   Location for the result
 *
 * @return Status code
 */
te_errno
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
        return TE_RC(TE_TRC, TE_EFMT);
    }
    if (xmlStrcmp(node->children->name, CONST_CHAR2XML("text")) != 0)
    {
        ERROR("Unexpected element '%s' in the node '%s' with text "
              "content", node->children->name, name);
        return TE_RC(TE_TRC, TE_EFMT);
    }
    if (node->children->content == NULL)
    {
        ERROR("Empty content of the node '%s'", name);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    *content = XML2CHAR_DUP(node->children->content);
    if (*content == NULL)
    {
        ERROR("String duplication failed");
        return TE_RC(TE_TRC, TE_ENOMEM);
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
static te_errno
get_node_with_text_content(xmlNodePtr *node, const char *name,
                           char **content)
{
    te_errno rc;

    if (xmlStrcmp((*node)->name, CONST_CHAR2XML(name)) != 0)
        return TE_ENOENT;

    rc = get_text_content(*node, name, content);
    if (rc == 0)
        *node = xmlNodeNext(*node);

    return rc;
}

/**
 * Get property. If value is empty string, override as NULL.
 *
 * @param node      XML node pointer
 * @param name      Name of the property to get
 * @param value     Location for value
 *
 * @return Status code.
 */
static te_errno
get_node_property(xmlNodePtr node, const char *name, char **value)
{
    xmlChar *xml_value = xmlGetProp(node, CONST_CHAR2XML(name));

    if (xml_value == NULL)
        return TE_ENOENT;

    assert(value != NULL);
    *value = XML2CHAR(xml_value);
    assert(*value != NULL);

    if (**value == '\0')
    {
        xmlFree(xml_value);
        *value = NULL;
    }

    return 0;
}


/**
 * Allocate and get test iteration argument.
 *
 * @param node      XML node
 * @param args      List of arguments to be filled in
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_test_arg(xmlNodePtr node, trc_test_iter_args *args)
{
    te_errno    rc;
    trc_test_iter_arg   *p;

    p = TE_ALLOC(sizeof(*p));
    if (p == NULL)
        return TE_RC(TE_TRC, TE_ENOMEM);
    p->node = node;
    TAILQ_INSERT_TAIL(&args->head, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name of the argument is missing");
        return TE_RC(TE_TRC, TE_EFMT);
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
static te_errno
get_test_args(xmlNodePtr *node, trc_test_iter_args *args)
{
    te_errno rc = 0;

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
static te_errno
get_result(xmlNodePtr node, const char *name, te_test_status *result)
{
    char *tmp;

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML(name)));
    if (tmp == NULL)
        return ENOENT;
    INFO("Expected result is '%s'", tmp);
    if (strcmp(tmp, "PASSED") == 0)
        *result = TE_TEST_PASSED;
    else if (strcmp(tmp, "FAILED") == 0)
        *result = TE_TEST_FAILED;
    else if (strcmp(tmp, "SKIPPED") == 0)
        *result = TE_TEST_SKIPPED;
    else if (strcmp(tmp, "UNSPEC") == 0)
        *result = TE_TEST_UNSPEC;
    else
    {
        ERROR("Unknown result '%s' of the test iteration", tmp);
        free(tmp);
        return TE_RC(TE_TRC, TE_EFMT);
    }
    free(tmp);

    return 0;
}

/**
 * Get expected result in accordance with list of tags.
 *
 * @param node      Location of the first non-arg node of the test 
 *                  iteration (IN), of the first non-result node of the
 *                  test iteration (OUT)
 * @param results   Location for the expected results data
 *
 * @return Status code.
 */
static te_errno
get_expected_results(xmlNodePtr *node, trc_exp_results *results)
{
    te_errno                rc = 0;
    trc_exp_result         *result;
    trc_exp_result_entry   *entry;
    xmlNodePtr              p, q;
    te_test_verdict        *v;

    assert(node != NULL);
    assert(results != NULL);

    /* Get all tagged results and remember corresponding XML node */
    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("results")) == 0)
    {
        result = TE_ALLOC(sizeof(*result));
        if (result == NULL)
            return TE_ENOMEM;
        TAILQ_INIT(&result->results);
        SLIST_INSERT_HEAD(results, result, links);

        result->tags_str = XML2CHAR(xmlGetProp(*node,
                                               CONST_CHAR2XML("tags")));
        if (logic_expr_parse(result->tags_str, &result->tags_expr) != 0)
            return TE_RC(TE_TRC, TE_EINVAL);

        get_node_property(*node, "key", &result->key);
        get_node_property(*node, "notes", &result->notes);

        for (p = xmlNodeChildren(*node); p != NULL; p = xmlNodeNext(p))
        {
            if (xmlStrcmp(p->name, CONST_CHAR2XML("result")) != 0)
            {
                ERROR("Unexpected node '%s' in the tagged result",
                      p->name);
                return TE_RC(TE_TRC, TE_EFMT);
            }

            entry = TE_ALLOC(sizeof(*entry));
            if (entry == NULL)
                return TE_ENOMEM;
            te_test_result_init(&entry->result);
            TAILQ_INSERT_TAIL(&result->results, entry, links);

            rc = get_result(p, "value", &entry->result.status);
            if (rc != 0)
                return rc;

            get_node_property(p, "key", &entry->key);
            get_node_property(p, "notes", &entry->notes);

            q = xmlNodeChildren(p);
            while (q != NULL)
            {
                char *s;

                rc = get_node_with_text_content(&q, "verdict", &s);
                if (rc == TE_ENOENT)
                {
                    ERROR("Unexpected node '%s' in the tagged result "
                          "entry", q->name);
                    return TE_RC(TE_TRC, TE_EFMT);
                }
                v = TE_ALLOC(sizeof(*v));
                if (v == NULL)
                {
                    free(s);
                    return TE_ENOMEM;
                }
                v->str = s;
                TAILQ_INSERT_TAIL(&entry->result.verdicts, v, links);
            }
        }

        *node = xmlNodeNext(*node); 
    }

    return 0;
}

/**
 * Allocate and get test iteration.
 *
 * @param node          XML node
 * @param test          Parent test
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_test_iter(xmlNodePtr node, trc_test *test)
{
    te_errno        rc;
    trc_test_iter  *p;
    char           *tmp;
    te_test_status  def;

    INFO("New iteration of the test %s", test->name);

    p = trc_db_new_test_iter(test, 0, NULL, NULL);
    if (p == NULL)
        return TE_RC(TE_TRC, TE_ENOMEM);
    p->node = p->tests.node = node;
    
    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("n")));
    if (tmp != NULL)
    {
        ERROR("Number of iterations is not supported yet");
        free(tmp);
        return TE_RC(TE_TRC, TE_ENOSYS);
    }

    rc = get_result(node, "result", &def);
    if (rc != 0)
        return rc;
    p->exp_default = exp_defaults_get(def);
    if (p->exp_default == NULL)
        return TE_ENOMEM;

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
        if ((rc != 0) && (rc != TE_ENOENT))
        {
            ERROR("Failed to get notes for the test iteration");
            return rc;
        }
    }

    /* Get expected results */
    rc = get_expected_results(&node, &p->exp_results);
    if (rc != 0)
    {
        ERROR("Expected results of the test iteration is "
              "missing/invalid");
        return rc;
    }

    /* Get sub-tests */
    rc = get_tests(&node, &p->tests, p);
    if (rc != 0)
        return rc;

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in test iteration", node->name);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    return 0;
}

/**
 * Get test iterations.
 *
 * @param node          XML node
 * @param test          Parent test
 *
 * @return Status code
 */
static te_errno
get_test_iters(xmlNodePtr *node, trc_test *parent)
{
    te_errno rc = 0;

    assert(node != NULL);
    assert(parent != NULL);

    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("iter")) == 0 &&
           (rc = alloc_and_get_test_iter(*node, parent)) == 0)
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
 * @param parent        Parent iteration
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_test(xmlNodePtr node, trc_tests *tests,
                   trc_test_iter *parent)
{
    te_errno    rc;
    trc_test   *p;
    char       *tmp;

    assert(tests != NULL);

    p = trc_db_new_test(tests, parent, NULL);
    if (p == NULL)
        return TE_RC(TE_TRC, TE_ENOMEM);
    p->node = p->iters.node = node;

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name of the test is missing");
        return TE_RC(TE_TRC, TE_EFMT);
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
        return TE_RC(TE_TRC, TE_EFMT);
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
        return TE_RC(TE_TRC, TE_EFMT);
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
        if ((rc != 0) && (rc != TE_ENOENT))
        {
            ERROR("Failed to get objective of the test '%s'", p->name);
            return rc;
        }
    }

    rc = get_test_iters(&node, p);
    if (rc != 0)
    {
        ERROR("Failed to get iterations of the test '%s'", p->name);
        return rc;
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in test entry", node->name);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    return 0;
}

/**
 * Get tests.
 *
 * @param node          XML node
 * @param tests         List of tests to be filled in
 * @param parent        Parent iteration
 *
 * @return Status code
 */
static te_errno
get_tests(xmlNodePtr *node, trc_tests *tests, trc_test_iter *parent)
{
    te_errno rc = 0;

    assert(node != NULL);
    assert(tests != NULL);

    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("test")) == 0 &&
           (rc = alloc_and_get_test(*node, tests, parent)) == 0)
    {
        *node = xmlNodeNext(*node);
    }
    if (*node != NULL)
    {
        ERROR("Unexpected element '%s'", (*node)->name);
        rc = TE_RC(TE_TRC, TE_EFMT);
    }

    return rc;
}


/* See description in te_trc.h */
te_errno
trc_db_open(const char *location, te_trc_db **db)
{
    xmlParserCtxtPtr    parser;
    xmlNodePtr          node;
#if HAVE_XMLERROR
    xmlError           *err;
#endif
    te_errno            rc;

    if (location == NULL)
    {
        ERROR("Invalid location of the TRC database");
        return TE_RC(TE_TRC, TE_EFAULT);
    }

    *db = TE_ALLOC(sizeof(**db));
    if (*db == NULL)
        return TE_ENOMEM;
    TAILQ_INIT(&(*db)->tests.head);

    (*db)->filename = strdup(location);
    if ((*db)->filename == NULL)
        return TE_ENOMEM;

    parser = xmlNewParserCtxt();
    if (parser == NULL)
    {
        ERROR("xmlNewParserCtxt() failed");
        return TE_ENOMEM;
    }
    if (((*db)->xml_doc = xmlCtxtReadFile(parser, (*db)->filename, NULL,
                                      XML_PARSE_NOBLANKS |
                                      XML_PARSE_XINCLUDE |
                                      XML_PARSE_NONET)) == NULL)
    {
#if HAVE_XMLERROR
        err = xmlCtxtGetLastError(parser);
        ERROR("Error occured during parsing configuration file:\n"
              "    %s:%d\n    %s", (*db)->filename,
              err->line, err->message);
#else
        ERROR("Error occured during parsing configuration file:\n"
              "%s", (*db)->filename);
#endif
        xmlFreeParserCtxt(parser);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    node = xmlDocGetRootElement((*db)->xml_doc);

    if (node == NULL)
    {
        ERROR("Empty XML document of the DB with expected testing "
              "results");
        rc = TE_RC(TE_TRC, TE_EINVAL);
    }
    else if (xmlStrcmp(node->name, CONST_CHAR2XML("trc_db")) != 0)
    {
        ERROR("Unexpected root element of the DB XML file");
        rc = TE_RC(TE_TRC, TE_EFMT);
    }
    else
    {
        (*db)->version = XML2CHAR(xmlGetProp(node,
                                          CONST_CHAR2XML("version")));
        if ((*db)->version == NULL)
        {
            INFO("Version of the TRC DB is missing");
        }

        node = xmlNodeChildren(node);
        rc = get_tests(&node, &(*db)->tests, NULL);
        if (rc != 0)
        {
            ERROR("Preprocessing of DB with expected testing results in "
                  "file '%s' failed", (*db)->filename);
        }
        else
        {
            INFO("DB with expected testing results in file '%s' "
                 "parsed successfully", (*db)->filename);
        }
    }

    xmlFreeParserCtxt(parser);
    xmlCleanupParser();

    return rc;
}

static te_errno trc_update_tests(trc_tests *tests);

static te_errno
trc_update_iters(trc_test_iters *iters)
{
    te_errno    rc;
    trc_test_iter  *p;

    TAILQ_FOREACH(p, &iters->head, links)
    {
        if (p->node == NULL)
        {
            trc_test_iter_arg  *a;
            xmlNodePtr          node;

            INFO("Add node for iteration %p node=%p", iters, iters->node);
            p->tests.node = xmlNewChild(iters->node, NULL,
                                        BAD_CAST "iter", NULL);
            if (p->tests.node == NULL)
            {
                ERROR("xmlNewChild() failed");
                return TE_ENOMEM;
            }
            xmlNewProp(p->tests.node, BAD_CAST "result",
                       BAD_CAST "PASSED");
            TAILQ_FOREACH(a, &p->args.head, links)
            {
                xmlNodePtr arg = xmlNewChild(p->tests.node, NULL,
                                            BAD_CAST "arg",
                                            BAD_CAST a->value);
                if (arg == NULL)
                {
                    ERROR("xmlNewChild() failed for 'arg'");
                    return TE_ENOMEM;
                }
                xmlNewProp(arg, BAD_CAST "name", BAD_CAST a->name);
            }
            node = xmlNewChild(p->tests.node, NULL,
                               BAD_CAST "notes", NULL);
            if (node == NULL)
            {
                ERROR("xmlNewChild() failed for 'notes'");
                return TE_ENOMEM;
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

static te_errno
trc_update_tests(trc_tests *tests)
{
    te_errno    rc;
    trc_test   *p;
    xmlNodePtr  node;

    TAILQ_FOREACH(p, &tests->head, links)
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
                return TE_ENOMEM;
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
                return TE_ENOMEM;
            }
            node = xmlNewChild(p->iters.node, NULL,
                               BAD_CAST "notes", NULL);
            if (node == NULL)
            {
                ERROR("xmlNewChild() failed for 'notes'");
                return TE_ENOMEM;
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
te_errno
trc_db_save(te_trc_db *db, const char *filename)
{
    const char *fn = (filename != NULL) ? filename : db->filename;
    te_errno    rc;

    if (db->xml_doc == NULL)
    {
        xmlNodePtr node;

        db->xml_doc = xmlNewDoc(BAD_CAST "1.0");
        if (db->xml_doc == NULL)
        {
            ERROR("xmlNewDoc() failed");
            return TE_ENOMEM;
        }
        node = xmlNewNode(NULL, BAD_CAST "trc_db");
        if (node == NULL)
        {
            ERROR("xmlNewNode() failed");
            return TE_ENOMEM;
        }
        xmlDocSetRootElement(db->xml_doc, node);
        db->tests.node = node;
    }
    if ((rc = trc_update_tests(&db->tests)) != 0)
    {
        ERROR("Failed to update DB XML document");
        return rc;
    }
    if (xmlSaveFormatFileEnc(fn, db->xml_doc, "UTF-8", 1) == -1)
    {
        ERROR("xmlSaveFormatFileEnc(%s) failed", fn);
        return TE_RC(TE_TRC, TE_EFAULT);
    }
    else
    {
        RING("DB with expected testing results has been updated:\n%s\n\n",
             fn);
    }

    return 0;
}
