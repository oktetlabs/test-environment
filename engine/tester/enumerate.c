/** @file
 * @brief Tester Subsystem
 *
 * Routines to enumerate arguments and values.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
#define TE_LGR_USER     "Enumerate"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_queue.h"
#include "te_errno.h"
#include "logger_api.h"

#include "tester_conf.h"
#include "tester_run.h"



/* See the description in tester_conf.h */
te_errno
test_run_item_enum_args(const run_item *ri, test_var_arg_enum_cb callback,
                        void *opaque)
{
    te_errno            rc = TE_RC(TE_TESTER, TE_ENOENT);
    const test_var_arg *var;
    const test_var_arg *arg;

    assert(ri != NULL);
    assert(callback != NULL);

    /* Run items in the configuration file do not have parent session */
    if (ri->context != NULL)
    {
        /*
         * First, enumerate session variables.
         */
        TAILQ_FOREACH(var, &ri->context->vars, links)
        {
            /* Session variable with true 'handdown' are arguments */
            if (var->handdown)
            {
                /* 
                 * Check that handdown variable is not overridden by
                 * argument with the same name.
                 */
                for (arg = TAILQ_FIRST(&ri->args);
                     arg != NULL && strcmp(var->name, arg->name) != 0;
                     arg = TAILQ_NEXT(arg, links));

                if (arg == NULL)
                {
                    /* Variable is not overridden */
                    rc = callback(var, opaque);
                    if (rc != 0)
                        return rc;
                }
            }
        }
    }

    /*
     * Second, enumerate run item arguments.
     */
    TAILQ_FOREACH(arg, &ri->args, links)
    {
        rc = callback(arg, opaque);
        if (rc != 0)
            return rc;
    }

    return rc;
}


/**
 * Data to be passed as opaque to test_run_item_find_arg_cb() function.
 */
typedef struct test_run_item_find_arg_cb_data {
    const run_item     *ri;         /**< Run item context */
    const char         *name;       /**< Name of the argument to find */
    unsigned int        n_iters;    /**< Number of outer iterations of
                                         the argument */
    unsigned int        n_values;   /**< Number of argument values */
    const test_var_arg *found;      /**< Found argument */
} test_run_item_find_arg_cb_data;

/**
 * Callback function for test_run_item_enum_args() routine to find
 * argument by name.
 *
 * The function complies with test_var_arg_enum_cb() prototype.
 *
 * @retval 0            Argument does not match
 * @retval TE_EEXIST    Argument found
 */
static te_errno
test_run_item_find_arg_cb(const test_var_arg *va, void *opaque)
{
    test_run_item_find_arg_cb_data *data = opaque;
    const test_var_arg_list        *list = NULL;

    if (va->list != NULL)
    {
        for (list = SLIST_FIRST(&data->ri->lists);
             list != NULL && strcmp(list->name, va->list) != 0;
             list = SLIST_NEXT(list, links));

        assert(list != NULL);
        data->n_values = list->len;
    }
    else
    {
        data->n_values = test_var_arg_values(va)->num;
    }

    if (strcmp(data->name, va->name) != 0)
    {
        if ((list == NULL) || (list->n_iters == data->n_iters))
        {
            data->n_iters *= data->n_values;
        }
        data->n_values = 0;
        return 0;
    }

    if (list != NULL)
    {
        /* 
         * Iteration of the list is started at position of the first
         * member.
         */
        data->n_iters = list->n_iters;
    }

    data->found = va;

    return TE_RC(TE_TESTER, TE_EEXIST);
}

/* See the description in tester_conf.h */
const test_var_arg *
test_run_item_find_arg(const run_item *ri, const char *name,
                       unsigned int *n_values, unsigned int *outer_iters)
{
    test_run_item_find_arg_cb_data  data;
    te_errno                        rc;

    data.ri = ri;
    data.name = name;
    data.n_iters = 1;

    rc = test_run_item_enum_args(ri, test_run_item_find_arg_cb, &data);
    if (TE_RC_GET_ERROR(rc) == TE_EEXIST)
    {
        if (n_values != NULL)
            *n_values = data.n_values;
        if (outer_iters != NULL)
            *outer_iters = data.n_iters;

        return data.found;
    }
    else if (rc == 0)
    {
        INFO("%s(): Argument '%s' not found in run item '%s' context",
             __FUNCTION__, name, test_get_name(ri));
    }
    else
    {
        ERROR("%s(): test_run_item_enum_args() failed unexpectedly: %r",
              __FUNCTION__, rc);
    }
    return NULL;
}


/**
 * Enumerate singleton values of the entity value in current variables
 * context.
 *
 * @param vars          Variables context or NULL
 * @param value         Entity value which may be not a singleton
 * @param callback      Function to be called for each singleton value
 * @param opaque        Data to be passed in callback function
 * @param enum_error_cb Function to be called on back path when
 *                      enumeration error occur (for example, when
 *                      value has been found)
 * @param ee_opaque     Opaque data for @a enum_error_cb function
 *
 * @return Status code.
 */
static te_errno
test_entity_value_enum_values(const test_vars_args *vars,
                              const test_entity_value *value,
                              test_entity_value_enum_cb callback,
                              void *opaque,
                              test_entity_value_enum_error_cb enum_error_cb,
                              void *ee_opaque)
{
    te_errno    rc;

    assert(value != NULL);
    assert(callback != NULL);

    if (value->plain != NULL)
    {
        /* Typical singleton */
        rc = callback(value, opaque);

        if (rc != 0 && enum_error_cb != NULL)
            enum_error_cb(value, rc, ee_opaque);
            
        return rc;
    }
    else if (value->ref != NULL)
    {
        assert(value->ref != value);
        /* 
         * Forward variable to the reference since it is in the same
         * context.
         */
        rc = test_entity_value_enum_values(vars, value->ref,
                                           callback, opaque,
                                           enum_error_cb, ee_opaque);

        if (rc != 0 && enum_error_cb != NULL)
            enum_error_cb(value, rc, ee_opaque);
            
        return rc;
    }
    else if (value->ext != NULL)
    {
        if (vars == NULL)
        {
            /*
             * No variables context, therefore, it is a singleton
             * with external value.
             */
            rc = callback(value, opaque);

            if (rc != 0 && enum_error_cb != NULL)
                enum_error_cb(value, rc, ee_opaque);
                
            return rc;
        }
        else
        {
            test_var_arg *var;

            for (var = TAILQ_FIRST(vars);
                 var != NULL && strcmp(value->ext, var->name) != 0;
                 var = TAILQ_NEXT(var, links));

            if (var != NULL)
            {
                /* 
                 * Found variable with required name, enumerate its
                 * values, but variable does not have any context.
                 */
                rc = test_var_arg_enum_values(NULL, var,
                                                callback, opaque,
                                                enum_error_cb, ee_opaque);

                if (rc != 0 && enum_error_cb != NULL)
                    enum_error_cb(value, rc, ee_opaque);
                    
                return rc;
            }
            else
            {
                ERROR("Cannot resolve reference to '%s'", value->ext);
                return TE_RC(TE_TESTER, TE_ESRCH);
            }
        }
    }
    else if (value->type != NULL)
    {
        VERB("%s(): Enumerate type '%s' values %p", __FUNCTION__,
             value->type->name, &value->type->values);
        /* 
         * Enumerate values of the specified type.
         * Type does not have variables context.
         */
        rc = test_entity_values_enum(NULL, &value->type->values,
                                     callback, opaque,
                                     enum_error_cb, ee_opaque);

        if (rc != 0 && enum_error_cb != NULL)
            enum_error_cb(value, rc, ee_opaque);
            
        return rc;
    }
    else
    {
        assert(FALSE);
        return TE_RC(TE_TESTER, TE_EFAULT);
    }
}

/* See the description in tester_conf.h */
te_errno
test_entity_values_enum(const test_vars_args            *vars,
                        const test_entity_values        *values,
                        test_entity_value_enum_cb        callback,
                        void                            *opaque,
                        test_entity_value_enum_error_cb  enum_error_cb,
                        void                            *ee_opaque)
{
    te_errno                    rc = TE_RC(TE_TESTER, TE_ENOENT);
    const test_entity_value    *v;

    ENTRY("vars=%p values=%p callback=%p opaque=%p",
          vars, values, callback, opaque);

    assert(values != NULL);

    TAILQ_FOREACH(v, &values->head, links)
    {
        rc = test_entity_value_enum_values(vars, v, callback, opaque,
                                           enum_error_cb, ee_opaque);
        if (rc != 0)
            break;
    }

    EXIT("%r", rc);
    return rc;
}

/* See the description in tester_conf.h */
te_errno
test_var_arg_enum_values(const run_item *ri, const test_var_arg *va,
                         test_entity_value_enum_cb callback, void *opaque,
                         test_entity_value_enum_error_cb enum_error_cb,
                         void *ee_opaque)
{
    assert(va != NULL);

    if (TAILQ_EMPTY(&va->values.head))
    {
        assert(va->type != NULL);
        return test_entity_values_enum(NULL, &va->type->values,
                                       callback, opaque,
                                       enum_error_cb, ee_opaque);
    }
    else
    {
        const test_vars_args   *vars =
            (ri != NULL && ri->context != NULL) ? &ri->context->vars : NULL;

        return test_entity_values_enum(vars, &va->values, callback, opaque,
                                       enum_error_cb, ee_opaque);
    }
}


/** Opaque data for test_var_arg_get_plain_value_cb() function */
typedef struct test_var_arg_get_plain_value_cb_data {

    unsigned int    search; /**< Index of the value to get */
    unsigned int    index;  /**< Index of the current value */

    const test_entity_value    *value;  /**< Found value */

} test_var_arg_get_plain_value_cb_data;

/**
 * Callback function to find argument value by index.
 *
 * The function complies with test_entity_value_enum_cb() prototype.
 */
static te_errno
test_var_arg_get_plain_value_cb(const test_entity_value *value,
                                void *opaque)
{
    test_var_arg_get_plain_value_cb_data   *data = opaque;

    if (data->index < data->search)
    {
        data->index++;
        return 0;
    }

    data->value = value;

    return TE_RC(TE_TESTER, TE_EEXIST);
}

/* See the description in tester_conf.h */
te_errno
test_var_arg_get_value(const run_item                   *ri,
                       const test_var_arg               *va,
                       const unsigned int                index,
                       test_entity_value_enum_error_cb   enum_error_cb,  
                       void                             *ee_opaque,
                       const test_entity_value         **value)
{
    test_var_arg_get_plain_value_cb_data    data;
    te_errno                                rc;
    unsigned int                            n_values;

    /*
     * Assume that request is corrent and try to find first.
     */

    data.search = index;
    data.index = 0;

    rc = test_var_arg_enum_values(ri, va,
                                  test_var_arg_get_plain_value_cb,
                                  &data, enum_error_cb, ee_opaque);
    if (TE_RC_GET_ERROR(rc) == TE_EEXIST)
    {
        *value = data.value;
        /* It has to be either 'plain' or 'ext' */
        assert((*value)->plain != NULL || (*value)->ext != NULL);
        return 0;
    }
    else if (rc != 0)
    {
        ERROR("%s(): test_var_arg_enum_values() failed unexpectedly: %r",
              __FUNCTION__, rc);
        return rc;
    }

    /*
     * Opps, request seems to be incorrect or list preferred value
     * should be used.
     */

    if (va->list != NULL)
    {
        const test_var_arg_list *list;

        for (list = SLIST_FIRST(&ri->lists);
             list != NULL && strcmp(list->name, va->list) != 0;
             list = SLIST_NEXT(list, links));

        assert(list != NULL);
        n_values = list->len;
    }
    else
    {
        n_values = test_var_arg_values(va)->num;
    }

    if (index >= n_values)
    {
        ERROR("%s(): Run item '%s' argument '%s' value with too big "
              "index %u is requested", __FUNCTION__, test_get_name(ri),
              va->name, index);
        return TE_RC(TE_TESTER, TE_E2BIG);
    }

    /* List preferred value should be used */
    assert(va->list != NULL);
    assert(index >= test_var_arg_values(va)->num);

    if (va->preferred == NULL)
        *value = TAILQ_FIRST(&test_var_arg_values(va)->head);
    else
        *value = va->preferred;

    /* Preferred value has to be a singleton */
    assert((*value)->plain != NULL || (*value)->ref != NULL ||
           (*value)->ext != NULL);
    while ((*value)->ref != NULL)
    {
        *value = (*value)->ref;
        assert((*value)->plain != NULL || (*value)->ref != NULL ||
               (*value)->ext != NULL);
    }
    /* Now it is either 'plain' or 'ext' */
    assert((*value)->plain != NULL || (*value)->ext != NULL);

    return 0;
}
