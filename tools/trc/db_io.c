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


/** Testing results comparison database */
trc_database trc_db;

/** XML document with expected testing results database */
static xmlDocPtr trc_db_doc = NULL;


static int get_tests(xmlNodePtr *node, test_runs *tests);


/**
 * Get boolean property.
 *
 * @param node      Node with boolean property
 * @param name      Name of the property to get
 * @param value     Location for value
 *
 * @return Status code.
 * @retval ENOENT   Property does not exists. Value is not modified.
 */
static int
get_bool_prop(xmlNodePtr node, const char *name, te_bool *value)
{
    xmlChar *s = xmlGetProp(node, CONST_CHAR2XML(name));

    if (s == NULL)
        return ENOENT;
    if (xmlStrcmp(s, CONST_CHAR2XML("true")) == 0)
        *value = TRUE;
    else if (xmlStrcmp(s, CONST_CHAR2XML("false")) == 0)
        *value = FALSE;
    else
    {
        ERROR("Invalid value '%s' of the boolean property '%s'",
              XML2CHAR(s), name);
        xmlFree(s);
        return EINVAL;
    }
    xmlFree(s);

    return 0;
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
int
get_text_content(xmlNodePtr node, const char *name, char **content)
{
    if (node->children == NULL)
    {
        *content = strdup("");
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
        ERROR("calloc(1, %u) failed", sizeof(*p));
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
 * Allocate and get test iteration.
 *
 * @param node      XML node
 * @param iters     List of iterations to be filled in
 *
 * @return Status code.
 */
static int
alloc_and_get_test_iter(xmlNodePtr node, test_iters *iters)
{
    int         rc;
    test_iter  *p;
    char       *tmp;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", sizeof(*p));
        return errno;
    }
    p->node = node;
    p->stats.not_run = 1;
    TAILQ_INIT(&p->args.head);
    TAILQ_INIT(&p->tests.head);
    TAILQ_INSERT_TAIL(&iters->head, p, links);

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("result")));
    if (tmp == NULL)
    {
        ERROR("Expected result of the test iteration is missing");
        return EINVAL;
    }
    INFO("Expected result is '%s'", tmp);
    if (strcmp(tmp, "passed") == 0)
        p->exp_result = TRC_TEST_PASSED;
    else if (strcmp(tmp, "failed") == 0)
        p->exp_result = TRC_TEST_FAILED;
    else if (strcmp(tmp, "killed") == 0)
        p->exp_result = TRC_TEST_KILLED;
    else if (strcmp(tmp, "cored") == 0)
        p->exp_result = TRC_TEST_CORED;
    else if (strcmp(tmp, "skipped") == 0)
        p->exp_result = TRC_TEST_SKIPPED;
    else if (strcmp(tmp, "UNSPEC") == 0)
        p->exp_result = TRC_TEST_UNSPEC;
    else
    {
        ERROR("Unknown result '%s' of the test iteration", tmp);
        free(tmp);
        return EINVAL;
    }
    free(tmp);

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
    rc = get_node_with_text_content(&node, "notes", &p->notes);
    if ((rc != 0) && (rc != ENOENT))
    {
        ERROR("Failed to get notes for the test iteration");
        return rc;
    }

    /* Get sub-tests */
    rc = get_tests(&node, &p->tests);
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
 * @param node      XML node
 * @param iters     List of iterations to be filled in
 *
 * @return Status code
 */
static int
get_test_iters(xmlNodePtr *node, test_iters *iters)
{
    int rc = 0;

    assert(node != NULL);
    assert(iters != NULL);

    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("iter")) == 0 &&
           (rc = alloc_and_get_test_iter(*node, iters)) == 0)
    {
        *node = xmlNodeNext(*node);
    }

    return rc;
}

/**
 * Get expected result of the test.
 *
 * @param node      XML node
 * @param tests     List of tests to add the new test
 *
 * @return Status code.
 */
static int
alloc_and_get_test(xmlNodePtr node, test_runs *tests)
{
    int         rc;
    test_run   *p;
    char       *tmp;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", sizeof(*p));
        return errno;
    }
    p->node = node;
    TAILQ_INIT(&p->iters.head);
    TAILQ_INSERT_TAIL(&tests->head, p, links);
    
    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name of the test is missing");
        return EINVAL;
    }
    INFO("Parsing test '%s'", p->name);

#if 0
    /* 'package' property is optional, by default it is false */
    p->is_package = FALSE;
    rc = get_bool_prop(node, "package", &p->is_package);
    if (rc != 0 && rc != ENOENT)
        return rc;
#endif

    node = xmlNodeChildren(node);
    
    rc = get_node_with_text_content(&node, "objective", &p->objective);
    if (rc != 0)
    {
        ERROR("Failed to get objective of the test '%s'", p->name);
        return rc;
    }

    rc = get_test_iters(&node, &p->iters);
    if (rc != 0)
    {
        ERROR("Failed to get iterations of the test '%s'", p->name);
        return rc;
    }

    return 0;
}

/**
 * Get tests.
 *
 * @param node      XML node
 * @param tests     List of tests to be filled in
 *
 * @return Status code
 */
static int
get_tests(xmlNodePtr *node, test_runs *tests)
{
    int rc = 0;

    assert(node != NULL);
    assert(tests != NULL);

    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("test")) == 0 &&
           (rc = alloc_and_get_test(*node, tests)) == 0)
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
        node = xmlNodeChildren(node);
        rc = get_tests(&node, &trc_db.tests);
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

static int
trc_update_tests(test_runs *tests);

static int
trc_update_iters(test_iters *iters)
{
    int         rc;
    test_iter  *p;

    for (p = iters->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->node == NULL)
        {
            INFO("Add node for iteration");                 
            p->tests.node = xmlNewChild(iters->node, NULL,
                                        BAD_CAST "iter", NULL);
            if (p->tests.node == NULL)
            {
                ERROR("xmlNewChild() failed");
                return rc;
            }
            xmlNewProp(p->tests.node, BAD_CAST "result",
                       BAD_CAST "UNSPEC");
            if (xmlNewChild(p->tests.node, NULL,
                            BAD_CAST "notes",
                            BAD_CAST (p->notes ? : "")) == NULL)
            {
                ERROR("xmlNewChild() failed for 'notes'");
                return rc;
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
        case TRC_TEST_SCRIPT:   return "test";
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
                return rc;
            }
            xmlNewProp(p->iters.node, BAD_CAST "name",
                       BAD_CAST p->name);
            xmlNewProp(p->iters.node, BAD_CAST "type",
                       BAD_CAST trc_test_type_to_str(p->type));
            if (xmlNewChild(p->iters.node, NULL,
                            BAD_CAST "objective",
                            BAD_CAST (p->objective ? : "")) == NULL)
            {
                ERROR("xmlNewChild() failed for 'objective'");
                return rc;
            }
        }
        rc = trc_update_iters(&p->iters);
        if (rc != 0)
            return rc;
    }
    return 0;
}


/* See description in trc_db.h */
int
trc_dump_db(const char *filename)
{
    int rc;

    if (trc_init_db)
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

    return 0;
}
