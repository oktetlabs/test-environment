/** @file
 * @brief Tester Subsystem
 *
 * Code dealing with configuration files parsing and preprocessing.
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

#define TE_LGR_USER     "Parser"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#include <strings.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include "internal.h"


#define CONST_CHAR2XML  (const xmlChar *)
#define XML2CHAR(p)     ((char *)p)
#define XML2CHAR_DUP(p) XML2CHAR(xmlStrdup(p))


enum {
    TESTER_RUN_ITEM_EXECUTABLE = 1 << 0,
    TESTER_RUN_ITEM_INHERITABLE = 1 << 1,
};


static int alloc_and_get_run_item(xmlNodePtr node, tester_cfg *cfg,
                                  unsigned int opts,
                                  run_items *runs, run_item **p_run);

static int parse_test_package(tester_cfg *cfg, test_package *pkg);


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
 * Go to the next XML node, skip 'comment' nodes.
 *
 * @param node      XML node
 *
 * @return The next XML node.
 */
static xmlNodePtr
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
static xmlNodePtr
xmlNodeNext(xmlNodePtr node)
{
    assert(node != NULL);
    return xmlNodeSkipComment(node->next);
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
static int
alloc_and_get_tqe_string(xmlNodePtr node, tqh_strings *strs)
{
    tqe_string  *p;

#ifndef XML_DOC_ASSUME_VALID
    if (node->children != NULL)
    {
        ERROR("'string' cannot have children");
        return EINVAL;
    }
#endif
    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
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
 * @param flags         Tester context flags
 *
 * @return Status code.
 */
static int
alloc_and_get_test_suite_info(xmlNodePtr node, test_suites_info *suites_info,
                              unsigned int flags)
{
    test_suite_info *p;

#ifndef XML_DOC_ASSUME_VALID
    if (node->children != NULL)
    {
        ERROR("'suite' cannot have children");
        return EINVAL;
    }
#endif
    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    TAILQ_INSERT_TAIL(suites_info, p, links);

    /* Name is mandatory */
    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("'name' attribute is missing in suite information");
        return EINVAL;
    }
    /* Path to sources is optional */
    p->src = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("src")));
    /* Path to binaries is optional */
    p->bin = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("bin")));

    if (p->src != NULL && p->bin != NULL)
    {
        ERROR("Two paths are specified for Test Suite '%s'", p->name);
        return EINVAL;
    }
    if (p->src == NULL && p->bin == NULL)
    {
        p->src = strdup(p->name);
        if (p->src == NULL)
        {
            ERROR("strdup(%s) failed", p->name);
            return ENOMEM;
        }
    }

    if ((p->src != NULL) && (~flags & TESTER_NOBUILD))
    {
        int rc = tester_build_suite(flags, p);

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
static int
alloc_and_get_person_info(xmlNodePtr node, persons_info *persons)
{
    person_info  *p;

    assert(node != NULL);
    assert(persons != NULL);

#ifndef XML_DOC_ASSUME_VALID
    if (node->children != NULL)
    {
        ERROR("'person_info' cannot have children");
        return EINVAL;
    }
#endif

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    TAILQ_INSERT_TAIL(persons, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    p->mailto = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("mailto")));
#ifndef XML_DOC_ASSUME_VALID
    if (p->mailto == NULL)
    {
        ERROR("'mailto' attribute is mandatory in person info");
        return EINVAL;
    }
#endif

    return 0;
}

/**
 * Get (possibly empty) list of information about persons.
 *
 * @param node      Location of the XML node pointer
 * @param persons   List with information about persons
 *
 * @return Status code.
 */
static int
get_persons_info(xmlNodePtr *node, const char *node_name,
                 persons_info *persons)
{
    int rc;

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
static int
alloc_and_get_option(xmlNodePtr node, test_options *opts)
{
    xmlChar     *name;
    xmlChar     *value;
    test_option *p;
    xmlNodePtr   q;
    int          rc;

    /* Name is mandatory */
    name = xmlGetProp(node, CONST_CHAR2XML("name"));
    if (name == NULL)
    {
        ERROR("'name' attribute of the option is missing");
        return EINVAL;
    }
    /* Path is optional */
    value = xmlGetProp(node, CONST_CHAR2XML("value"));

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        xmlFree(name);
        xmlFree(value);
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    p->name = XML2CHAR(name);
    p->value = XML2CHAR(name);

    /* Get optional information about contexts */
    for (q = xmlNodeChildren(node);
         (q != NULL) && (xmlStrcmp(q->name, CONST_CHAR2XML("context")) == 0);
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
        return EINVAL;
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
 * Get 'int' property.
 *
 * @param node      Node with 'int' property
 * @param name      Name of the property to get
 * @param value     Location for value
 *
 * @return Status code.
 * @retval ENOENT   Property does not exists. Value is not modified.
 */
static int
get_int_prop(xmlNodePtr node, const char *name, int *value)
{
    xmlChar *s = xmlGetProp(node, CONST_CHAR2XML(name));
    char    *end;

    if (s == NULL)
        return ENOENT;

    *value = strtol(XML2CHAR(s), &end, 16);
    if (XML2CHAR(s) == end)
    {
        ERROR("Invalid value '%s' of the integer property '%s'",
              XML2CHAR(s), name);
        xmlFree(s);
        return EINVAL;
    }
    xmlFree(s);

    return 0;
}


/**
 * Get requirement.
 *
 * @param node      Node with requirement
 * @param reqs      List of requirements
 *
 * @return Status code.
 */
static int
alloc_and_get_requirement(xmlNodePtr node, test_requirements *reqs)
{
    int                 rc;
    test_requirement   *p;

#ifndef XML_DOC_ASSUME_VALID
    if (node->children != NULL)
    {
        ERROR("'requirement' cannot have children");
        return EINVAL;
    }
#endif
    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    p->id = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("id")));
    p->ref = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("ref")));
    if ((p->id == NULL) == (p->ref == NULL))
    {
        ERROR("One and only one of 'id' or 'ref' attributes must "
              "present for requirement");
        return EINVAL;
    }

    /* 'exclude' is optional, default value is false */
    p->exclude = FALSE;
    rc = get_bool_prop(node, "exclude", &p->exclude);
    if (rc != 0 && rc != ENOENT)
        return rc;

    TAILQ_INSERT_TAIL(reqs, p, links);

    return 0;
}

/**
 * Get (possibly empty) set of requirements.
 *
 * @param node      Location of the XML node pointer
 * @param reqs      List of requirements to be updated
 *
 * @return Status code.
 */
static int
get_requirements(xmlNodePtr *node, test_requirements *reqs)
{
    int rc;

    assert(node != NULL);
    assert(reqs != NULL);

    /* Get opddtional information about requirements to be tested */
    while (*node != NULL &&
           xmlStrcmp((*node)->name, CONST_CHAR2XML("req")) == 0)
    {
        rc = alloc_and_get_requirement(*node, reqs);
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
static int
get_run_item_attrs(xmlNodePtr node, run_item_attrs *attrs)
{
    int rc;
    int timeout;

    /* Main session of the test package is not direct run item */
    if (attrs == NULL)
    {
        return 0;
    }

    /* 'timeout' is optional */
    timeout = TESTER_TIMEOUT_DEF;
    rc = get_int_prop(node, "timeout", &timeout);
    if (rc != 0 && rc != ENOENT)
        return rc;
    attrs->timeout.tv_sec = timeout;
    attrs->timeout.tv_usec = 0;

    /* 'track_conf' is optional, default value is true */
    attrs->track_conf = TRUE;
    rc = get_bool_prop(node, "track_conf", &attrs->track_conf);
    if (rc != 0 && rc != ENOENT)
        return rc;

    return 0;
}


/**
 * Get script call description.
 * 
 * @param node      XML node with script call description
 * @param cfg       Tester configuration context
 * @param script    Location for script call description
 * @param attrs     Run item attributes
 *
 * @return Status code.
 */
static int
get_script(xmlNodePtr node, tester_cfg *cfg, test_script *script,
           run_item_attrs *attrs)
{
    int rc;

    /* 'name' is mandatory */
    script->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (script->name == NULL)
    {
        ERROR("'name' attribute is missing in script call description");
        return EINVAL;
    }

    /* Get run item attributes */
    rc = get_run_item_attrs(node, attrs);
    if (rc != 0)
        return rc;

    node = xmlNodeChildren(node);

    /* Get optional 'description' */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("description")) == 0)
    {
        script->descr = XML2CHAR_DUP(node->content);
        node = xmlNodeNext(node);
    }
    /* Get optional set of tested 'requirement's */
    rc = get_requirements(&node, &script->reqs);
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
        return ENOMEM;
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in script '%s' call description",
              XML2CHAR(node->name), script->name);
        return EINVAL;
    }
    VERB("Got script '%s'", script->name);

    return 0;
}


/**
 * Allocate and get argument or variable value.
 *
 * @param node      Node with new run item
 * @param values    List of values
 *
 * @return Status code.
 */
static int
alloc_and_get_value(xmlNodePtr node, test_var_arg_values *values)
{
    test_var_arg_value *p;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    TAILQ_INSERT_TAIL(values, p, links);
    
    /* 'id' is optional */
    p->id = xmlGetProp(node, CONST_CHAR2XML("id"));
    /* 'refvalue' is optional */
    p->refvalue = xmlGetProp(node, CONST_CHAR2XML("refvalue"));
    /* 'ext' is optional */
    p->ext = xmlGetProp(node, CONST_CHAR2XML("ext"));
    
    /* Simple text content is represented as 'text' elements */
    if (node->children != NULL)
    {
        if ((xmlStrcmp(node->children->name, CONST_CHAR2XML("text")) != 0) ||
            (node->children->content == NULL))
        {
            ERROR("'value' content is empty or not 'text'");
            return EINVAL;
        }
        if (node->children != node->last)
        {
            ERROR("Too many children in 'value' element");
            return EINVAL;
        }
        p->value = XML2CHAR_DUP(node->children->content);
    }

    if (((!!(p->refvalue)) + (!!(p->ext) + (!!(p->value)))) != 1)
    {
        ERROR("Too many sources of value");
        return EINVAL;
    }

    return 0;
}

/**
 * Get attributes common for reffered variables and arguments.
 *
 * @param node      XML node
 * @param attrs     Location for attributes
 *
 * @return Status code.
 */
static int
get_ref_var_arg_attrs(xmlNodePtr node, test_ref_var_arg_attrs *attrs)
{
    /* 'refer' attribute is optional */
    attrs->refer = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("refer")));

    return 0;
}

/**
 * Get attributes common for simple variables and arguments.
 *
 * @param node      XML node
 * @param values    Set of variable/argument values
 * @param attrs     Location for attributes
 *
 * @return Status code.
 */
static int
get_var_arg_attrs(xmlNodePtr node, test_var_arg_values *values,
                  test_var_arg_attrs *attrs)
{
    int   rc;
    char *s;

    /* 'random' is optional, default value is false */
    attrs->random = FALSE;
    rc = get_bool_prop(node, "random", &attrs->random);
    if (rc == 0)
        attrs->flags |= TEST_RANDOM_SPECIFIED;
    else if (rc != ENOENT)
        return rc;

    /* 'list' is optional */
    attrs->list = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("list")));

    /* 'type' is optional */
    s = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("type")));
    if (s != NULL)
    {
        static te_bool warn_type = FALSE;
        
        if (!warn_type)
        {
            WARN("Types of variables/attributes are not supported yet");
            warn_type = TRUE;
        }
        xmlFree(s);
    }

    /* 'preferred' is optional */
    s = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("preferred")));
    if (s != NULL)
    {
        test_var_arg_value  *p;

        if (attrs->list == NULL)
        {
            WARN("'preferred' attribute is useless without 'list'");
        }
        for (p = values->tqh_first; p != NULL; p = p->links.tqe_next)
        {
            if (p->id != NULL && strcmp(p->id, s) == 0)
            {
                attrs->preferred = p;
                break;
            }
        }
        if (attrs->preferred == NULL)
        {
            ERROR("Value with 'id'='%s' not found to be preferred", s);
            xmlFree(s);
            return EINVAL;
        }
        xmlFree(s);
    }

    return 0;
}


/**
 * Allocate and get session reffered variable.
 *
 * @param node      Node with new run item
 * @param vars      List of session variables
 *
 * @return Status code.
 */
static int
alloc_and_get_refvar(xmlNodePtr node, test_session_vars *vars)
{
    int                 rc;
    test_session_var   *p;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    p->type = TEST_SESSION_VAR_REFERRED;
    TAILQ_INSERT_TAIL(vars, p, links);

    /* 'name' is mandatory */
    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name is required for reffered argument");
        return EINVAL;
    }

    /* Get attributes common for reffered variables and attributes */
    rc = get_ref_var_arg_attrs(node, &p->u.ref.attrs);
    if (rc != 0)
        return rc;

    /* 'handdown' is optional, default value is false */
    p->handdown = FALSE;
    rc = get_bool_prop(node, "handdown", &p->handdown);
    if (rc != 0 && rc != ENOENT)
        return rc;

    node = xmlNodeChildren(node);
    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in reffered variable",
              XML2CHAR(node->name));
        return EINVAL;
    }

    return 0;
}

/**
 * Allocate and get session simple variable.
 *
 * @param node      Node with simple variable
 * @param vars      List of session variables
 *
 * @return Status code.
 */
static int
alloc_and_get_var(xmlNodePtr node, test_session_vars *vars)
{
    xmlNodePtr          root = node;
    int                 rc;
    test_session_var   *p;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    p->type = TEST_SESSION_VAR_SIMPLE;
    TAILQ_INIT(&p->u.var.values);
    TAILQ_INSERT_TAIL(vars, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name is required for simple variable");
        return EINVAL;
    }

    /* 'handdown' is optional, default value is false */
    p->handdown = FALSE;
    rc = get_bool_prop(node, "handdown", &p->handdown);
    if (rc != 0 && rc != ENOENT)
        return rc;

    node = xmlNodeChildren(node);
    while ((node != NULL) &&
           (xmlStrcmp(node->name, CONST_CHAR2XML("value")) == 0))
    {
        rc = alloc_and_get_value(node, &p->u.var.values);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
    if (p->u.var.values.tqh_first == NULL)
    {
        ERROR("Empty list of argument values");
        return EINVAL;
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in argument",
              XML2CHAR(node->name));
        return EINVAL;
    }

    /* It must be done when values have already been processed */
    rc = get_var_arg_attrs(root, &p->u.var.values, &p->u.var.attrs);
    if (rc != 0)
        return rc;

    return 0;
}


/**
 * Get session description.
 *
 * @param node      XML node with session description
 * @param cfg       Tester configuration context
 * @param session   Location for session description
 * @param attrs     Run item attributes
 *
 * @return Status code.
 */
static int
get_session(xmlNodePtr node, tester_cfg *cfg, test_session *session,
            run_item_attrs *attrs)
{
    int rc;

    /* Get run item attributes */
    rc = get_run_item_attrs(node, attrs);
    if (rc != 0)
        return rc;

    /* 'simultaneous' is optional, default value is false */
    session->simultaneous = FALSE;
    rc = get_bool_prop(node, "simultaneous", &session->simultaneous);
    if (rc != 0 && rc != ENOENT)
        return rc;

    /* 'random' is optional */
    rc = get_bool_prop(node, "random", &session->random);
    if (rc == 0)
        session->flags |= TEST_RANDOM_SPECIFIED;
    else if (rc != ENOENT)
        return rc;

    node = xmlNodeChildren(node);

    /* Get information about variables */
    while (node != NULL)
    {
        if (xmlStrcmp(node->name, CONST_CHAR2XML("var")) == 0)
        {
            rc = alloc_and_get_var(node, &session->vars);
        }
        else if (xmlStrcmp(node->name, CONST_CHAR2XML("refvar")) == 0)
        {
            rc = alloc_and_get_refvar(node, &session->vars);
        }
        else
        {
            break;
        }
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    /* Get 'exception' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("exception")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_EXECUTABLE |
                                    TESTER_RUN_ITEM_INHERITABLE,
                                    NULL, &session->exception);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
    /* Get 'keepalive' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("keepalive")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_EXECUTABLE |
                                    TESTER_RUN_ITEM_INHERITABLE,
                                    NULL, &session->keepalive);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
    /* Get 'prologue' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("prologue")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_EXECUTABLE,
                                    NULL, &session->prologue);
        if (rc != 0)
            return rc;
        /* TODO */
        session->prologue->attrs.track_conf = FALSE;
        node = xmlNodeNext(node);
    }
    /* Get 'epilogue' handler */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("epilogue")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg,
                                    TESTER_RUN_ITEM_EXECUTABLE,
                                    NULL, &session->epilogue);
        if (rc != 0)
            return rc;
        /* TODO */
        session->prologue->attrs.track_conf = FALSE;
        node = xmlNodeNext(node);
    }
    /* Get 'run' items */
    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("run")) == 0)
    {
        rc = alloc_and_get_run_item(node, cfg, 0,
                                    &session->run_items, NULL);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in session: line=%u",
              XML2CHAR(node->name), node->line);
        return EINVAL;
    }

    return 0;
}

/**
 * Get package as run item description.
 *
 * @param node      XML node with package as run item description
 * @param cfg       Tester configuration context
 * @param pkg       Location for package description
 * @param attrs     Run item attributes
 *
 * @return Status code.
 */
static int
get_package(xmlNodePtr node, tester_cfg *cfg, test_package **pkg,
            run_item_attrs *attrs)
{
    int             rc;
    test_package   *p;

    /* Get run item attributes */
    rc = get_run_item_attrs(node, attrs);
    if (rc != 0)
        return rc;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
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
        return EINVAL;
    }

    rc = parse_test_package(cfg, p);
    if (rc != 0)
    {
        ERROR("Parsing/preprocessing of the package '%s' failed",
              p->name);
    }

    return rc;
}


/**
 * Allocate and get run item reffered argument.
 *
 * @param node      Node with new run item
 * @param args      List of run item arguments
 *
 * @return Status code.
 */
static int
alloc_and_get_refarg(xmlNodePtr node, test_args *args)
{
    int         rc;
    test_arg   *p;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    p->type = TEST_ARG_REFERRED;
    TAILQ_INSERT_TAIL(args, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name is required for reffered argument");
        return EINVAL;
    }

    /* Get attributes common for reffered variables and attributes */
    rc = get_ref_var_arg_attrs(node, &p->u.ref.attrs);
    if (rc != 0)
        return rc;

    node = xmlNodeChildren(node);
    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in reffered argument",
              XML2CHAR(node->name));
        return EINVAL;
    }

    return 0;
}

/**
 * Allocate and get run item argument.
 *
 * @param node      Node with new run item
 * @param args      List of run item arguments
 *
 * @return Status code.
 */
static int
alloc_and_get_arg(xmlNodePtr node, test_args *args)
{
    xmlNodePtr  root = node;
    int         rc;
    test_arg   *p;

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    p->type = TEST_ARG_SIMPLE;
    TAILQ_INIT(&p->u.arg.values);
    TAILQ_INSERT_TAIL(args, p, links);

    p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
    if (p->name == NULL)
    {
        ERROR("Name is required for reffered argument");
        return EINVAL;
    }

    node = xmlNodeChildren(node);
    while ((node != NULL) &&
           (xmlStrcmp(node->name, CONST_CHAR2XML("value")) == 0))
    {
        rc = alloc_and_get_value(node, &p->u.arg.values);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
    if (p->u.arg.values.tqh_first == NULL)
    {
        ERROR("Empty list of argument values");
        return EINVAL;
    }

    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in argument",
              XML2CHAR(node->name));
        return EINVAL;
    }

    /* It must be done when values have already been processed */
    rc = get_var_arg_attrs(root, &p->u.arg.values, &p->u.arg.attrs);
    if (rc != 0)
        return rc;

    return 0;
}


/**
 * Get run item from configuration file tree.
 *
 * @param node      Node with new run item
 * @param cfg       Tester configuration context
 * @param opts      Run item options (types)
 * @param runs      List of run items or @c NULL
 * @param p_run     Location for pointer to allocated run item or @c NULL
 *
 * One of @a runs or @a p_run arguments must be @c NULL and another 
 * not @c NULL.
 *
 * @return Status code.
 */
static int
alloc_and_get_run_item(xmlNodePtr node, tester_cfg *cfg, unsigned int opts,
                       run_items *runs, run_item **p_run)
{
    run_item    *p;
    int          rc;

    assert((runs == NULL) != (p_run == NULL));
    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("malloc(%u) failed", sizeof(*p));
        return ENOMEM;
    }
    TAILQ_INIT(&p->args);

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

    if (!(opts & TESTER_RUN_ITEM_EXECUTABLE))
    {
        /* 'name' is optional */
        p->name = XML2CHAR(xmlGetProp(node, CONST_CHAR2XML("name")));
        VERB("Preprocessing 'run' item '%s'", p->name ? : "(noname)");

        /* 'loglevel' is optional */
        p->loglevel = 0;
        rc = get_int_prop(node, "loglevel", &p->loglevel);
        if (rc != 0 && rc != ENOENT)
            return rc;

        /* 'allow_configure' is optional, default value is true */
        p->allow_configure = TRUE;
        rc = get_bool_prop(node, "allow_configure", &p->allow_configure);
        if (rc != 0 && rc != ENOENT)
            return rc;

        /* 'allow_keepalive' is optional, default value is true */
        p->allow_keepalive = TRUE;
        rc = get_bool_prop(node, "allow_keepalive", &p->allow_keepalive);
        if (rc != 0 && rc != ENOENT)
            return rc;

        /* 'forcerandom' is optional */
        rc = get_bool_prop(node, "forcerandom", &p->forcerandom);
        if (rc == 0)
            p->flags |= TESTER_RUN_ITEM_FORCERANDOM;
        else if (rc != ENOENT)
            return rc;
    }
    
    node = xmlNodeChildren(node);
    if (node == NULL)
    {
        ERROR("Empty 'run' item");
        return EINVAL;
    }
    
    if (xmlStrcmp(node->name, CONST_CHAR2XML("script")) == 0)
    {
        p->type = RUN_ITEM_SCRIPT;
        TAILQ_INIT(&p->u.script.reqs);
        rc = get_script(node, cfg, &p->u.script, &p->attrs);
        if (rc != 0)
            return rc;
    }
    else if (xmlStrcmp(node->name, CONST_CHAR2XML("session")) == 0)
    {
        p->type = RUN_ITEM_SESSION;
        TAILQ_INIT(&p->u.session.vars);
        TAILQ_INIT(&p->u.session.run_items);
        rc = get_session(node, cfg, &p->u.session, &p->attrs);
        if (rc != 0)
            return rc;
    }
    else if (!(opts & TESTER_RUN_ITEM_EXECUTABLE) &&
             xmlStrcmp(node->name, CONST_CHAR2XML("package")) == 0)
    {
        p->type = RUN_ITEM_PACKAGE;
        rc = get_package(node, cfg, &p->u.package, &p->attrs);
        if (rc != 0)
            return rc;
    }
    else
    {
        ERROR("The first element '%s' in run item is incorrect",
              XML2CHAR(node->name));
        return EINVAL;
    }
    node = xmlNodeNext(node);
    
    /* Get information about arguments */
    while (node != NULL)
    {
        if (xmlStrcmp(node->name, CONST_CHAR2XML("arg")) == 0)
        {
            rc = alloc_and_get_arg(node, &p->args);
        }
        else if (xmlStrcmp(node->name, CONST_CHAR2XML("refarg")) == 0)
        {
            rc = alloc_and_get_refarg(node, &p->args);
        }
        else
        {
            break;
        }
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }
    
    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in run item",
              XML2CHAR(node->name));
        return EINVAL;
    }

    return 0;
}


/**
 * Preprocess parsed Test Package file.
 *
 * @param root      Root element of the Test Package file
 * @param cfg       Tester configuration context
 * @param pkg       Test Package structure to be filled in
 *
 * @return Status code.
 */
static int
get_test_package(xmlNodePtr root, tester_cfg *cfg, test_package *pkg)
{
    xmlNodePtr  node;
    xmlChar    *s;
    int         rc;

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
        return EINVAL;
    }
    if (root->next != NULL)
    {
        ERROR("'package' element must be singleton in Test Package file");
        return EINVAL;
    }
#endif

    /* Check version of the Tester configuration file */
    s = xmlGetProp(root, CONST_CHAR2XML("version"));
#ifndef XML_DOC_ASSUME_VALID
    if (s == NULL)
    {
        ERROR("'version' of the Test Package file is not specified");
        return EINVAL;
    }
#endif
    if (xmlStrcmp(s, CONST_CHAR2XML("1.0")) != 0)
    {
        ERROR("Unsupported version %s of the Test Package file",
              XML2CHAR(s));
        xmlFree(s);
        return EINVAL;
    }
    xmlFree(s);

    node = root->children;

    /* Get mandatory description */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("description")) == 0)
    {
        /* Simple text content is represented as 'text' elements */
        if ((node->children == NULL) ||
            (xmlStrcmp(node->children->name, CONST_CHAR2XML("text")) != 0) ||
            (node->children->content == NULL))
        {
            ERROR("'description' content is empty or not 'text'");
            return EINVAL;
        }
        if (node->children != node->last)
        {
            ERROR("Too many children in 'description' element");
            return EINVAL;
        }
        pkg->descr = XML2CHAR_DUP(node->children->content);
        node = xmlNodeNext(node);
    }
#ifndef XML_DOC_ASSUME_VALID
    if (pkg->descr == NULL)
    {
        ERROR("'description' is mandatory for any Test Package");
        return EINVAL;
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
    rc = get_requirements(&node, &pkg->reqs);
    if (rc != 0)
    {
        ERROR("Failed to get information about Test Package requirements");
        return rc;
    }

    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("session")) == 0)
    {
        rc = get_session(node, cfg, &pkg->session, NULL);
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
        return EINVAL;
    }
#endif

    return 0;
}


/**
 * Preprocess parsed Tester configuration file.
 *
 * @param root      Root element of the Tester configuration file
 * @param cfg       Configuration structure to be filled in
 * @param flags     Tester context flags
 *
 * @return Status code.
 */
static int
get_tester_config(xmlNodePtr root, tester_cfg *cfg, unsigned int flags)
{
    xmlNodePtr  node;
    xmlChar    *s;
    int         rc;

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
        return EINVAL;
    }
    if (root->next != NULL)
    {
        ERROR("'tester_cfg' element must be singleton");
        return EINVAL;
    }
#endif

    /* Check version of the Tester configuration file */
    s = xmlGetProp(root, CONST_CHAR2XML("version"));
#ifndef XML_DOC_ASSUME_VALID
    if (s == NULL)
    {
        ERROR("'version' of the Tester configuration file is not specified");
        return EINVAL;
    }
#endif
    if (xmlStrcmp(s, CONST_CHAR2XML("1.0")) != 0)
    {
        ERROR("Unsupported version %s of the Tester configuration file",
              XML2CHAR(s));
        xmlFree(s);
        return EINVAL;
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
        return EINVAL;
    }
    
    /* Get optional description */
    if (node != NULL &&
        xmlStrcmp(node->name, CONST_CHAR2XML("description")) == 0)
    {
        if (node->children == NULL)
        {
            ERROR("Tester configuration 'description' must not be "
                  "empty, if it is present");
            return EINVAL;
        }
        /* Simple text content is represented as 'text' elements */
        if ((xmlStrcmp(node->children->name, CONST_CHAR2XML("text")) != 0) ||
            (node->children->content == NULL))
        {
            ERROR("Tester configuration 'description' content is "
                  "empty or not 'text'");
            return EINVAL;
        }
        if (node->children != node->last)
        {
            ERROR("Too many children in 'description' element");
            return EINVAL;
        }
        cfg->descr = XML2CHAR_DUP(node->children->content);
        node = xmlNodeNext(node);
    }

    /* Get optional information about suites */
    while (node != NULL &&
           xmlStrcmp(node->name, CONST_CHAR2XML("suite")) == 0)
    {
        rc = alloc_and_get_test_suite_info(node, &cfg->suites, flags);
        if (rc != 0)
            return rc;
        node = xmlNodeNext(node);
    }

    /* Get optional information about requirements to be tested */
    rc = get_requirements(&node, &cfg->reqs);
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
        rc = alloc_and_get_run_item(node, cfg, 0,
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
            return EINVAL;
    }

    /* No more */
    if (node != NULL)
    {
        ERROR("Unexpected element '%s' in Tester configuration file",
              XML2CHAR(node->name));
        return EINVAL;
    }
#endif

    return 0;
}


/**
 * Parse and preprocess Test Package description file.
 *
 * @param cfg       Tester configuration context
 * @param pkg       Test Package description structure to be filled in
 *
 * @return Status code.
 */
static int
parse_test_package(tester_cfg *cfg, test_package *pkg)
{
    xmlParserCtxtPtr    parser;
    xmlDocPtr           doc;
    xmlError           *err;
    test_package       *cur_pkg_save;
    int                 rc;

    pkg->path = name_to_path(cfg, pkg->name, TRUE);
    if (pkg->path == NULL)
    {
        ERROR("Failed to make path to Test Package file by name and "
              "context");
        return EINVAL;
    }

    parser = xmlNewParserCtxt();
    if (parser == NULL)
    {
        ERROR("xmlNewParserCtxt() failed");
        return ENOMEM;
    }
    if ((doc = xmlCtxtReadFile(parser, pkg->path, NULL,
                               XML_PARSE_NOBLANKS |
                               XML_PARSE_XINCLUDE |
                               XML_PARSE_NONET)) == NULL)
    {
        err = xmlCtxtGetLastError(parser);
        ERROR("Error occured during parsing Test Package file:\n"
              "    %s:%d\n    %s", pkg->path, err->line, err->message);
        xmlFreeParserCtxt(parser);
        return EINVAL;
    }

    cur_pkg_save = cfg->cur_pkg;
    cfg->cur_pkg = pkg;
    rc = get_test_package(xmlDocGetRootElement(doc), cfg, pkg);
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
    cfg->cur_pkg = cur_pkg_save;
    
    xmlFreeParserCtxt(parser);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return rc;  
}


/**
 * Parse Tester configuratin file.
 *
 * @param cfg   Tester configuratin with not parsed file
 *
 * @return Status code
 */
int
tester_parse_config(tester_cfg *cfg, const tester_ctx *ctx)
{
    xmlParserCtxtPtr    parser;
    xmlDocPtr           doc;
    xmlError           *err;
    int                 rc;

    if (cfg->filename == NULL)
    {
        ERROR("Invalid configuration file name");
        return EINVAL;
    }
    parser = xmlNewParserCtxt();
    if (parser == NULL)
    {
        ERROR("xmlNewParserCtxt() failed");
        return ENOMEM;
    }
    if ((doc = xmlCtxtReadFile(parser, cfg->filename, NULL,
                               XML_PARSE_NOBLANKS |
                               XML_PARSE_XINCLUDE |
                               XML_PARSE_NONET)) == NULL)
    {
        err = xmlCtxtGetLastError(parser);
        ERROR("Error occured during parsing configuration file:\n"
              "    %s:%d\n    %s", cfg->filename, err->line, err->message);
        xmlFreeParserCtxt(parser);
        return EINVAL;
    }

    rc = get_tester_config(xmlDocGetRootElement(doc), cfg, ctx->flags);
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

