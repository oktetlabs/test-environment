/** @file
 * macro definitions for in-kernel TCE module
 *
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 *
 */

#ifndef __TE_TCE_BBINIT_DEFS_H__
#define __TE_TCE_BBINIT_DEFS_H__


/**
 * Define TCE-related data structure and most necessary sysfs/kobject
 * stuff: ktype, attribute type, accessors, sysfs_ops, kobject destructor.
 * A table of attributes need to be provided, named `@p name _attributes'.
 *
 * @param _name         Structure name
 * @param _destructor   Destructor name
 */                                                                             \
#define TCE_STRUCTURE(_name, _destructor)                                       \
                                                                                \
typedef struct _name {                                                          \
    struct kobject kobj;                                                        \
    struct _name##_data data;                                                   \
} _name;                                                                        \
                                                                                \
typedef struct _name##_attribute {                                              \
    struct attribute attr;                                                      \
    ssize_t (*show)(_name *, char *);                                           \
    ssize_t (*store)(_name *, const char *, size_t);                            \
} _name##_attribute;                                                            \
                                                                                \
static ssize_t                                                                  \
_name##_read(struct kobject *kobj, struct attribute *attr, char *value)         \
{                                                                               \
    _name *obj = container_of(kobj, _name, kobj);                               \
    _name##_attribute *dispatch = container_of(attr, _name##_attribute, attr);  \
                                                                                \
    if (dispatch->show == NULL)                                                 \
        return -EIO;                                                            \
    return dispatch->show(obj, value);                                          \
}                                                                               \
                                                                                \
                                                                                \
static ssize_t                                                                  \
_name##_write(struct kobject *kobj, struct attribute *attr, const char *value, size_t count)  \
{                                                                               \
    _name *obj = container_of(kobj, _name, kobj);                               \
    _name##_attribute *dispatch = container_of(attr, _name##_attribute, attr);  \
                                                                                \
    if (dispatch->store == NULL)                                                \
        return -EIO;                                                            \
    return dispatch->store(obj, value, count);                                         \
}                                                                               \
                                                                                \
static struct sysfs_ops _name##_sysfs_ops = {                                   \
    .show = _name##_read,                                                       \
    .store = _name##_write                                                      \
};                                                                              \
                                                                                \
static struct attribute *_name##_attributes[];                                  \
static struct kobj_type ktype_##_name = {                                       \
    .release = _destructor,                                                     \
    .sysfs_ops = &_name##_sysfs_ops,                                            \
    .default_attrs = _name##_attributes                                         \
}


/** Define an attribute record
 *  
 * @param _type  TCE structure type
 * @param _name  Attribute name (used as a name of sysfs dir, too)
 * @param _mode  sysfs access mode
 * @param _show  Read accessor expression (function ref or NULL)
 * @param _store Write accessor expression
 *
 * @sa TCE_STRUCTURE()
 */
#define TCE_ATTR_DEF(_type, _name, _mode, _show, _store)    \
static _type##_attribute _type##_##_name##_attr =           \
{                                                           \
    .attr  = {.name = #_name, .mode = _mode},               \
    .show  = _show,                                         \
    .store = _store                                         \
}


#define TCE_ATTR_TABLE(_type) static struct attribute *_type##_attributes[] 
#define TCE_ATTR_REF(_type, _name) (&_type##_##_name##_attr.attr)

/**
 * Define a simple read/write attribute. Accessors are implied
 * to have names `@p _type _ @p _name _ show' and 
 * `@p _type _ @p _name _ show'.
 *
 * @param _type TCE structure type
 * @param _name Attribute name
 *
 * @sa TCE_STRUCTURE()
 * @sa TCE_ATTR_DEF()
 */
#define TCE_SIMPLE_ATTR_DEF(_type, _name)       \
TCE_ATTR_DEF(_type, _name, S_IRUSR | S_IWUSR,   \
             _type##_##_name##_show,            \
             _type##_##_name##_store)

#define TCE_SIMPLE_ATTR_RO_DEF(_type, _name)       \
TCE_ATTR_DEF(_type, _name, S_IRUSR,   \
             _type##_##_name##_show,            \
             NULL)



#define TCE_ATTR_STORE(_type, _name, _objname, _valuename, _count)          \
static ssize_t                                                      \
_type##_##_name##_store(_type *_objname, const char *_valuename, size_t _count)

#define TCE_ATTR_SHOW(_type, _name, _objname, _resname)     \
static ssize_t                                              \
_type##_##_name##_show(_type *_objname, char *_resname)


/**
 * Format a TCE attribute value
 *
 * @param _result       Buffer name
 * @param _format       printf() format string for the attribute value
 *                      (must not require more than one argument)
 * @param _code         Value-displaying code
 */
#define TCE_ATTR_FMT(_result, _format, _code) \
return snprintf(result, PAGE_SIZE, _format, _code)

/**
 * Yields a parent object of the TCE object
 *
 * @param _obj          Object reference
 * @param _ptype        Parent type
 */
#define TCE_PARENT(_obj, _ptype) container_of(_obj->kobj.parent,  _ptype, kobj)


/**
 * Define an attribute to dynamically add children to the object, and
 * the function to do it.
 * The function has a prototype of `unsigned (@p _ptype *, @p _type **)' and
 * it returns the sequence number of the fresh object and a pointer to it
 * in the second argument if it's not NULL.
 *
 * @param _name         Allocator function name
 * @param _type         TCE structure type to be allocated
 * @param _ptype        TCE structure type of the parent
 * @param _allocator    Memory allocator
 * @param _seqno        A field within @p _ptype that holds sequence numbers
 * @param _constructor  Constructor function name
 */
#define TCE_CHILD_ALLOCATOR(_name, _prefix, _type, _ptype, _allocator, _seqno, _constructor)    \
static unsigned                                                                                 \
_name(_ptype *PARENT, _type **result)                                                           \
{                                                                                               \
    int NEXT = PARENT->data._seqno++;                                                           \
    _type *NEW = _allocator;                                                                    \
                                                                                                \
    memset(NEW, 0, sizeof(*NEW));                                                               \
    kobject_set_name(&NEW->kobj, _prefix "%u", NEXT);                                           \
    NEW->kobj.kset = NULL;                                                                      \
    NEW->kobj.ktype = &ktype_##_type;                                                           \
    NEW->kobj.parent = &PARENT->kobj;                                                           \
    __builtin_choose_expr (__builtin_constant_p(_constructor),                                  \
                           (void)0,                                                             \
                           _constructor(PARENT, NEW));                                          \
    kobject_register(&NEW->kobj);                                                               \
    if (result != NULL) *result = NEW;                                                          \
    return NEXT;                                                                                \
}                                                                                               \
                                                                                                \
TCE_ATTR_SHOW(_ptype, _seqno, obj, result)                                                      \
{                                                                                               \
    TCE_ATTR_FMT(result, "%u", _name(obj, NULL));                                               \
}                                                                                               \
TCE_SIMPLE_ATTR_RO_DEF(_ptype, _seqno)


/** Memory allocator for TCE_CHILD_ALLOCATOR() 
 *  It allocates memory from heap 
 */
#define TCE_DYNAMIC kmalloc(sizeof(*NEW), GFP_KERNEL)

/** Memory allocator for TCE_CHILD_ALLOCATOR() 
 *  It allocates memory from a pre-allocated array inside
 *  the parent named @p _field
 */
#define TCE_STATIC(_field) (PARENT->data._field + NEXT)


#define TCE_NULL_FUN ((void (*)())NULL)

/** Define an attribute @p _name and its accessors that simply
 *  show/store the contents of a field of the same name
 *
 * @param _tname        TCE structure name
 * @param _name         Attribute name
 * @param _format       Format string for printf()/scanf()
 * @param _cast         Typecast for showing the value
 */
#define TCE_SIMPLE_ATTR(_tname, _name, _format,  _cast)                 \
                                                                        \
TCE_ATTR_SHOW(_tname, _name, obj, result)                               \
{                                                                       \
    TCE_ATTR_FMT(result, _format, (_cast)obj->data._name);              \
}                                                                       \
                                                                        \
TCE_ATTR_STORE(_tname, _name, obj, value, count)                        \
{                                                                       \
    _cast tmpval;                                                       \
    if (sscanf(value, _format, &tmpval) != 1)                           \
        return -EINVAL;                                                 \
    obj->data._name = tmpval;                                           \
    return count;                                                       \
}                                                                       \
TCE_SIMPLE_ATTR_DEF(_tname, _name)
                                                                        

/**
 * Like TCE_SIMPLE_ATTR(), but only defines the read accessor
 *
 * @param _tname        TCE structure name
 * @param _name         Attribute name
 * @param _format       Format string for printf()
 * @param _cast         Typecast for showing the value
 */
#define TCE_SIMPLE_RO_ATTR(_tname, _name, _format,  _cast)      \
TCE_ATTR_SHOW(_tname, _name, obj, result)                       \
{                                                               \
    TCE_ATTR_FMT(result, _format, (_cast)obj->data._name);  \
}                                                               \
                                                                \
TCE_ATTR_DEF(_tname, _name, S_IRUSR,                            \
             _tname##_##_name##_show, NULL)

/**
 * Defines an attribute @p _name to show/store a string value of
 * the corresponding field. Memory management is done appropriately
 *
 * @param _tname        TCE structure name
 * @param _name         Attribute name
 * @param _postprocess  Function to call after the value is stored
 *
 */
#define TCE_STRING_ATTR(_tname, _name, _postprocess)                \
TCE_ATTR_SHOW(_tname, _name, obj, result)                           \
{                                                                   \
    if (obj->data._name == NULL)                                    \
    {                                                               \
        *result = '\0';                                             \
        return 0;                                                   \
    }                                                               \
    strncpy(result, obj->data._name, PAGE_SIZE);                    \
    return strnlen(result, PAGE_SIZE);                              \
}                                                                   \
TCE_ATTR_STORE(_tname, _name, obj, value, len)                      \
{                                                                   \
    if (obj->data._name != NULL)                                    \
        kfree(obj->data._name);                                     \
    obj->data._name = kmalloc(len + 1, GFP_KERNEL);                 \
    memcpy(obj->data._name, value, len);                            \
    obj->data._name[len] = '\0';                                    \
    __builtin_choose_expr (__builtin_constant_p(_postprocess),      \
                           (void)0,                                 \
                           _postprocess(obj));                      \
                                                                    \
    return len;                                                     \
};                                                                  \
                                                                    \
TCE_SIMPLE_ATTR_DEF(_tname, _name)

/** Definitions taken from GCC TCE code **/


/** GCC 3.4+ relevant data **/

typedef unsigned gcov_unsigned_t;
typedef unsigned gcov_position_t;
typedef signed long long gcov_type;

/* Counters that are collected.  */
#define GCOV_COUNTER_ARCS   0  /* Arc transitions.  */
#define GCOV_COUNTERS_SUMMABLE  1  /* Counters which can be
                      summaried.  */
#define GCOV_FIRST_VALUE_COUNTER 1 /* The first of counters used for value
                      profiling.  They must form a consecutive
                      interval and their order must match
                      the order of HIST_TYPEs in
                      value-prof.h.  */
#define GCOV_COUNTER_V_INTERVAL 1  /* Histogram of value 
                                      inside an interval.*/
#define GCOV_COUNTER_V_POW2 2  /* Histogram of exact power2 logarithm
                      of a value.  */
#define GCOV_COUNTER_V_SINGLE   3  /* The most common value 
                                      of expression.  */
#define GCOV_COUNTER_V_DELTA    4  /* The most common difference between
                      consecutive values of expression.  */
#define GCOV_LAST_VALUE_COUNTER 4  /* The last of counters used for value
                      profiling.  */
#define GCOV_COUNTERS       5

#define GCOV_LOCKED 0

/* Cumulative counter data.  */
struct gcov_ctr_summary
{
  gcov_unsigned_t num;      /* number of counters.  */
  gcov_unsigned_t runs;     /* number of program runs */
  gcov_type sum_all;        /* sum of all counters accumulated.  */
  gcov_type run_max;        /* maximum value on a single run.  */
  gcov_type sum_max;        /* sum of individual run max values.  */
};

/* Object & program summary record.  */
struct gcov_summary
{
  gcov_unsigned_t checksum; /* checksum of program */
  struct gcov_ctr_summary ctrs[GCOV_COUNTERS_SUMMABLE];
};

/* Structures embedded in coveraged program.  The structures generated
   by write_profile must match these.  */

/* Information about a single function.  This uses the trailing array
   idiom. The number of counters is determined from the counter_mask
   in gcov_info.  We hold an array of function info, so have to
   explicitly calculate the correct array stride.  */
struct gcov_fn_info
{
  gcov_unsigned_t ident;    /* unique ident of function */
  gcov_unsigned_t checksum; /* function checksum */
  unsigned n_ctrs[0];       /* instrumented counters */
};

/* Type of function used to merge counters.  */
typedef void (*gcov_merge_fn) (gcov_type *, gcov_unsigned_t);

/* Information about counters.  */
struct gcov_ctr_info
{
  gcov_unsigned_t num;      /* number of counters.  */
  gcov_type *values;        /* their values.  */
  gcov_merge_fn merge;      /* The function used to merge them.  */
};

/* Information about a single object file.  */
struct gcov_info
{
  gcov_unsigned_t version;  /* expected version number */
  struct gcov_info *next;   /* link to next, used by libgcov */

  gcov_unsigned_t stamp;    /* uniquifying time stamp */
  const char *filename;     /* output file name */
  
  unsigned n_functions;     /* number of functions */
  const struct gcov_fn_info *functions; /* table of functions */

  unsigned ctr_mask;        /* mask of counters instrumented.  */
  struct gcov_ctr_info counts[0]; /* count data. The number of bits
                     set in the ctr_mask field
                     determines how big this array
                     is.  */
};

/** pre-GCC 3.4 relevant data */

struct bb_function_info 
{
    long checksum;
    int arc_count;
    const char *name;
};

/* Structure emitted by --profile-arcs  */
struct bb
{
    long zero_word;
    const char *filename;
    long long *counts;
    long ncounts;
    struct bb *next;
    
    /* Older GCC's did not emit these fields.  */
    long sizeof_bb;
    struct bb_function_info *function_infos;
};



#endif /* __TE_TCE_BBINIT_DEFS_H__ */
