/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Tester Subsystem
 *
 * Code dealing with configuration files parsing and preprocessing.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

/** Logging user name to be used here */
#define TE_LGR_USER     "Config File Parser"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include "te_alloc.h"
#include "te_param.h"
#include "te_expand.h"
#include "te_str.h"
#include "tester_conf.h"
#include "type_lib.h"
#include "tester_cmd_monitor.h"

#include "tester.h"

/**
 * Cast 'const char *' to 'const xmlChar *' required by libxml2
 * routines.
 */

#define CONST_CHAR2XML  (const xmlChar *)

/** Cast 'xmlChar *' to 'char *' used in Tester data structures. */
#define XML2CHAR(p)     ((char *)p)

/** Duplicate 'xmlChar *' as 'char *' */
#define XML2CHAR_DUP(p) XML2CHAR(xmlStrdup(p))

enum {
    TESTER_RUN_ITEM_SERVICE  = 1 << 0,
    TESTER_RUN_ITEM_INHERITABLE = 1 << 1,
};


static const test_info * find_test_info(const tests_info *ti,
                                        const char *name);

static te_errno alloc_and_get_run_item(xmlNodePtr node, tester_cfg *cfg,
                                       unsigned int opts,
                                       run_item_role role,
                                       const test_session *session,
                                       run_items *runs, run_item **p_run);

static te_errno parse_test_package(tester_cfg         *cfg,
                                   const test_session *session,
                                   test_package       *pkg,
                                   char const         *src,
                                   run_item           *ritem);

static void run_item_free(run_item *run);
static void run_items_free(run_items *runs);


/**
 * Allocatate and initialize Tester configuration.
 *
 * @param filename      Name of the file with configuration
 *
 * @return Pointer to allocated and initialized Tester configuration or
 *         NULL.
 */
tester_cfg *
tester_cfg_new(const char *filename)
{
    tester_cfg *p = TE_ALLOC(sizeof(*p));

    p->filename = filename;
    TAILQ_INIT(&p->maintainers);
    TAILQ_INIT(&p->suites);
    TAILQ_INIT(&p->options);
    TAILQ_INIT(&p->runs);

    return p;
}


/**
 * Skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return Current, one of next or NULL.
 */
static xmlNodePtr
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
 * Skip 'text' nodes.
 *
 * @param node      XML node
 *
 * @return Current, one of next or NULL.
 */
static xmlNodePtr
xmlNodeSkipText(xmlNodePtr node)
{
    while ((node != NULL) &&
           (xmlStrcmp(node->name, CONST_CHAR2XML("text")) == 0))
    {
        node = node->next;
    }
    return node;
}

/**
 * Go to the next XML node, skip 'comment' nodes and 'text' notes
 * (to cope with unexpected text only content).
 *
 * @param node      XML node
 *
 * @return The next XML node.
 */
static xmlNodePtr
xmlNodeChildren(xmlNodePtr node)
{
    assert(node != NULL);
    return xmlNodeSkipText(xmlNodeSkipComment(node->children));
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
static xmlNodePtr
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
 * @return Status code.
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
        return TE_EINVAL;
    }
    if (xmlStrcmp(node->children->name, CONST_CHAR2XML("text")) != 0)
    {
        ERROR("Unexpected element '%s' in the node '%s' with text "
              "content", node->children->name, name);
        return TE_EINVAL;
    }
    if (node->children->content == NULL)
    {
        ERROR("Empty content of the node '%s'", name);
        return TE_EINVAL;
    }

    *content = XML2CHAR_DUP(node->children->content);
    if (*content == NULL)
    {
        ERROR("String duplication failed");
        return TE_ENOMEM;
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
 * @return Status code.
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
 * Make path to the Test Package or Test Script file by the name and
 * context.
 *
 * @param cfg           Tester configuration context
 * @param name          Name in the current test package
 * @param is_package    Is requested entity package or test script?
 *
 * @return Allocated path or NULL.
 */
static char *
name_to_path(tester_cfg *cfg, const char *name, bool is_package)
{
    char *path = NULL;

    if (name == NULL)
    {
        ERROR("Invalid name in the Test Package");
        return NULL;
    }

    if (cfg->cur_pkg != NULL)
    {
        size_t package_add = (is_package) ? strlen("/package.xml") : 0;
        char *base_name_end = rindex(cfg->cur_pkg->path, '/');

        if (base_name_end == NULL)
        {
            ERROR("Invalid path to the parent Test Package file");
            return NULL;
        }
        path = malloc((base_name_end - cfg->cur_pkg->path) + 1 +
                      strlen(name) + package_add + 1);
        if (path == NULL)
        {
            ERROR("malloc() failed");
            return NULL;
        }
        /* Copy base path with the last '/' */
        memcpy(path, cfg->cur_pkg->path,
               (base_name_end - cfg->cur_pkg->path) + 1);
        path[(base_name_end - cfg->cur_pkg->path) + 1] = '\0';
        /* Concat package name to the path */
        strcat(path, name);
        if (is_package)
        {
            /* Concat "/package.xml" */
            strcat(path, "/package.xml");
        }
    }
    else if (is_package)
    {
        test_suite_info *p;
        const char      *base_path = NULL;

        TAILQ_FOREACH(p, &cfg->suites, links)
        {
            if (strcmp(p->name, name) == 0)
            {
                base_path = p->bin;
                break;
            }
        }
        if (base_path == NULL)
        {
            const char *base = getenv("TE_INSTALL_SUITE");

            if (base == NULL)
            {
                ERROR("Cannot guess path to the Test Package '%s' - "
                      "TE_INSTALL_SUITE is unspecified in Environment",
                      name);
                return NULL;
            }

            path = malloc(strlen(base) + strlen("/") + strlen(name) +
                          strlen("/package.xml") + 1);
            if (path == NULL)
            {
                ERROR("malloc() failed");
                return NULL;
            }
            /* Copy base path and /suites/ */
            strcpy(path, base);
            strcat(path, "/");
            /* Concat package name to the path */
            strcat(path, name);
        }
        else
        {
            path = malloc(strlen(base_path) + strlen("/package.xml") + 1);
            if (path == NULL)
            {
                ERROR("malloc() failed");
                return NULL;
            }
            /* Copy base path */
            strcpy(path, base_path);
        }
        /* Concat "/package.xml" */
        strcat(path, "/package.xml");
    }
    else
    {
        ERROR("Test script without test package");
    }

    return path;
}


/**
 * Get string.
 *
 * @param node      Node with string
 * @param strs      List of strings
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_tqe_string(xmlNodePtr node, tqh_strings *strs)
{
    tqe_string  *p;

#ifndef XML_DOC_ASSUME_VALID
    if (node->children != NULL)
    {
        ERROR("'string' cannot have children");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif
    p = TE_ALLOC(sizeof(*p));

    p->v = XML2CHAR_DUP(node->content);

    TAILQ_INSERT_TAIL(strs, p, links);

    return 0;
}


/**
 * Get information about suite.
 *
 * @param node          Node with information
 * @param suites_info   List with information about suites
 * @param build         Build test suite
 * @param verbose       Be verbose in the case of build failure
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_test_suite_info(xmlNodePtr        node,
                              test_suites_info *suites_info,
                              bool build,
                              bool verbose)
{
    test_suite_info *p;

#ifndef XML_DOC_ASSUME_VALID
    if (node->children != NULL)
    {
        ERROR("'suite' cannot have children");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif
    p = TE_ALLOC(sizeof(*p));

    TAILQ_INSERT_TAIL(suites_info, p, links);

    /* Name is mandatory */
    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("'name' attribute is missing in suite information");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    /* Path to sources is optional */
    p->src = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("src")));
    /* Path to binaries is optional */
    p->bin = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("bin")));

    if (p->src != NULL && p->bin != NULL)
    {
        ERROR("Two paths are specified for Test Suite '%s'", p->name);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    if (p->src == NULL && p->bin == NULL)
    {
        p->src = strdup(p->name);
        if (p->src == NULL)
        {
            ERROR("strdup(%s) failed", p->name);
            return TE_RC(TE_TESTER, TE_ENOMEM);
        }
    }

    if ((p->src != NULL) && build)
    {
        te_errno rc = tester_build_suite(p, verbose);

        if (rc != 0)
            return rc;
    }

    return 0;
}


/**
 * Get information about person.
 *
 * @param node      Node with information about person
 * @param persons   List with information about persons
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_person_info(xmlNodePtr node, persons_info *persons)
{
    person_info  *p;

    assert(node != NULL);
    assert(persons != NULL);

#ifndef XML_DOC_ASSUME_VALID
    if (node->children != NULL)
    {
        ERROR("'person_info' cannot have children");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif

    p = TE_ALLOC(sizeof(*p));

    TAILQ_INSERT_TAIL(persons, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    p->mailto = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("mailto")));
#ifndef XML_DOC_ASSUME_VALID
    if (p->mailto == NULL)
    {
        ERROR("'mailto' attribute is mandatory in person info");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif

    return 0;
}

/**
 * Get (possibly empty) list of information about persons.
 *
 * @param node      Location of the XML node pointer
 * @param node_name Name of the not with information
 * @param persons   List with information about persons
 *
 * @return Status code.
 */
static te_errno
get_persons_info(xmlNodePtr *node, const char *node_name,
                 persons_info *persons)
{
    te_errno rc;

    assert(*node != NULL);
    assert(node_name != NULL);
    assert(persons != NULL);

    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML(node_name)) == 0)
    {
        rc = alloc_and_get_person_info(*node, persons);
        if (rc != 0)
            return rc;
        (*node) = xmlNodeNext(*node);
    }

    return 0;
}


/**
 * Get option.
 *
 * @param opts      List of options
 * @param node      Node with new option
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_option(xmlNodePtr node, test_options *opts)
{
    xmlChar     *name;
    xmlChar     *value;
    test_option *p;
    xmlNodePtr   q;
    te_errno     rc;

    /* Name is mandatory */
    name = xmlGetProp(node, CONST_CHAR2XML("name"));
    if (name == NULL)
    {
        ERROR("'name' attribute of the option is missing");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    /* Path is optional */
    value = xmlGetProp(node, CONST_CHAR2XML("value"));

    p = TE_ALLOC(sizeof(*p));

    p->name = XML2CHAR(name);
    p->value = XML2CHAR(value);

    /* Get optional information about contexts */
    for (q = xmlNodeChildren(node);
         (q != NULL) &&
             (xmlStrcmp(q->name, CONST_CHAR2XML("context")) == 0);
         q = xmlNodeNext(q))
    {
        rc = alloc_and_get_tqe_string(q, &p->contexts);
        if (rc != 0)
        {
            /* FREE */
            return rc;
        }
    }
    if (q != NULL)
    {
        ERROR("'option' cannot have any children except 'context'");
        /* FREE */
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    TAILQ_INSERT_TAIL(opts, p, links);

    return 0;
}


/**
 * Get boolean property.
 *
 * @param node      Node with boolean property
 * @param name      Name of the property to get
 * @param value     Location for value
 *
 * @return Status code.
 * @retval TE_ENOENT    Property does not exists. Value is not modified.
 */
static te_errno
get_bool_prop(xmlNodePtr node, const char *name, bool *value)
{
    xmlChar *s = xmlGetProp(node, CONST_CHAR2XML(name));

    if (s == NULL)
        return TE_RC(TE_TESTER, TE_ENOENT);
    if (xmlStrcmp(s, CONST_CHAR2XML("true")) == 0)
        *value = true;
    else if (xmlStrcmp(s, CONST_CHAR2XML("false")) == 0)
        *value = false;
    else
    {
        ERROR("Invalid value '%s' of the boolean property '%s'",
              XML2CHAR(s), name);
        xmlFree(s);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    xmlFree(s);

    return 0;
}


/**
 * Get 'int' property.
 *
 * @param node      Node with 'int' property
 * @param name      Name of the property to get
 * @param value     Location for value
 *
 * @return Status code.
 * @retval TE_ENOENT    Property does not exists. Value is not modified.
 */
static te_errno
get_uint_prop(xmlNodePtr node, const char *name, unsigned int *value)
{
    xmlChar        *s = xmlGetProp(node, CONST_CHAR2XML(name));
    unsigned long   v;
    char           *end;

    if (s == NULL)
        return TE_RC(TE_TESTER, TE_ENOENT);

    v = strtoul(XML2CHAR(s), &end, 10);
    if (XML2CHAR(s) == end)
    {
        ERROR("Invalid value '%s' of the integer property '%s'",
              XML2CHAR(s), name);
        xmlFree(s);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    xmlFree(s);
    *value = v;

    return 0;
}

/**
 * Get double property.
 *
 * @param node      Node with double property.
 * @param name      Name of the property to get.
 * @param value     Location for value.
 *
 * @return Status code.
 * @retval TE_ENOENT    Property does not exists. Value is not modified.
 */
static te_errno
get_double_prop(xmlNodePtr node, const char *name, double *value)
{
    xmlChar *s = xmlGetProp(node, CONST_CHAR2XML(name));
    double v;
    te_errno rc;

    if (s == NULL)
        return TE_RC(TE_TESTER, TE_ENOENT);

    rc = te_strtod(XML2CHAR(s), &v);
    if (rc != 0)
    {
        ERROR("%s(): failed to parse property '%s'", __FUNCTION__, name);
        return TE_RC(TE_TESTER, rc);
    }

    xmlFree(s);
    *value = v;

    return 0;
}

/**
 * Get attribute with inheritance specification.
 *
 * @param node      Node with requested property
 * @param name      Name of the property to get
 * @param value     Location for value
 * @param def       Default value (used if inheritance
 *                  attribute is not specified)
 *
 * @return Status code.
 */
static te_errno
get_handdown_attr(xmlNodePtr node, const char *name,
                  tester_handdown *value, tester_handdown def)
{
    xmlChar *s;

    /* 'handdown' is optional */
    *value = def;
    s = xmlGetProp(node, CONST_CHAR2XML(name));
    if (s != NULL)
    {
        if (xmlStrcmp(s, CONST_CHAR2XML("none")) == 0)
            *value = TESTER_HANDDOWN_NONE;
        else if (xmlStrcmp(s, CONST_CHAR2XML("children")) == 0)
            *value = TESTER_HANDDOWN_CHILDREN;
        else if (xmlStrcmp(s, CONST_CHAR2XML("descendants")) == 0)
            *value = TESTER_HANDDOWN_DESCENDANTS;
        else
        {
            ERROR("Invalid value '%s' of 'handdown' property",
                  XML2CHAR(s));
            xmlFree(s);
            return TE_RC(TE_TESTER, TE_EINVAL);
        }
        xmlFree(s);
    }
    return 0;
}

/**
 * Get requirement.
 *
 * @param node          Node with requirement
 * @param reqs          List of requirements
 * @param allow_sticky  Allow sticky requirements
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_requirement(xmlNodePtr node, test_requirements *reqs,
                          bool allow_sticky)
{
    te_errno            rc;
    test_requirement   *p;

#ifndef XML_DOC_ASSUME_VALID
    if (node->children != NULL)
    {
        ERROR("'requirement' cannot have children");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif
    p = TE_ALLOC(sizeof(*p));

    TAILQ_INSERT_TAIL(reqs, p, links);

    p->id = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("id")));
    p->ref = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("ref")));
    if ((p->id == NULL) == (p->ref == NULL))
    {
        ERROR("One and only one of 'id' or 'ref' attributes must "
              "present for requirement");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

#if 1
    /* 'exclude' is deprecated */
    rc = get_bool_prop(node, "exclude", &p->sticky);
    if (rc != TE_RC(TE_TESTER, TE_ENOENT))
    {
        if (rc == 0)
        {
            ERROR("Unexpected 'exclude' property");
            rc = TE_RC(TE_TESTER, TE_EINVAL);
        }
        return rc;
    }
#endif

    /* 'sticky' is optional, default value is false */
    p->sticky = false;
    rc = get_bool_prop(node, "sticky", &p->sticky);
    if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
        return rc;
    if (rc == 0 && !allow_sticky)
    {
        ERROR("'sticky' requirements are not allowed for "
              "configurations and scripts");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    /* is req listed? */
    if (p->id != NULL)
    {
        test_requirement *r;
        test_requirement *new;
        test_requirement *before = NULL;

        TAILQ_FOREACH(r, &tester_global_context.reqs, links)
        {
            if (strcmp(p->id, r->id) == 0)
                goto req_exists;
            else if (strcmp(p->id, r->id) < 0 && before == NULL)
                before = r;
        }
        /* new requirement */
        new = TE_ALLOC(sizeof(*r));
        new->id = strdup(p->id);
        new->ref = NULL;
        new->sticky = false;
        if (before != NULL)
            TAILQ_INSERT_BEFORE(before, new, links);
        else
            TAILQ_INSERT_TAIL(&tester_global_context.reqs, new, links);
    }
req_exists:

    return 0;
}

/**
 * Get (possibly empty) set of requirements.
 *
 * @param node          Location of the XML node pointer
 * @param reqs          List of requirements to be updated
 * @param allow_sticky  Allow sticky requirements
 *
 * @return Status code.
 */
static te_errno
get_requirements(xmlNodePtr *node, test_requirements *reqs,
                 bool allow_sticky)
{
    te_errno rc;

    assert(node != NULL);
    assert(reqs != NULL);

    /* Get additional information about requirements to be tested */
    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("req")) == 0)
    {
        rc = alloc_and_get_requirement(*node, reqs, allow_sticky);
        if (rc != 0)
            return rc;
        (*node) = xmlNodeNext(*node);
    }

    return 0;
}

/* Parse track_conf attribute value */
static te_errno
parse_track_conf(const char *str, unsigned int *result)
{
    unsigned int parsed_val;
    char *str_copy = NULL;
    char *saveptr = NULL;
    char *token = NULL;
    te_errno rc = 0;

    if (str == NULL || *str == '\0')
    {
        *result = TESTER_TRACK_CONF_UNSPEC;
        return 0;
    }

    str_copy = strdup(str);
    if (str_copy == NULL)
    {
        ERROR("%s(): out of memory", __FUNCTION__);
        return TE_ENOMEM;
    }

    parsed_val = TESTER_TRACK_CONF_DEF;

    token = strtok_r(str_copy, "|", &saveptr);
    while (token != NULL)
    {
        if (strcmp(token, "yes") == 0 || strcmp(token, "barf") == 0)
        {
            /* Nothing to do */
        }
        else if (strcmp(token, "barf_nohistory") == 0 ||
                 strcmp(token, "yes_nohistory") == 0)
        {
            parsed_val &= ~TESTER_TRACK_CONF_ROLLBACK_HISTORY;
        }
        else if (strcmp(token, "no") == 0)
        {
            parsed_val &= ~TESTER_TRACK_CONF_ENABLED;
        }
        else if (strcmp(token, "silent") == 0)
        {
            parsed_val &= ~TESTER_TRACK_CONF_MARK_DIRTY;
        }
        else if (strcmp(token, "nohistory") == 0 ||
                 strcmp(token, "silent_nohistory") == 0)
        {

            parsed_val &= ~(TESTER_TRACK_CONF_ROLLBACK_HISTORY |
                            TESTER_TRACK_CONF_MARK_DIRTY);
        }
        else if (strcmp(token, "sync") == 0)
        {
            parsed_val |= TESTER_TRACK_CONF_SYNC;
        }
        else
        {
            ERROR("%s(): invalid name '%s' in 'track_conf' property",
                  __FUNCTION__, token);
            rc = TE_EINVAL;
            goto out;
        }

        token = strtok_r(NULL, "|", &saveptr);
    }

out:

    free(str_copy);

    if (rc == 0)
        *result = parsed_val;

    return rc;
}

/**
 * Get attributes common for all run items.
 *
 * @param node      XML node
 * @param attrs     Location for attributes or NULL
 *
 * @return Status code.
 */
static te_errno
get_test_attrs(xmlNodePtr node, test_attrs *attrs)
{
    te_errno        rc;
    unsigned int    timeout;
    xmlChar        *s;

    /* Main session of the test package is not direct run item */
    if (attrs == NULL)
    {
        return 0;
    }

    /* 'timeout' is optional */
    timeout = TESTER_TIMEOUT_DEF;
    rc = get_uint_prop(node, "timeout", &timeout);
    if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
        return rc;
    attrs->timeout.tv_sec = timeout;
    attrs->timeout.tv_usec = 0;

    /* 'track_conf' is optional, default value is 'yes' */
    attrs->track_conf = TESTER_TRACK_CONF_UNSPEC;
    s = xmlGetProp(node, CONST_CHAR2XML("track_conf"));
    if (s != NULL)
    {
        rc = parse_track_conf(XML2CHAR(s), &attrs->track_conf);
        if (rc != 0)
        {
            ERROR("Invalid value '%s' of 'track_conf' property",
                  XML2CHAR(s));
            xmlFree(s);
            return TE_RC(TE_TESTER, rc);
        }
        xmlFree(s);

        /*
         * Default value is TESTER_HANDDOWN_CHILDREN here because
         * it worked this way before fixing bug 10047 by default,
         * even though default was TESTER_HANDDOWN_DESCENDANTS.
         */
        rc = get_handdown_attr(node, "track_conf_handdown",
                               &attrs->track_conf_hd,
                               TESTER_HANDDOWN_CHILDREN);
        if (rc != 0)
            return rc;
    }

    return 0;
}


/**
 * Get script call description.
 *
 * @param node      XML node with script call description
 * @param cfg       Tester configuration context
 * @param script    Location for script call description
 *
 * @return Status code.
 */
static te_errno
get_script(xmlNodePtr node, tester_cfg *cfg, test_script *script)
{
    te_errno            rc;
    const test_info    *ti;

    /* 'name' is mandatory */
    script->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (script->name == NULL)
    {
        ERROR("'name' attribute is missing in script call description");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    /* Get run item attributes */
    rc = get_test_attrs(node, &script->attrs);
    if (rc != 0)
        return rc;

    node = xmlNodeChildren(node);

    /* Get optional 'objective' */
    if (node != NULL &&
        (rc = get_node_with_text_content(&node, "objective",
                                         &script->objective)) != 0)
    {
        if (rc != TE_ENOENT)
            return rc;
    }

    ti = find_test_info(cfg->cur_pkg->ti, script->name);
    if (ti != NULL)
    {
        if (ti->objective != NULL && script->objective == NULL)
        {
            script->objective = strdup(ti->objective);
            if (script->objective == NULL)
            {
                ERROR("strdup(%s) failed", ti->objective);
                return TE_RC(TE_TESTER, TE_ENOMEM);
            }
        }
        if (ti->page != NULL)
        {
            script->page = strdup(ti->page);
            if (script->page == NULL)
            {
                ERROR("strdup(%s) failed", ti->page);
                return TE_RC(TE_TESTER, TE_ENOMEM);
            }
        }
    }

    /* Get optional set of tested 'requirement's */
    rc = get_requirements(&node, &script->reqs, false);
    if (rc != 0)
    {
        ERROR("Failed to get requirements of the script '%s'",
              script->name);
        return rc;
    }
    /* Get optional 'execute' parameter */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("execute")) == 0)
    {
        script->execute = XML2CHAR_DUP(node->content);
        node = xmlNodeNext(node);
    }
    else
    {
        script->execute = name_to_path(cfg, script->name, false);
    }
    if (script->execute == NULL)
    {
        ERROR("Failed to create execution path to the test script '%s'",
              script->name);
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in script '%s' call description",
              XML2CHAR(node->name), script->name);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    VERB("Got script '%s'", script->name);

    return 0;
}

/**
 * Find value in the list by name.
 *
 * @param values        List of values
 * @param name          Name of the value to find
 *
 * @return Pointer to found value or NULL.
 */
static const test_entity_value *
find_value(const test_entity_values *values, const char *name)
{
    const test_entity_value *p;

    TAILQ_FOREACH(p, &values->head, links)
    {
        if (p->name != NULL && strcmp(p->name, name) == 0)
            return p;
    }
    return NULL;
}

/**
 * Allocate and get argument or variable value.
 *
 * @param node      Node with new run item
 * @param values    List of values
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_value(xmlNodePtr node, const test_session *session,
                    const test_value_type *type,
                    test_entity_values *values)
{
    test_entity_value  *p;
    char               *tmp;

    p = TE_ALLOC(sizeof(*p));

    TAILQ_INIT(&p->reqs);
    TAILQ_INSERT_TAIL(&values->head, p, links);

    /* 'name' is optional */
    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    /* 'type' is optional */
    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("type")));
    if (tmp != NULL)
    {
        /* Allow to override type specified for all values */
        p->type = tester_find_type(session, tmp);
        if (p->type == NULL)
        {
            ERROR("Type '%s' not found", tmp);
            free(tmp);
            return TE_RC(TE_TESTER, TE_ESRCH);
        }
        free(tmp);
    }
    else
    {
        /*
         * Type of the value is not specified, may be it is specified
         * for all values.
         */
        p->type = type;
    }
    VERB("%s(): New value '%s' of type '%s'", __FUNCTION__,
         p->name, p->type == NULL ? "" : p->type->name);
    /* 'ref' is optional */
    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("ref")));
    if (tmp != NULL)
    {
        /*
         * Reference to another value of this group is top priority
         */
        p->ref = find_value(values, tmp);
        if (p->ref == p)
        {
            INFO("Ignore self-reference of the value '%s'", p->name);
            p->ref = NULL;
        }
        /*
         * If type is specified, references to type values are the next
         * priority.
         */
        if (p->ref == NULL && p->type != NULL)
        {
            p->ref = find_value(&p->type->values, tmp);
        }
        if (p->ref == NULL)
        {
            INFO("Reference '%s' is considered as external", tmp);
            p->ext = tmp;
        }
    }
    /* 'reqs' is optional */
    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("reqs")));
    if (tmp != NULL)
    {
        char       *s = tmp;
        bool end;

        do {
            size_t              len = strcspn(s, ",");
            test_requirement   *req = TE_ALLOC(sizeof(*req));

            req->id = strndup(s, len);
            if (req->id == NULL)
            {
                ERROR("strndup() failed");
                free(tmp);
                return TE_RC(TE_TESTER, TE_ENOMEM);
            }
            TAILQ_INSERT_TAIL(&p->reqs, req, links);

            end = (s[len] == '\0');
            s += len + (end ? 0 : 1);
        } while (!end);
        free(tmp);
    }

    /* Simple text content is represented as 'text' elements */
    if (node->children != NULL)
    {
        if ((xmlStrcmp(node->children->name,
                       CONST_CHAR2XML("text")) != 0) ||
            (node->children->content == NULL))
        {
            ERROR("'value' content is empty or not 'text'");
            return TE_RC(TE_TESTER, TE_EINVAL);
        }
        if (node->children != node->last)
        {
            ERROR("Too many children in 'value' element");
            return TE_RC(TE_TESTER, TE_EINVAL);
        }
        p->plain = XML2CHAR_DUP(node->children->content);
        if (p->type != NULL)
        {
            const test_entity_value *tv =
                tester_type_check_plain_value(p->type, p->plain);

            VERB("%s(): Checked value '%s' by type '%s' -> %p",
                 __FUNCTION__, p->plain, p->type->name, tv);
            if (tv == NULL)
            {
                ERROR("Plain value '%s' does not conform to type '%s'",
                      p->plain, p->type->name);
                return TE_RC(TE_TESTER, TE_EINVAL);
            }
            if (p->ref == NULL)
            {
                p->ref = tv;
                free(p->plain);
                p->plain = NULL;
            }
        }
    }

    if ((p->ref != NULL) + (p->ext != NULL) + (p->plain != NULL) > 1)
    {
        ERROR("Too many sources of value: ref=%p ext=%s plain=%s",
              p->ref, (p->ext != NULL) ? p->ext : "(empty)",
              (p->plain != NULL) ? p->plain : "(empty)");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    else if ((p->plain == NULL) && (p->ref == NULL) &&
             (p->type == NULL) && (p->ext == NULL))
    {
        ERROR("There is no source of value");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    if ((p->plain != NULL) || (p->ref != NULL) || (p->ext != NULL))
    {
        values->num++;
    }
    else
    {
        assert(p->type != NULL);
        values->num += p->type->values.num;
    }

    VERB("%s(): Got value plain=%s ref=%p ext=%s type=%s reqs=%p",
         __FUNCTION__, p->plain, p->ref, p->ext,
         (p->type == NULL) ? "" : p->type->name, TAILQ_FIRST(&p->reqs));

    return 0;
}


/**
 * Allocate and get enum definition.
 *
 * @param node      Node with simple variable
 * @param list      List of session types
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_enum(xmlNodePtr node, const test_session *session,
                   test_value_types *list)
{
    te_errno            rc;
    test_value_type    *p;
    char               *tmp;

    p = TE_ALLOC(sizeof(*p));
    p->context = session;
    TAILQ_INIT(&p->values.head);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name is required for types");
        /*
         * Do not insert before tester_find_type() including indirect
         * from alloc_and_get_value(), but required for clean up.
         */
        SLIST_INSERT_HEAD(list, p, links);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    /* 'type' is optional */
    tmp = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("type")));
    if (tmp != NULL)
    {
        /* Allow to override type specified for all values */
        p->type = tester_find_type(session, tmp);
        if (p->type == NULL)
        {
            ERROR("Type '%s' not found", tmp);
            free(tmp);
            /*
             * Do not insert before tester_find_type() including indirect
             * from alloc_and_get_value(), but required for clean up.
             */
            SLIST_INSERT_HEAD(list, p, links);
            return TE_RC(TE_TESTER, TE_ESRCH);
        }
        free(tmp);
    }
    VERB("%s(): New enum '%s' of type '%s'", __FUNCTION__, p->name,
         p->type == NULL ? "" : p->type->name);

    node = xmlNodeChildren(node);
    while ((node != NULL) &&
           (xmlStrcmp(node->name, CONST_CHAR2XML("value")) == 0))
    {
        rc = alloc_and_get_value(node, session, p->type, &p->values);
        if (rc != 0)
        {
            ERROR("Processing of the type '%s' value failed: %r",
                  p->name, rc);
            /*
             * Do not insert before tester_find_type() including indirect
             * from alloc_and_get_value(), but required for clean up.
             */
            SLIST_INSERT_HEAD(list, p, links);
            return rc;
        }
        node = xmlNodeNext(node);
    }

    /*
     * Do not insert before tester_find_type() including indirect
     * from alloc_and_get_value(), but required for clean up.
     */
    SLIST_INSERT_HEAD(list, p, links);

    if (TAILQ_EMPTY(&p->values.head))
    {
        ERROR("Enum '%s' is empty", p->name);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in enum '%s'",
              XML2CHAR(node->name), p->name);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    return 0;
}


/**
 * Allocate and get session variable or run item argument.
 *
 * @param node          Node with simple variable
 * @param is_var        Is it variable?
 * @param check_unique  Whiether to check that name of variable is unique
 * @param list          List of session variables or run item arguments
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_var_arg(xmlNodePtr node, bool is_var, bool check_unique,
                      const test_session *session, test_vars_args *list)
{
    xmlNodePtr          root = node;
    te_errno            rc;
    test_var_arg       *p;
    test_var_arg       *arg;
    char               *ref;
    char               *value;
    char               *global = NULL;
    char               *s;

    ENTRY("session=%p", session);
    p = TE_ALLOC(sizeof(*p));
    p->handdown = true;
    p->variable = is_var;
    p->global = false;
    TAILQ_INIT(&p->values.head);
    TAILQ_INSERT_TAIL(list, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name is required for simple variable");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    /* Check that there is no such name in parent session arguments */
    if (session != NULL && check_unique)
    {
        TAILQ_FOREACH(arg, &session->vars, links)
        {
            if ((strcmp(arg->name, p->name) == 0) && !is_var && !arg->variable)
            {
                ERROR("Ambiguous %s argument specification", p->name);
                return TE_RC(TE_TESTER, TE_EINVAL);
            }
        }
    }

    /* 'type' is optional */
    s = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("type")));
    if (s != NULL)
    {
        /* Allow to override type specified for all values */
        p->type = tester_find_type(session, s);
        if (p->type == NULL)
        {
            ERROR("Type '%s' not found", s);
            free(s);
            return TE_RC(TE_TESTER, TE_ESRCH);
        }
        free(s);
    }

    node = xmlNodeChildren(node);
    while ((node != NULL) &&
           (xmlStrcmp(node->name, CONST_CHAR2XML("value")) == 0))
    {
        rc = alloc_and_get_value(node, session, p->type, &p->values);
        if (rc != 0)
        {
            ERROR("Processing of the %s '%s' value failed: %r",
                  is_var ? "variable" : "argument", p->name, rc);
            return rc;
        }
        node = xmlNodeNext(node);
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in argument",
              XML2CHAR(node->name));
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    node = root;

    /* 'list' is optional */
    p->list = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("list")));

    /* It must be done when values have already been processed */
    /* 'preferred' is optional */
    s = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("preferred")));
    if (s != NULL)
    {
        if (p->list == NULL)
        {
            WARN("'preferred' attribute is useless without 'list'");
        }

        p->preferred = find_value(&p->values, s);
        if (p->preferred == NULL)
        {
            ERROR("Value with 'name'='%s' not found to be preferred", s);
            xmlFree(s);
            return TE_RC(TE_TESTER, TE_EINVAL);
        }
        xmlFree(s);
    }

    /* 'global' is optional, possible only for variables */
    if (is_var)
        global = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("global")));

    /* 'ref' is optional */
    ref = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("ref")));
    /* 'value' is optional */
    value = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("value")));

    if (((ref != NULL) + (value != NULL) +
         !TAILQ_EMPTY(&p->values.head)) > 1)
    {
        ERROR("Too many sources of %s '%s' value: ref=%s value=%s "
               "values=%s", (is_var) ? "variable" : "argument", p->name,
               (ref) ? : "(empty)", (value) ? : "(empty)",
               TAILQ_EMPTY(&p->values.head) ? "(empty)" : "(not empty)");
        free(ref);
        free(value);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    if (TAILQ_EMPTY(&p->values.head) && p->type == NULL)
    {
        test_entity_value *v;

        v = TE_ALLOC(sizeof(*v));
        TAILQ_INIT(&v->reqs);
        TAILQ_INSERT_TAIL(&p->values.head, v, links);
        p->values.num++;

        if (value != NULL)
        {
            v->plain = value;
        }
        else
        {
            v->ext = (ref != NULL) ? ref : strdup(p->name);
            if (v->ext == NULL)
            {
                ERROR("strdup(%s) failed", p->name);
                return TE_RC(TE_TESTER, TE_ENOMEM);
            }
        }
        v->name = strdup(p->name);
        /* ignore error */
    }

    if (is_var && global && strcmp(global, "true") == 0)
    {
        char env_name[128];
        int rc;
        char *val = value;

        if (val == NULL)
        {
            test_entity_value *v = TAILQ_FIRST(&p->values.head);

            val = v->plain;
            v->global = true;
            te_asprintf(&v->name, "VAR.%s", p->name);
            te_var_name2env(v->name, env_name, sizeof(env_name));
        }
        p->global = true;

        VERB("%s: setenv %s=%s", __FUNCTION__, env_name, val);
        rc = setenv(env_name, val, 1);
        if (rc < 0)
        {
            ERROR("Failed to set environment variable %s", env_name);
            return TE_RC(TE_TESTER, TE_ENOSPC);
        }
        VERB("%s: getenv->%s", __FUNCTION__, getenv(env_name));
    }

    return 0;
}

static te_errno
vars_process(xmlNodePtr *node, test_session *session,
             bool children, int *parse_break)
{
    te_errno rc = 0;
    xmlNodePtr local_node = *node;

    assert(parse_break != NULL);
    ENTRY("session=%p", session);

    if (children)
        *node = xmlNodeChildren(*node);

    *parse_break = 0;
    while (*node != NULL)
    {
        VERB("%s: node->name=%s", __FUNCTION__, (*node)->name);
        if (xmlStrcmp((*node)->name, CONST_CHAR2XML("arg")) == 0)
            rc = alloc_and_get_var_arg(*node, false, false, session,
                                       &session->vars);
        else if (xmlStrcmp((*node)->name, CONST_CHAR2XML("var")) == 0)
            rc = alloc_and_get_var_arg(*node, true, false, session,
                                       &session->vars);
        else if (xmlStrcmp((*node)->name, CONST_CHAR2XML("enum")) == 0)
            rc = alloc_and_get_enum(*node, session, &session->types);
        else if (xmlStrcmp((*node)->name, CONST_CHAR2XML("include")) == 0)
        {
            VERB("%s: includes processing: %s", __FUNCTION__,
                 XML2CHAR(xmlGetProp(*node, CONST_CHAR2XML("href"))));
            rc = 0;
        }
        else if (xmlStrcmp((*node)->name, CONST_CHAR2XML("vars")) == 0)
        {
            VERB("%s: vars list", __FUNCTION__);
            rc = vars_process(node, session, true, parse_break);
            *node = local_node;
        }
        else
        {
            VERB("%s: breaking, node->name=%s", __FUNCTION__,
                 (*node)->name);
            *parse_break = 1;
            rc = 0;
            break;
        }
        if (rc != 0)
        {
            ERROR("%s: something failed: %r", __FUNCTION__, rc);
            break;
        }
        if (*parse_break == 1)
            break;

        (*node) = xmlNodeNext(*node);
        local_node = *node;
    }

    *node = local_node;

    return rc;
}

/**
 * Get command monitor property value and expand environment
 * variables in it
 *
 * @param node        XML node with a property
 * @param[out] value  Where to store property value
 * @param name        Property name
 *
 * @return 0 on success, error code on failure
 */
static te_errno
cmd_monitor_get_prop(xmlNodePtr *node, char **value, char *name)
{
    char *expanded = NULL;
    int   rc = 0;

    if (*value != NULL)
    {
        ERROR("%s(): duplicated <%s> encountered",
             __FUNCTION__, name);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    if ((rc = get_node_with_text_content(
                                    node, name,
                                    value)) != 0)
        return rc;

    if (te_expand_env_vars(*value,
                           NULL, &expanded) == 0)
    {
        free(*value);
        *value = expanded;
    }
    else
    {
        ERROR("%s(): failed to expand environment "
              "variables in '%s'",
             __FUNCTION__, *value);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    return 0;
}

/**
 * Get command monitor descriptions from <command_monitor> nodes
 *
 * @param node    The first <command_monitor> node
 * @param ritem   run_item where to store command monitor descriptions
 *
 * @return 0 on success, error code on failure
 */
static te_errno
monitors_process(xmlNodePtr *node, run_item *ritem)
{
    te_errno rc = 0;
    xmlNodePtr p;
    cmd_monitor_descr *monitor;

    while (*node != NULL)
    {
        char *time_to_wait = NULL;
        char *run_monitor = NULL;

        if (xmlStrcmp((*node)->name,
                      CONST_CHAR2XML("command_monitor")) != 0)
            break;

        monitor = calloc(1, sizeof(*monitor));
        p = xmlNodeChildren(*node);
        while (p != NULL)
        {
            if (xmlStrcmp(p->name,
                          CONST_CHAR2XML("command")) == 0)
            {
                if ((rc = cmd_monitor_get_prop(&p, &monitor->command,
                                               "command")) != 0)
                {
                    free_cmd_monitor(monitor);
                    return rc;
                }
            }
            if (xmlStrcmp(p->name,
                          CONST_CHAR2XML("ta")) == 0)
            {
                if ((rc = cmd_monitor_get_prop(&p, &monitor->ta,
                                               "ta")) != 0)
                {
                    free_cmd_monitor(monitor);
                    return rc;
                }
            }
            else if (xmlStrcmp(p->name,
                               CONST_CHAR2XML("time_to_wait")) == 0)
            {
                if ((rc = cmd_monitor_get_prop(&p, &time_to_wait,
                                               "time_to_wait")) != 0)
                {
                    free_cmd_monitor(monitor);
                    return rc;
                }
                monitor->time_to_wait = atoi(time_to_wait);
                free(time_to_wait);
            }
            else if (xmlStrcmp(p->name,
                               CONST_CHAR2XML("run_monitor")) == 0)
            {
                if ((rc = cmd_monitor_get_prop(&p, &run_monitor,
                                               "run_monitor")) != 0)
                {
                    free_cmd_monitor(monitor);
                    return rc;
                }
                if (strcasecmp(run_monitor, "yes") == 0)
                    monitor->run_monitor = true;
                else if (strcasecmp(run_monitor, "no") == 0)
                    monitor->run_monitor = false;
                else
                    monitor->run_monitor =
                        (atoi(run_monitor) == 0 ? false : true);
                free(run_monitor);
            }
            else
            {
                ERROR("%s(): unexpected node name '%s' encountered",
                      __FUNCTION__, XML2CHAR(p->name));
                free_cmd_monitor(monitor);
                return TE_RC(TE_TESTER, TE_EINVAL);
            }
        }

        tester_monitor_id++;
        snprintf(monitor->name, TESTER_CMD_MONITOR_NAME_LEN,
                 "tester_monitor%d", tester_monitor_id);

        if (monitor->ta == NULL)
        {
            char *ta = getenv("TE_IUT_TA_NAME");

            if (ta != NULL)
                monitor->ta = strdup(ta);
        }

        TAILQ_INSERT_TAIL(&ritem->cmd_monitors, monitor, links);
        (*node) = xmlNodeNext(*node);
    }

    return 0;
}

/**
 * Get session description.
 *
 * @param node      XML node with session description
 * @param cfg       Tester configuration context
 * @param parent    Parent test session or NULL
 * @param session   Location for session description
 *
 * @return Status code.
 */
static te_errno
get_session(xmlNodePtr node, tester_cfg *cfg, const test_session *parent,
            test_session *session, run_item *ritem)
{
    te_errno rc;
    int parse_break;

    TAILQ_INIT(&session->reqs);

    ENTRY("session=%p", session);
    session->parent = parent;

    /* Get run item attributes */
    rc = get_test_attrs(node, &session->attrs);
    if (rc != 0)
        return rc;

    /* 'simultaneous' is optional, default value is false */
    session->simultaneous = false;
    rc = get_bool_prop(node, "simultaneous", &session->simultaneous);
    if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
        return rc;

    node = xmlNodeChildren(node);

    /* Get optional 'objective' */
    if (node != NULL &&
        (rc = get_node_with_text_content(&node, "objective",
                                         &session->objective)) != 0)
    {
        if (rc != TE_ENOENT)
            return rc;
    }

    monitors_process(&node, ritem);

    /* Get information about variables */
    while (node != NULL)
    {
        if ((rc = vars_process(&node, session, false, &parse_break)) == 0 &&
                 parse_break == 1)
            break;

        if (rc != 0)
        {
            VERB("%s: something failed: %r", __FUNCTION__, rc);
            return rc;
        }
        node = xmlNodeNext(node);
    }

    /* Information about requirements is optional */
    rc = get_requirements(&node, &session->reqs, true);
    if (rc != 0)
    {
        ERROR("Failed to get information about session requirements");
        return rc;
    }

    /* Get 'exception' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("exception")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_SERVICE |
                                    TESTER_RUN_ITEM_INHERITABLE,
                                    RI_ROLE_EXCEPTION,
                                    session, NULL, &session->exception);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
    /* Get 'keepalive' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("keepalive")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_SERVICE |
                                    TESTER_RUN_ITEM_INHERITABLE,
                                    RI_ROLE_KEEPALIVE,
                                    session, NULL, &session->keepalive);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
    /* Get 'prologue' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("prologue")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_SERVICE,
                                    RI_ROLE_PROLOGUE,
                                    session, NULL, &session->prologue);
        if (rc != 0)
            return rc;

        /* By default, configuration is not tracked after prologue */
        if (test_get_attrs(session->prologue)->track_conf ==
            TESTER_TRACK_CONF_UNSPEC)
        {
            test_get_attrs(session->prologue)->track_conf =
                TESTER_TRACK_CONF_SPECIFIED;
        }

        node = xmlNodeNext(node);
    }
    /* Get 'epilogue' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("epilogue")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_SERVICE,
                                    RI_ROLE_EPILOGUE,
                                    session, NULL, &session->epilogue);
        if (rc != 0)
            return rc;

        /* By default, configuration is not tracked after epilogue */
        if (test_get_attrs(session->epilogue)->track_conf ==
            TESTER_TRACK_CONF_UNSPEC)
        {
            test_get_attrs(session->epilogue)->track_conf =
                TESTER_TRACK_CONF_SPECIFIED;
        }

        node = xmlNodeNext(node);
    }
    /* Get 'run' items */
    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("run")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg, TESTER_RUN_ITEM_INHERITABLE,
                                    RI_ROLE_NORMAL,
                                    session, &session->run_items, NULL);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in session: line=%u",
              XML2CHAR(node->name), node->line);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    return 0;
}

/**
 * Get package as run item description.
 *
 * @param node      XML node with package as run item description
 * @param cfg       Tester configuration context
 * @param session   Test session the package belongs to or NULL
 * @param pkg       Location for package description
 *
 * @return Status code.
 */
static te_errno
get_package(xmlNodePtr node, tester_cfg *cfg, const test_session *session,
            test_package **pkg, run_item *ritem)
{
    te_errno        rc;
    test_package   *p;

    p = TE_ALLOC(sizeof(*p));
    TAILQ_INIT(&p->authors);
    TAILQ_INIT(&p->reqs);

    TAILQ_INIT(&p->session.vars);
    TAILQ_INIT(&p->session.run_items);

    *pkg = p;

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name of the Test Package to run is unspecified");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    /* Parse test package also getting optional 'src' attribute */
    rc = parse_test_package(cfg, session, p,
                            XML2CHAR(xmlGetProp(node,
                                                CONST_CHAR2XML("src"))),
                            ritem);
    if (rc != 0)
    {
        ERROR("Parsing/preprocessing of the package '%s' failed",
              p->name);
    }

    return rc;
}

/**
 * Get run item from configuration file tree.
 *
 * @param node      Node with new run item
 * @param cfg       Tester configuration context
 * @param opts      Run item options (types)
 * @param session   Session the run item belongs to or NULL
 * @param runs      List of run items or @c NULL
 * @param p_run     Location for pointer to allocated run item or @c NULL
 *
 * One of @a runs or @a p_run arguments must be @c NULL and another
 * not @c NULL.
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_run_item(xmlNodePtr node, tester_cfg *cfg, unsigned int opts,
                       run_item_role role,
                       const test_session *session,
                       run_items *runs, run_item **p_run)
{
    run_item    *p;
    te_errno     rc;

    assert((runs == NULL) != (p_run == NULL));
    p = TE_ALLOC(sizeof(*p));
    TAILQ_INIT(&p->args);
    SLIST_INIT(&p->lists);
    TAILQ_INIT(&p->cmd_monitors);
    p->context = session;
    p->iterate = 1;
    p->role = role;
    p->dial_coef = -1;

    /* Just for current clean up in the case of failure */
    p->type = RUN_ITEM_NONE;
    if (runs != NULL)
    {
        TAILQ_INSERT_TAIL(runs, p, links);
    }
    else
    {
        *p_run = p;
    }

    if (~opts & TESTER_RUN_ITEM_SERVICE)
    {
        /* 'name' is optional */
        p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
        VERB("Preprocessing 'run' item '%s'", p->name ? : "(noname)");

        if (p->name != NULL)
        {
            const test_info *ti =
                find_test_info(cfg->cur_pkg->ti, p->name);

            if (ti != NULL)
            {
                p->objective =
                    ti->objective != NULL ? strdup(ti->objective) : NULL;
                p->page = ti->page != NULL ? strdup(ti->page) : NULL;
            }
        }

        /* 'iterate' is optional */
        rc = get_uint_prop(node, "iterate", &p->iterate);
        if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
            return rc;

        /* 'dial_coef' is optional */
        rc = get_double_prop(node, "dial_coef", &p->dial_coef);
        if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
            return rc;
    }

    if (opts & TESTER_RUN_ITEM_INHERITABLE)
    {
        rc = get_handdown_attr(node, "handdown", &p->handdown,
                               TESTER_HANDDOWN_DEF);
        if (rc != 0)
            return rc;
    }

    /* 'loglevel' is optional */
    p->loglevel = 0;
    rc = get_uint_prop(node, "loglevel", &p->loglevel);
    if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
        return rc;

    node = xmlNodeChildren(node);
    if (node == NULL)
    {
        ERROR("Empty 'run' item");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    if (monitors_process(&node, p) != 0)
    {
        ERROR("Failed to process <command_monitor> nodes");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    if (xmlStrcmp(node->name, CONST_CHAR2XML("script")) == 0)
    {
        p->type = RUN_ITEM_SCRIPT;
        TAILQ_INIT(&p->u.script.reqs);
        rc = get_script(node, cfg, &p->u.script);
        if (rc != 0)
            return rc;
    }
    else if (xmlStrcmp(node->name, CONST_CHAR2XML("session")) == 0)
    {
        p->type = RUN_ITEM_SESSION;
        TAILQ_INIT(&p->u.session.vars);
        TAILQ_INIT(&p->u.session.run_items);
        rc = get_session(node, cfg, session, &p->u.session, p);
        if (rc != 0)
            return rc;
    }
    else if ((~opts & TESTER_RUN_ITEM_SERVICE) &&
             xmlStrcmp(node->name, CONST_CHAR2XML("package")) == 0)
    {
        p->type = RUN_ITEM_PACKAGE;
        rc = get_package(node, cfg, session, &p->u.package, p);
        if (rc != 0)
            return rc;
    }
    else
    {
        ERROR("The first element '%s' in run item is incorrect",
              XML2CHAR(node->name));
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    node = xmlNodeNext(node);

    /* Get information about arguments */
    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("arg")) == 0)
    {
        rc = alloc_and_get_var_arg(node, false, true, session, &p->args);
        if (rc != 0)
        {
            ERROR("Processing of the run item '%s' arguments "
                  "failed: %r", run_item_name(p), rc);
            return rc;
        }
        node = xmlNodeNext(node);
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in run item",
              XML2CHAR(node->name));
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    return 0;
}


/**
 * Preprocess parsed Test Package file.
 *
 * @param root      Root element of the Test Package file
 * @param cfg       Tester configuration context
 * @param session   Test Session the package belongs to or NULL
 * @param pkg       Test Package structure to be filled in
 *
 * @return Status code.
 */
static te_errno
get_test_package(xmlNodePtr root, tester_cfg *cfg,
                 const test_session *session, test_package *pkg,
                 run_item *ritem)
{
    xmlNodePtr  node;
    xmlChar    *s;
    te_errno    rc;

    assert(cfg != NULL);
    assert(pkg != NULL);

    if (root == NULL)
    {
        VERB("Empty configuration file is provided");
        return 0;
    }
#ifndef XML_DOC_ASSUME_VALID
    if (xmlStrcmp(root->name, CONST_CHAR2XML("package")) != 0)
    {
        ERROR("Incorrect root node '%s' in the Test Package file",
              root->name);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    if (root->next != NULL)
    {
        ERROR("'package' element must be singleton in Test Package file");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif

    /* Check version of the Tester configuration file */
    s = xmlGetProp(root, CONST_CHAR2XML("version"));
#ifndef XML_DOC_ASSUME_VALID
    if (s == NULL)
    {
        ERROR("'version' of the Test Package file is not specified");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif
    if (xmlStrcmp(s, CONST_CHAR2XML("1.0")) != 0)
    {
        ERROR("Unsupported version %s of the Test Package file",
              XML2CHAR(s));
        xmlFree(s);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    xmlFree(s);

    node = root->children;

    /* Get mandatory description */
    if (node != NULL &&
        (rc = get_node_with_text_content(&node, "description",
                                         &pkg->objective)) != 0)
    {
        ERROR("Failed to get mandatory description of the test "
              "package '%s': %r", pkg->name, rc);
        return rc;
    }
#ifndef XML_DOC_ASSUME_VALID
    if (pkg->objective == NULL)
    {
        ERROR("'description' is mandatory for any Test Package");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif

    /* Information about author is optional */
    rc = get_persons_info(&node, "author", &pkg->authors);
    if (rc != 0)
    {
        ERROR("Failed to get information about Test Package author(s)");
        return rc;
    }

    /* Information about requirements is optional */
    rc = get_requirements(&node, &pkg->reqs, true);
    if (rc != 0)
    {
        ERROR("Failed to get information about Test Package requirements");
        return rc;
    }

    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("session")) == 0)
    {
        rc = get_session(node, cfg, session, &pkg->session, ritem);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
#ifndef XML_DOC_ASSUME_VALID
    else
    {
        ERROR("'session' is mandatory in Test Package description");
    }

    /* No more */
    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in Test Package file",
              XML2CHAR(node->name));
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif

    return 0;
}


/**
 * Get set of target requirements.
 *
 * @param node      XML node location
 * @param targets   Location for pointer to resulting expression
 *
 * @return Status code.
 */
static te_errno
get_target_reqs(xmlNodePtr *node, logic_expr **targets)
{
    te_errno rc;

    assert(node != NULL);
    assert(targets != NULL);

    /* Get additional information about requirements to be tested */
    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("req")) == 0)
    {
        char *str = XML2CHAR(xmlGetProp(*node, CONST_CHAR2XML("expr")));

        if (str == NULL)
        {
            ERROR("Expression of the target requirement is not "
                  "specified");
            return TE_RC(TE_TESTER, TE_EINVAL);
        }
        rc = tester_new_target_reqs(targets, str);
        free(str);
        if (rc != 0)
            return rc;
        (*node) = xmlNodeNext(*node);
    }

    return 0;
}


/**
 * Preprocess parsed Tester configuration file.
 *
 * @param root          Root element of the Tester configuration file
 * @param cfg           Configuration structure to be filled in
 * @param build         Build test suite
 * @param verbose       Be verbose in the case of build failure
 *
 * @return Status code.
 */
static te_errno
get_tester_config(xmlNodePtr root, tester_cfg *cfg,
                  bool build, bool verbose)
{
    xmlNodePtr  node;
    xmlChar    *s;
    te_errno    rc;

    if (root == NULL)
    {
        VERB("Empty configuration file is provided");
        return 0;
    }
#ifndef XML_DOC_ASSUME_VALID
    if (xmlStrcmp(root->name, CONST_CHAR2XML("tester_cfg")) != 0)
    {
        ERROR("Incorrect root node '%s' in the configuration file",
              root->name);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    if (root->next != NULL)
    {
        ERROR("'tester_cfg' element must be singleton");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif

    /* Check version of the Tester configuration file */
    s = xmlGetProp(root, CONST_CHAR2XML("version"));
#ifndef XML_DOC_ASSUME_VALID
    if (s == NULL)
    {
        ERROR("'version' of the Tester configuration file is not "
              "specified");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif
    if (xmlStrcmp(s, CONST_CHAR2XML("1.0")) != 0)
    {
        ERROR("Unsupported version %s of the Tester configuration file",
              XML2CHAR(s));
        xmlFree(s);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    xmlFree(s);

    node = root->children;

    /* Get maintainers */
    rc = get_persons_info(&node, "maintainer", &cfg->maintainers);
    if (rc != 0)
    {
        ERROR("Failed to get information about Tester configuration "
              "maintainer(s)");
        return rc;
    }
    if (TAILQ_EMPTY(&cfg->maintainers))
    {
        ERROR("The first element of the Tester configuration must be "
              "'maintainer' (not %s)",
              (node != NULL) ? XML2CHAR(node->name) : "(NULL)");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    /* Get optional description */
    if (node != NULL &&
        (rc = get_node_with_text_content(&node, "description",
                                         &cfg->descr)) != 0)
    {
        if (rc != TE_ENOENT)
            return rc;
    }

    /* Get optional information about suites */
    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("suite")) == 0)
    {
        rc = alloc_and_get_test_suite_info(node, &cfg->suites, build,
                                           verbose);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    /* Get optional information about requirements to be tested */
    rc = get_target_reqs(&node, &cfg->targets);
    if (rc != 0)
    {
        ERROR("Failed to get requirements of the Tester configuration");
        return rc;
    }

    /* get optional information about options */
    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("option")) == 0)
    {
        rc = alloc_and_get_option(node, &cfg->options);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    /* Get information about run items */
    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("run")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg, 0, RI_ROLE_NORMAL,
                                    NULL, &cfg->runs, NULL);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
#ifndef XML_DOC_ASSUME_VALID
    if (TAILQ_EMPTY(&cfg->runs))
    {
        ERROR("No 'run' items are specified in the configuration file");
        if (node == NULL)
            return TE_RC(TE_TESTER, TE_EINVAL);
    }

    /* No more */
    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in Tester configuration file",
              XML2CHAR(node->name));
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif

    return 0;
}


/**
 * Allocate and get information about test.
 *
 * @param node      XML node
 * @param ti        List with information about tests
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_test_info(xmlNodePtr node, tests_info *ti)
{
    test_info  *p;

    p = TE_ALLOC(sizeof(*p));
    TAILQ_INSERT_TAIL(ti, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Missing name of the test in info");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    /* Page is optional */
    p->page = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("page")));

    node = xmlNodeChildren(node);

    if ((node == NULL) ||
        (node->children == NULL) ||
        (xmlStrcmp(node->children->name, CONST_CHAR2XML("text")) != 0) ||
        (node->children->content == NULL))
    {
        ERROR("Missing objective of the test '%s'", p->name);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    if (node->children != node->last)
    {
        ERROR("Too many children in 'objective' element");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    p->objective = XML2CHAR_DUP(node->children->content);
    if (p->objective == NULL)
    {
        ERROR("Failed to duplicate string");
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }

    return 0;
}

/**
 * Get information about tests.
 *
 * @param node      XML document root
 * @param ti        List with information about tests
 *
 * @return Status code.
 */
static te_errno
get_tests_info(xmlNodePtr node, tests_info *ti)
{
    te_errno rc;

    if (node == NULL)
    {
        VERB("Empty configuration file is provided");
        return 0;
    }
    if (xmlStrcmp(node->name, CONST_CHAR2XML("tests-info")) != 0)
    {
        ERROR("Incorrect root node '%s' in the configuration file",
              node->name);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    if (node->next != NULL)
    {
        ERROR("'tests-info' element must be singleton");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    node = xmlNodeChildren(node);

    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("test")) == 0)
    {
        rc = alloc_and_get_test_info(node, ti);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    /* No more */
    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in Tests Info file",
              XML2CHAR(node->name));
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    return 0;
}


/**
 * Find information about a test.
 *
 * @param ti        List with information about tests
 * @param name      Name of the test to find
 */
static const test_info *
find_test_info(const tests_info *ti, const char *name)
{
    const test_info *p;

    for (p = (ti != NULL) ? TAILQ_FIRST(ti) : NULL;
         p != NULL;
         p = TAILQ_NEXT(p, links))
    {
        if (strcmp(p->name, name) == 0)
            return p;
    }
    return NULL;
}

/**
 * Free information about tests.
 *
 * @param ti        List with information about tests
 */
static void
tests_info_free(tests_info *ti)
{
    test_info *p;

    while ((p = TAILQ_FIRST(ti)) != NULL)
    {
        TAILQ_REMOVE(ti, p, links);
        free(p->name);
        free(p->page);
        free(p->objective);
        free(p);
    }
}


/**
 * Parse and preprocess Test Package description file.
 *
 * @param cfg       Tester configuration context
 * @param session   Test session the package belongs to or NULL
 * @param pkg       Test Package description structure to be filled in
 *
 * @return Status code.
 */
static te_errno
parse_test_package(tester_cfg *cfg, const test_session *session,
                   test_package *pkg, char const *src, run_item *ritem)
{
    xmlParserCtxtPtr    parser = NULL;
    xmlDocPtr           doc = NULL;
    test_package       *cur_pkg_save = NULL;
    te_errno            rc;

    char               *ti_path = NULL;
    struct stat         st_buf;
    xmlDocPtr           ti_doc = NULL;
    tests_info          ti;


    TAILQ_INIT(&ti);

    if ((pkg->path = name_to_path(cfg,
                                src != NULL ? src : pkg->name,
                                src != NULL ? false : true)) == NULL)
    {
        ERROR("Failed to make path to Test Package file by name and "
              "context");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    cur_pkg_save = cfg->cur_pkg;
    cfg->cur_pkg = pkg;

    ti_path = name_to_path(cfg, "tests-info.xml", false);
    if (ti_path == NULL)
    {
        ERROR("Failed to make path to Test Package file by name and "
              "context");
        rc = TE_RC(TE_TESTER, TE_EINVAL);
        goto cleanup;
    }

    parser = xmlNewParserCtxt();
    if (parser == NULL)
    {
        ERROR("xmlNewParserCtxt() failed");
        rc = TE_RC(TE_TESTER, TE_ENOMEM);
        goto cleanup;
    }

    if ((doc = xmlCtxtReadFile(parser, pkg->path, NULL,
                               XML_PARSE_NOBLANKS |
                               XML_PARSE_XINCLUDE |
                               XML_PARSE_NONET)) == NULL)
    {
#if HAVE_XMLERROR
        xmlError   *err = xmlCtxtGetLastError(parser);

        ERROR("Error occurred during parsing Test Package file:\n"
              "    %s:%d\n    %s", pkg->path, err->line, err->message);
#else
        ERROR("Error occurred during parsing Test Package file:\n"
              "%s", pkg->path);
#endif
        rc = TE_RC(TE_TESTER, TE_EINVAL);
        goto cleanup;
    }

    (void)xmlXIncludeProcess(doc);

    if (stat(ti_path, &st_buf) == 0)
    {
        if ((ti_doc = xmlCtxtReadFile(parser, ti_path, NULL,
                                      XML_PARSE_NOBLANKS |
                                      XML_PARSE_XINCLUDE |
                                      XML_PARSE_NONET)) == NULL)
        {
#if HAVE_XMLERROR
            xmlError   *err = xmlCtxtGetLastError(parser);

            ERROR("Error occurred during parsing Tests Info file:\n"
                  "    %s:%d\n    %s", pkg->path, err->line, err->message);
#else
            ERROR("Error occurred during parsing Tests Info file:\n"
                  "%s", pkg->path);
#endif
            rc = TE_RC(TE_TESTER, TE_EINVAL);
            goto cleanup;
        }

        rc = get_tests_info(xmlDocGetRootElement(ti_doc), &ti);
        if (rc != 0)
        {
            ERROR("Failed to get information about tests");
            goto cleanup;
        }
        pkg->ti = &ti;
    }

    rc = get_test_package(xmlDocGetRootElement(doc), cfg, session,
                          pkg, ritem);
    if (rc != 0)
    {
        ERROR("Preprocessing of Test Package '%s' from file '%s' failed",
              pkg->name, pkg->path);
    }
    else
    {
        INFO("Test Package '%s' from file '%s' preprocessed successfully",
             pkg->name, pkg->path);
    }

cleanup:
    pkg->ti = NULL;
    cfg->cur_pkg = cur_pkg_save;
    free(ti_path);
    tests_info_free(&ti);
    xmlFreeDoc(ti_doc);
    xmlFreeDoc(doc);
    xmlFreeParserCtxt(parser);
    xmlCleanupParser();

    return rc;
}


/**
 * Parse Tester configuration file.
 *
 * @param cfg           Tester configuration with not parsed file
 * @param build         Build test suites
 * @param verbose       Be verbose in the case of build failure
 *
 * @return Status code.
 */
static te_errno
tester_parse_config(tester_cfg *cfg, bool build, bool verbose)
{
    xmlParserCtxtPtr    parser;
    xmlDocPtr           doc;
    te_errno            rc;

    if (cfg->filename == NULL)
    {
        ERROR("Invalid configuration file name");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    parser = xmlNewParserCtxt();
    if (parser == NULL)
    {
        ERROR("xmlNewParserCtxt() failed");
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
    if ((doc = xmlCtxtReadFile(parser, cfg->filename, NULL,
                               XML_PARSE_NOBLANKS |
                               XML_PARSE_XINCLUDE |
                               XML_PARSE_NONET)) == NULL)
    {
#if HAVE_XMLERROR
        xmlError   *err = xmlCtxtGetLastError(parser);

        ERROR("Error occurred during parsing configuration file:\n"
              "    %s:%d\n    %s", cfg->filename, err->line, err->message);
#else
        ERROR("Error occurred during parsing configuration file:\n"
              "%s", cfg->filename);
#endif
        xmlFreeParserCtxt(parser);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    rc = get_tester_config(xmlDocGetRootElement(doc), cfg, build, verbose);
    if (rc != 0)
    {
        ERROR("Preprocessing of Tester configuration file '%s' failed",
              cfg->filename);
    }
    else
    {
        INFO("Tester configuration file '%s' preprocessed successfully",
             cfg->filename);
    }

    xmlFreeDoc(doc);
    xmlFreeParserCtxt(parser);
    xmlCleanupParser();

    return rc;
}

/* See the description in tester_conf.h */
te_errno
tester_parse_configs(tester_cfgs *cfgs, bool build, bool verbose)
{
    te_errno    rc;
    tester_cfg *cfg;

    TAILQ_FOREACH(cfg, &cfgs->head, links)
    {
        rc = tester_parse_config(cfg, build, verbose);
        if (rc != 0)
            return rc;
    }
    return 0;
}


/**
 * Free information about person.
 *
 * @param p         Information about person to be freed.
 */
static void
person_info_free(person_info *p)
{
    free(p->name);
    free(p->mailto);
    free(p);
}

/**
 * Free list of information about persons.
 *
 * @param persons   List of persons to be freed
 */
static void
persons_info_free(persons_info *persons)
{
    person_info *p;

    while ((p = TAILQ_FIRST(persons)) != NULL)
    {
        TAILQ_REMOVE(persons, p, links);
        person_info_free(p);
    }
}


/**
 * Free test script.
 *
 * @param p         Test script to be freed
 */
static void
test_script_free(test_script *p)
{
    free(p->name);
    free(p->objective);
    free(p->page);
    free(p->execute);
    test_requirements_free(&p->reqs);
}


/**
 * Free variable/argument value.
 *
 * @param p     Value to be freed
 */
static void
test_var_arg_value_free(test_entity_value *p)
{
    free(p->name);
    free(p->ext);
    free(p->plain);
    test_requirements_free(&p->reqs);
    free(p);
}

/**
 * Free list of variable/argument values.
 *
 * @param values    List of variable/argument values to be freed
 */
static void
test_var_arg_values_free(test_entity_values *values)
{
    test_entity_value *p;

    while ((p = TAILQ_FIRST(&values->head)) != NULL)
    {
        TAILQ_REMOVE(&values->head, p, links);
        test_var_arg_value_free(p);
    }
}

/**
 * Free value type.
 *
 * @param p         Value type to be freed.
 */
static void
test_value_type_free(test_value_type *p)
{
    free(p->name);
    test_var_arg_values_free(&p->values);
    free(p);
}

/**
 * Free list of session variables.
 *
 * @param vars      List of session variables to be freed
 */
static void
test_value_types_free(test_value_types *types)
{
    test_value_type *p;

    while ((p = SLIST_FIRST(types)) != NULL)
    {
        SLIST_REMOVE(types, p, test_value_type, links);
        test_value_type_free(p);
    }
}

/**
 * Free session variable.
 *
 * @param p         Session variable to be freed.
 */
static void
test_var_arg_free(test_var_arg *p)
{
    free(p->name);
    test_var_arg_values_free(&p->values);
    free(p->list);
    free(p);
}

/**
 * Free list of session variables.
 *
 * @param vars      List of session variables to be freed
 */
static void
test_vars_args_free(test_vars_args *vars)
{
    test_var_arg *p;

    while ((p = TAILQ_FIRST(vars)) != NULL)
    {
        TAILQ_REMOVE(vars, p, links);
        test_var_arg_free(p);
    }
}

/**
 * Free test session.
 *
 * @param p         Test session to be freed
 */
static void
test_session_free(test_session *p)
{
    free(p->name);
    free(p->objective);
    test_vars_args_free(&p->vars);
    test_value_types_free(&p->types);
    if (~p->flags & TEST_INHERITED_EXCEPTION)
        run_item_free(p->exception);
    if (~p->flags & TEST_INHERITED_KEEPALIVE)
        run_item_free(p->keepalive);
    run_item_free(p->prologue);
    run_item_free(p->epilogue);
    run_items_free(&p->run_items);
}

/**
 * Free test package.
 *
 * @param p         Test package to be freed
 */
static void
test_package_free(test_package *p)
{
    if (p == NULL)
        return;

    free(p->name);
    free(p->path);
    free(p->objective);
    persons_info_free(&p->authors);
    test_requirements_free(&p->reqs);
    test_session_free(&p->session);
    free(p);
}

/**
 * Free run item.
 *
 * @param run       Run item to be freed
 */
static void
run_item_free(run_item *run)
{
    test_var_arg_list  *list;

    if (run == NULL)
        return;

    free(run->name);
    free(run->objective);
    free(run->page);
    switch (run->type)
    {
        case RUN_ITEM_NONE:
            break;

        case RUN_ITEM_SCRIPT:
            test_script_free(&run->u.script);
            break;

        case RUN_ITEM_SESSION:
            test_session_free(&run->u.session);
            break;

        case RUN_ITEM_PACKAGE:
            test_package_free(run->u.package);
            break;

        default:
            assert(0);
    }

    test_vars_args_free(&run->args);

    while ((list = SLIST_FIRST(&run->lists)) != NULL)
    {
        SLIST_REMOVE(&run->lists, list, test_var_arg_list, links);
        free(list);
    }

    free_cmd_monitors(&run->cmd_monitors);

    free(run);
}


/**
 * Free list of run items.
 *
 * @param runs      List of run items to be freed
 */
static void
run_items_free(run_items *runs)
{
    run_item *p;

    while ((p = TAILQ_FIRST(runs)) != NULL)
    {
        TAILQ_REMOVE(runs, p, links);
        run_item_free(p);
    }
}

/**
 * Free Tester configuration.
 *
 * @param cfg       Tester configuration to be freed
 */
static void
tester_cfg_free(tester_cfg *cfg)
{
    persons_info_free(&cfg->maintainers);
    free(cfg->descr);
    test_suites_info_free(&cfg->suites);
    logic_expr_free(cfg->targets);
    run_items_free(&cfg->runs);
    free(cfg);
}

/* See the description in tester_conf.h */
void
tester_cfgs_free(tester_cfgs *cfgs)
{
    tester_cfg *cfg;

    while ((cfg = TAILQ_FIRST(&cfgs->head)) != NULL)
    {
        TAILQ_REMOVE(&cfgs->head, cfg, links);
        cfgs->total_iters -= cfg->total_iters;
        tester_cfg_free(cfg);
    }
}
