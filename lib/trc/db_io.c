/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator
 *
 * Parser/dumper of expected results data base (XML format).
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include <libxml/tree.h>
#include <libxml/xinclude.h>

#include "logger_api.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_test_result.h"
#include "logic_expr.h"

#include "te_trc.h"
#include "trc_db.h"

#if 0
#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL (TE_LL_WARN | TE_LL_ERROR | \
                      TE_LL_VERB | TE_LL_ENTRY_EXIT | TE_LL_RING)
#endif

#define CONST_CHAR2XML  (const xmlChar *)
#define XML2CHAR(p)     ((char *)p)
#define XML2CHAR_DUP(p) XML2CHAR(xmlStrdup(p))

/** Maximum length of "pos" attribute */
#define MAX_POS_LEN     10

/* global database */
/* fixme kostik: may be the code is written in assumption
 * that there can be several databases. Then I'm very very sorry. */
te_trc_db *current_db;

/** Widely used expected results */
static trc_exp_results  exp_defaults;
/** Are exp_defaults initialized? */
static te_bool          exp_defaults_inited = FALSE;


static te_errno get_tests(xmlNodePtr *node, trc_tests *tests,
                          trc_test_iter *parent);

/** Queue of included files */
static trc_files       *inc_files = NULL;

/* insert markers to show where files were included */
static te_errno
trc_include_markers_add(xmlNodePtr parent, int flags)
{
    te_errno    rc = 0;
    xmlNodePtr  node;
    xmlNodePtr  aux_node;
    xmlNodePtr  marker;
    int         n = 0;

    if (parent == NULL)
        return 0;

    for (node = parent->children; node != NULL; node = node->next)
    {
        if (node->type == XML_XINCLUDE_START)
        {
            if (flags & TRC_SAVE_NO_VOID_XINCL)
            {
                n = 0;
                for (aux_node = node->next; aux_node != NULL;
                     aux_node = aux_node->next)
                {
                    if (aux_node->type == XML_XINCLUDE_START)
                        n++;
                    if (aux_node->type == XML_XINCLUDE_END)
                        n--;
                    if (aux_node->type == XML_ELEMENT_NODE ||
                        n == -1)
                        break;
                }

                if (n == -1)
                {
                    node = aux_node;
                    continue;
                }
            }

            marker = xmlNewNode(NULL, BAD_CAST "xinclude_start");
            marker->properties = xmlCopyPropList(marker,
                                                 node->properties);

            if (xmlAddNextSibling(node, marker) == NULL)
            {
                ERROR("Failed to add marker after include");
                return TE_RC(TE_TRC, TE_EFAULT);
            }
        }
        else if (node->type == XML_XINCLUDE_END)
        {
            marker = xmlNewNode(NULL, BAD_CAST "xinclude_end");

            if (xmlAddPrevSibling(node, marker) == NULL)
            {
                ERROR("Failed to add marker after include");
                return TE_RC(TE_TRC, TE_EFAULT);
            }
        }
        else if ((rc = trc_include_markers_add(node, flags)) != 0)
            return rc;
    }

    if (flags & TRC_SAVE_DEL_XINCL)
    {
        for (node = parent->children; node != NULL; node = aux_node)
        {
            aux_node = node->next;
            if (node->type == XML_XINCLUDE_START ||
                node->type == XML_XINCLUDE_END)
            {
                xmlUnlinkNode(node);
                xmlFreeNode(node);
            }
        }
    }

    return 0;
}

/**
 * Check whether node is xInclude node or its marker,
 * update queue of included files if so.
 *
 * @param node      XML node pointer
 */
static void
update_files(xmlNodePtr node)
{
    trc_file  *file;

    if (node == NULL)
        return;

    if (node->type == XML_XINCLUDE_START ||
        xmlStrcmp(node->name,
                  CONST_CHAR2XML("xinclude_start")) == 0)
    {
        xmlAttrPtr prop = node->properties;

        /*
         * This code is used because xmlGetProp() refuses to
         * work with non-element nodes with which original
         * xInclude nodes are replaced during processing.
         */
        for (prop = node->properties; prop != NULL; prop = prop->next)
        {
            if (xmlStrEqual(prop->name,
                            CONST_CHAR2XML("href")))
            {
                file = TE_ALLOC(sizeof(*file));
                file->filename = (char *)xmlStrdup(prop->children->content);
                TAILQ_INSERT_TAIL(inc_files, file, links);
                break;
            }
        }
        if (prop == NULL)
        {
            file = TE_ALLOC(sizeof(*file));
            file->filename = strdup("<unknown>");
            TAILQ_INSERT_TAIL(inc_files, file, links);
        }
    }
    else if (node->type == XML_XINCLUDE_END ||
             xmlStrcmp(node->name,
                       CONST_CHAR2XML("xinclude_end")) == 0)
    {
        file = TAILQ_LAST(inc_files, trc_files);
        assert(file != NULL);
        TAILQ_REMOVE(inc_files, file, links);
        free(file->filename);
        free(file);
    }
}

/**
 * Free resourses allocated for widely used expected results.
 */
static void
exp_defaults_free(void)
{
    if (exp_defaults_inited)
    {
        trc_exp_result *p;

        while ((p = STAILQ_FIRST(&exp_defaults)) != NULL)
        {
            STAILQ_REMOVE(&exp_defaults, p, trc_exp_result, links);
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
        STAILQ_INIT(&exp_defaults);
        atexit(exp_defaults_free);
        exp_defaults_inited = TRUE;
    }
}

/* See description in trc_db.h */
const trc_exp_result *
exp_defaults_get(te_test_status status)
{
    trc_exp_result         *p;
    trc_exp_result_entry   *entry;

    exp_defaults_init();

    STAILQ_FOREACH(p, &exp_defaults, links)
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

    STAILQ_INSERT_TAIL(&exp_defaults, p, links);

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

static xmlNodePtr
xmlNodeSkipExtra(xmlNodePtr node)
{
    while ((node != NULL) &&
           ((xmlStrcmp(node->name, (const xmlChar *)("comment")) == 0) ||
            (xmlStrcmp(node->name, (const xmlChar *)("text")) == 0)))
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
    return xmlNodeSkipExtra(node->children);
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
    return xmlNodeSkipExtra(node->next);
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
static te_errno
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

/* See description in trc_db.h */
te_errno
trc_db_get_text_content(xmlNodePtr node, char **content)
{
    return get_text_content(node, XML2CHAR(node->name),
                            content);
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
    te_errno            rc;
    trc_test_iter_arg  *p;
    trc_test_iter_arg  *insert_after = NULL;

    p = TE_ALLOC(sizeof(*p));
    if (p == NULL)
        return TE_RC(TE_TRC, TE_ENOMEM);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name of the argument is missing");
        return TE_RC(TE_TRC, TE_EFMT);
    }

    TAILQ_FOREACH_REVERSE(insert_after, &args->head,
                          trc_test_iter_args_head,
                          links)
    {
        if (strcmp(insert_after->name, p->name) < 0)
            break;
    }

    p->node = node;
    if (insert_after == NULL)
        TAILQ_INSERT_HEAD(&args->head, p, links);
    else
        TAILQ_INSERT_AFTER(&args->head, insert_after, p, links);

    tq_strings_add_uniq_dup(&args->save_order, p->name);

    rc = get_text_content(node, "arg", &p->value);
    if (rc != 0)
    {
        ERROR("Failed to get value of the argument '%s'", p->name);
    }
    else if (p->value == NULL)
    {
        p->value = strdup("");
        if (p->value == NULL)
            return TE_RC(TE_TRC, TE_ENOMEM);
    }

    return rc;
}

/* See description in trc_db.h */
te_errno
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
    else if (strcmp(tmp, "FAKED") == 0)
        *result = TE_TEST_FAKED;
    else
    {
        ERROR("Unknown result '%s' of the test iteration", tmp);
        free(tmp);
        return TE_RC(TE_TRC, TE_EFMT);
    }
    free(tmp);

    return 0;
}

/* See description in trc_db.h */
te_errno
get_expected_verdict(xmlNodePtr node, char **verdict)
{
    int rc;

    if (xmlStrcmp(node->name, CONST_CHAR2XML("verdict")) != 0)
    {
        ERROR("Unexpected node '%s' in the tagged result entry",
              node->name);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    rc = get_text_content(node, "verdict", verdict);
    if (rc != 0)
    {
        ERROR("Failed to get verdict text");
        return TE_RC(TE_TRC, TE_EFMT);
    }

    return 0;
}

/* See description in trc_db.h */
te_errno
get_expected_rentry(xmlNodePtr node, trc_exp_result_entry *rentry)
{
    te_test_verdict        *v;
    xmlNodePtr              q;
    int                     rc;

    if (xmlStrcmp(node->name, CONST_CHAR2XML("result")) != 0)
    {
        ERROR("Unexpected node '%s' in the tagged result",
              node->name);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    te_test_result_init(&rentry->result);

    rc = get_result(node, "value", &rentry->result.status);
    if (rc != 0)
        return rc;

    get_node_property(node, "key", &rentry->key);
    get_node_property(node, "notes", &rentry->notes);

    q = xmlNodeChildren(node);
    while (q != NULL)
    {
        v = TE_ALLOC(sizeof(*v));
        if (v == NULL)
        {
            trc_exp_result_entry_free(rentry);
            return TE_ENOMEM;
        }

        rc = get_expected_verdict(q, &v->str);
        if (rc != 0)
        {
            trc_exp_result_entry_free(rentry);
            free(v);
            return rc;
        }

        TAILQ_INSERT_TAIL(&rentry->result.verdicts, v, links);
        q = xmlNodeNext(q);
    }

    return 0;
}

/* See description in trc_db.h */
te_errno
get_expected_result(xmlNodePtr node, trc_exp_result *result,
                    te_bool tags_tolerate)
{
    te_errno                rc = 0;
    trc_exp_result_entry   *entry;
    xmlNodePtr              p;

    result->tags_str = XML2CHAR(xmlGetProp(node,
                                           CONST_CHAR2XML("tags")));
    if (result->tags_str == NULL && !tags_tolerate)
    {
        ERROR("%s: tags attribute should be specified for "
              "<results> element", __FUNCTION__);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    if (result->tags_str != NULL)
    {
        if (strlen(result->tags_str) == 0 && tags_tolerate)
            result->tags_expr = NULL;
        else if (logic_expr_parse(result->tags_str,
                                  &result->tags_expr) != 0)
            return TE_RC(TE_TRC, TE_EINVAL);
    }
    else
        result->tags_expr = NULL;

    get_node_property(node, "key", &result->key);
    get_node_property(node, "notes", &result->notes);

    for (p = xmlNodeChildren(node); p != NULL; p = xmlNodeNext(p))
    {
        entry = TE_ALLOC(sizeof(*entry));
        if (entry == NULL)
            return TE_ENOMEM;

        rc = get_expected_rentry(p, entry);
        if (rc != 0)
        {
            free(entry);
            return rc;
        }

        TAILQ_INSERT_TAIL(&result->results, entry, links);
    }

    return 0;
}

/* See description in trc_db.h */
te_errno
get_expected_results(xmlNodePtr *node, trc_exp_results *results)
{
    trc_exp_result         *result;

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
        STAILQ_INSERT_TAIL(results, result, links);
        get_expected_result(*node, result, FALSE);
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
    trc_file       *file;

    INFO("New iteration of the test %s", test->name);

    p = trc_db_new_test_iter(test, 0, NULL, NULL);
    if (p == NULL)
        return TE_RC(TE_TRC, TE_ENOMEM);

    if (inc_files != NULL)
    {
        file = TAILQ_LAST(inc_files, trc_files);
        if (file != NULL)
            p->filename = strdup(file->filename);
    }

    p->node = p->tests.node = node;

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("n")));
    if (tmp != NULL)
    {
        ERROR("Number of iterations is not supported yet");
        free(tmp);
        return TE_RC(TE_TRC, TE_ENOSYS);
    }

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("pos")));
    if (tmp != NULL)
    {
        p->file_pos = atoi(tmp);
        free(tmp);
    }

    rc = get_result(node, "result", &def);
    if (rc != 0)
    {
        ERROR("Cannot get test iteration result: %r", rc);
        return rc;
    }

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
        ERROR("Expected results of the test iteration are "
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

/*
 * Update globals list with globals from specific test (mostly
 * test package).
 */
static te_errno
get_globals(xmlNodePtr node, trc_test *parent)
{
    te_errno rc = 0;

    assert(parent != NULL);

    /* check if we have globals */

    /* assume that we're w */
    node = xmlNodeChildren(node);

    for (; node != NULL; node = xmlNodeNext(node))
    {
        if (xmlStrcmp(node->name, CONST_CHAR2XML("global")) == 0)
        {
            /* alloc global */
            trc_global *g = TE_ALLOC(sizeof(*g));

            assert(g);

            g->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
            if (g->name == NULL)
            {
                ERROR("Name of the global is missing");
                return TE_RC(TE_TRC, TE_EFMT);
            }
            g->value = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("value")));
            if (g->value == NULL)
            {
                xmlNodePtr val_node = xmlNodeChildren(node);

                if (xmlStrcmp(val_node->name, CONST_CHAR2XML("value")) == 0)
                    get_text_content(val_node, "value", &g->value);
            }
            if (rc || g->value == NULL)
            {
                ERROR("%s: no value for global %s", __FUNCTION__, g->name);
                return TE_RC(TE_TRC, TE_EFMT);
            }

            TAILQ_INSERT_HEAD(&current_db->globals.head, g, links);
        }
        else
            /* unexpected entry */
            break;
    }

    return rc;
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

    for (; *node != NULL; *node = xmlNodeNext(*node))
    {
        if (xmlStrcmp((*node)->name, CONST_CHAR2XML("iter")) == 0)
        {
            if ((rc = alloc_and_get_test_iter(*node, parent)) != 0)
                break;
        }
        else if (xmlStrcmp((*node)->name,
                           CONST_CHAR2XML("include")) == 0 ||
                 xmlStrcmp((*node)->name,
                           CONST_CHAR2XML("xinclude_start")) == 0 ||
                 xmlStrcmp((*node)->name,
                           CONST_CHAR2XML("xinclude_end")) == 0)
        {
            INFO("%s(): found 'include' entry", __FUNCTION__);
            update_files(*node);
        }
        else
        {
            /* Unexpected entry found */
            break;
        }
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
    trc_file   *file;

    assert(tests != NULL);

    p = trc_db_new_test(tests, parent, NULL);
    if (p == NULL)
    {
        ERROR("%s: failed to alloc\n", __FUNCTION__);
        return TE_RC(TE_TRC, TE_ENOMEM);
    }

    if (inc_files != NULL)
    {
        file = TAILQ_LAST(inc_files, trc_files);
        if (file != NULL)
            p->filename = strdup(file->filename);
    }

    p->node = p->iters.node = node;

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name of the test is missing");
        return TE_RC(TE_TRC, TE_EFMT);
    }
    trc_db_test_update_path(p);

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

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("pos")));
    if (tmp != NULL)
    {
        p->file_pos = atoi(tmp);
        free(tmp);
    }

    INFO("Parsing test '%s' type=%d aux=%d", p->name, p->type, p->aux);

    node = xmlNodeChildren(node);

    rc = get_node_with_text_content(&node, "objective", &p->objective);
    if (rc != 0)
    {
        ERROR("Failed to get objective of the test '%s': %r", p->name, rc);
        return rc;
    }

    if (node != NULL && xmlStrcmp(node->name, CONST_CHAR2XML("notes")) == 0)
    {
        rc = get_node_with_text_content(&node, "notes", &p->notes);
        if ((rc != 0) && (rc != TE_ENOENT))
        {
            ERROR("Failed to get objective of the test '%s'", p->name);
            return rc;
        }
    }

    /* possible include with globals */
    if (xmlStrcmp(node->name, CONST_CHAR2XML("include")) == 0 ||
        xmlStrcmp(node->name,
                  CONST_CHAR2XML("xinclude_start")) == 0 ||
        xmlStrcmp(node->name,
                  CONST_CHAR2XML("xinclude_end")) == 0)
    {
        update_files(node);
        node = xmlNodeNext(node);
    }
    /* get test globals - they're added to globals set */
    if (xmlStrcmp(node->name, CONST_CHAR2XML("globals")) == 0)
    {
        rc = get_globals(node, p);
        if (rc != 0)
        {
            ERROR("%s: failed to update globals with test '%s': %r",
                  __FUNCTION__, p->name, rc);
            return rc;
        }
        node = xmlNodeNext(node);
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

    if (*node != NULL &&
        xmlStrcmp((*node)->name, CONST_CHAR2XML("command")) == 0)
    {
        *node = xmlNodeNext(*node);
    }

    while (*node != NULL &&
           (
               (xmlStrcmp((*node)->name, CONST_CHAR2XML("test")) == 0 &&
                    (rc = alloc_and_get_test(*node, tests, parent)) == 0) ||
               (xmlStrcmp((*node)->name, CONST_CHAR2XML("include")) == 0 ||
                xmlStrcmp((*node)->name,
                          CONST_CHAR2XML("xinclude_start")) == 0 ||
                xmlStrcmp((*node)->name,
                          CONST_CHAR2XML("xinclude_end")) == 0)
           )
         )
    {
        update_files(*node);
        *node = xmlNodeNext(*node);
    }
    if (*node != NULL)
    {
        ERROR("%s: Unexpected element '%s'", __FUNCTION__, (*node)->name);
        rc = TE_RC(TE_TRC, TE_EFMT);
    }

    return rc;
}

/**
 * Free queue of included files.
 *
 * @param files     Queue of included files
 */
static void
trc_files_free(trc_files *files)
{
    trc_file    *p;

    if (files == NULL)
        return;

    while ((p = TAILQ_FIRST(files)) != NULL)
    {
        free(p->filename);
        TAILQ_REMOVE(files, p, links);
        free(p);
    }
}

/**
 * Read and parse XML document from a given location.
 *
 * @param location        Path to XML document.
 * @param doc             Where to save parsed XML document.
 *
 * @return @c 0 on success or error code in case of failure.
 */
static te_errno
trc_read_doc(const char *location, xmlDocPtr *doc)
{
#if HAVE_XMLERROR
    xmlError           *err;
#endif
    xmlParserCtxtPtr    parser;

    parser = xmlNewParserCtxt();
    if (parser == NULL)
    {
        ERROR("xmlNewParserCtxt() failed");
        return TE_ENOMEM;
    }
    if ((*doc = xmlCtxtReadFile(parser, location, NULL,
                                XML_PARSE_NOBLANKS |
                                XML_PARSE_XINCLUDE |
                                XML_PARSE_NONET)) == NULL)
    {
#if HAVE_XMLERROR
        err = xmlCtxtGetLastError(parser);
        ERROR("Error occured during parsing configuration file:\n"
              "    %s:%d\n    %s", location,
              err->line, err->message);
#else
        ERROR("Error occured during parsing configuration file:\n"
              "%s", location);
#endif
        xmlFreeParserCtxt(parser);
        xmlCleanupParser();
        return TE_RC(TE_TRC, TE_EFMT);
    }

    xmlFreeParserCtxt(parser);
    xmlCleanupParser();
    return 0;
}

/**
 * Auxiliary function used by @b trc_xinclude_process() to
 * recursively resolve xi:include references.
 *
 * @param parent_doc      XML document in which to insert
 *                        included content.
 * @param parent          XML node whose children to process.
 * @param trc_dir         Path to folder where TRC DB XML files
 *                        can be found.
 *
 * @return @c 0 on success or error code in case of failure.
 */
static te_errno
trc_xinclude_process_do(xmlDocPtr parent_doc, xmlNodePtr parent,
                        const char *trc_dir)
{
    xmlNodePtr  node;
    xmlNodePtr  node_next;

    xmlDocPtr   doc = NULL;
    xmlNodePtr  include_root = NULL;
    xmlNodePtr  included_node = NULL;
    xmlNodePtr  include_start = NULL;
    xmlNodePtr  include_end = NULL;

    te_errno rc = 0;

    for (node = parent->children; node != NULL; node = node_next)
    {
        node_next = node->next;

        if (node->type != XML_XINCLUDE_START &&
            node->type != XML_XINCLUDE_END &&
            xmlStrcmp(node->name, CONST_CHAR2XML("include")) == 0)
        {
            const char *href;
            char        full_path[PATH_MAX];

            href = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("href")));

            if (href == NULL)
            {
                ERROR("Failed to obtain href property value "
                      "of xi:include");
                return TE_RC(TE_TRC, TE_EINVAL);
            }

            if (href[0] == '/')
                snprintf(full_path, PATH_MAX, "%s", href);
            else
                snprintf(full_path, PATH_MAX, "%s/%s", trc_dir, href);

            rc = trc_read_doc(full_path, &doc);
            if (rc != 0)
                return rc;

            include_start = xmlCopyNode(node, 1);
            include_end = xmlCopyNode(node, 1);

            if (include_start == NULL || include_end == NULL)
            {
                ERROR("Failed to clone xi:include node");
                rc = TE_RC(TE_TRC, TE_EFAULT);
                goto cleanup;
            }

            if (xmlAddPrevSibling(node, include_start) == NULL ||
                xmlAddNextSibling(node, include_end) == NULL)
            {
                ERROR("Failed to add auxiliary nodes for xi:include node");
                rc = TE_RC(TE_TRC, TE_EFAULT);
                goto cleanup;
            }

            include_start->type = XML_XINCLUDE_START;
            include_end->type = XML_XINCLUDE_END;

            include_root = xmlDocGetRootElement(doc);
            if (include_root == NULL)
            {
                ERROR("Empty XML document is found in the included file "
                      "with expected testing results '%s'",
                      full_path);
                rc = TE_RC(TE_TRC, TE_EINVAL);
                goto cleanup;
            }

            included_node = xmlDocCopyNode(include_root, parent_doc, 1);
            if (included_node == NULL)
            {
                ERROR("Failed to copy root for '%s'", full_path);
                rc = TE_RC(TE_TRC, TE_EFAULT);
                goto cleanup;
            }

            if (xmlAddNextSibling(node, included_node) == NULL)
            {
                ERROR("Failed to include '%s'", full_path);
                rc = TE_RC(TE_TRC, TE_EFAULT);
                goto cleanup;
            }

            xmlFreeDoc(doc);
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            node_next = included_node;
            doc = NULL;
            include_start = NULL;
            include_end = NULL;
            included_node = NULL;
        }
        else
        {
            rc = trc_xinclude_process_do(parent_doc, node, trc_dir);
            if (rc != 0)
                return rc;
        }
    }

cleanup:

    if (included_node != NULL)
    {
        xmlUnlinkNode(included_node);
        xmlFreeNode(included_node);
    }

    if (include_start != NULL)
    {
        xmlUnlinkNode(include_start);
        xmlFreeNode(include_start);
    }

    if (include_end != NULL)
    {
        xmlUnlinkNode(include_end);
        xmlFreeNode(include_end);
    }

    if (doc != NULL)
        xmlFreeDoc(doc);

    return rc;
}

/**
 * Replace xi:include nodes with XML they reference. This function
 * circumvents a bug in @b xmlXIncludeProcess() which do not save
 * href property for lower level xi:include nodes found in included XML.
 *
 * @param doc       XML document pointer.
 * @param location  Path to XML document.
 *
 * @return @c 0 on success, or error code in case of failure.
 */
static te_errno
trc_xinclude_process(xmlDocPtr doc, const char *location)
{
    xmlNodePtr root;

    char trc_dir[PATH_MAX];
    int  i;

    te_strlcpy(trc_dir, location, PATH_MAX);
    for (i = strlen(trc_dir); i >= 0; i--)
    {
        if (trc_dir[i] == '/')
        {
            trc_dir[i] = '\0';
            break;
        }
    }
    if (i < 0)
    {
        snprintf(trc_dir, PATH_MAX, "%s", ".");
    }

    root = xmlDocGetRootElement(doc);
    if (root == NULL)
    {
        ERROR("%s: empty XML document of the DB with expected testing "
              "results", __FUNCTION__);
        return TE_RC(TE_TRC, TE_EINVAL);
    }

    return trc_xinclude_process_do(doc, root, trc_dir);
}

/* See description in te_trc.h */
te_errno
trc_db_open_ext(const char *location, te_trc_db **db, int flags)
{
    xmlNodePtr          node;
    te_errno            rc;
    trc_file           *file;
    int subst;

    if (location == NULL)
    {
        ERROR("Invalid location of the TRC database");
        return TE_RC(TE_TRC, TE_EFAULT);
    }

    *db = TE_ALLOC(sizeof(**db));
    if (*db == NULL)
        return TE_ENOMEM;
    TAILQ_INIT(&(*db)->tests.head);
    TAILQ_INIT(&(*db)->globals.head);

    (*db)->filename = strdup(location);
    if ((*db)->filename == NULL)
        return TE_ENOMEM;

    rc = trc_read_doc((*db)->filename, &(*db)->xml_doc);
    if (rc != 0)
        return rc;

    //xmlDocFormatDump(stdout, (*db)->xml_doc, 1);
    if (flags & TRC_OPEN_FIX_XINCLUDE)
    {
        rc = trc_xinclude_process((*db)->xml_doc, (*db)->filename);
        if (rc != 0)
            return rc;
    }
    else
    {
        subst = xmlXIncludeProcess((*db)->xml_doc);
        if (subst < 0)
        {
#if HAVE_XMLERROR
            xmlError *err = xmlGetLastError();

            ERROR("XInclude processing failed: %s", err->message);
#else
            ERROR("XInclude processing failed");
#endif
            xmlCleanupParser();
            return TE_EINVAL;
        }
    }

    /* store current db pointer */
    current_db = (*db);

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
        const char *last_match;

        last_match = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("last_match")));
        if (last_match != NULL &&
            (strcmp(last_match, "true") == 0 || strcmp(last_match, "1") == 0))
        {
            (*db)->last_match = TRUE;
        }

        (*db)->version = XML2CHAR(xmlGetProp(node,
                                          CONST_CHAR2XML("version")));
        if ((*db)->version == NULL)
        {
            INFO("Version of the TRC DB is missing");
        }

        node = xmlNodeChildren(node);
        (*db)->tests.node = node;

        inc_files = TE_ALLOC(sizeof(*inc_files));
        if (inc_files == NULL)
        {
            ERROR("Out of memory");
            return TE_ENOMEM;
        }

        TAILQ_INIT(inc_files);
        file = TE_ALLOC(sizeof(*file));
        if (file == NULL)
        {
            ERROR("Out of memory");
            return TE_ENOMEM;
        }

        file->filename = strdup((*db)->filename);
        if (file->filename == NULL)
        {
            ERROR("Out of memory");
            return TE_ENOMEM;
        }

        TAILQ_INSERT_TAIL(inc_files, file, links);
        rc = get_tests(&node, &(*db)->tests, NULL);
        trc_files_free(inc_files);

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

    return rc;
}

/* See description in te_trc.h */
te_errno
trc_db_open(const char *location, te_trc_db **db)
{
    return trc_db_open_ext(location, db, 0);
}

static te_errno trc_update_tests(trc_tests *tests, int flags,
                                 int uid,
                                 te_bool (*to_save)(void *, te_bool),
                                 char *(*set_user_attr)(void *, te_bool));

te_errno
trc_verdict_to_xml(char *v, xmlNodePtr result_node)
{
    xmlNodePtr               verd_node;

    if (v == NULL)
        return 0;

    verd_node =
        xmlNewChild(result_node, NULL,
                    BAD_CAST "verdict",
                    BAD_CAST v);
    if (verd_node == NULL)
        return TE_ENOMEM;

    return 0;
}

te_errno
trc_exp_result_entry_to_xml(trc_exp_result_entry *res_entry,
                            xmlNodePtr results_node)
{
    te_test_verdict         *verdict;
    xmlNodePtr               result_node;

    result_node = xmlNewChild(results_node,
                              NULL,
                              BAD_CAST "result",
                              NULL);

    xmlNewProp(result_node, BAD_CAST "value",
               BAD_CAST te_test_status_to_str(
                            res_entry->result.status));

    if (res_entry->key != NULL &&
            strlen(res_entry->key) != 0)
        xmlNewProp(result_node, BAD_CAST "key",
                   BAD_CAST res_entry->key);

    if (res_entry->notes != NULL &&
            strlen(res_entry->notes) != 0)
        xmlNewProp(result_node, BAD_CAST "notes",
                   BAD_CAST res_entry->notes);

    TAILQ_FOREACH(verdict,
                  &res_entry->result.verdicts,
                  links)
        trc_verdict_to_xml(verdict->str, result_node);

    return 0;
}

te_errno
trc_exp_result_to_xml(trc_exp_result *exp_result, xmlNodePtr results_node,
                      te_bool is_default)
{
    trc_exp_result_entry    *res_entry;

    if (exp_result == NULL)
        return 0;

    if (!is_default)
        xmlNewProp(results_node, BAD_CAST "tags",
                   BAD_CAST exp_result->tags_str);

    if (exp_result->key != NULL &&
            strlen(exp_result->key) != 0)
        xmlNewProp(results_node, BAD_CAST "key",
                   BAD_CAST exp_result->key);

    if (exp_result->notes != NULL &&
            strlen(exp_result->notes) != 0)
        xmlNewProp(results_node, BAD_CAST "notes",
                   BAD_CAST exp_result->notes);

    if (is_default &&
        (res_entry = TAILQ_FIRST(&exp_result->results)) ==
                                   TAILQ_LAST(&exp_result->results,
                                              trc_exp_result_entry_head) &&
        res_entry != NULL &&
        TAILQ_EMPTY(&res_entry->result.verdicts))
    {
         xmlNewProp(results_node, BAD_CAST "value",
                    BAD_CAST te_test_status_to_str(
                                    res_entry->result.status));
    }
    else
    {
        TAILQ_FOREACH(res_entry, &exp_result->results,
                      links)
            trc_exp_result_entry_to_xml(res_entry, results_node);
    }

    return 0;
}

te_errno
trc_exp_results_to_xml(trc_exp_results *exp_results, xmlNodePtr node,
                       te_bool insert_after)
{
    xmlNodePtr               results_node;
    xmlNodePtr               prev_node;
    trc_exp_result          *result;

    if (exp_results == NULL)
        return 0;

    prev_node = node;

    STAILQ_FOREACH(result, exp_results, links)
    {
        results_node = xmlNewNode(NULL, BAD_CAST "results");

        if (insert_after)
        {
            xmlAddNextSibling(prev_node, results_node);
            prev_node = results_node;
        }
        else
            xmlAddChild(node, results_node);

        trc_exp_result_to_xml(result, results_node, FALSE);
    }

    return 0;
}

static te_errno
trc_update_iters(trc_test_iters *iters, int flags, int uid,
                 te_bool (*to_save)(void *, te_bool),
                 char *(*set_user_attr)(void *, te_bool))
{
    te_errno        rc;
    trc_test_iter  *p;
    void           *user_data;
    te_bool         is_saved;
    char           *user_attr;
    xmlNodePtr      node;
    xmlNodePtr      prev_node;
    te_bool         renew_content;

    TAILQ_FOREACH(p, &iters->head, links)
    {
        /*
         * If we have initially deleted old XML, this is
         * just incorrect pointer.
         */
        if (flags & TRC_SAVE_REMOVE_OLD)
            p->node = NULL;

        user_data = trc_db_iter_get_user_data(p, uid);
        if (to_save != NULL)
            is_saved = to_save(user_data, TRUE);
        else
            is_saved = TRUE;

        if (!is_saved && p->node != NULL)
        {
            xmlUnlinkNode(p->node);
            xmlFreeNode(p->node);
            p->node = NULL;
        }

        if (is_saved)
        {
            trc_test_iter_arg  *a;

            renew_content = TRUE;

            if (p->node == NULL)
            {
                INFO("Add node for iteration %p node=%p", iters,
                     iters->node);
                p->node = p->tests.node = xmlNewChild(iters->node, NULL,
                                                      BAD_CAST "iter",
                                                      NULL);
                if (p->tests.node == NULL)
                {
                    ERROR("xmlNewChild() failed for 'iter'");
                    return TE_ENOMEM;
                }
            }
            else if (flags & TRC_SAVE_UPDATE_OLD)
            {
                xmlNodePtr  aux_ptr = NULL;

                for (node = p->node->children;
                     node != NULL;
                     node = aux_ptr)
                {
                    aux_ptr = node->next;

                    if (xmlStrcmp(node->name,
                                  CONST_CHAR2XML("notes")) == 0 ||
                        xmlStrcmp(node->name,
                                  CONST_CHAR2XML("arg")) == 0 ||
                        xmlStrcmp(node->name,
                                  CONST_CHAR2XML("results")) == 0 ||
                        (node->type != XML_ELEMENT_NODE &&
                         node->type != XML_XINCLUDE_START &&
                         node->type != XML_XINCLUDE_END))
                    {
                        xmlUnlinkNode(node);
                        xmlFreeNode(node);
                    }
                }
            }
            else
                renew_content = FALSE;

            if (renew_content)
            {
                tqe_string *tq_str;

                xmlSetProp(p->tests.node, BAD_CAST "result",
                           BAD_CAST "PASSED");

                if (set_user_attr != NULL)
                {
                    user_attr = set_user_attr(user_data, TRUE);
                    if (user_attr != NULL)
                    {
                        xmlNewProp(p->tests.node, BAD_CAST "user_attr",
                                   BAD_CAST user_attr);
                        free(user_attr);
                    }
                }

                if (flags & TRC_SAVE_POS_ATTR)
                {
                    user_attr = TE_ALLOC(MAX_POS_LEN);
                    if (user_attr == NULL)
                        return TE_ENOMEM;

                    snprintf(user_attr, MAX_POS_LEN, "%d", p->file_pos);
                    xmlSetProp(p->tests.node, BAD_CAST "pos",
                               BAD_CAST user_attr);
                    free(user_attr);
                }

                prev_node = NULL;

                for (tq_str = TAILQ_FIRST(&p->args.save_order),
                     a = TAILQ_FIRST(&p->args.head);
                     tq_str != NULL || a != NULL; )
                {
                    /*
                     * If save_order is filled - save arguments to
                     * XML in order defined by it.
                     */
                    if (tq_str != NULL)
                    {
                        te_bool found = FALSE;

                        TAILQ_FOREACH(a, &p->args.head, links)
                        {
                            if (strcmp(a->name, tq_str->v) == 0)
                            {
                                found = TRUE;
                                break;
                            }
                        }

                        if (!found)
                        {
                            ERROR("Failed to find argument '%s' from "
                                  "saving order list", tq_str->v);
                            return TE_ENOENT;
                        }
                    }

                    if ((node = xmlNewNode(NULL, BAD_CAST "arg")) == NULL)
                    {
                        ERROR("xmlNewChild() failed for 'arg'");
                        return TE_ENOMEM;
                    }

                    xmlNodeSetContent(node, a->value == NULL ?
                                                BAD_CAST a->value :
                                                strlen(a->value) == 0 ?
                                                        NULL :
                                                        BAD_CAST a->value);
                    xmlNewProp(node, BAD_CAST "name", BAD_CAST a->name);

                    if (prev_node == NULL)
                    {
                        if (p->tests.node->children != NULL)
                            xmlAddPrevSibling(p->tests.node->children,
                                              node);
                        else
                            xmlAddChild(p->tests.node, node);
                    }
                    else
                        xmlAddNextSibling(prev_node, node);

                    prev_node = node;

                    if (tq_str == NULL)
                    {
                        a = TAILQ_NEXT(a, links);
                    }
                    else
                    {
                        tq_str = TAILQ_NEXT(tq_str, links);
                        a = NULL;
                    }
                }

                if ((node = xmlNewNode(NULL, BAD_CAST "notes")) == NULL)
                {
                    ERROR("xmlNewChild() failed for 'notes'");
                    return TE_ENOMEM;
                }

                xmlNodeSetContent(node, BAD_CAST p->notes);

                if (prev_node == NULL)
                {
                    if (p->tests.node->children != NULL)
                        xmlAddPrevSibling(p->tests.node->children, node);
                    else
                        xmlAddChild(p->tests.node, node);
                }
                else
                    xmlAddNextSibling(prev_node, node);

                prev_node = node;

                if ((flags & TRC_SAVE_RESULTS) &&
                    !STAILQ_EMPTY(&p->exp_results))
                    trc_exp_results_to_xml(&p->exp_results,
                                           prev_node,
                                           TRUE);
            }

            if (is_saved)
            {
                rc = trc_update_tests(&p->tests, flags, uid, to_save,
                                      set_user_attr);
                if (rc != 0)
                    return rc;
            }
        }
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
trc_update_tests(trc_tests *tests, int flags, int uid,
                 te_bool (*to_save)(void *, te_bool),
                 char *(*set_user_attr)(void *, te_bool))
{
    te_errno    rc;
    trc_test   *p;
    xmlNodePtr  node;
    xmlNodePtr  prev_node;
    void       *user_data;
    te_bool     is_saved;
    te_bool     renew_content = FALSE;
    char       *user_attr;

    TAILQ_FOREACH(p, &tests->head, links)
    {
        /*
         * If we have initially deleted old XML, this is
         * just incorrect pointer.
         */
        if (flags & TRC_SAVE_REMOVE_OLD)
            p->node = NULL;

        user_data = trc_db_test_get_user_data(p, uid);
        if (to_save != NULL)
            is_saved = to_save(user_data, FALSE);
        else
            is_saved = TRUE;

        if (!is_saved && p->node != NULL)
        {
            xmlUnlinkNode(p->node);
            xmlFreeNode(p->node);
            p->node = NULL;
        }

        if (is_saved)
        {
            renew_content = TRUE;

            if (p->node == NULL)
            {
                INFO("Add node for '%s'", p->name);
                p->node = p->iters.node = xmlNewChild(tests->node, NULL,
                                                      BAD_CAST "test",
                                                      NULL);
                if (p->iters.node == NULL)
                {
                    ERROR("xmlNewChild() failed for 'test'");
                    return TE_ENOMEM;
                }
            }
            else if (flags & TRC_SAVE_UPDATE_OLD)
            {
                xmlNodePtr  aux_ptr = NULL;

                for (node = p->node->children;
                     node != NULL;
                     node = aux_ptr)
                {
                    aux_ptr = node->next;

                    if (xmlStrcmp(node->name,
                                  CONST_CHAR2XML("notes")) == 0 ||
                        xmlStrcmp(node->name,
                                  CONST_CHAR2XML("globals")) == 0 ||
                        xmlStrcmp(node->name,
                                  CONST_CHAR2XML("objective")) == 0 ||
                        (node->type != XML_ELEMENT_NODE &&
                         node->type != XML_XINCLUDE_START &&
                         node->type != XML_XINCLUDE_END))
                    {
                        xmlUnlinkNode(node);
                        xmlFreeNode(node);
                    }
                }
            }
            else
                renew_content = FALSE;

            if (renew_content)
            {
                xmlSetProp(p->iters.node, BAD_CAST "name",
                           BAD_CAST p->name);
                xmlSetProp(p->iters.node, BAD_CAST "type",
                           BAD_CAST trc_test_type_to_str(p->type));

                if (flags & TRC_SAVE_POS_ATTR)
                {
                    user_attr = TE_ALLOC(MAX_POS_LEN);
                    if (user_attr == NULL)
                        return TE_ENOMEM;

                    snprintf(user_attr, MAX_POS_LEN, "%d", p->file_pos);
                    xmlSetProp(p->iters.node, BAD_CAST "pos",
                               BAD_CAST user_attr);
                    free(user_attr);
                }

                if ((node = xmlNewNode(NULL,
                                       BAD_CAST "objective")) == NULL)
                {
                    ERROR("xmlNewNode() failed for 'objective'");
                    return TE_ENOMEM;
                }

                xmlNodeSetContent(node, BAD_CAST p->objective);

                if (p->iters.node->children != NULL)
                    xmlAddPrevSibling(p->iters.node->children, node);
                else
                    xmlAddChild(p->iters.node, node);

                prev_node = node;

                if ((node = xmlNewNode(NULL, BAD_CAST "notes")) == NULL)
                {
                    ERROR("xmlNewNode() failed for 'notes'");
                    return TE_ENOMEM;
                }

                xmlNodeSetContent(node, BAD_CAST p->notes);
                xmlAddNextSibling(prev_node, node);
                prev_node = node;

                if ((flags & TRC_SAVE_GLOBALS) &&
                    !TAILQ_EMPTY(&current_db->globals.head))
                {
                    trc_global      *g;
                    xmlNodePtr       globals_node;
                    xmlNodePtr       global_node;
                    xmlNodePtr       value_node;

                    flags &= ~TRC_SAVE_GLOBALS;

                    if ((globals_node = xmlNewNode(NULL,
                                            BAD_CAST "globals")) == NULL)
                    {
                        ERROR("xmlNewNode() failed for 'globals'");
                        return TE_ENOMEM;
                    }

                    xmlAddNextSibling(prev_node, globals_node);

                    TAILQ_FOREACH(g, &current_db->globals.head, links)
                    {

                        global_node = xmlNewChild(globals_node, NULL,
                                                  BAD_CAST "global",
                                                  NULL);
                        if (global_node == NULL)
                        {
                            ERROR("xmlNewChild() failed for 'global'");
                            return TE_ENOMEM;
                        }

                        value_node = xmlNewChild(global_node, NULL,
                                                 BAD_CAST "value",
                                                 BAD_CAST g->value);
                        if (value_node == NULL)
                        {
                            ERROR("xmlNewChild() failed for 'value'");
                            return TE_ENOMEM;
                        }

                        xmlNewProp(global_node, BAD_CAST "name",
                                   BAD_CAST g->name);
                    }
                }
            }
        }

        if (is_saved)
        {
            rc = trc_update_iters(&p->iters, flags, uid, to_save,
                                  set_user_attr);
            if (rc != 0)
                return rc;
        }
    }
    return 0;
}

/**
 * Compute "file_pos" properties value for
 * all tests and they descendants, starting
 * from a given test.
 *
 * @param test      From which test to start
 * @param is_first  Whether this test is first in
 *                  list of its siblings or not
 * @param is_top    Whether this is a top element in TRC DB
 */
static te_errno trc_tests_pos(trc_test *test, te_bool is_first,
                              te_bool is_top);

/**
 * Compute "file_pos" properties value for
 * all iterations and they descendants, starting
 * from a given iteration.
 *
 * @param iter      From which iteration to start
 * @param is_first  Whether this iteration is first in
 *                  list of its siblings or not
 */
static te_errno
trc_iters_pos(trc_test_iter *iter, te_bool is_first)
{
    int               pos = 0;
    char             *filename;
    trc_test         *test;
    trc_test_iter    *p;

    if (iter == NULL)
        return 0;

    p = iter;
    filename = iter->filename;

    do {
        if (strcmp_null(filename, p->filename) == 0)
        {
            p->file_pos = ++pos;
            test = TAILQ_FIRST(&p->tests.head);
            if (trc_tests_pos(test, TRUE, FALSE) != 0)
                return -1;
        }
        else if (is_first)
        {
            const char            *filename_oth = p->filename;
            trc_test_iter         *p_prev = p;

            if (trc_iters_pos(p, FALSE) != 0)
                return -1;

            /* Skip iterations that were handled by recursive call. */
            while ((p = TAILQ_NEXT(p, links)) != NULL &&
                   strcmp_null(p->filename, filename_oth) == 0)
                p_prev = p;
            p = p_prev;
        }
        else
            break;

    } while ((p = TAILQ_NEXT(p, links)) != NULL);

    return 0;
}

/** See description above */
static te_errno
trc_tests_pos(trc_test *test, te_bool is_first, te_bool is_top)
{
    int              pos = 0;
    char            *filename;
    trc_test_iter   *iter;
    trc_test        *p;

    if (test == NULL)
        return 0;

    p = test;
    filename = test->filename;

    do {
        if (strcmp_null(filename, p->filename) == 0)
        {
            /* Check that "pos" attribute was not already set in file
             * we loaded previously by checking the top <test> element in
             * TRC DB - it is unlikely that somebody can set it by
             * mistake for it, as it can be done by copying manually output
             * of TRC update tool for some test. */
            if (is_top && p->node != NULL &&
                xmlGetProp(p->node, CONST_CHAR2XML("pos")) != NULL)
                return -1;

            p->file_pos = ++pos;
            iter = TAILQ_FIRST(&p->iters.head);
            if (trc_iters_pos(iter, TRUE) != 0)
                return -1;
        }
        else if (is_first)
        {
            const char       *filename_oth = p->filename;
            trc_test         *p_prev = p;

            if (trc_tests_pos(p, FALSE, FALSE) != 0)
                return -1;

            /* Skip tests that were handled by recursive call. */

            while ((p = TAILQ_NEXT(p, links)) != NULL &&
                   strcmp_null(p->filename, filename_oth) == 0)
                p_prev = p;
            p = p_prev;
        }
        else
            break;

        is_top = FALSE;
    } while ((p = TAILQ_NEXT(p, links)) != NULL);

    return 0;
}

/* See description in trc_db.h */
te_errno
trc_db_save(te_trc_db *db, const char *filename, int flags,
            int uid, te_bool (*to_save)(void *, te_bool),
            char *(*set_user_attr)(void *, te_bool),
            char *cmd)
{
    const char          *fn = (filename != NULL) ?
                                    filename : db->filename;
    te_errno             rc;
    trc_test            *test;

    if (flags & TRC_SAVE_REMOVE_OLD)
    {
        xmlFreeDoc(db->xml_doc);
        db->xml_doc = NULL;
    }

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

        if (cmd != NULL)
        {
            xmlNodePtr  child_node = xmlFirstElementChild(node);
            xmlNodePtr  cmd_node = NULL;
            xmlChar    *xml_cmd = NULL;

            xml_cmd = xmlEncodeEntitiesReentrant(db->xml_doc,
                                                 BAD_CAST cmd);
            if (xml_cmd == NULL)
            {
                ERROR("xmlEncodeEntitiesReentrant() failed\n");
                return TE_ENOMEM;
            }

            if (child_node == NULL)
            {
                cmd_node = xmlNewChild(node, NULL, BAD_CAST "command",
                                       xml_cmd);
                if (cmd_node == NULL)
                {
                    ERROR("xmlNewChild() failed\n");
                    free(xml_cmd);
                    return TE_ENOMEM;
                }

            }
            else if (xmlStrcmp(child_node->name, BAD_CAST "command") == 0)
            {
                xmlNodeSetContent(child_node, xml_cmd);
            }
            else
            {
                cmd_node = xmlNewChild(node, NULL, BAD_CAST "command",
                                       xml_cmd);
                if (cmd_node == NULL)
                {
                    ERROR("xmlNewChild() failed\n");
                    free(xml_cmd);
                    return TE_ENOMEM;
                }

                if (xmlAddPrevSibling(child_node, cmd_node) == NULL)
                {
                    ERROR("xmlPrevSibling() failed\n");
                    free(xml_cmd);
                    return TE_ENOMEM;
                }
            }
        }
    }

    test = TAILQ_FIRST(&db->tests.head);
    if (flags & TRC_SAVE_POS_ATTR)
        trc_tests_pos(test, TRUE, TRUE);

    if ((rc = trc_update_tests(&db->tests, flags, uid, to_save,
                               set_user_attr)) != 0)
    {
        ERROR("Failed to update DB XML document");
        return rc;
    }

    rc = trc_include_markers_add(xmlDocGetRootElement(db->xml_doc), flags);
    if (rc != 0)
    {
        ERROR("Failed to add XInclude markers to XML document");
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

    trc_files_free(inc_files);
    free(inc_files);
    inc_files = NULL;

    return 0;
}
