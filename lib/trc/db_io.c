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
#include "te_compound.h"
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

/** Widely used expected results */
static trc_exp_results  exp_defaults;
/** Are exp_defaults initialized? */
static bool exp_defaults_inited = false;


static te_errno get_tests(xmlNodePtr *node, te_trc_db *db, trc_tests *tests,
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
        exp_defaults_inited = false;
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
        exp_defaults_inited = true;
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

static xmlNodePtr
xmlNodeSkipExtra(xmlNodePtr node)
{
    while ((node != NULL) &&
           (node->type == XML_COMMENT_NODE || node->type == XML_TEXT_NODE))
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
    if (node->children->type != XML_TEXT_NODE)
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

static te_errno
process_simple_value(te_string *dest, const char *content)
{
    te_compound_kind kind = te_compound_classify(dest);

    if (kind != TE_COMPOUND_NULL && kind != TE_COMPOUND_PLAIN)
    {
        if (te_str_isspace(content))
            return 0;

        ERROR("Simple text '%s' follows subvalue definitions", content);
        return TE_RC(TE_TRC, TE_EFMT);
    }
    te_string_append(dest, "%s", content);

    return 0;
}

static te_errno
process_subvalue(te_string *dest, xmlNodePtr field_node, const char *elt_name)
{
    te_string collect = TE_STRING_INIT;
    char *name;
    xmlNodePtr child;

    if (xmlStrcmp(field_node->name, CONST_CHAR2XML("field")) != 0)
    {
        ERROR("Unexpected element <%s> inside <%s>",
              (const char *)field_node->name, elt_name);
        te_string_free(&collect);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    if (te_compound_classify(dest) == TE_COMPOUND_PLAIN)
    {
        if (!te_str_isspace(dest->ptr))
        {
            ERROR("<field> follows simple text");
            te_string_free(&collect);
            return TE_RC(TE_TRC, TE_EFMT);
        }
        te_string_reset(dest);
    }

    for (child = field_node->children; child != NULL; child = child->next)
    {
        switch (child->type)
        {
            case XML_COMMENT_NODE:
                /* Just skip comments */
                break;
            case XML_TEXT_NODE:
                te_string_append(&collect, (const char *)child->content);
                break;
            case XML_ELEMENT_NODE:
                ERROR("Unexpected element <%s> inside <field>",
                      (const char *)child->name);
                te_string_free(&collect);
                return TE_RC(TE_TRC, TE_EFMT);
            default:
                ERROR("Something strange inside <field>, node type = %d",
                      child->type);
                te_string_free(&collect);
                return TE_RC(TE_TRC, TE_EFMT);
        }
    }

    name = XML2CHAR(xmlGetProp(field_node, CONST_CHAR2XML("name")));
    te_compound_set(dest, name, TE_COMPOUND_MOD_OP_APPEND, "%s", collect.ptr);
    xmlFree(name);
    te_string_free(&collect);
    return 0;
}

static te_errno
get_structured_text_content(xmlNodePtr node, const char *name, char **content)
{
    te_string compound = TE_STRING_INIT;
    xmlNodePtr child;
    te_errno rc = 0;

    for (child = node->children; child != NULL && rc == 0; child = child->next)
    {
        switch (child->type)
        {
            case XML_COMMENT_NODE:
                /* Just skip comments */
                break;
            case XML_TEXT_NODE:
                rc = process_simple_value(&compound,
                                          (const char *)child->content);
                break;
            case XML_ELEMENT_NODE:
                rc = process_subvalue(&compound, child, name);
                break;
            default:
                ERROR("Something strange inside <%s>, node type = %d",
                      name, child->type);
                rc = TE_RC(TE_TRC, TE_EINVAL);
                break;
        }
    }

    if (rc == 0)
        te_string_move(content, &compound);
    else
        *content = NULL;
    te_string_free(&compound);

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
    te_errno            rc;
    trc_test_iter_arg  *p;
    trc_test_iter_arg  *insert_after = NULL;

    p = TE_ALLOC(sizeof(*p));

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

    rc = get_structured_text_content(node, "arg", &p->value);
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
get_expected_result(xmlNodePtr node, trc_exp_result *result)
{
    te_errno                rc = 0;
    trc_exp_result_entry   *entry;
    xmlNodePtr              p;

    result->tags_str = XML2CHAR(xmlGetProp(node,
                                           CONST_CHAR2XML("tags")));
    if (result->tags_str != NULL)
    {
        if (strlen(result->tags_str) == 0)
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

        TAILQ_INIT(&result->results);
        STAILQ_INSERT_TAIL(results, result, links);
        get_expected_result(*node, result);
        *node = xmlNodeNext(*node);
    }

    return 0;
}

/**
 * Allocate and get test iteration.
 *
 * @param node          XML node
 * @param db            TRC database
 * @param test          Parent test
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_test_iter(xmlNodePtr node, te_trc_db *db, trc_test *test)
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
    rc = get_tests(&node, db, &p->tests, p);
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
get_globals(xmlNodePtr node, te_trc_db *db, trc_test *parent)
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
                    get_structured_text_content(val_node, "value", &g->value);
            }
            if (rc || g->value == NULL)
            {
                ERROR("%s: no value for global %s", __FUNCTION__, g->name);
                return TE_RC(TE_TRC, TE_EFMT);
            }

            TAILQ_INSERT_TAIL(&db->globals.head, g, links);
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
 * @param db            TRC database
 * @param test          Parent test
 *
 * @return Status code
 */
static te_errno
get_test_iters(xmlNodePtr *node, te_trc_db *db, trc_test *parent)
{
    te_errno rc = 0;

    assert(node != NULL);
    assert(parent != NULL);

    for (; *node != NULL; *node = xmlNodeNext(*node))
    {
        if (xmlStrcmp((*node)->name, CONST_CHAR2XML("iter")) == 0)
        {
            if ((rc = alloc_and_get_test_iter(*node, db, parent)) != 0)
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
 * Get value of a boolean property of XML node.
 * If the property is missing, it is considered to be false value.
 *
 * @param node        XML node.
 * @param property    Name of the property.
 * @param value       Location for obtained and parsed boolean value.
 *
 * @return 0 on success or error code.
 */
static te_errno
get_boolean_prop(xmlNodePtr node, const char *property, bool *value)
{
    char *tmp = NULL;
    te_errno rc = 0;

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML(property)));
    if (tmp == NULL)
    {
        *value = false;
    }
    else if (strcmp(tmp, "true") == 0 || strcmp(tmp, "1") == 0)
    {
        *value = true;
    }
    else if (strcmp(tmp, "false") == 0 || strcmp(tmp, "0") == 0)
    {
        *value = false;
    }
    else
    {
        ERROR("Invalid value of boolean property '%s': '%s'",
              property, tmp);
        rc = TE_RC(TE_TRC, TE_EFMT);
    }

    free(tmp);
    return rc;
}

/**
 * Update boolean property.
 *
 * @param node      XML node pointer.
 * @param property  Property name.
 * @param value     Desired value.
 *
 * @return Status code.
 */
static te_errno
update_boolean_prop(xmlNodePtr node, const char *property, bool value)
{
    te_errno rc;
    bool cur_val;

    if (value)
    {
        if (xmlSetProp(node, BAD_CAST property, BAD_CAST "true") == NULL)
        {
            ERROR("Failed to set property '%s'", property);
            return TE_ENOMEM;
        }
    }
    else
    {
        /*
         * Remove property if it has incorrect value or is set to true.
         */
        rc = get_boolean_prop(node, property, &cur_val);
        if (rc != 0 || cur_val)
            xmlUnsetProp(node, BAD_CAST property);
    }

    return 0;
}

/**
 * Get expected result of the test.
 *
 * @param node          XML node
 * @param db            TRC database
 * @param tests         List of tests to add the new test
 * @param parent        Parent iteration
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_test(xmlNodePtr node, te_trc_db *db, trc_tests *tests,
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

    rc = get_boolean_prop(node, "auxiliary", &p->aux);
    if (rc != 0)
        return rc;

    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("pos")));
    if (tmp != NULL)
    {
        p->file_pos = atoi(tmp);
        free(tmp);
    }

    rc = get_boolean_prop(node, "override", &p->override_iters);
    if (rc != 0)
        return rc;

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
    if (node != NULL &&
        (xmlStrcmp(node->name, CONST_CHAR2XML("include")) == 0 ||
         xmlStrcmp(node->name,
                   CONST_CHAR2XML("xinclude_start")) == 0 ||
         xmlStrcmp(node->name,
                   CONST_CHAR2XML("xinclude_end")) == 0))
    {
        update_files(node);
        node = xmlNodeNext(node);
    }
    /* get test globals - they're added to globals set */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("globals")) == 0)
    {
        rc = get_globals(node, db, p);
        if (rc != 0)
        {
            ERROR("%s: failed to update globals with test '%s': %r",
                  __FUNCTION__, p->name, rc);
            return rc;
        }
        node = xmlNodeNext(node);
    }

    rc = get_test_iters(&node, db, p);
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
 * @param db            TRC database
 * @param tests         List of tests to be filled in
 * @param parent        Parent iteration
 *
 * @return Status code
 */
static te_errno
get_tests(xmlNodePtr *node, te_trc_db *db, trc_tests *tests,
          trc_test_iter *parent)
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
                    (rc = alloc_and_get_test(*node, db,
                                             tests, parent)) == 0) ||
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
    const xmlError     *err;
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
        ERROR("Error occurred during parsing configuration file:\n"
              "    %s:%d\n    %s", location,
              err->line, err->message);
#else
        ERROR("Error occurred during parsing configuration file:\n"
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
    xmlNodePtr  prev_node;
    xmlNodePtr  child_node;

    xmlDocPtr   doc = NULL;
    xmlNodePtr  included_node = NULL;
    xmlNodePtr  include_start = NULL;
    xmlNodePtr  include_end = NULL;

    te_errno rc = 0;
    char *p;

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
            include_start = NULL;
            include_end = NULL;

            p = strrchr(full_path, '/');
            if (p != NULL)
                *p = '\0';

            for (child_node = doc->children, prev_node = node;
                 child_node != NULL; child_node = child_node->next)
            {
                included_node = xmlDocCopyNode(child_node, parent_doc, 1);
                if (included_node == NULL)
                {
                    ERROR("Failed to copy node from included document");
                    rc = TE_RC(TE_TRC, TE_ENOMEM);
                    goto cleanup;
                }

                if (xmlAddNextSibling(prev_node, included_node) == NULL)
                {
                    ERROR("Failed to add copied node from included document");
                    rc = TE_RC(TE_TRC, TE_ENOMEM);
                    goto cleanup;
                }

                rc = trc_xinclude_process_do(parent_doc, included_node,
                                             full_path);
                if (rc != 0)
                    goto cleanup;

                prev_node = included_node;
                included_node = NULL;
            }

            xmlFreeDoc(doc);
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            doc = NULL;
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
 * Also it includes XML comments from referenced files.
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
            const xmlError *err = xmlGetLastError();

            ERROR("XInclude processing failed: %s", err->message);
#else
            ERROR("XInclude processing failed");
#endif
            xmlCleanupParser();
            return TE_EINVAL;
        }
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
        rc = get_boolean_prop(node, "last_match", &(*db)->last_match);
        if (rc != 0)
            return rc;

        rc = get_boolean_prop(node, "merged", &(*db)->merged);
        if (rc != 0)
            return rc;

        (*db)->version = XML2CHAR(xmlGetProp(node,
                                          CONST_CHAR2XML("version")));
        if ((*db)->version == NULL)
        {
            INFO("Version of the TRC DB is missing");
        }

        node = xmlNodeChildren(node);
        (*db)->tests.node = node;

        inc_files = TE_ALLOC(sizeof(*inc_files));

        TAILQ_INIT(inc_files);
        file = TE_ALLOC(sizeof(*file));

        file->filename = strdup((*db)->filename);
        if (file->filename == NULL)
        {
            ERROR("Out of memory");
            return TE_ENOMEM;
        }

        TAILQ_INSERT_TAIL(inc_files, file, links);
        rc = get_tests(&node, *db, &(*db)->tests, NULL);
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

static void db_merge_tests(trc_test_iter *parent_iter,
                           trc_tests *target_tests, trc_tests *merged_tests,
                           te_trc_db *target_db, te_trc_db *merged_db);

/**
 * Resolve argument value in TRC DB.
 *
 * @param db        TRC database.
 * @param value     Value to be resolved.
 *
 * @return Resolved value (the original one or value of
 *         the global variable which is referenced by it).
 */
static const char *
resolve_value(te_trc_db *db, const char *value)
{
    if (strcmp_start(TEST_ARG_VAR_PREFIX, value) == 0)
    {
        trc_global *g = NULL;

        TAILQ_FOREACH(g, &db->globals.head, links)
        {
            if (strcmp(g->name, value + strlen(TEST_ARG_VAR_PREFIX)) == 0)
                return g->value;
        }

        return NULL;
    }
    else
    {
        return value;
    }
}

/** Possible results of iters_match() */
typedef enum iters_match_result {
    /** No match */
    ITERS_MATCH_NO = 0,
    /** Exact match: all arguments have the same values */
    ITERS_MATCH_EXACT,
    /**
     * All test iterations matching the first record match
     * the second one, but not necessarily the other way around
     */
    ITERS_MATCH_SUBSET,
    /**
     * All test iterations matching the second record match
     * the first one, but not necessarily the other way around
     */
    ITERS_MATCH_SUPERSET,
    /**
     * Sets of iterations corresponding to the two iteration
     * records may intersect
     */
    ITERS_MATCH_INTERSECT,
} iters_match_result;

/**
 * Match the first iteration record against the second one.
 *
 * @param db      TRC database.
 * @param iter1   The first iteration record.
 * @param iter2   The second iteration record.
 *
 * @return One of the values in iters_match_result.
 */
static iters_match_result
iters_match(te_trc_db *db, trc_test_iter *iter1, trc_test_iter *iter2)
{
    trc_test_iter_arg *arg1;
    trc_test_iter_arg *arg2;
    iters_match_result result = ITERS_MATCH_EXACT;

    arg2 = TAILQ_FIRST(&iter2->args.head);
    TAILQ_FOREACH(arg1, &iter1->args.head, links)
    {
        if (arg2 == NULL)
            return ITERS_MATCH_NO;

        if (strcmp(arg1->name, arg2->name) != 0)
            return ITERS_MATCH_NO;

        if (strcmp(arg1->value, arg2->value) != 0)
        {
            if (*(arg1->value) != '\0' && *(arg2->value) != '\0')
            {
                const char *val1 = resolve_value(db, arg1->value);
                const char *val2 = resolve_value(db, arg2->value);

                if (val1 == NULL || val2 == NULL)
                    return ITERS_MATCH_NO;
                if (trc_db_compare_values(val1, val2) != 0)
                    return ITERS_MATCH_NO;
            }
            else
            {
                bool first_empty = (*(arg1->value) == '\0');
                bool second_empty = (*(arg2->value) == '\0');

                if (first_empty && second_empty)
                {
                    /* Do nothing */
                }
                else if (first_empty)
                {
                    if (result == ITERS_MATCH_EXACT)
                        result = ITERS_MATCH_SUPERSET;
                    else if (result == ITERS_MATCH_SUBSET)
                        result = ITERS_MATCH_INTERSECT;
                }
                else
                {
                    if (result == ITERS_MATCH_EXACT)
                        result = ITERS_MATCH_SUBSET;
                    else if (result == ITERS_MATCH_SUPERSET)
                        result = ITERS_MATCH_INTERSECT;
                }
            }
        }

        arg2 = TAILQ_NEXT(arg2, links);
    }

    if (arg2 != NULL)
        return ITERS_MATCH_NO;

    return result;
}

/**
 * Merge expected results from the source iteration record to the
 * destination one.
 *
 * @param dst               Destination iteration record.
 * @param src               Source iteration record.
 * @param dst_last_match    Value of 'last_match' attribute for the
 *                          destination TRC database.
 * @param src_last_match    Value of 'last_match' attribute for the
 *                          source TRC database.
 */
static void
merge_results(trc_test_iter *dst, trc_test_iter *src, bool dst_last_match,
              bool src_last_match)
{
    trc_exp_result *r;
    trc_exp_result *r_prev = NULL;
    trc_exp_result *r_dup;

    if (STAILQ_EMPTY(&src->exp_results))
    {
        if (src->exp_default != NULL)
        {
            /*
             * Default expected result of the source iteration record
             * replaces expected results of the destination iteration
             * record.
             */
            trc_exp_results_free(&dst->exp_results);
            dst->exp_default = src->exp_default;
        }

        return;
    }

    if (src_last_match != dst_last_match)
    {
        if (dst_last_match)
        {
#ifdef STAILQ_LAST
            r_prev = STAILQ_LAST(&dst->exp_results, trc_exp_result, links);
#else
            STAILQ_FOREACH(r, &dst->exp_results, links)
                r_prev = r;
#endif
        }
    }

    STAILQ_FOREACH(r, &src->exp_results, links)
    {
        r_dup = trc_exp_result_dup(r);
        if (r_prev == NULL)
        {
            if (dst_last_match)
                STAILQ_INSERT_TAIL(&dst->exp_results, r_dup, links);
            else
                STAILQ_INSERT_HEAD(&dst->exp_results, r_dup, links);
        }
        else
        {
            STAILQ_INSERT_AFTER(&dst->exp_results, r_prev, r_dup, links);
        }

        if (src_last_match == dst_last_match)
            r_prev = r_dup;
    }
}

/**
 * Insert merged iteration record after a given one.
 *
 * @param iters     Queue of iteration records.
 * @param tgt       Iteration record after which to insert.
 *                  If NULL, the iteration record should be
 *                  inserted at the beginning of the queue.
 * @param iter      Iteration record to be inserted.
 */
static void
insert_iter_after(trc_test_iters *iters, trc_test_iter *tgt,
                  trc_test_iter *iter)
{
    trc_test_iter *p;

    p = tgt;

    while (true)
    {
        if (p == NULL)
            p = TAILQ_FIRST(&iters->head);
        else
            p = TAILQ_NEXT(p, links);

        /*
         * NULL parent means this iteration record was inserted
         * recently, and we should insert a new one after it
         * to keep order of iteration records from the merged
         * database.
         */
        if (p != NULL && p->parent == NULL)
            tgt = p;
        else
            break;
    }

    if (tgt == NULL)
        TAILQ_INSERT_HEAD(&iters->head, iter, links);
    else
        TAILQ_INSERT_AFTER(&iters->head, tgt, iter, links);
}

static void fix_merged_tests(trc_tests *tests);

/**
 * Reset pointers to XML nodes for iterations copied from
 * a merged TRC database - these pointers are invalid
 * in the target database.
 *
 * @param iters     Queue of iterations.
 */
static void
fix_merged_iters(trc_test_iters *iters)
{
    trc_test_iter *iter;

    TAILQ_FOREACH(iter, &iters->head, links)
    {
        iter->node = NULL;
        fix_merged_tests(&iter->tests);
    }
}

/**
 * Reset pointers to XML nodes for tests copied from
 * a merged TRC database - these pointers are invalid
 * in the target database.
 *
 * @param tests    Queue of tests.
 */
static void
fix_merged_tests(trc_tests *tests)
{
    trc_test *test;

    TAILQ_FOREACH(test, &tests->head, links)
    {
        test->node = NULL;
        fix_merged_iters(&test->iters);
    }
}

static trc_test_iter *iter_dup(trc_test_iter *iter);

/**
 * Duplicate a given test.
 *
 * @param test      Test to duplicate.
 *
 * @return Created copy of the test.
 */
static trc_test *
test_dup(trc_test *test)
{
    trc_test *dup_test;
    trc_test_iter *iter;
    trc_test_iter *dup_iter;

    dup_test = TE_ALLOC(sizeof(*test));
    dup_test->type = test->type;

    if (test->name != NULL)
        dup_test->name = strdup(test->name);
    if (test->path != NULL)
        dup_test->path = strdup(test->path);
    if (test->notes != NULL)
        dup_test->notes = strdup(test->notes);
    if (test->objective != NULL)
        dup_test->objective = strdup(test->objective);

    TAILQ_INIT(&dup_test->iters.head);
    TAILQ_FOREACH(iter, &test->iters.head, links)
    {
        dup_iter = iter_dup(iter);
        dup_iter->parent = dup_test;
        TAILQ_INSERT_TAIL(&dup_test->iters.head, dup_iter, links);
    }

    return dup_test;
}

/**
 * Duplicate a given iteration.
 *
 * @param iter      Iteration to duplicate.
 *
 * @return Created copy of the iteration.
 */
static trc_test_iter *
iter_dup(trc_test_iter *iter)
{
    trc_test_iter *dup_iter;
    trc_test *test;
    trc_test *dup_test;

    dup_iter = TE_ALLOC(sizeof(*iter));
    trc_test_iter_args_init(&dup_iter->args);
    trc_test_iter_args_copy(&dup_iter->args, &iter->args);
    dup_iter->exp_default = iter->exp_default;
    STAILQ_INIT(&dup_iter->exp_results);
    trc_exp_results_cpy(&dup_iter->exp_results, &iter->exp_results);

    if (iter->notes != NULL)
        dup_iter->notes = strdup(iter->notes);

    TAILQ_INIT(&dup_iter->tests.head);
    TAILQ_FOREACH(test, &iter->tests.head, links)
    {
        dup_test = test_dup(test);
        dup_test->parent = dup_iter;
        TAILQ_INSERT_TAIL(&dup_iter->tests.head, dup_test, links);
    }

    return dup_iter;
}

/**
 * Merge iteration from another TRC database. This may result in
 * adding new iterations and/or updating preexisting ones.
 *
 * @param target_test   Test where to merge iteration.
 * @param merged_iter   Merged iteration.
 * @param target_db     TRC database of the target test.
 * @param merged_db     TRC database of the merged iteration.
 */
static void
merge_iter(trc_test *target_test, trc_test_iter *merged_iter,
           te_trc_db *target_db, te_trc_db *merged_db)
{
    trc_test_iter *p;
    trc_test_iter *target_iter;
    trc_test_iter_arg *arg1;
    trc_test_iter_arg *arg2;
    iters_match_result match;
    bool add_same_iter = true;
    bool same_results;

    TAILQ_FOREACH(p, &target_test->iters.head, links)
    {
        /*
         * Do not match against newly added iterations,
         * only against preexisting ones.
         */
        if (p->parent == NULL)
            continue;

        match = iters_match(target_db, p, merged_iter);
        if (match == ITERS_MATCH_NO)
            continue;

        same_results = false;
        if (trc_exp_results_cmp(
                &merged_iter->exp_results, &p->exp_results,
                RESULTS_CMP_NO_NOTES) == 0)
        {
            same_results = true;
        }

        if (match == ITERS_MATCH_EXACT || match == ITERS_MATCH_SUBSET)
        {
            p->exp_default = merged_iter->exp_default;
            if (!same_results)
            {
                merge_results(p, merged_iter, target_db->last_match,
                              merged_db->last_match);
            }

            if (match == ITERS_MATCH_EXACT)
                add_same_iter = false;
        }
        else if (match == ITERS_MATCH_SUPERSET)
        {
            add_same_iter = false;
        }

        if (match == ITERS_MATCH_EXACT || match == ITERS_MATCH_SUBSET ||
            same_results)
        {
            target_iter = p;
        }
        else
        {
            /*
             * If matching iteration record in the target database
             * can describe not only test iterations matching
             * the merged iteration record, create a new
             * iteration record describing intersection of the
             * matching iteration record and the merged iteration
             * record.
             */
            target_iter = TE_ALLOC(sizeof(*merged_iter));

            /*
             * Parent is filled later to distinguish preexisting
             * iterations and newly added ones and avoid matching
             * the currently merged iteration to those merged from
             * the same database before.
             */
            target_iter->parent = NULL;
            trc_test_iter_args_init(&target_iter->args);
            trc_test_iter_args_copy(&target_iter->args, &merged_iter->args);
            /* exp_default is stored in a global queue */
            target_iter->exp_default = merged_iter->exp_default;
            STAILQ_INIT(&target_iter->exp_results);

            /*
             * Add expected results from both iteration records so
             * that results from the merged iteration record
             * will have priority when determining expected
             * result.
             */
            merge_results(target_iter, p, target_db->last_match,
                          merged_db->last_match);
            merge_results(target_iter, merged_iter,
                          target_db->last_match,
                          merged_db->last_match);

            arg2 = TAILQ_FIRST(&p->args.head);
            TAILQ_FOREACH(arg1, &target_iter->args.head, links)
            {
                if (*(arg2->value) != '\0' && *(arg1->value) == '\0')
                {
                    free(arg1->value);
                    arg1->value = strdup(arg2->value);
                }

                arg2 = TAILQ_NEXT(arg2, links);
            }

            insert_iter_after(&target_test->iters, p, target_iter);
        }

        if (!TAILQ_EMPTY(&merged_iter->tests.head))
        {
            db_merge_tests(target_iter, &target_iter->tests,
                           &merged_iter->tests,
                           target_db, merged_db);
        }
    }

    if (add_same_iter)
    {
        /*
         * If there was no exact or superset match, add copy
         * of the merged iteration at the beginning to match
         * test iterations not described by subset or intersect
         * matches.
         */
        target_iter = iter_dup(merged_iter);
        insert_iter_after(&target_test->iters, NULL, target_iter);
    }
}

/**
 * Merge iterations from another TRC database.
 *
 * @param target_test     Test where to merge iterations.
 * @param merged_test     Test with merged iterations.
 * @param target_db       Target TRC database.
 * @param merged_db       Merged TRC database.
 */
static void
db_merge_iters(trc_test *target_test, trc_test *merged_test,
               te_trc_db *target_db, te_trc_db *merged_db)
{
    trc_test_iter *iter;
    trc_test_iter *iter_aux;

    if (merged_test->override_iters)
    {
        TAILQ_FOREACH(iter, &target_test->iters.head, links)
        {
            xmlUnlinkNode(iter->node);
            xmlFreeNode(iter->node);
        }

        trc_free_test_iters(&target_test->iters);
    }

    TAILQ_FOREACH_SAFE(iter, &merged_test->iters.head, links, iter_aux)
    {
        merge_iter(target_test, iter, target_db, merged_db);
    }

    TAILQ_FOREACH(iter, &target_test->iters.head, links)
        iter->parent = target_test;
}

/**
 * Merge tests from another TRC database.
 *
 * @param parent_iter     Parent iteration in the target database.
 * @param target_tests    Target queue of tests.
 * @param merged_tests    Merged queue of tests.
 * @param target_db       Target TRC database.
 * @param merged_db       Merged TRC database.
 */
static void
db_merge_tests(trc_test_iter *parent_iter,
               trc_tests *target_tests, trc_tests *merged_tests,
               te_trc_db *target_db, te_trc_db *merged_db)
{
    trc_test *merged_test;
    trc_test *target_test;
    bool found;

    TAILQ_FOREACH(merged_test, &merged_tests->head, links)
    {
        found = false;
        TAILQ_FOREACH(target_test, &target_tests->head, links)
        {
            if (strcmp(target_test->name, merged_test->name) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            target_test = trc_db_new_test(target_tests, parent_iter,
                                          merged_test->name);
        }

        target_test->type = merged_test->type;
        if (target_test->objective == NULL && merged_test->objective != NULL)
            target_test->objective = strdup(merged_test->objective);

        db_merge_iters(target_test, merged_test,
                       target_db, merged_db);
    }
}

/**
 * Merge globals from another database.
 *
 * @param target_globals    Target queue of globals.
 * @param merged_globals    Merged queue of globals.
 */
static void
db_merge_globals(trc_globals *target_globals, trc_globals *merged_globals)
{
    trc_global *p;
    trc_global *p_aux;
    trc_global *q;
    bool found;

    TAILQ_FOREACH_SAFE(p, &merged_globals->head, links, p_aux)
    {
        found = false;
        TAILQ_FOREACH(q, &target_globals->head, links)
        {
            if (strcmp(p->name, q->name) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            TAILQ_REMOVE(&merged_globals->head, p, links);
            TAILQ_INSERT_TAIL(&target_globals->head, p, links);
            p->node = NULL;
        }
    }
}

/* See description in te_trc.h */
te_errno
trc_db_open_merge(te_trc_db *db, const char *location, int flags)
{
    te_trc_db *merged_db = NULL;
    te_errno rc;

    rc = trc_db_open_ext(location, &merged_db, flags);
    if (rc != 0)
        return rc;

    db_merge_globals(&db->globals, &merged_db->globals);
    db_merge_tests(NULL, &db->tests, &merged_db->tests, db, merged_db);
    trc_db_close(merged_db);

    db->merged = true;
    return 0;
}

static te_errno trc_update_tests(te_trc_db *db, trc_tests *tests, int flags,
                                 int uid,
                                 bool (*to_save)(void *, bool),
                                 char *(*set_user_attr)(void *, bool));

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
                      bool is_default)
{
    trc_exp_result_entry    *res_entry;

    if (exp_result == NULL)
        return 0;

    if (!is_default && exp_result->tags_str != NULL)
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
                       bool insert_after)
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

        trc_exp_result_to_xml(result, results_node, false);
    }

    return 0;
}

/**
 * Add a child right after initial comments under a given parent.
 * If there is no comments, it is added as the first child.
 *
 * @param parent      Pointer to parent node.
 * @param child       Pointer to added child node.
 *
 * @return Pointer to added child or NULL in case of failure.
 */
static xmlNodePtr
add_child_after_comments(xmlNodePtr parent, xmlNodePtr child)
{
    xmlNodePtr aux;

    for (aux = parent->children; aux != NULL; aux = aux->next)
    {
        if (aux->type != XML_COMMENT_NODE)
            break;
    }

    if (aux == NULL)
        return xmlAddChild(parent, child);
    else
        return xmlAddPrevSibling(aux, child);
}

static te_errno
put_subvalue(char *key, size_t idx, char *value, bool has_more, void *user)
{
    xmlNodePtr target = user;
    xmlNodePtr field = xmlNewChild(target, NULL, BAD_CAST "field", NULL);

    if (field == NULL)
    {
        ERROR("%s(): xmlNewChild failed", __func__);
        return TE_RC(TE_TRC, TE_ENOMEM);
    }

    if (key != NULL)
        xmlSetProp(field, BAD_CAST "name", BAD_CAST key);

    xmlNodeSetContent(field, BAD_CAST value);

    return 0;
}

static te_errno
make_compound_value(xmlNodePtr target, const char *value)
{
    te_string compound;

    if (te_str_is_null_or_empty(value))
        return 0;

    compound = (te_string)TE_STRING_INIT_RO_PTR(value);
    switch (te_compound_classify(&compound))
    {
        case TE_COMPOUND_NULL:
        case TE_COMPOUND_PLAIN:
            xmlNodeSetContent(target, BAD_CAST value);
            return 0;
        case TE_COMPOUND_ARRAY:
        case TE_COMPOUND_OBJECT:
            return te_compound_iterate(&compound, put_subvalue, target);
    }

    return 0;
}

static te_errno
trc_update_iters(te_trc_db *db, trc_test_iters *iters, int flags, int uid,
                 bool (*to_save)(void *, bool),
                 char *(*set_user_attr)(void *, bool))
{
    te_errno        rc;
    trc_test_iter  *p;
    void           *user_data;
    bool is_saved;
    char           *user_attr;
    xmlNodePtr      node;
    xmlNodePtr      prev_node;
    xmlNodePtr      prev_iter_node = NULL;
    xmlNodePtr      first_iter_node = NULL;
    bool renew_content;

    TAILQ_FOREACH(p, &iters->head, links)
    {
        if (p->node != NULL)
        {
            first_iter_node = p->node;
            break;
        }
    }

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
            is_saved = to_save(user_data, true);
        else
            is_saved = true;

        if (!is_saved && p->node != NULL)
        {
            if (first_iter_node == p->node)
                first_iter_node = xmlNodeNext(p->node);

            xmlUnlinkNode(p->node);
            xmlFreeNode(p->node);
            p->node = NULL;
        }

        if (is_saved)
        {
            trc_test_iter_arg  *a;

            renew_content = true;

            if (p->node == NULL)
            {
                INFO("Add node for iteration %p node=%p", iters,
                     iters->node);
                p->node = p->tests.node = xmlNewNode(NULL,
                                                     BAD_CAST "iter");
                if (p->tests.node == NULL)
                {
                    ERROR("xmlNewNode() failed for 'iter'");
                    return TE_ENOMEM;
                }

                if (prev_iter_node != NULL)
                    node = xmlAddNextSibling(prev_iter_node, p->node);
                else if (first_iter_node != NULL)
                    node = xmlAddPrevSibling(first_iter_node, p->node);
                else
                    node = xmlAddChild(iters->node, p->node);

                if (node == NULL)
                {
                    ERROR("Failed to add 'iter' node to the tree");
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
                        if ((flags & TRC_SAVE_COMMENTS) &&
                            node->type == XML_COMMENT_NODE)
                        {
                            continue;
                        }

                        xmlUnlinkNode(node);
                        xmlFreeNode(node);
                    }
                }
            }
            else
                renew_content = false;

            if (renew_content)
            {
                tqe_string *tq_str;

                xmlSetProp(p->tests.node, BAD_CAST "result",
                           BAD_CAST te_test_status_to_str(
                               TAILQ_FIRST(&p->exp_default->results)->
                               result.status));

                if (set_user_attr != NULL)
                {
                    user_attr = set_user_attr(user_data, true);
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
                        bool found = false;

                        TAILQ_FOREACH(a, &p->args.head, links)
                        {
                            if (strcmp(a->name, tq_str->v) == 0)
                            {
                                found = true;
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

                    make_compound_value(node, a->value);

                    xmlNewProp(node, BAD_CAST "name", BAD_CAST a->name);

                    if (prev_node == NULL)
                        add_child_after_comments(p->tests.node, node);
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
                    add_child_after_comments(p->tests.node, node);
                else
                    xmlAddNextSibling(prev_node, node);

                prev_node = node;

                if ((flags & TRC_SAVE_RESULTS) &&
                    !STAILQ_EMPTY(&p->exp_results))
                    trc_exp_results_to_xml(&p->exp_results,
                                           prev_node,
                                           true);
            }

            if (is_saved)
            {
                rc = trc_update_tests(db, &p->tests, flags, uid, to_save,
                                      set_user_attr);
                if (rc != 0)
                    return rc;
            }
        }

        if (p->node != NULL)
            prev_iter_node = p->node;
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
trc_update_tests(te_trc_db *db, trc_tests *tests, int flags, int uid,
                 bool (*to_save)(void *, bool),
                 char *(*set_user_attr)(void *, bool))
{
    te_errno    rc;
    trc_test   *p;
    xmlNodePtr  node;
    xmlNodePtr  prev_node;
    void       *user_data;
    bool is_saved;
    bool renew_content = false;
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
            is_saved = to_save(user_data, false);
        else
            is_saved = true;

        if (!is_saved && p->node != NULL)
        {
            xmlUnlinkNode(p->node);
            xmlFreeNode(p->node);
            p->node = NULL;
        }

        if (is_saved)
        {
            xmlNodePtr globals_node = NULL;
            renew_content = true;

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
                                  CONST_CHAR2XML("globals")) == 0)
                    {
                        globals_node = node;
                        continue;
                    }

                    if (xmlStrcmp(node->name,
                                  CONST_CHAR2XML("notes")) == 0 ||
                        xmlStrcmp(node->name,
                                  CONST_CHAR2XML("objective")) == 0 ||
                        (node->type != XML_ELEMENT_NODE &&
                         node->type != XML_XINCLUDE_START &&
                         node->type != XML_XINCLUDE_END))
                    {
                        if ((flags & TRC_SAVE_COMMENTS) &&
                            node->type == XML_COMMENT_NODE)
                        {
                            continue;
                        }

                        xmlUnlinkNode(node);
                        xmlFreeNode(node);
                    }
                }
            }
            else
                renew_content = false;

            if (renew_content)
            {
                xmlSetProp(p->iters.node, BAD_CAST "name",
                           BAD_CAST p->name);
                xmlSetProp(p->iters.node, BAD_CAST "type",
                           BAD_CAST trc_test_type_to_str(p->type));

                if (flags & TRC_SAVE_POS_ATTR)
                {
                    user_attr = TE_ALLOC(MAX_POS_LEN);

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

                add_child_after_comments(p->iters.node, node);

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
                    !TAILQ_EMPTY(&db->globals.head))
                {
                    trc_global      *g;
                    xmlNodePtr       global_node;
                    xmlNodePtr       value_node;

                    flags &= ~TRC_SAVE_GLOBALS;

                    if (globals_node == NULL)
                    {
                        if ((globals_node = xmlNewNode(NULL,
                                                BAD_CAST "globals")) == NULL)
                        {
                            ERROR("xmlNewNode() failed for 'globals'");
                            return TE_ENOMEM;
                        }

                        xmlAddNextSibling(prev_node, globals_node);
                    }
                    else
                    {
                        while ((global_node = globals_node->children) !=
                                                                      NULL)
                        {
                            xmlUnlinkNode(global_node);
                            xmlFreeNode(global_node);
                        }
                    }

                    TAILQ_FOREACH(g, &db->globals.head, links)
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
                                                 NULL);
                        if (value_node == NULL)
                        {
                            ERROR("xmlNewChild() failed for 'value'");
                            return TE_ENOMEM;
                        }
                        make_compound_value(value_node, g->value);

                        xmlNewProp(global_node, BAD_CAST "name",
                                   BAD_CAST g->name);
                    }
                }
                else if (globals_node != NULL)
                {
                    xmlUnlinkNode(globals_node);
                    xmlFreeNode(globals_node);
                }
            }
        }

        if (is_saved)
        {
            rc = trc_update_iters(db, &p->iters, flags, uid, to_save,
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
static te_errno trc_tests_pos(trc_test *test, bool is_first,
                              bool is_top);

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
trc_iters_pos(trc_test_iter *iter, bool is_first)
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
            if (trc_tests_pos(test, true, false) != 0)
                return -1;
        }
        else if (is_first)
        {
            const char            *filename_oth = p->filename;
            trc_test_iter         *p_prev = p;

            if (trc_iters_pos(p, false) != 0)
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
trc_tests_pos(trc_test *test, bool is_first, bool is_top)
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
            if (trc_iters_pos(iter, true) != 0)
                return -1;
        }
        else if (is_first)
        {
            const char       *filename_oth = p->filename;
            trc_test         *p_prev = p;

            if (trc_tests_pos(p, false, false) != 0)
                return -1;

            /* Skip tests that were handled by recursive call. */

            while ((p = TAILQ_NEXT(p, links)) != NULL &&
                   strcmp_null(p->filename, filename_oth) == 0)
                p_prev = p;
            p = p_prev;
        }
        else
            break;

        is_top = false;
    } while ((p = TAILQ_NEXT(p, links)) != NULL);

    return 0;
}

/**
 * Check whether queue of tags contain a given name.
 *
 * @param tags      Queue of tags.
 * @param name      Tag name to check for.
 *
 * @return true if tags contain the name, false otherwise.
 */
static bool
tags_contain(const tqh_strings *tags, const char *name)
{
    const tqe_string *s;
    int name_len = strlen(name);

    TAILQ_FOREACH(s, tags, links)
    {
        char *c;

        c = strchr(s->v, ':');
        if (c == NULL)
        {
            if (strcmp(s->v, name) == 0)
                return true;
        }
        else
        {
            int len = c - s->v;

            /*
             * Any tag value specified after ':' is currently
             * ignored.
             */
            if (len == name_len && strncmp(name, s->v, c - s->v) == 0)
                return true;
        }
    }

    return false;
}

/**
 * Check whether a given logical expression mentions one of
 * the given tags not negated with NOT.
 *
 * @param expr_dnf      Logical expression in DNF.
 * @param tags          Tags to check for.
 *
 * @return true if one of the tags is mentioned, false otherwise.
 */
static bool
check_tags_mention(const logic_expr *expr_dnf, const tqh_strings *tags)
{
    if (expr_dnf->type == LOGIC_EXPR_VALUE)
    {
        return tags_contain(tags, expr_dnf->u.value);
    }
    else if (expr_dnf->type == LOGIC_EXPR_NOT)
    {
        if (expr_dnf->u.unary->type == LOGIC_EXPR_VALUE)
            return false;
        else
            return check_tags_mention(expr_dnf->u.unary, tags);
    }

    return check_tags_mention(expr_dnf->u.binary.lhv, tags) ||
           check_tags_mention(expr_dnf->u.binary.rhv, tags);
}

/**
 * Split DNF logical expression into two subexpressions:
 * one mentioning one of the given tags (not negated), another
 * one not mentioning any of them.
 *
 * @param expr_dnf      Logical expression to split.
 * @param tags          Tags to look for.
 * @param match         Will be set to the subexpression mentioning
 *                      the tags.
 * @param nomatch       Will be set to the subexpression not mentioning
 *                      the tags.
 */
static void
split_expr_dnf(logic_expr *expr_dnf, const tqh_strings *tags,
               logic_expr **match, logic_expr **nomatch)
{
    if (expr_dnf->type == LOGIC_EXPR_OR)
    {
        split_expr_dnf(expr_dnf->u.binary.lhv, tags, match, nomatch);
        split_expr_dnf(expr_dnf->u.binary.rhv, tags, match, nomatch);
    }
    else
    {
        bool mentions = check_tags_mention(expr_dnf, tags);
        logic_expr **target;
        logic_expr *new_child;
        logic_expr *new_parent;

        new_child = logic_expr_dup(expr_dnf);
        assert(new_child != NULL);

        if (mentions)
            target = match;
        else
            target = nomatch;

        if (*target == NULL)
        {
            *target = new_child;
        }
        else
        {
            new_parent = TE_ALLOC(sizeof(logic_expr));
            new_parent->type = LOGIC_EXPR_OR;
            new_parent->u.binary.lhv = new_child;
            new_parent->u.binary.rhv = *target;

            *target = new_parent;
        }
    }
}

static bool tests_filter_by_tags(trc_tests *tests, const tqh_strings *tags,
                                 unsigned int flags);

/**
 * Remove XML node together with any comments directly preceding
 * it.
 *
 * @param node    XML node pointer.
 */
static void
del_node_with_comments(xmlNodePtr node)
{
    xmlNodePtr node_prev;

    if (node == NULL)
        return;

    node_prev = node->prev;
    xmlUnlinkNode(node);
    xmlFreeNode(node);

    /* Remove any comments preceding removed XML node. */
    for (node = node_prev; node != NULL; node = node_prev)
    {
        node_prev = node_prev->prev;

        if (node->type == XML_COMMENT_NODE)
        {
            xmlUnlinkNode(node);
            xmlFreeNode(node);
        }
        else
        {
            break;
        }
    }
}

/**
 * Perform the job of trc_db_filter_by_tags() for all iterations from
 * a queue.
 *
 * @param iters     Queue of iterations.
 * @param tags      TRC tags.
 * @param flags     See trc_filter_flags.
 *
 * @return true if some expected results remain after filtering,
 *         false otherwise.
 */
static bool
iters_filter_by_tags(trc_test_iters *iters, const tqh_strings *tags,
                     unsigned int flags)
{
    trc_test_iter *iter;
    trc_test_iter *iter_aux;
    trc_exp_result *result;
    trc_exp_result *result_aux;
    bool iters_exp_result = false;
    bool iter_exp_result = false;
    bool reverse = !!(flags & TRC_FILTER_REVERSE);

    TAILQ_FOREACH_SAFE(iter, &iters->head, links, iter_aux)
    {
        iter_exp_result = false;

        STAILQ_FOREACH_SAFE(result, &iter->exp_results, links, result_aux)
        {
            logic_expr *dup;
            logic_expr *match = NULL;
            logic_expr *nomatch = NULL;
            bool remove = false;
            bool update_expr = true;

            if (result->tags_expr == NULL)
            {
                if (!reverse)
                    remove = true;
                else
                    update_expr = false;
            }
            else
            {
                dup = logic_expr_dup(result->tags_expr);
                assert(dup != NULL);

                logic_expr_dnf(&dup, NULL);
                split_expr_dnf(dup, tags, &match, &nomatch);
                logic_expr_free(dup);

                if (match == NULL)
                {
                    if (reverse)
                        update_expr = false;
                    else
                        remove = true;
                }
                else if (nomatch == NULL)
                {
                    if (reverse)
                        remove = true;
                    else
                        update_expr = false;
                }
            }

            if (remove)
            {
                STAILQ_REMOVE(&iter->exp_results, result,
                              trc_exp_result, links);
                trc_exp_result_free(result);
                free(result);
                logic_expr_free(match);
                logic_expr_free(nomatch);
            }
            else
            {
                iter_exp_result = true;

                if (update_expr)
                {
                    result->tags_expr = (reverse ? nomatch : match);
                    logic_expr_free(reverse ? match : nomatch);
                    free(result->tags_str);
                    result->tags_str = logic_expr_to_str(result->tags_expr);
                }
            }
        }

        if (tests_filter_by_tags(&iter->tests, tags, flags))
            iter_exp_result = true;

        if (iter_exp_result)
        {
            iters_exp_result = true;
        }
        else if (flags & TRC_FILTER_DEL_NO_RES)
        {
            TAILQ_REMOVE(&iters->head, iter, links);

            if (iter->node != NULL)
            {
                del_node_with_comments(iter->node);
                iter->node = NULL;
            }

            trc_free_test_iter(iter);
        }
    }

    return iters_exp_result;
}

/**
 * Perform the job of trc_db_filter_by_tags() for all tests from
 * a queue.
 *
 * @param tests     Queue of tests.
 * @param tags      TRC tags.
 * @param flags     See trc_filter_flags.
 *
 * @return true if some expected results remain after filtering,
 *         false otherwise.
 */
static bool
tests_filter_by_tags(trc_tests *tests, const tqh_strings *tags,
                     unsigned int flags)
{
    trc_test *test;
    trc_test *test_aux;
    bool result = false;

    TAILQ_FOREACH_SAFE(test, &tests->head, links, test_aux)
    {
        if (iters_filter_by_tags(&test->iters, tags, flags))
        {
            result = true;
        }
        else if (flags & TRC_FILTER_DEL_NO_RES)
        {
            TAILQ_REMOVE(&tests->head, test, links);

            if (test->node != NULL)
            {
                del_node_with_comments(test->node);
                test->node = NULL;
            }

            trc_free_trc_test(test);
        }
    }

    return result;
}

/* See description in trc_db.h */
void
trc_db_filter_by_tags(te_trc_db *db, const tqh_strings *tags,
                      unsigned int flags)
{
    tests_filter_by_tags(&db->tests, tags, flags);
}

/* See description in trc_db.h */
te_errno
trc_db_save(te_trc_db *db, const char *filename, int flags,
            int uid, bool (*to_save)(void *, bool),
            char *(*set_user_attr)(void *, bool),
            char *cmd, bool quiet)
{
    const char          *fn = (filename != NULL) ?
                                    filename : db->filename;
    te_errno             rc;
    trc_test            *test;

    xmlNodePtr node;

    if (flags & TRC_SAVE_REMOVE_OLD)
    {
        xmlFreeDoc(db->xml_doc);
        db->xml_doc = NULL;
    }

    if (db->xml_doc == NULL)
    {
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

    node = xmlDocGetRootElement(db->xml_doc);

    rc = update_boolean_prop(node, "last_match", db->last_match);
    if (rc != 0)
        return rc;

    rc = update_boolean_prop(node, "merged", db->merged);
    if (rc != 0)
        return rc;

    test = TAILQ_FIRST(&db->tests.head);
    if (flags & TRC_SAVE_POS_ATTR)
        trc_tests_pos(test, true, true);

    if ((rc = trc_update_tests(db, &db->tests, flags, uid, to_save,
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
    else if (!quiet)
    {
        RING("DB with expected testing results has been updated:\n%s\n\n",
             fn);
    }

    trc_files_free(inc_files);
    free(inc_files);
    inc_files = NULL;

    return 0;
}
