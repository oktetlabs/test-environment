/** @file
 * @brief Logical Expressions
 *
 * Interface definitions.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#ifndef __TE_LOGIC_EXPR_H__
#define __TE_LOGIC_EXPR_H__

#include "te_errno.h"
#include "tq_string.h"


#ifdef __cplusplus
extern "C" {
#endif

#define LOGIC_EXPR_RES_INIT { .res_type = LOGIC_EXPR_RES_UNSPEC }

/** Types of expression elements, */
typedef enum logic_expr_type {
    LOGIC_EXPR_VALUE,   /**< Simple value */
    LOGIC_EXPR_NOT,     /**< Logical 'not' */
    LOGIC_EXPR_AND,     /**< Logical 'and' */
    LOGIC_EXPR_OR,      /**< Logical 'or' */
    LOGIC_EXPR_GT,      /**< Greater */
    LOGIC_EXPR_GE,      /**< Greater or equal */
    LOGIC_EXPR_LT,      /**< Less */
    LOGIC_EXPR_LE,      /**< Less or equal */
    LOGIC_EXPR_EQ,      /**< Equal */
    LOGIC_EXPR_NEQ,     /**< Not equal */
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

typedef enum logic_expr_res_type {
    LOGIC_EXPR_RES_UNSPEC,
    LOGIC_EXPR_RES_SIMPLE,      /**< Simple value  */
    LOGIC_EXPR_RES_BOOLEAN      /**< Boolean value */
} logic_expr_res_type;

typedef struct logic_expr_res {
    logic_expr_res_type res_type;
    union {
        char       *simple;
        te_bool     boolean;
    } value;
} logic_expr_res;

/**
 * Evaluate simple value node of logical expression.
 *
 * @param parsed        Simple value node
 * @param cookie        Callback opaque data
 * @param res           Result
 *
 * @return  Status code
 */
typedef te_errno (logic_expr_get_val)(const logic_expr *parsed,
                                      void *cookie,
                                      logic_expr_res *res);

/**
 * Evaluate logical expression tree.
 *
 * @param parsed        Logical expression tree
 * @param get_val       Function to evaluate simple value nodes
 * @param cookie        Callback opaque data
 * @param res           Result
 *
 * @return  Status code
 *
 * @note Evaluation is not lazy.
 *
 */
extern te_errno logic_expr_eval(const logic_expr *parsed,
                                logic_expr_get_val *get_val,
                                void *cookie,
                                logic_expr_res *res);
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
 * Free logical expression result.
 *
 * @param res      Logical expression result
 */
extern void logic_expr_free_res(logic_expr_res *res);

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
 * Duplicate logical expression.
 *
 * @param expr      Expression to be duplicated
 *
 * @return
 *      Duplicate logical expression or NULL
 */
extern logic_expr *logic_expr_dup(logic_expr *expr);

/**
 * Is set of string match logical expression?
 *
 * @param re        Logical expression
 * @param set       Set of strings
 *
 * @return Value characterizing degree of matching or -1 if
 *         there is no matching
 */
extern int logic_expr_match(const logic_expr *re, const tqh_strings *set);

/**
 * Destroy logical expressions lexer global context.
 */
extern int logic_expr_int_lex_destroy(void);

/**
 * Transform logic expression into disjunctive
 * normal form:
 *          ||
 *        /     \
 *      &&       ||
 *     /  \     /  \
 *    &&   x   &&   ||
 *   /  \     ...  /  \
 * ...   y        &&   ...
 *               ...
 *
 * @param expr      Logical expression
 * @param comp_func Function comparing logical expressions
 *
 * @retrurn 0 on success or error code
 */
extern te_errno logic_expr_dnf(logic_expr **expr,
                               int (*comp_func)(logic_expr *,
                                                logic_expr *));

/**
 * Split DNF in disjuncts, copy them to array.
 *
 * @param dnf       DNF
 * @param array     Pointer to array of disjuncts to be set
 * @param size      Pointer to size of array to be set
 *
 * @return 0 on success or error code
 */
extern te_errno logic_expr_dnf_split(logic_expr *dnf, logic_expr ***array,
                                     int *size);
/**
 * Get string representation of logical expression.
 *
 * @param expr  logical expression
 *
 * @return String representation
 */
extern char *logic_expr_to_str(logic_expr *expr);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOGIC_EXPR_H__ */
