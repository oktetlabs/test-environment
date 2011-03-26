/** @file
 * @brief Logical Expressions
 *
 * Interface definitions.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOGIC_EXPR_H__
#define __TE_LOGIC_EXPR_H__

#include "te_errno.h"
#include "tq_string.h"


#ifdef __cplusplus
extern "C" {
#endif


/** Types of expression elements, */
typedef enum logic_expr_type {
    LOGIC_EXPR_VALUE,   /**< Simple value */
    LOGIC_EXPR_NOT,     /**< Logical 'not' */
    LOGIC_EXPR_AND,     /**< Logical 'and' */
    LOGIC_EXPR_OR,      /**< Logical 'or' */
    LOGIC_EXPR_GT,      /**< Greater */
    LOGIC_EXPR_GE,      /**< Greater or equal */
    LOGIC_EXPR_LT,      /**< Less */
    LOGIC_EXPR_LE,      /**< Less or equeal */
} logic_expr_type;

/** Element of the requirements expression */
typedef struct logic_expr {
    logic_expr_type  type;    /**< Type of expression element */
    union {
        char                     *value;  /**< Simple value */
        struct logic_expr        *unary;  /**< Unary expression */
        struct {
            struct logic_expr    *lhv;        /**< Left hand value */
            struct logic_expr    *rhv;        /**< Right hand value */
        } binary;                           /**< Binary expression */
    } u;    /**< Type specific data */
} logic_expr;


/**
 * Parse string-based logical expression.
 *
 * @param str       String to be parsed
 * @param expr      Location for pointer to parsed expression
 *
 * @return Status code.
 */
extern te_errno logic_expr_parse(const char *str, logic_expr **expr);

/**
 * Create binary logical expression.
 *
 * @param type      AND or OR
 * @param lhv       Left hand value
 * @param rhv       Right hand value
 *
 * @return Pointer to allocated value or NULL.
 */
extern logic_expr *logic_expr_binary(logic_expr_type type,
                                     logic_expr *lhv, logic_expr *rhv);

/**
 * Non-recursive free of logical expressions.
 *
 * @param expr      Expression to be freed
 */
extern void logic_expr_free_nr(logic_expr *expr);

/**
 * Free logical expression.
 *
 * @param expr      Expression to be freed
 */
extern void logic_expr_free(logic_expr *expr);

/**
 * Is set of string match logical expression?
 *
 * @param re        Logical expression
 * @param set       Set of strings
 *
 * @return Index in the set (starting from 1) or 0.
 */
extern int logic_expr_match(const logic_expr *re, const tqh_strings *set);

/**
 * Destroy logical expressions lexer global context.
 */
extern int logic_expr_int_lex_destroy(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOGIC_EXPR_H__ */
