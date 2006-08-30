/** @file
 * @brief Tester Subsystem
 *
 * Code dealing with configuration files parsing and preprocessing.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

/** Logging user name to be used here */
#define TE_LGR_USER     "Config File Parser"

/** To get strndup() */
#define _GNU_SOURCE     1
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
#include <strings.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include "tester_conf.h"
#include "type_lib.h"


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
                                       const test_session *session,
                                       run_items *runs, run_item **p_run);

static te_errno parse_test_package(tester_cfg         *cfg,
                                   const test_session *session,
                                   test_package       *pkg);

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
    tester_cfg *p = calloc(1, sizeof(*p));

    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return NULL;
    }
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
    return xmlNodeSkipComment(node->next);
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
name_to_path(tester_cfg *cfg, const char *name, te_bool is_package)
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

        for (p = cfg->suites.tqh_first; p != NULL; p = p->links.tqe_next)
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
    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
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
                              te_bool           build,
                              te_bool           verbose)
{
    test_suite_info *p;

#ifndef XML_DOC_ASSUME_VALID
    if (node->children != NULL)
    {
        ERROR("'suite' cannot have children");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
#endif
    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
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

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
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

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        xmlFree(name);
        xmlFree(value);
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
    p->name = XML2CHAR(name);
    p->value = XML2CHAR(name);

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
get_bool_prop(xmlNodePtr node, const char *name, te_bool *value)
{
    xmlChar *s = xmlGetProp(node, CONST_CHAR2XML(name));

    if (s == NULL)
        return TE_RC(TE_TESTER, TE_ENOENT);
    if (xmlStrcmp(s, CONST_CHAR2XML("true")) == 0)
        *value = TRUE;
    else if (xmlStrcmp(s, CONST_CHAR2XML("false")) == 0)
        *value = FALSE;
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
 * @param is_signed Is signed value expected
 * @param value     Location for value
 *
 * @return Status code.
 * @retval TE_ENOENT    Property does not exists. Value is not modified.
 */
static te_errno
get_int_prop(xmlNodePtr node, const char *name, te_bool is_signed,
             int *value)
{
    xmlChar *s = xmlGetProp(node, CONST_CHAR2XML(name));
    long     v;
    char    *end;

    if (s == NULL)
        return TE_RC(TE_TESTER, TE_ENOENT);

    v = strtol(XML2CHAR(s), &end, 10);
    if (XML2CHAR(s) == end)
    {
        ERROR("Invalid value '%s' of the integer property '%s'",
              XML2CHAR(s), name);
        xmlFree(s);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    xmlFree(s);
    if (!is_signed && v < 0)
    {
        ERROR("Attribute '%s' may have unsigned integer value, "
              "but signed is specified (%d)", name, v);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }
    *value = v;

    return 0;
}

/**
 * Get attribute with inheritance specification.
 *
 * @param node      Node with requested property
 * @param name      Name of the property to get
 * @param value     Location for value
 *
 * @return Status code.
 */
static te_errno
get_handdown_attr(xmlNodePtr node, const char *name,
                  tester_handdown *value)
{
    char *s;

    /* 'handdown' is optional */
    *value = TESTER_HANDDOWN_DEF;
    s = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML(name)));
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
                          te_bool allow_sticky)
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
    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
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
    /* 'exclude' is depricated */
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
    p->sticky = FALSE;
    rc = get_bool_prop(node, "sticky", &p->sticky);
    if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
        return rc;
    if (rc == 0 && !allow_sticky)
    {
        ERROR("'sticky' requirements are not allowed for "
              "configurations and scripts");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

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
                 te_bool allow_sticky)
{
    te_errno rc;

    assert(node != NULL);
    assert(reqs != NULL);

    /* Get opddtional information about requirements to be tested */
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
    te_errno    rc;
    int         timeout;
    char       *s;

    /* Main session of the test package is not direct run item */
    if (attrs == NULL)
    {
        return 0;
    }

    /* 'timeout' is optional */
    timeout = TESTER_TIMEOUT_DEF;
    rc = get_int_prop(node, "timeout", FALSE, &timeout);
    if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
        return rc;
    attrs->timeout.tv_sec = timeout;
    attrs->timeout.tv_usec = 0;

    /* 'track_conf' is optional, default value is 'yes' */
    attrs->track_conf = TESTER_TRACK_CONF_UNSPEC;
    s = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("track_conf")));
    if (s != NULL)
    {
        if (xmlStrcmp(s, CONST_CHAR2XML("yes")) == 0)
            attrs->track_conf = TESTER_TRACK_CONF_YES;
        else if (xmlStrcmp(s, CONST_CHAR2XML("no")) == 0)
            attrs->track_conf = TESTER_TRACK_CONF_NO;
        else if (xmlStrcmp(s, CONST_CHAR2XML("silent")) == 0)
            attrs->track_conf = TESTER_TRACK_CONF_SILENT;
        else
        {
            ERROR("Invalid value '%s' of 'track_conf' property",
                  XML2CHAR(s));
            xmlFree(s);
            return TE_RC(TE_TESTER, TE_EINVAL);
        }
        xmlFree(s);

        rc = get_handdown_attr(node, "track_conf_handdown",
                               &attrs->track_conf_hd);
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
    te_errno rc;

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
    if (script->objective == NULL)
    {
        const test_info *ti = find_test_info(cfg->cur_pkg->ti,
                                             script->name);

        if (ti != NULL)
        {
            script->objective = strdup(ti->objective);
            if (script->objective == NULL)
            {
                ERROR("strdup(%s) failed", ti->objective);
                return TE_RC(TE_TESTER, TE_ENOMEM);
            }
        }
    }

    /* Get optional set of tested 'requirement's */
    rc = get_requirements(&node, &script->reqs, FALSE);
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
        script->execute = name_to_path(cfg, script->name, FALSE);
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

    for (p = values->head.tqh_first; p != NULL; p = p->links.tqe_next)
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

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
    TAILQ_INIT(&p->reqs);
    TAILQ_INSERT_TAIL(&values->head, p, links);
    
    /* 'name' is optional */
    p->name = xmlGetProp(node, CONST_CHAR2XML("name"));
    /* 'type' is optional */
    tmp = xmlGetProp(node, CONST_CHAR2XML("type"));
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
    tmp = xmlGetProp(node, CONST_CHAR2XML("ref"));
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
         * If type is specified, referencies to type values are the next
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
    tmp = xmlGetProp(node, CONST_CHAR2XML("reqs"));
    if (tmp != NULL)
    {
        char       *s = tmp;
        te_bool     end;

        do {
            size_t              len = strcspn(s, ",");
            test_requirement   *req = calloc(1, sizeof(*req));

            if (req == NULL)
            {
                ERROR("malloc(%u) failed", sizeof(*req));
                free(tmp);
                return TE_RC(TE_TESTER, TE_ENOMEM);
            }
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
         (p->type == NULL) ? "" : p->type->name, p->reqs.tqh_first);

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

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
    TAILQ_INIT(&p->values.head);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name is required for types");
        /* 
         * Do not insert before tester_find_type() including indirect
         * from alloc_and_get_value(), but required for clean up.
         */
        LIST_INSERT_HEAD(list, p, links);
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
            LIST_INSERT_HEAD(list, p, links);
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
            /* 
             * Do not insert before tester_find_type() including indirect
             * from alloc_and_get_value(), but required for clean up.
             */
            LIST_INSERT_HEAD(list, p, links);
            return rc;
        }
        node = xmlNodeNext(node);
    }

    /* 
     * Do not insert before tester_find_type() including indirect
     * from alloc_and_get_value(), but required for clean up.
     */
    LIST_INSERT_HEAD(list, p, links);

    if (p->values.head.tqh_first == NULL)
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
 * @param node      Node with simple variable
 * @param is_var    Is it variable?
 * @param list      List of session variables or run item arguments
 *
 * @return Status code.
 */
static te_errno
alloc_and_get_var_arg(xmlNodePtr node, te_bool is_var,
                      const test_session *session, test_vars_args *list)
{
    xmlNodePtr          root = node;
    te_errno            rc;
    test_var_arg       *p;
    char               *ref;
    char               *value;
    char               *s;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
    p->handdown = !is_var;
    TAILQ_INIT(&p->values.head);
    TAILQ_INSERT_TAIL(list, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name is required for simple variable");
        return TE_RC(TE_TESTER, TE_EINVAL);
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
            return rc;
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

    /* 'ref' is optional */
    ref = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("ref")));
    /* 'value' is optional */
    value = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("value")));

    if (((ref != NULL) + (value != NULL) +
         (p->values.head.tqh_first != NULL)) > 1)
    {
        ERROR("Too many sources of %s '%s' value: ref=%s value=%s "
               "values=%s", (is_var) ? "variable" : "argument", p->name,
               (ref) ? : "(empty)", (value) ? : "(empty)",
               (p->values.head.tqh_first) ? "(not empty)" : "(empty)");
        free(ref);
        free(value);
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    if (p->values.head.tqh_first == NULL && p->type == NULL)
    {
        test_entity_value *v;

        v = calloc(1, sizeof(*v));
        if (v == NULL)
        {
            ERROR("malloc(%u) failed", sizeof(*v));
            return TE_RC(TE_TESTER, TE_ENOMEM);
        }
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
            test_session *session)
{
    te_errno rc;

    session->parent = parent;

    /* Get run item attributes */
    rc = get_test_attrs(node, &session->attrs);
    if (rc != 0)
        return rc;

    /* 'simultaneous' is optional, default value is false */
    session->simultaneous = FALSE;
    rc = get_bool_prop(node, "simultaneous", &session->simultaneous);
    if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
        return rc;

    node = xmlNodeChildren(node);

    /* Get information about types */
    while (node != NULL)
    {
        if (xmlStrcmp(node->name, CONST_CHAR2XML("enum")) == 0)
            rc = alloc_and_get_enum(node, session, &session->types);
        else
            break;
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    /* Get information about variables */
    while (node != NULL)
    {
        if (xmlStrcmp(node->name, CONST_CHAR2XML("var")) == 0)
            rc = alloc_and_get_var_arg(node, TRUE, session,
                                       &session->vars);
        else if (xmlStrcmp(node->name, CONST_CHAR2XML("arg")) == 0)
            rc = alloc_and_get_var_arg(node, FALSE, session,
                                       &session->vars);
        else
            break;
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    /* Get 'exception' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("exception")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_SERVICE |
                                    TESTER_RUN_ITEM_INHERITABLE,
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
                                    session, NULL, &session->prologue);
        if (rc != 0)
            return rc;

        /* By default, configuration is not tracked after prologue */
        if (test_get_attrs(session->prologue)->track_conf ==
            TESTER_TRACK_CONF_UNSPEC)
        {
            test_get_attrs(session->prologue)->track_conf =
                TESTER_TRACK_CONF_NO;
        }

        node = xmlNodeNext(node);
    }
    /* Get 'epilogue' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("epilogue")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_SERVICE,
                                    session, NULL, &session->epilogue);
        if (rc != 0)
            return rc;

        /* By default, configuration is not tracked after epilogue */
        if (test_get_attrs(session->epilogue)->track_conf ==
            TESTER_TRACK_CONF_UNSPEC)
        {
            test_get_attrs(session->epilogue)->track_conf =
                TESTER_TRACK_CONF_NO;
        }

        node = xmlNodeNext(node);
    }
    /* Get 'run' items */
    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("run")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg, 0, session, 
                                    &session->run_items, NULL);
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
            test_package **pkg)
{
    te_errno        rc;
    test_package   *p;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
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

    rc = parse_test_package(cfg, session, p);
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
                       const test_session *session,
                       run_items *runs, run_item **p_run)
{
    run_item    *p;
    te_errno     rc;

    assert((runs == NULL) != (p_run == NULL));
    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
    TAILQ_INIT(&p->args);
    LIST_INIT(&p->lists);
    p->context = session;
    p->iterate = 1;

    /* Just for corrent clean up in the case of failure */
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

        /* 'iterate' is optional */
        rc = get_int_prop(node, "iterate", FALSE, &p->iterate);
        if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
            return rc;
    }

    if (opts & TESTER_RUN_ITEM_INHERITABLE)
    {
        rc = get_handdown_attr(node, "handdown", &p->handdown);
        if (rc != 0)
            return rc;
    }

    /* 'loglevel' is optional */
    p->loglevel = 0;
    rc = get_int_prop(node, "loglevel", FALSE, &p->loglevel);
    if (rc != 0 && rc != TE_RC(TE_TESTER, TE_ENOENT))
        return rc;

    node = xmlNodeChildren(node);
    if (node == NULL)
    {
        ERROR("Empty 'run' item");
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
        rc = get_session(node, cfg, session, &p->u.session);
        if (rc != 0)
            return rc;
    }
    else if ((~opts & TESTER_RUN_ITEM_SERVICE) &&
             xmlStrcmp(node->name, CONST_CHAR2XML("package")) == 0)
    {
        p->type = RUN_ITEM_PACKAGE;
        rc = get_package(node, cfg, session, &p->u.package);
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
        rc = alloc_and_get_var_arg(node, FALSE, session, &p->args);
        if (rc != 0)
            return rc;
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
                 const test_session *session, test_package *pkg)
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
    rc = get_requirements(&node, &pkg->reqs, TRUE);
    if (rc != 0)
    {
        ERROR("Failed to get information about Test Package requirements");
        return rc;
    }

    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("session")) == 0)
    {
        rc = get_session(node, cfg, session, &pkg->session);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
#ifndef XML_DOC_ASSUME_VALID
    else
    {
        ERROR("'session' is mandarory in Test Package description");
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

    /* Get opddtional information about requirements to be tested */
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
                  te_bool build, te_bool verbose)
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
    if (cfg->maintainers.tqh_first == NULL)
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
        rc = alloc_and_get_run_item(node, cfg, 0, NULL, 
                                    &cfg->runs, NULL);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
#ifndef XML_DOC_ASSUME_VALID
    if (cfg->runs.tqh_first == NULL)
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

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
    TAILQ_INSERT_TAIL(ti, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Missing name of the test in info");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

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

    for (p = (ti != NULL) ? ti->tqh_first : NULL;
         p != NULL;
         p = p->links.tqe_next)
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

    while ((p = ti->tqh_first) != NULL)
    {
        TAILQ_REMOVE(ti, p, links);
        free(p->name);
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
                   test_package *pkg)
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

    pkg->path = name_to_path(cfg, pkg->name, TRUE);
    if (pkg->path == NULL)
    {
        ERROR("Failed to make path to Test Package file by name and "
              "context");
        return TE_RC(TE_TESTER, TE_EINVAL);
    }

    cur_pkg_save = cfg->cur_pkg;
    cfg->cur_pkg = pkg;

    ti_path = name_to_path(cfg, "tests-info.xml", FALSE);
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

        ERROR("Error occured during parsing Test Package file:\n"
              "    %s:%d\n    %s", pkg->path, err->line, err->message);
#else
        ERROR("Error occured during parsing Test Package file:\n"
              "%s", pkg->path);
#endif
        rc = TE_RC(TE_TESTER, TE_EINVAL);
        goto cleanup;
    }

    if (stat(ti_path, &st_buf) == 0)
    {
        if ((ti_doc = xmlCtxtReadFile(parser, ti_path, NULL,
                                      XML_PARSE_NOBLANKS |
                                      XML_PARSE_NONET)) == NULL)
        {
#if HAVE_XMLERROR
            xmlError   *err = xmlCtxtGetLastError(parser);

            ERROR("Error occured during parsing Tests Info file:\n"
                  "    %s:%d\n    %s", pkg->path, err->line, err->message);
#else
            ERROR("Error occured during parsing Tests Info file:\n"
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

    rc = get_test_package(xmlDocGetRootElement(doc), cfg, session, pkg);
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
 * Parse Tester configuratin file.
 *
 * @param cfg           Tester configuratin with not parsed file
 * @param build         Build test suites
 * @param verbose       Be verbose in the case of build failure
 *
 * @return Status code.
 */
static te_errno
tester_parse_config(tester_cfg *cfg, te_bool build, te_bool verbose)
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

        ERROR("Error occured during parsing configuration file:\n"
              "    %s:%d\n    %s", cfg->filename, err->line, err->message);
#else
        ERROR("Error occured during parsing configuration file:\n"
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
tester_parse_configs(tester_cfgs *cfgs, te_bool build, te_bool verbose)
{
    te_errno    rc;
    tester_cfg *cfg;

    for (cfg = cfgs->tqh_first; cfg != NULL; cfg = cfg->links.tqe_next)
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

    while ((p = persons->tqh_first) != NULL)
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

    while ((p = values->head.tqh_first) != NULL)
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

    while ((p = types->lh_first) != NULL)
    {
        LIST_REMOVE(p, links);
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

    while ((p = vars->tqh_first) != NULL)
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

    while ((list = run->lists.lh_first) != NULL)
    {
        LIST_REMOVE(list, links);
        free(list);
    }

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

    while ((p = runs->tqh_first) != NULL)
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

    while ((cfg = cfgs->tqh_first) != NULL)
    {
        TAILQ_REMOVE(cfgs, cfg, links);
        tester_cfg_free(cfg);
    }
}
