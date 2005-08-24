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
#include "trc_tag.h"
#include "trc_db.h"
#include "trc_xml.h"


/** Testing results comparison database */
trc_database trc_db;

/** XML document with expected testing results database */
static xmlDocPtr trc_db_doc = NULL;


static int get_tests(xmlNodePtr *node, test_runs *tests);


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
 * Allocate and get test iteration.
 *
 * @param node      XML node
 * @param iters     List of iterations to be filled in
 *
 * @return Status code.
 */
static int
alloc_and_get_test_iter(xmlNodePtr node, trc_test_type type,
                        test_iters *iters)
{
    int             rc;
    test_iter       *p;
    char            *tmp;
    trc_tag         *tag;
    trc_tags_entry  *tags_entry;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", (unsigned)sizeof(*p));
        return errno;
    }
    p->node = p->tests.node = node;
    if (type == TRC_TEST_SCRIPT)
        p->stats.not_run = 1;
    p->got_result = TRC_TEST_UNSPEC;
    TAILQ_INIT(&p->args.head);
    TAILQ_INIT(&p->tests.head);
    TAILQ_INSERT_TAIL(&iters->head, p, links);
    
    /* Get expected result */
    for (rc = ENOENT, tag = tags.tqh_first;
         tag != NULL;
         tag = tag->links.tqe_next)
    {
        rc = get_result(node, tag->name, &p->exp_result);
        if (rc != ENOENT)
            break;
    }
    if (rc != 0)
    {
        ERROR("Expected result of the test iteration is missing/invalid");
        return rc;
    }

    /* Get expected result in accordance with sets to diff */
    for (tags_entry = tags_diff.tqh_first;
         tags_entry != NULL;
         tags_entry = tags_entry->links.tqe_next)
    {
        for (tag = tags_entry->tags.tqh_first;
             tag != NULL;
             tag = tag->links.tqe_next)
        {
            rc = get_result(node, tag->name, &p->diff_exp[tags_entry->id]);
            if (rc != ENOENT)
                break;
        }
        if (rc != 0)
        {
            ERROR("Expected result of the test iteration is "
                  "missing/invalid");
            return rc;
        }
    }

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
        /* 'bug' property of the notes is optional */
        p->bug = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("bug")));

        rc = get_node_with_text_content(&node, "notes", &p->notes);
        if ((rc != 0) && (rc != ENOENT))
        {
            ERROR("Failed to get notes for the test iteration");
            return rc;
        }
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
get_test_iters(xmlNodePtr *node, trc_test_type type, test_iters *iters)
{
    int rc = 0;

    assert(node != NULL);
    assert(iters != NULL);

    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("iter")) == 0 &&
           (rc = alloc_and_get_test_iter(*node, type, iters)) == 0)
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
    if (p->name == NULL)
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
    INFO("Parsing test '%s' type=%d", p->name, p->type);
    free(tmp);

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
        /* 'bug' property of the notes is optional */
        p->bug = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("bug")));

        rc = get_node_with_text_content(&node, "notes", &p->notes);
        if ((rc != 0) && (rc != ENOENT))
        {
            ERROR("Failed to get objective of the test '%s'", p->name);
            return rc;
        }
    }

    rc = get_test_iters(&node, p->type, &p->iters);
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
                       BAD_CAST "UNSPEC");
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
                               BAD_CAST "notes",
                               BAD_CAST "");
            if (node == NULL)
            {
                ERROR("xmlNewChild() failed for 'notes'");
                return ENOMEM;
            }
            xmlNewProp(node, BAD_CAST "bug", BAD_CAST "");
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
                               BAD_CAST "notes", BAD_CAST "");
            if (node == NULL)
            {
                ERROR("xmlNewChild() failed for 'notes'");
                return ENOMEM;
            }
            xmlNewProp(node, BAD_CAST "bug", BAD_CAST "");
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

    while ((p = iters->head.tqh_first) != NULL)
    {
        TAILQ_REMOVE(&iters->head, p, links);
        trc_free_test_args(&p->args);
        free(p->notes);
        free(p->bug);
        trc_free_test_runs(&p->tests);
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
        free(p->bug);
        free(p->objective);
        free(p->obj_link);
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
