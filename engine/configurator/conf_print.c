/** @file
 * @brief Configurator
 *
 * Configurator database print out.
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
 * @author Igor Baryshev <Igor.Baryshev@oktetlabs.ru>
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include <stdio.h>
#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "conf_defs.h"

/* Minimal buffer size to be allocated */
#define BUF_SIZE_MIN    (16 * 1024)

/* Realloc coefficient */
#define BUF_GROW_COEFF  0.2

#define CHECK(x) \
    do {                                            \
        if (!(x))                                   \
        {                                           \
            free(buf);                              \
            buf = NULL;                             \
            offset = 0;                             \
            sz = BUF_SIZE_MIN;                      \
            return NULL;                            \
        }                                           \
    } while (0)

#define BUF_RESET \
    do {                                            \
            buf = NULL;                             \
            offset = 0;                             \
            sz = BUF_SIZE_MIN;                      \
    } while (0)

static char *obj_tree_bufprint(cfg_object *obj,   const int indent);
static char *ins_tree_bufprint(cfg_instance *ins, const int indent);
static char *obj_bufprint_deps(cfg_object *obj);
static char *bufprintf(char **p_buf, int *p_offset, size_t *p_sz,
                       const char *format, ...);

/**
 * log_msg() helper to print a log upon arrival of this type of msg.
 *
 * @param msg               Message (usually received by Configurator).
 * @param cfg_log_lvl       Level at which to log the message itself
 *                          (maybe different from  msg->log_lvl)
 *
 * @return
 */
void
cfg_db_tree_print_msg_log(cfg_tree_print_msg *msg,
                          const unsigned int cfg_log_lvl)
{
    char *flname;

    if (msg->flname_len != 0)
        flname = (msg->buf) + (msg->id_len);
    else
        flname = "NULL";

    LOG_MSG(cfg_log_lvl, "Msg: tree print request: root id: %s, "
            "output filename: %s, log level: %d\n",
            msg->buf, flname, msg->log_lvl);
}

/**
 * Starting from a given prefix, print a tree of objects or instances
 * into a file and(or) log.
 *
 * @param filename          output filename (NULL to skip)
 * @param log_lvl           TE log level (0 to skip)
 * @param id_fmt            a format string for the id of the root
 *                          from which we print.
 *
 * @return                  Status code.
 */
te_errno
cfg_db_tree_print(const char *filename,
                  const unsigned int log_lvl,
                  const char *id_fmt, ...)
{
    static char *func_name_prfx[2] = {"obj", "ins"};
    static char *title[2] = {"Objects", "Instances" };
    cfg_oid     *idsplit = NULL;
    char        *buf = NULL;
    FILE        *f;
    void        *root = NULL;
    int         which = 0;      /**< 0 for objects, 1 for instances */
    char        id[CFG_OID_MAX];
    size_t      id_len;
    va_list ap;

    if (id_fmt == NULL)
        return TE_RC(TE_CS, TE_EINVAL);
    va_start(ap, id_fmt);
    id_len = (size_t)vsnprintf(id, sizeof(id), id_fmt, ap);
    va_end(ap);
    if (id_len >= sizeof(id))
        return TE_RC(TE_CS, TE_EINVAL);

    if ((idsplit = cfg_convert_oid_str(id)) == NULL)
        return TE_RC(TE_CS, TE_EINVAL);
    if (idsplit->inst)
        which = 1;
    cfg_free_oid(idsplit);

    if (which == 0)
        root = (void *)cfg_get_obj_by_obj_id_str(id);
    else
        root = (void *)cfg_get_ins_by_ins_id_str(id);
    if (root == NULL)
    {
        TE_LOG(log_lvl, TE_LGR_ENTITY, TE_LGR_USER,
               "no node with id string: %s\n", id);
        return TE_RC(TE_CS, TE_EINVAL);
    }

    if (which == 0)
        buf = obj_tree_bufprint((cfg_object *)root, 0);
    else
        buf = ins_tree_bufprint((cfg_instance *)root, 0);
    if (buf == NULL)
    {
        ERROR("%s_tree_bufprint() failed\n", func_name_prfx[which]);
        return TE_RC(TE_CS, TE_ENOMEM);
    }
    /* Wanted to keep the "title" separate from the buffer: */
    if (log_lvl != 0)
        TE_LOG(log_lvl, TE_LGR_ENTITY, TE_LGR_USER,
               "tree of %s %s:\n%s", title[which], id, buf);
    if (filename != NULL)
    {
        if ((f = fopen(filename, "w")) == NULL)
            ERROR("Can't open file: %s", filename);
        fprintf(f, "tree of %s %s:\n%s", title[which], id, buf);
        fclose(f);
    }

    free(buf);
    return 0;
}



/**
 * Print (recursively) a tree of objects into the buffer.
 * Based on conf_main.c:print_otree() by Elena Vengerova.
 *
 * @param obj       the root of a tree.
 * @param indent    indentation for the current level.
 *
 * @return
 *     Pointer to the allocated buffer containing the printed tree string.
 *     NULL, if error occurred.
 */
static char *
obj_tree_bufprint(cfg_object *obj, const int indent)
{
    static size_t   sz     = BUF_SIZE_MIN;
    static char    *buf    = NULL;
    static int      offset = 0;
    int             i;
    char           *tmp;
    cfg_dependency *dep;

    for (i = 0; i < indent; i++)
        CHECK(bufprintf(&buf, &offset, &sz, " ") != NULL);

    CHECK(bufprintf(&buf, &offset, &sz, "%s  %s %s %s\n",
                    obj->oid,
                    obj->access == CFG_READ_CREATE ? "RC" :
                    obj->access == CFG_READ_WRITE ? "RW" :
                    "R",
                    obj->type == CVT_NONE ? "none" :
                    obj->type == CVT_INTEGER ? "int" :
                    obj->type == CVT_ADDRESS ? "addr" : "str",
                    obj->vol == TRUE ? "V" : "")
          != NULL);
    for (dep = obj->depends_on; dep != NULL; dep = dep->next)
    {
        if (strcmp_start(dep->depends->oid, obj->oid) != 0)
        {
            for (i = 0; i < indent; i++)
                CHECK(bufprintf(&buf, &offset, &sz, " ") != NULL);
            CHECK(bufprintf(&buf, &offset, &sz, "-> %s\n",
                            dep->depends->oid) != NULL);
        }
    }

    for (obj = obj->son; obj != NULL; obj = obj->brother)
        CHECK(obj_tree_bufprint(obj, indent + 2) != NULL);

    if (indent == 0)
    {
        tmp = buf;
        BUF_RESET;
        return tmp;
    }
    return buf;
}

/**
 * Print (recursively) a tree of instances into the buffer.
 * Based on conf_main.c:print_tree() by Elena Vengerova.
 *
 * @param ins       the root of the tree.
 * @param indent    indentation for the current level.
 *
 * @return
 *     Pointer to the allocated buffer containing the printed tree string.
 *     NULL, if error occurred.
 */
static char *
ins_tree_bufprint(cfg_instance *ins, const int indent)
{
    static size_t sz = BUF_SIZE_MIN;
    static char  *buf = NULL;
    static int    offset = 0;
    int           i;
    char          *tmp;
    char          *str;

    for (i = 0; i < indent; i++)
        CHECK(bufprintf(&buf, &offset, &sz, " ") != NULL);

    if (ins->obj->type == CVT_NONE ||
        cfg_types[ins->obj->type].val2str(ins->val, &str) != 0)
    {
        str = NULL;
    }
    CHECK(bufprintf(&buf, &offset, &sz, "%s = %s\n",
                    ins->oid, (str != NULL) ? str : "")
          != NULL);
    free(str);
    for (ins = ins->son; ins != NULL; ins = ins->brother)
        CHECK(ins_tree_bufprint(ins, indent + 2) != NULL);

    if (indent == 0)
    {
        tmp = buf;
        BUF_RESET;
        return tmp;
    }
    return buf;
}

/**
 * Print all dependancies of an object into a file and(or) log.
 *
 * @param filename          output filename (NULL to skip)
 * @param log_lvl           TE log level (0 to skip)
 * @param id_fmt            a format string for the id of an obj.
 *
 * @return                  Status code.
 */
te_errno
cfg_db_obj_print_deps(const char *filename,
                      const unsigned int log_lvl,
                      const char *id_fmt, ...)
{
    char            *buf = NULL;
    FILE            *f;
    char            id[CFG_OID_MAX];
    size_t          id_len;
    cfg_object      *obj;
    va_list ap;

    if (id_fmt == NULL)
        return TE_RC(TE_CS, TE_EINVAL);
    va_start(ap, id_fmt);
    id_len = (size_t)vsnprintf(id, sizeof(id), id_fmt, ap);
    va_end(ap);
    if (id_len >= sizeof(id))
        return TE_RC(TE_CS, TE_EINVAL);

    obj = cfg_get_obj_by_obj_id_str(id);
    if (obj == NULL)
    {
        TE_LOG(log_lvl, TE_LGR_ENTITY, TE_LGR_USER,
               "no node with id string: %s\n", id);
        return TE_RC(TE_CS, TE_EINVAL);
    }

    buf = obj_bufprint_deps(obj);
    if (buf == NULL)
    {
        ERROR("obj_bufprint_deps() failed\n");
        return TE_RC(TE_CS, TE_ENOMEM);
    }

    if (log_lvl != 0)
        TE_LOG(log_lvl, TE_LGR_ENTITY, TE_LGR_USER, "%s", buf);
    if (filename != NULL)
    {
        if ((f = fopen(filename, "w")) == NULL)
            ERROR("Can't open file: %s", filename);
        fprintf(f, "%s", buf);
        fclose(f);
    }

    free(buf);
    return 0;
}

/**
 * Print all dependancies of an object into the buffer.
 *
 * @param obj       The object of interest.
 *
 * @return
 *     Pointer to the allocated buffer containing the output.
 *     NULL, if error occurred.
 */
static char *
obj_bufprint_deps(cfg_object *obj)
{
    static size_t   sz = BUF_SIZE_MIN;
    static char     *buf = NULL;
    static int      offset = 0;
    char            *tmp;
    cfg_object      *otmp;
    cfg_dependency  *depends_on;
    cfg_dependency  *dependants;

    CHECK(bufprintf(&buf, &offset, &sz, "Masters of the object: %s\n",
                    obj->oid) != NULL);
    for (depends_on = obj->depends_on;
         depends_on != NULL; depends_on = depends_on->next)
    {
        otmp = depends_on->depends;
        CHECK(bufprintf(&buf, &offset, &sz, "%s\n",
                        otmp != NULL ? otmp->oid : "NULL") != NULL);
    }

    CHECK(bufprintf(&buf, &offset, &sz, "Dependants of the object: %s\n",
                    obj->oid) != NULL);
    for (dependants = obj->dependants;
         dependants != NULL; dependants = dependants->next)
    {
        otmp = dependants->depends;
        CHECK(bufprintf(&buf, &offset, &sz, "%s\n",
                        otmp != NULL ? otmp->oid : "NULL") != NULL);
    }

    tmp = buf;
    BUF_RESET;
    return tmp;
}

#undef BUF_RESET
#undef CHECK

/**
 * Print into auto-growing buffer according to format.
 *
 * @param p_buf     pointer to the buffer (IN/OUT).
 * @param p_offset  address of a current offset in the buffer (IN/OUT).
 * @param p_sz      current size of the buffer (IN/OUT).
 * @param format    format string.
 *
 * @return  Pointer to an allocated buffer, or NULL if error occured.
 */
static char *
bufprintf(char **p_buf, int *p_offset, size_t *p_sz,
          const char *format, ...)
{
    size_t  grow_min = BUF_SIZE_MIN;
    float   grow_coeff = BUF_GROW_COEFF;
    char    *buf = *p_buf;
    int     offset = *p_offset;
    size_t  sz = *p_sz;
    int     n;
    va_list ap;

    if (buf == NULL && (buf = (char *)malloc(sz)) == NULL)
    {
        ERROR("Can't allocate ini buffer\n");
        return NULL;
    }

    va_start(ap, format);
    n = vsnprintf(buf + offset, sz - offset, format, ap);
    va_end(ap);
    if (n >= (int)sz - offset)
    {
        while (n >= (int)sz - offset)
        {
            sz +=
            (sz * grow_coeff < grow_min) ? grow_min : sz * grow_coeff;
         }

        buf = (char *)realloc(buf, sz);
        if (buf == NULL)
        {
            ERROR("realloc() failed\n");
            return NULL;
        }
        va_start(ap, format);
        n = vsnprintf(buf + offset, sz - offset, format, ap);
        va_end(ap);
    }

    (*p_offset) += n;
    *p_sz = sz;
    *p_buf = buf;

    return buf;
}

