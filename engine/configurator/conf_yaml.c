/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief YAML configuration file processing facility
 *
 * Implementation.
 *
 * Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "conf_defs.h"
#include "conf_dh.h"
#include "conf_ta.h"
#include "conf_yaml.h"
#include "te_str.h"
#include "logic_expr.h"
#include "te_alloc.h"
#include "te_expand.h"
#include "te_file.h"

#include <ctype.h>
#include <libgen.h>
#include <yaml.h>

#define CS_YAML_ERR_PREFIX "YAML configuration file parser "

#define YAML_NODE_LINE_COLUMN_FMT   "line %lu column %lu"
#define YAML_NODE_LINE_COLUMN(_n)   \
    (_n)->start_mark.line + 1, (_n)->start_mark.column + 1

typedef enum cs_yaml_node_type {
    CS_YAML_NODE_TYPE_COMMENT,
    CS_YAML_NODE_TYPE_INCLUDE,
    CS_YAML_NODE_TYPE_COND,
    CS_YAML_NODE_TYPE_REGISTER,
    CS_YAML_NODE_TYPE_UNREGISTER,
    CS_YAML_NODE_TYPE_ADD,
    CS_YAML_NODE_TYPE_GET,
    CS_YAML_NODE_TYPE_DELETE,
    CS_YAML_NODE_TYPE_COPY,
    CS_YAML_NODE_TYPE_SET,
    CS_YAML_NODE_TYPE_REBOOT,/* Should it be added? */
} cs_yaml_node_type;

const te_enum_map cs_yaml_node_type_mapping[] = {
    {.name = "comment",    .value = CS_YAML_NODE_TYPE_COMMENT},
    {.name = "include",    .value = CS_YAML_NODE_TYPE_INCLUDE},
    {.name = "cond",       .value = CS_YAML_NODE_TYPE_COND},
    {.name = "register",   .value = CS_YAML_NODE_TYPE_REGISTER},
    {.name = "unregister", .value = CS_YAML_NODE_TYPE_UNREGISTER},
    {.name = "add",        .value = CS_YAML_NODE_TYPE_ADD},
    {.name = "get",        .value = CS_YAML_NODE_TYPE_GET},
    {.name = "delete",     .value = CS_YAML_NODE_TYPE_DELETE},
    {.name = "copy",       .value = CS_YAML_NODE_TYPE_COPY},
    {.name = "set",        .value = CS_YAML_NODE_TYPE_SET},
    {.name = "reboot_ta",  .value = CS_YAML_NODE_TYPE_REBOOT},
    TE_ENUM_MAP_END
};

typedef enum cs_yaml_instance_field {
    CS_YAML_INSTANCE_IF_COND,
    CS_YAML_INSTANCE_OID,
    CS_YAML_INSTANCE_VALUE,
} cs_yaml_instance_field;

const te_enum_map cs_yaml_instance_fields_mapping[] = {
    {.name = "if",      .value = CS_YAML_INSTANCE_IF_COND},
    {.name = "oid",     .value = CS_YAML_INSTANCE_OID},
    {.name = "value",   .value = CS_YAML_INSTANCE_VALUE},
    TE_ENUM_MAP_END
};

typedef enum cs_yaml_cond_field {
    CS_YAML_COND_IF_COND,
    CS_YAML_COND_THEN_COND,
    CS_YAML_COND_ELSE_COND,
} cs_yaml_cond_field;

const te_enum_map cs_yaml_cond_fields_mapping[] = {
    {.name = "if",      .value = CS_YAML_COND_IF_COND},
    {.name = "then",    .value = CS_YAML_COND_THEN_COND},
    {.name = "else",    .value = CS_YAML_COND_ELSE_COND},
    TE_ENUM_MAP_END
};

typedef enum cs_yaml_object_field {
    CS_YAML_OBJECT_D,
    CS_YAML_OBJECT_OID,
    CS_YAML_OBJECT_ACCESS,
    CS_YAML_OBJECT_TYPE,
    CS_YAML_OBJECT_UNIT,
    CS_YAML_OBJECT_DEF_VAL,
    CS_YAML_OBJECT_VOLAT,
    CS_YAML_OBJECT_SUBSTITUTION,
    CS_YAML_OBJECT_NO_PARENT_DEP,
    CS_YAML_OBJECT_DEPENDS,
} cs_yaml_object_field;

const te_enum_map cs_yaml_object_fields_mapping[] = {
    {.name = "d",             .value = CS_YAML_OBJECT_D},
    {.name = "oid",           .value = CS_YAML_OBJECT_OID},
    {.name = "access",        .value = CS_YAML_OBJECT_ACCESS},
    {.name = "type",          .value = CS_YAML_OBJECT_TYPE},
    {.name = "unit",          .value = CS_YAML_OBJECT_UNIT},
    {.name = "default",       .value = CS_YAML_OBJECT_DEF_VAL},
    {.name = "volatile",      .value = CS_YAML_OBJECT_VOLAT},
    {.name = "substitution",  .value = CS_YAML_OBJECT_SUBSTITUTION},
    {.name = "parent_dep",    .value = CS_YAML_OBJECT_NO_PARENT_DEP},
    {.name = "depends",       .value = CS_YAML_OBJECT_DEPENDS},
    TE_ENUM_MAP_END
};

const te_enum_map cs_yaml_object_access_fields_mapping[] = {
    {.name = "read_write",    .value = CFG_READ_WRITE},
    {.name = "read_only",     .value = CFG_READ_ONLY},
    {.name = "read_create",   .value = CFG_READ_CREATE},
    TE_ENUM_MAP_END
};

const te_enum_map cs_yaml_bool_mapping[] = {
    {.name = "false",     .value = FALSE},
    {.name = "true",      .value = TRUE},
    TE_ENUM_MAP_END
};

const te_enum_map cs_yaml_object_no_parent_dep_mapping[] = {
    {.name = "yes",     .value = FALSE},
    {.name = "no",      .value = TRUE},
    TE_ENUM_MAP_END
};

typedef enum cs_yaml_object_depends_field {
    CS_YAML_OBJECT_DEPENDS_OID,
    CS_YAML_OBJECT_DEPENDS_SCOPE,
} cs_yaml_object_depends_field;

const te_enum_map cs_yaml_object_depends_fields_mapping[] = {
    {.name = "oid",    .value = CS_YAML_OBJECT_DEPENDS_OID},
    {.name = "scope",  .value = CS_YAML_OBJECT_DEPENDS_SCOPE},
    TE_ENUM_MAP_END
};

const te_enum_map cs_yaml_object_depends_scope_mapping[] = {
    {.name = "object",        .value = TRUE},
    {.name = "instance",      .value = FALSE},
    TE_ENUM_MAP_END
};

typedef struct parse_config_yaml_ctx {
    char            *file_path;
    yaml_document_t *doc;
    history_seq     *history;
    te_kvpair_h     *expand_vars;
    const char      *conf_dirs;
} parse_config_yaml_ctx;

typedef struct config_yaml_target_s {
    const char *command_name;
    const char *target_name;
} config_yaml_target_t;


static te_errno
get_val(const logic_expr *parsed, void *expand_vars, logic_expr_res *res)
{
    te_errno rc;
    int rc_errno;

    if (expand_vars != NULL)
        rc_errno = te_expand_kvpairs(parsed->u.value, NULL,
                                     (te_kvpair_h *)expand_vars,
                                     &res->value.simple);
    else
        rc_errno = te_expand_env_vars(parsed->u.value, NULL,
                                      &res->value.simple);
    if (rc_errno != 0)
    {
        rc = te_rc_os2te(rc_errno);
        goto out;
    }
    else
    {
        rc = 0;
    }
    res->res_type = LOGIC_EXPR_RES_SIMPLE;

out:
    return rc;
}

/**
 * Evaluate logical expression.
 *
 * @param str               String representation of the expression
 * @param res               Location for the result
 * @param expand_vars       List of key-value pairs for expansion in file,
 *                          @c NULL if environment variables are used for
 *                          substitutions
 *
 * @return Status code.
 */
static te_errno
parse_logic_expr_str(const char *str, te_bool *res, te_kvpair_h *expand_vars)
{
    logic_expr *parsed = NULL;
    logic_expr_res parsed_res;
    te_errno rc;

    rc = logic_expr_parse(str, &parsed);
    if (rc != 0)
    {
        ERROR("Failed to parse expression '%s'", str);
        goto out;
    }

    rc = logic_expr_eval(parsed, get_val, expand_vars, &parsed_res);
    if (rc != 0)
    {
        ERROR("Failed to evaluate expression '%s'", str);
        goto out;
    }

    if (parsed_res.res_type != LOGIC_EXPR_RES_BOOLEAN)
    {
        rc = TE_EINVAL;
        logic_expr_free_res(&parsed_res);
        goto out;
    }

    *res = parsed_res.value.boolean;

out:
    logic_expr_free(parsed);

    return rc;
}

static te_errno
parse_config_str(yaml_node_t *n, char **str)
{
    te_errno rc = 0;

    if (n->type == YAML_SCALAR_NODE)
    {
        *str = strdup((char *)n->data.scalar.value);
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX
              "the expected scalar node is not a scalar node");
        return TE_EINVAL;
    }

    return rc;
}

static te_errno
parse_config_str_by_mapping(yaml_node_t *n, uint8_t *num,
                            const te_enum_map mapping[])
{
    te_errno rc = 0;
    char *str = NULL;
    int i;

    rc = parse_config_str(n, &str);
    if (rc == 0)
    {
        i = te_enum_map_from_str(mapping, str, -1);
        if (i == -1)
            rc = TE_EINVAL;
        else
            *num = i;
    }
    free(str);
    return rc;
}

static te_errno
parse_config_inst(parse_config_yaml_ctx *ctx, yaml_node_t *n,
                  instance_type *inst)
{
    yaml_document_t *d = ctx->doc;
    te_errno rc = 0;

    if (n->type == YAML_MAPPING_NODE)
    {
        yaml_node_pair_t *pair = n->data.mapping.pairs.start;

        do {
            yaml_node_t *k = yaml_document_get_node(d, pair->key);
            yaml_node_t *v = yaml_document_get_node(d, pair->value);
            int inst_field_name;

            if (k->type != YAML_SCALAR_NODE || k->data.scalar.length == 0)
            {
                ERROR(CS_YAML_ERR_PREFIX
                      "the target attribute node is badly formatted");
                return TE_EINVAL;
            }
            inst_field_name = te_enum_map_from_str(
                                            cs_yaml_instance_fields_mapping,
                                            (const char *)k->data.scalar.value,
                                            -1);

            switch (inst_field_name)
            {
                case CS_YAML_INSTANCE_IF_COND:
                    rc = parse_config_str(v, &inst->if_cond);
                    break;

                case CS_YAML_INSTANCE_OID:
                    rc = parse_config_str(v, &inst->oid);
                    break;

                case CS_YAML_INSTANCE_VALUE:
                    rc = parse_config_str(v, &inst->value);
                    break;

                default:
                    ERROR(CS_YAML_ERR_PREFIX
                          "failed to recognise the command '%s'",
                          (const char *)k->data.scalar.value);
                    rc = TE_EINVAL;
            }
            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to process %s "
                      "attribute at " YAML_NODE_LINE_COLUMN_FMT "",
                      (const char *)k->data.scalar.value,
                      YAML_NODE_LINE_COLUMN(k));
                return rc;
            }
        } while (++pair < n->data.mapping.pairs.top);
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "the 'instance' node is not a mapping node");
        return TE_EINVAL;
    }
    if (inst->oid == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX
              "the 'oid' field is absent in 'instance' node");
        return TE_EINVAL;
    }

    return rc;
}

static te_errno
parse_config_yaml_dependency(yaml_document_t *d, yaml_node_t *n,
                             object_type *obj);

static te_errno
parse_config_obj(parse_config_yaml_ctx *ctx, yaml_node_t *n, object_type *obj)
{
    yaml_document_t *d = ctx->doc;
    te_errno rc = 0;

    if (n->type == YAML_MAPPING_NODE)
    {
        yaml_node_pair_t *pair = n->data.mapping.pairs.start;

        do {
            yaml_node_t *k = yaml_document_get_node(d, pair->key);
            yaml_node_t *v = yaml_document_get_node(d, pair->value);
            int obj_field_name;
            uint8_t temp;

            if (k->type != YAML_SCALAR_NODE || k->data.scalar.length == 0)
            {
                ERROR(CS_YAML_ERR_PREFIX
                      "the target attribute node is badly formatted");
                return TE_EINVAL;
            }
            obj_field_name = te_enum_map_from_str(
                                            cs_yaml_object_fields_mapping,
                                            (const char *)k->data.scalar.value,
                                            -1);

            switch (obj_field_name)
            {
                case CS_YAML_OBJECT_D:
                    rc = parse_config_str(v, &obj->d);
                    break;

                case CS_YAML_OBJECT_OID:
                    rc = parse_config_str(v, &obj->oid);
                    break;

                case CS_YAML_OBJECT_ACCESS:
                    rc = parse_config_str_by_mapping(v, &obj->access,
                                        cs_yaml_object_access_fields_mapping);
                    break;

                case CS_YAML_OBJECT_TYPE:
                    rc = parse_config_str_by_mapping(v, &obj->type,
                                                     cfg_cvt_mapping);
                    if (rc == 0 && obj->type == CVT_UNSPECIFIED)
                        rc = TE_EINVAL;
                    break;

                case CS_YAML_OBJECT_UNIT:
                    rc = parse_config_str_by_mapping(v, &temp,
                                                     cs_yaml_bool_mapping);
                    obj->unit = temp;
                    break;

                case CS_YAML_OBJECT_DEF_VAL:
                    rc = parse_config_str(v, &obj->def_val);
                    break;

                case CS_YAML_OBJECT_VOLAT:
                    rc = parse_config_str_by_mapping(v, &temp,
                                                     cs_yaml_bool_mapping);
                    obj->volat = temp;
                    break;

                case CS_YAML_OBJECT_SUBSTITUTION:
                    rc = parse_config_str_by_mapping(v, &temp,
                                                     cs_yaml_bool_mapping);
                    obj->substitution = temp;
                    break;

                case CS_YAML_OBJECT_NO_PARENT_DEP:
                    rc = parse_config_str_by_mapping(v, &temp,
                                          cs_yaml_object_no_parent_dep_mapping);
                    obj->type = temp;
                    break;

                case CS_YAML_OBJECT_DEPENDS:
                    rc = parse_config_yaml_dependency(ctx->doc, v, obj);
                    break;

                default:
                    ERROR(CS_YAML_ERR_PREFIX "failed to recognise the command '%s'",
                          (const char *)k->data.scalar.value);
                    rc = TE_EINVAL;
            }
            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to process %s "
                      "attribute at " YAML_NODE_LINE_COLUMN_FMT "",
                      (const char *)k->data.scalar.value,
                      YAML_NODE_LINE_COLUMN(k));
                return rc;
            }
        } while (++pair < n->data.mapping.pairs.top);
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX
              "the 'instance' node is not a mapping node");
        return TE_EINVAL;
    }
    if (obj->oid == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX
              "the 'oid' field is absent in 'instance' node");
        return TE_EINVAL;
    }

    return rc;
}

/**
 * Process an entry of the given dependency node
 *
 * @param  d       YAML document handle
 * @param  n       Handle of the parent node in the given document
 * @param  dep_ctx Entry context to store parsed properties
 *
 * @return         Status code
 */
static te_errno
parse_config_yaml_dependency_entry(yaml_document_t *d,
                                   yaml_node_t *n,
                                   depends_entry *dep)
{
    te_errno rc = 0;

    if (n->type == YAML_MAPPING_NODE)
    {
        yaml_node_pair_t *pair = n->data.mapping.pairs.start;

        do {
            yaml_node_t *k = yaml_document_get_node(d, pair->key);
            yaml_node_t *v = yaml_document_get_node(d, pair->value);
            int depends_field_name;

            if (k->type != YAML_SCALAR_NODE || k->data.scalar.length == 0)
            {
                ERROR(CS_YAML_ERR_PREFIX
                      "the 'depends' node to be badly formatted");
                return TE_EINVAL;
            }
            depends_field_name = te_enum_map_from_str(
                                        cs_yaml_object_depends_fields_mapping,
                                        (const char *)k->data.scalar.value,
                                        -1);

            switch (depends_field_name)
            {
                case CS_YAML_OBJECT_DEPENDS_OID:
                    rc = parse_config_str(v, &dep->oid);
                    break;

                case CS_YAML_OBJECT_DEPENDS_SCOPE:
                    rc = parse_config_str_by_mapping(v, &dep->scope,
                                        cs_yaml_object_depends_scope_mapping);
                    break;

                default:
                    ERROR(CS_YAML_ERR_PREFIX
                          "failed to recognise the command '%s'",
                          (const char *)k->data.scalar.value);
                    rc = TE_EINVAL;
            }

            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to process "
                      "attribute at " YAML_NODE_LINE_COLUMN_FMT "",
                      YAML_NODE_LINE_COLUMN(k));
                return TE_EINVAL;
            }
        } while (++pair < n->data.mapping.pairs.top);

        if (dep->oid == NULL)
        {
            ERROR(CS_YAML_ERR_PREFIX
                  "the 'oid' field is absent in 'depends' node");
            return TE_EINVAL;
        }
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "the dependency node is badly formatted");
        return TE_EINVAL;
    }

    return rc;
}

/**
 * Process a dependency node of the given parent node.
 *
 * @param d     YAML document handle
 * @param n     Handle of the parent node in the given document
 * @param c     Location for the result
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_dependency(yaml_document_t *d, yaml_node_t *n,
                             object_type *obj)
{
    te_errno        rc;

    if (n->type == YAML_SCALAR_NODE) /* Is it really used? */
    {
        if (n->data.scalar.length == 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "the dependency node is badly formatted");
            return TE_EINVAL;
        }

        obj->depends_count = 1;
        obj->depends = TE_ALLOC(sizeof(depends_entry));

        if (obj->depends == NULL) {
            ERROR(CS_YAML_ERR_PREFIX "failed to allocate memory");
            return TE_ENOMEM;
        }

        obj->depends->oid = strdup((char *)n->data.scalar.value);
    }
    else if (n->type == YAML_SEQUENCE_NODE)
    {
        yaml_node_item_t *item = n->data.sequence.items.start;
        unsigned int i = 0;

        obj->depends_count = 0;
        do {
            obj->depends_count++;
        } while (++item < n->data.sequence.items.top);
        obj->depends = TE_ALLOC(obj->depends_count * sizeof(depends_entry));

        item = n->data.sequence.items.start;
        do {
            yaml_node_t *in = yaml_document_get_node(d, *item);

            rc = parse_config_yaml_dependency_entry(d, in, &obj->depends[i]);
            if (rc != 0)
                return rc;

            i++;
        } while (++item < n->data.sequence.items.top);
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX "the dependency node is badly formatted");
        return TE_EINVAL;
    }

    return 0;
}

/**
 * Process the sequence of target nodes for the specified
 * command in the given YAML document.
 *
 * @param ctx               Current doc context
 * @param n                 Handle of the target sequence in the given YAML
 *                          document
 * @param xn_cmd            Handle of command node in the XML document being
 *                          created
 * @param cmd               String representation of command
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_cmd_process_targets(parse_config_yaml_ctx *ctx,
                                      yaml_node_t *n, history_entry *h_entry,
                                      cs_yaml_node_type node_type)
{
    yaml_document_t *d = ctx->doc;
    yaml_node_item_t *item = n->data.sequence.items.start;
    unsigned int count = 0;
    unsigned int i = 0;

    if (n->type != YAML_SEQUENCE_NODE)
    {
        ERROR(CS_YAML_ERR_PREFIX
              "the %s command's list of targets is badly formatted",
              te_enum_map_from_any_value(cs_yaml_node_type_mapping, node_type,
                                         "unknown"));
        return TE_EINVAL;
    }
    do {
        count++;
    } while (++item < n->data.sequence.items.top);
/* should I deal with with count = 0 also? */
    switch (node_type)
    {

#define CASE_INITIATE(enum_part_, struct_part_, type_) \
        case CS_YAML_NODE_TYPE_ ## enum_part_:                          \
            h_entry-> struct_part_ ## _count = count;                   \
            h_entry-> struct_part_ = TE_ALLOC(count * sizeof(type_));   \
            break

        CASE_INITIATE(INCLUDE, incl, char *);
        CASE_INITIATE(REGISTER, reg, object_type);
        CASE_INITIATE(UNREGISTER, unreg, object_type);
        CASE_INITIATE(ADD, add, instance_type);
        CASE_INITIATE(GET, get, instance_type);
        CASE_INITIATE(DELETE, delete, instance_type);
        CASE_INITIATE(COPY, copy, instance_type);
        CASE_INITIATE(SET, set, instance_type);

#undef CASE_INITIATE

        default:
           assert(0);
    }

    item = n->data.sequence.items.start;
    do {
        yaml_node_t *in = yaml_document_get_node(d, *item);
        te_errno     rc = 0;

        /*
         * Calling parse_config_str(),
         * parse_config_obj(),
         * parse_config_inst().
         */
        switch (node_type)
        {

#define CASE_PARSE(enum_part_, struct_part_, target_) \
            case CS_YAML_NODE_TYPE_ ## enum_part_:                          \
                rc = parse_config_ ## target_(ctx, in,                      \
                                              &h_entry-> struct_part_[i]);  \
                break

            case CS_YAML_NODE_TYPE_INCLUDE:
                rc = parse_config_str(in, &h_entry->incl[i]);
                break;
            CASE_PARSE(REGISTER, reg, obj);
            CASE_PARSE(UNREGISTER, unreg, obj);
            CASE_PARSE(ADD, add, inst);
            CASE_PARSE(GET, get, inst);
            CASE_PARSE(DELETE, delete, inst);
            CASE_PARSE(COPY, copy, inst);
            CASE_PARSE(SET, set, inst);

#undef CASE_PARSE

            default:
               assert(0);
        }
        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "failed to process the target in the "
                  "%s command's list at " YAML_NODE_LINE_COLUMN_FMT "",
                  te_enum_map_from_any_value(cs_yaml_node_type_mapping,
                                             node_type,
                                             "unknown"),
                  YAML_NODE_LINE_COLUMN(in));
            return rc;
        }
        i++;
    } while (++item < n->data.sequence.items.top);

    return 0;
}

static te_errno
parse_config_root_seq(parse_config_yaml_ctx *ctx, history_seq *history,
                          yaml_node_t *parent);

static te_errno
parse_config_yaml_only(const char *filename, te_kvpair_h *expand_vars,
                       history_seq *history_root, const char *conf_dirs);

static te_errno
parse_config_yaml_cond(parse_config_yaml_ctx *ctx, yaml_node_t *n,
                       history_entry *h_entry)
{
    yaml_document_t *d = ctx->doc;
    te_errno rc = 0;

    h_entry->cond = TE_ALLOC(sizeof(cond_entry));
    yaml_node_pair_t *pair = n->data.mapping.pairs.start;
    do {
        yaml_node_t *k = yaml_document_get_node(d, pair->key);
        yaml_node_t *v = yaml_document_get_node(d, pair->value);
        int cond_field_name;

        if (k->type != YAML_SCALAR_NODE || k->data.scalar.length == 0)
        {
            ERROR(CS_YAML_ERR_PREFIX
                  "the target attribute node is badly formatted");
            return TE_EINVAL;
        }

        cond_field_name = te_enum_map_from_str(cs_yaml_cond_fields_mapping,
                                           (const char *)k->data.scalar.value,
                                           -1);

        switch (cond_field_name)
        {
            case CS_YAML_COND_IF_COND:
                rc = parse_config_str(v, &h_entry->cond->if_cond);
                if (rc != 0)
                {
                    ERROR(CS_YAML_ERR_PREFIX
                          "the 'if' node in 'cond' node is badly formatted");
                }
                break;

            case CS_YAML_COND_THEN_COND:
                h_entry->cond->then_cond = TE_ALLOC(sizeof(history_seq));
                rc = parse_config_root_seq(ctx, h_entry->cond->then_cond, v);
                if (rc != 0)
                {
                    ERROR(CS_YAML_ERR_PREFIX
                          "the 'then' node in 'cond' node is badly formatted");
                }
                break;

            case CS_YAML_COND_ELSE_COND:
                h_entry->cond->else_cond = TE_ALLOC(sizeof(history_seq));
                rc = parse_config_root_seq(ctx, h_entry->cond->else_cond, v);
                if (rc != 0)
                {
                    ERROR(CS_YAML_ERR_PREFIX
                          "the 'else' node in 'cond' node is badly formatted");
                }
                break;

            default:
                ERROR(CS_YAML_ERR_PREFIX
                      "failed to recognize 'cond' command's child '%s'",
                      (const char *)k->data.scalar.value);
                rc = TE_EINVAL;
        }

        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "detected some error(s) in the 'cond' "
                  "command's nested node at " YAML_NODE_LINE_COLUMN_FMT "",
                  YAML_NODE_LINE_COLUMN(k));
            return rc;
        }
    } while (++pair < n->data.mapping.pairs.top);

    return rc;
}

/**
 * Process dynamic history specified command in the given YAML document.
 *
 * @param ctx               Current doc context
 * @param n                 Handle of the command node in the given YAML document
 * @param cmd               String representation of command
 *
 * @return Status code.
 */
static te_errno
parse_config_yaml_specified_cmd(parse_config_yaml_ctx *ctx, yaml_node_t *n,
                                history_entry *h_entry,
                                cs_yaml_node_type node_type)
{
    te_errno    rc = 0;

    if (n->type == YAML_SEQUENCE_NODE)
    {
        if (node_type == CS_YAML_NODE_TYPE_COMMENT ||
            node_type == CS_YAML_NODE_TYPE_COND ||
            node_type == CS_YAML_NODE_TYPE_REBOOT)
        {
            rc = TE_EINVAL;
            goto out;
        }
        rc = parse_config_yaml_cmd_process_targets(ctx, n, h_entry, node_type);
        if (rc != 0)
        {
            ERROR(CS_YAML_ERR_PREFIX "detected some error(s) in the '%s' "
                  "command's nested node at " YAML_NODE_LINE_COLUMN_FMT "",
                  te_enum_map_from_any_value(cs_yaml_node_type_mapping,
                                             node_type,
                                             "unknown"),
                  YAML_NODE_LINE_COLUMN(n));
            goto out;
        }
    }
    else if (n->type == YAML_MAPPING_NODE)
    {
        if (node_type != CS_YAML_NODE_TYPE_COND)
        {
            rc = TE_EINVAL;
            goto out;
        }
        rc = parse_config_yaml_cond(ctx, n, h_entry);
    }
    else if (n->type == YAML_SCALAR_NODE)
    {
        switch (node_type)
        {
            /*
             * Case of single included file, e.g.
             * - include: filename
             * And also comment and reboot.
             */
            case CS_YAML_NODE_TYPE_INCLUDE:
                h_entry->incl_count = 1;
                h_entry->incl = TE_ALLOC(sizeof(char *));
                rc = parse_config_str(n, h_entry->incl);
                if (rc != 0)
                    goto out;
                break;

            case CS_YAML_NODE_TYPE_COMMENT:
                rc = parse_config_str(n, &h_entry->comment);
                if (rc != 0)
                    goto out;
                break;

            case CS_YAML_NODE_TYPE_REBOOT:
                rc = parse_config_str(n, &h_entry->reboot_ta);
                if (rc != 0)
                    goto out;
                break;

            default:
                rc = TE_EINVAL;
                goto out;
        }
    }
    else
    {
        rc = TE_EINVAL;
        goto out;
    }
out:
    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX "the '%s' command node is badly formatted",
              te_enum_map_from_any_value(cs_yaml_node_type_mapping,
                                         node_type,
                                         "unknown"));
    }

    return rc;
}

static te_errno
parse_config_root_commands(parse_config_yaml_ctx *ctx, history_entry *h_entry,
                           yaml_node_t *n)
{
    yaml_document_t *d = ctx->doc;
    yaml_node_pair_t *pair = n->data.mapping.pairs.start;
    yaml_node_t *k = yaml_document_get_node(d, pair->key);
    yaml_node_t *v = yaml_document_get_node(d, pair->value);
    te_errno rc = 0;
    int node_type = te_enum_map_from_str(cs_yaml_node_type_mapping,
                                         (const char *)k->data.scalar.value,
                                         -1);

    if (node_type == -1)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to recognise the command '%s'",
              (const char *)k->data.scalar.value);
        rc = TE_EINVAL;
    }
    else
    {
         rc = parse_config_yaml_specified_cmd(ctx, v, h_entry, node_type);
    }

    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX "detected some error(s) in "
              "the command node at " YAML_NODE_LINE_COLUMN_FMT " in file %s",
              YAML_NODE_LINE_COLUMN(k), ctx->file_path);
    }

    return rc;
}

/**
 * Explore sequence of commands of the given parent node in the given YAML
 * document to detect and process dynamic history commands.
 *
 * @param ctx               Current doc context
 * @param parent            Parent node of sequence of commands
 *
 * @return Status code.
 */
static te_errno
parse_config_root_seq(parse_config_yaml_ctx *ctx, history_seq *history,
                      yaml_node_t *parent)
{
    yaml_document_t  *d = ctx->doc;
    yaml_node_item_t *item = NULL;
    te_errno          rc = 0;

    unsigned int i = 0;

    if (parent->type != YAML_SEQUENCE_NODE)
    {
        ERROR(CS_YAML_ERR_PREFIX "expected sequence node");
        return TE_EFMT;
    }

    history->entries_count = 0;
    item = parent->data.sequence.items.start;
    do {
    history->entries_count++;
    } while (++item < parent->data.sequence.items.top);

    if (history->entries_count > 0)
        history->entries = TE_ALLOC(history->entries_count *
                                  sizeof(history_entry));
    else
        history->entries = NULL;

    item = parent->data.sequence.items.start;
    for (i = 0; i < history->entries_count; i++)
    {
        yaml_node_t *n = yaml_document_get_node(d, *item);

        if (n->type != YAML_MAPPING_NODE)
        {
            ERROR(CS_YAML_ERR_PREFIX "the command node is badly formatted");
            rc = TE_EINVAL;
        }
        rc = parse_config_root_commands(ctx, &history->entries[i], n);
        if (rc != 0)
            break;
        item++;
    };

    return rc;
}

te_errno
parse_config_yaml(const char *filename, te_kvpair_h *expand_vars,
                  history_seq *history_root, const char *conf_dirs);

static te_errno
resolve_exp_vars_or_env(char **str, const te_kvpair_h *kvpairs)
{
    te_errno rc = 0;
    char **result = TE_ALLOC(sizeof(char *));

    if (*str != NULL)
    {
        if (kvpairs == NULL)
            rc = te_expand_env_vars(*str, NULL, result);
        else
            rc = te_expand_kvpairs(*str, NULL, kvpairs, result);
        if (rc == 0)
        {
            free(*str);
            *str = *result;
        }
        else
        {
            ERROR("Error substituting variables in %s : %s",
                  *str, strerror(rc));
            free(*str);
            *str = NULL;
        }
    }
    free(result);

    return rc;
}

static te_errno
reparse_instance_seq(parse_config_yaml_ctx *ctx, instance_type *a_inst,
                     unsigned int *count)
{
    te_errno rc = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    te_bool if_expr;

    for (i = 0; i < *count; i++)
    {
        if (a_inst[i].if_cond != NULL)
        {
            rc = parse_logic_expr_str(a_inst[i].if_cond, &if_expr,
                                      ctx->expand_vars);
            if (rc != 0)
            {
                ERROR(CS_YAML_ERR_PREFIX "failed to evaluate the expression "
                      "contained in the condition node");
                return rc;
            }
            if (!if_expr)
            {
                cfg_yaml_free_inst(&a_inst[i]);
            }
            else
            {
                free(a_inst[i].if_cond);
                a_inst[i].if_cond = NULL;
            }
        }
        else
        {
            if_expr = TRUE;
        }

        if (if_expr)
        {
            te_errno rc_resolve;

            rc_resolve = resolve_exp_vars_or_env(&a_inst[i].oid,
                                                 ctx->expand_vars);
            if (rc_resolve != 0)
            {
                ERROR("couldn't expand vars or env in oid");
                rc = te_rc_os2te(rc_resolve);
                return rc;
            }
            rc_resolve = resolve_exp_vars_or_env(&a_inst[i].value,
                                                 ctx->expand_vars);
            if (rc_resolve != 0)
            {
                ERROR("couldn't expand vars or env in value");
                rc = te_rc_os2te(rc_resolve);
                return rc;
            }
            a_inst[j] = a_inst[i];
            j++;
        }
    }
    *count = j;

    return rc;
}

/*
 * The value of *count doesn't changed. The pointer is used for uniformity
 * and to be used in one macro with instances.
 */
static te_errno
reparse_object_seq(parse_config_yaml_ctx *ctx, object_type *a_obj,
                   unsigned int *count)
{
    te_errno rc = 0;
    unsigned int i = 0;

    for (i = 0; i < *count; i++)
    {
        te_errno rc_resolve;

        rc_resolve = resolve_exp_vars_or_env(&a_obj[i].oid, ctx->expand_vars);
        if (rc_resolve != 0)
        {
            ERROR("couldn't expand vars or env in oid");
            rc = te_rc_os2te(rc_resolve);
            return rc;
        }
    }

    return rc;
}

static te_errno
parse_included_docs_to_array(parse_config_yaml_ctx *ctx, char *file_name,
                             history_seq *new_history)
{
    te_errno rc = 0;
    te_errno rc_resolve_pathname = 0;
    char *resolved_file_name = NULL;

    rc_resolve_pathname = te_file_resolve_pathname(file_name, ctx->conf_dirs,
                                                   F_OK, ctx->file_path,
                                                   &resolved_file_name);
    if (rc_resolve_pathname == 0)
    {
        rc = parse_config_yaml_only(resolved_file_name, ctx->expand_vars,
                                    new_history, ctx->conf_dirs);
    }
    else
    {
        ERROR(CS_YAML_ERR_PREFIX
              "document %s specified in 'include' node is not found. "
              "te_file_resolve_pathname() produce error %d",
              file_name, rc_resolve_pathname);
        rc = TE_EINVAL;
    }
    free(resolved_file_name);
    return rc;
}

static te_errno
add_hist_seq_to_hist_seq(parse_config_yaml_ctx *ctx, unsigned int i,
                         history_seq *array_h_seq, unsigned int array_size)
{
    te_errno rc = 0;
    history_entry *tail = NULL;
    unsigned int j;
    unsigned int counter = ctx->history->entries_count - 1;
    unsigned int tail_count = ctx->history->entries_count - 1 - i;

    if (tail_count > 0)
    {
        tail = TE_ALLOC(tail_count * sizeof(history_entry));
        memcpy(tail, ctx->history->entries + i + 1,
               tail_count * sizeof(history_entry));
    }

    for (j = 0; j< array_size; j++)
        counter += array_h_seq[j].entries_count;

    ctx->history->entries_count = counter;
    if (counter > 0)
    {
        ctx->history->entries = realloc(ctx->history->entries,
                                        sizeof(history_entry) * counter);
        if (ctx->history->entries == NULL)
        {
            ERROR("Failed to reallocate memory");
            return TE_ENOMEM;
        }
    }
    else
    {
        cfg_yaml_free_hist_seq(ctx->history);
    }

    counter = i;

    for (j = 0; j < array_size; j++)
    {
        memcpy(ctx->history->entries + counter, array_h_seq[j].entries,
               (array_h_seq[j].entries_count) * sizeof(history_entry));
        free(array_h_seq[j].entries);
        counter += array_h_seq[j].entries_count;
    }
    free(array_h_seq);
    if (tail_count > 0)
    {
        memcpy(ctx->history->entries + counter, tail,
               (tail_count) * sizeof(history_entry));
        free(tail);
    }

    return rc;
}

static te_errno
reparse_include(parse_config_yaml_ctx *ctx, unsigned int i)
{
    te_errno rc = 0;
    unsigned int j;
    history_seq *history = ctx->history;
    history_seq *array_h_seq;
    unsigned int num_incl = history->entries[i].incl_count;

    array_h_seq = TE_ALLOC(sizeof(history_seq) * num_incl);

    for (j = 0; j< num_incl; j++)
    {
        rc = parse_included_docs_to_array(ctx, history->entries[i].incl[j],
                                          &array_h_seq[j]);
        if (rc != 0)
        {
            ERROR("Failed to include files");
            return rc;
        }
    }

    rc = add_hist_seq_to_hist_seq(ctx, i, array_h_seq, num_incl);

    return rc;
}

static te_errno
reparse_cond(parse_config_yaml_ctx *ctx, history_seq *history, unsigned int i)
{
    te_errno rc = 0;
    te_bool if_expr;
    cond_entry *cond = history->entries[i].cond;

    rc = parse_logic_expr_str(cond->if_cond, &if_expr,
                              ctx->expand_vars);
    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to evaluate the expression "
              "contained in the condition node");
        return rc;
    }
    free(cond->if_cond);

    if (if_expr)
    {
        if (cond->else_cond != NULL)
        {
            cfg_yaml_free_hist_seq(cond->else_cond);
            free(cond->else_cond);
        }
        if (cond->then_cond != NULL)
        {
            add_hist_seq_to_hist_seq(ctx, i, cond->then_cond, 1);
        }
        else
        {
            add_hist_seq_to_hist_seq(ctx, i, NULL, 0);
        }
    }
    else
    {
        if (history->entries[i].cond->then_cond != NULL)
        {
            cfg_yaml_free_hist_seq(cond->then_cond);
            free(cond->then_cond);
        }
        if (cond->else_cond != NULL)
        {
            add_hist_seq_to_hist_seq(ctx, i, cond->else_cond, 1);
        }
        else
        {
            add_hist_seq_to_hist_seq(ctx, i, NULL, 0);
        }
    }
    free(cond);

    return rc;
}

static te_errno
reparse_config_history_entry(parse_config_yaml_ctx *ctx,
                                 unsigned int *p_i)
{
    te_errno rc = 0;
    history_seq *history = ctx->history;
    unsigned int i = *p_i;

    if (history->entries[i].comment != NULL)
    {
        free(history->entries[i].comment);
        history->entries[i].comment = NULL;
    }
    else if (history->entries[i].incl != NULL)
    {
        rc = reparse_include(ctx, i);
        (*p_i)--;
    }
    else if (history->entries[i].cond != NULL)
    {
        rc = reparse_cond(ctx, history, i);
        (*p_i)--;
    }
    /* reparse_instance_seq() and reparse_object_seq() */
#define PROCESS_SEQUENCE(scope_, cmd_) \
    else if (history->entries[i]. cmd_ != NULL)                              \
    {                                                                        \
        rc = reparse_ ## scope_ ##_seq(ctx, history->entries[i]. cmd_ ,      \
                                &history->entries[i]. cmd_ ## _count);       \
        if (rc != 0)                                                         \
        {                                                                    \
            ERROR("Failed to process one of the " #scope_ " in " #cmd_);     \
            return rc;                                                       \
        }                                                                    \
    }

    PROCESS_SEQUENCE(instance, add)
    PROCESS_SEQUENCE(instance, get)
    PROCESS_SEQUENCE(instance, delete)
    PROCESS_SEQUENCE(instance, copy)
    PROCESS_SEQUENCE(instance, set)
    PROCESS_SEQUENCE(object, reg)
    PROCESS_SEQUENCE(object, unreg)

#undef PROCESS_INSTANCE_SEQ

    return rc;
}


static te_errno
reparse_config_root_seq(parse_config_yaml_ctx *ctx)
{
    te_errno rc = 0;
    unsigned int i;

    for (i = 0; i< ctx->history->entries_count; i++)
    {
        if (rc == 0)
            rc = reparse_config_history_entry(ctx, &i);
    }

    return rc;
}

static te_errno
parse_config_yaml_file_to_seq(parse_config_yaml_ctx *ctx)
{
    FILE                   *f = NULL;
    yaml_parser_t           parser;
    yaml_document_t         dy;
    yaml_node_t            *root = NULL;
    te_errno                rc = 0;
    char                   *current_yaml_file_path;
    const char *filename = ctx->file_path;
    history_seq *history = ctx->history;

    f = fopen(filename, "rb");
    if (f == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to open the target file '%s'",
              filename);
        return TE_OS_RC(TE_CS, errno);
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);
    yaml_parser_load(&parser, &dy);
    fclose(f);

    current_yaml_file_path = strdup(filename);
    if (current_yaml_file_path == NULL)
    {
        rc = TE_ENOMEM;
        goto out;
    }

    root = yaml_document_get_root_node(&dy);
    if (root == NULL)
    {
        ERROR(CS_YAML_ERR_PREFIX "failed to get the root node in file %s'",
              filename);
        rc = TE_EINVAL;
        goto out;
    }

    if (root->type == YAML_SCALAR_NODE &&
        root->data.scalar.value[0] == '\0')
    {
        INFO(CS_YAML_ERR_PREFIX "empty file '%s'", filename);
        rc = 0;
        goto out;
    }

    ctx->doc = &dy;

    rc = parse_config_root_seq(ctx, history, root);
    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX
              "encountered some error(s) on file '%s' processing",
              filename);
        goto out;
    }

out:
    yaml_document_delete(&dy);
    yaml_parser_delete(&parser);
    free(current_yaml_file_path);

    return rc;
}

static te_errno
parse_config_yaml_only(const char *filename, te_kvpair_h *expand_vars,
                       history_seq *history_root, const char *conf_dirs)
{
    te_errno rc = 0;
    char *current_yaml_file_path;

    parse_config_yaml_ctx ctx;

    current_yaml_file_path = strdup(filename);
    if (current_yaml_file_path == NULL)
    {
        TE_FATAL_ERROR();
        goto out;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.file_path = current_yaml_file_path;
    ctx.history = history_root;
    ctx.expand_vars = expand_vars;
    ctx.conf_dirs = conf_dirs;
    rc = parse_config_yaml_file_to_seq(&ctx);
    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX
              "encountered some error(s) on file '%s' processing",
              filename);
        goto out;
    }

    rc = reparse_config_root_seq(&ctx);
    history_root = ctx.history;
    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX
              "encountered some error(s) on file '%s' reparse processing",
              filename);
        goto out;
    }

out:
    free(current_yaml_file_path);

    return rc;
}

/* See description in 'conf_yaml.h' */
te_errno
parse_config_yaml(const char *filename, te_kvpair_h *expand_vars,
                  history_seq *history_root, const char *conf_dirs)
{
    te_errno rc = 0;

    rc = parse_config_yaml_only(filename, expand_vars, history_root, conf_dirs);
    if (rc != 0)
    {
        ERROR(CS_YAML_ERR_PREFIX
              "encountered some error(s) on file '%s' parse/reparse processing",
              filename);
        return rc;
    }

    rcf_log_cfg_changes(TRUE);
    rc = parse_config_dh_sync(history_root, expand_vars);
    rcf_log_cfg_changes(FALSE);

    return rc;
}
