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
        for (var = ri->context->vars.tqh_first;
             var != NULL;
             var = var->links.tqe_next)
        {
            /* Session variable with true 'handdown' are arguments */
            if (var->handdown)
            {
                /* 
                 * Check that handdown variable is not overridden by
                 * argument with the same name.
                 */
                for (arg = ri->args.tqh_first;
                     arg != NULL && strcmp(var->name, arg->name) != 0;
                     arg = arg->links.tqe_next);

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
    for (arg = ri->args.tqh_first;
         arg != NULL;
         arg = arg->links.tqe_next)
    {
        rc = callback(arg, opaque);
        if (rc != 0)
            return rc;
    }

    return rc;
}


/**
 * Enumerate singleton values of the entity value in current variables
 * context.
 *
 * @param vars          Variables context or NULL
 * @param value         Entity value which may be not a singleton
 * @param callback      Function to be called for each singleton value
 * @param opaque        Data to be passed in callback function
 *
 * @return Status code.
 */
static te_errno
test_entity_value_enum_values(const test_vars_args *vars,
                              const test_entity_value *value,
                              test_entity_value_enum_cb callback,
                              void *opaque)
{
    assert(value != NULL);
    assert(callback != NULL);

    if (value->plain != NULL)
    {
        /* Typical singleton */
        return callback(value, opaque);
    }
    else if (value->ref != NULL)
    {
        assert(value->ref != value);
        /* 
         * Forward variable to the reference since it is in the same
         * context.
         */
        return test_entity_value_enum_values(vars, value->ref,
                                             callback, opaque);
    }
    else if (value->ext != NULL)
    {
        if (vars == NULL)
        {
            /*
             * No variables context, therefore, it is a singleton
             * with external value.
             */
            return callback(value, opaque);
        }
        else
        {
            test_var_arg *var;

            for (var = vars->tqh_first;
                 var != NULL && strcmp(value->ext, var->name) != 0;
                 var = var->links.tqe_next);

            if (var != NULL)
            {
                /* 
                 * Found variable with required name, enumerate its
                 * values, but variable does not have any context.
                 */
                return test_var_arg_enum_values(NULL, var,
                                                callback, opaque);
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
        return test_entity_values_enum(NULL, &value->type->values,
                                       callback, opaque);
    }
    else
    {
        assert(FALSE);
        return TE_RC(TE_TESTER, TE_EFAULT);
    }
}

/**
 * Enumerate values from the list in current variables context.
 *
 * @param vars          Variables context or NULL
 * @param values        List of values
 * @param callback      Function to be called for each singleton value
 * @param opaque        Data to be passed in callback function
 *
 * @return Status code.
 */
te_errno
test_entity_values_enum(const test_vars_args *vars,
                        const test_entity_values *values,
                        test_entity_value_enum_cb callback,
                        void *opaque)
{
    te_errno                    rc = TE_RC(TE_TESTER, TE_ENOENT);
    const test_entity_value    *v;

    ENTRY("vars=%p values=%p callback=%p opaque=%p",
          vars, values, callback, opaque);

    assert(values != NULL);

    for (v = values->head.tqh_first; v != NULL; v = v->links.tqe_next)
    {
        rc = test_entity_value_enum_values(vars, v, callback, opaque);
        if (rc != 0)
            break;
    }

    EXIT("%r", rc);
    return rc;
}

/* See the description in tester_conf.h */
te_errno
test_var_arg_enum_values(const run_item *ri, const test_var_arg *va,
                         test_entity_value_enum_cb callback, void *opaque)
{
    assert(va != NULL);

    if (va->values.head.tqh_first == NULL)
    {
        assert(va->type != NULL);
        return test_entity_values_enum(NULL, &va->type->values,
                                       callback, opaque);
    }
    else
    {
        const test_vars_args   *vars =
            (ri != NULL && ri->context != NULL) ? &ri->context->vars : NULL;

        return test_entity_values_enum(vars, &va->values, callback, opaque);
    }
}
