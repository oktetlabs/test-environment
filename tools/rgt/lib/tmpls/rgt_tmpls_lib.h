/** @file
 * @brief Test Environment: Generic interface for working with
 * output templates.
 *
 * The module provides functions for parsing and output templates
 *
 * There is a set of templates each for a particular log piece(s) such as
 * document start/end, log start/end and so on.
 * The templates can be constant or parametrized strings, which contain places
 * that should be filled in with appropriate values gathered from the parsing 
 * context. For example parametrized template is needed for start log message
 * parts, which should be expanded with log level, entity and user name of 
 * a log message.
 * The places where context-specific values should be output is marked with
 * a pair of "@@" signs (doubled "at" sign) and the name of a context variable
 * between them, whose value should be expanded there.
 *
 * For example:
 *
 * This an example of how a parametrized template might look:
 * <p>
 * Here @@user@@ should be inserted a value of context-specific variable "user"
 * </p>
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef RGT_TMPLS_LIB_H_
#define RGT_TMPLS_LIB_H_

#include <stdio.h>

#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

/** Delimiter to be used for determination variables in templates */
#define RGT_TMPLS_VAR_DELIMETER "@@"

/** The list of template types */
enum e_log_part {
    LOG_PART_DOCUMENT_START = 0,
    LOG_PART_DOCUMENT_END   = 1,
    LOG_PART_LOG_MSG_START  = 2,
    LOG_PART_LOG_MSG_END    = 3,
    LOG_PART_MEM_DUMP_START = 4,
    LOG_PART_MEM_DUMP_END   = 5,
    LOG_PART_MEM_ROW_START  = 6,
    LOG_PART_MEM_ROW_END    = 7,
    LOG_PART_MEM_ELEM_START = 8,
    LOG_PART_MEM_ELEM_END   = 9,
    LOG_PART_MEM_ELEM_EMPTY = 10,
    LOG_PART_BR             = 11,
};

/** The total number of all templates (depends on "enum e_log_part") */
#define RGT_TMPLS_NUM         12

/**
 * Each template before using is parsed onto blocks each block can be whether
 * a constant sting or a variable value.
 * For example the following single line template:
 *  The value of A is @@A@@, the value of B is @@B@@.
 * Will be split into five blocks:
 *  1. const string: "The value of A is "
 *  2. a value of variable: $A
 *  3. const string: ", the value of B is "
 *  4. a value of variable: $B
 *  5. const string: "."
 *
 */

/** Specifies the type of block */
enum e_block_type {
    BLK_TYPE_STR, /**< A block contains a constant string */
    BLK_TYPE_VAR, /**< A block contains a value of a variable */
};

/** The structure represents one block of a template */
struct block {
    enum e_block_type blk_type; /**< Defines the type of a block */

    /** The union field to access depends on the value of blk_type field */
    union {
        const char *start_data; /**< Pointer to the beginning of a constant 
                                     string, which should be output in 
                                     this block */
        const char *var_name; /**< Name of a variable whose value should be 
                                   output in this block */
    };
};

struct log_tmpl {
    char         *fname; /**< Themplate file name */
    char         *raw_ptr; /**< Pointer to the beginning of the memory 
                                allocated under a log template */
    struct block *blocks; /**< Pointer to an array of blocks */
    int           n_blocks; /**< Total number of blocks */
};


int rgt_tmpls_lib_parse(const char **files, struct log_tmpl *tmpls,
                        int tmpl_num);
void rgt_tmpls_lib_free(struct log_tmpl *tmpls, int tmpl_num);
int rgt_tmpls_lib_output(FILE *out_fd, struct log_tmpl *tmpl,
                         const char **vars, const char **user_vars);

#endif /* RGT_TMPLS_LIB_H_ */
