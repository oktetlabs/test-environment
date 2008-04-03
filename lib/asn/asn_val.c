/** @file
 * @brief ASN.1 library
 *
 * Implementation of method to dynamic ASN value processing. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */ 

#include "te_config.h"

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <ctype.h>

#include <stdarg.h>

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "asn_impl.h" 


#define ASN_STOP_AT_CHOICE (TE_EASNGENERAL + 50)

#define AVOID_STRSEP 1

/* this function defined in asn_text.c */
extern int number_of_digits(int value);

/*
 * Declarations of local methods, for descriptions see their definition.
 */ 



te_errno asn_impl_fall_down_to_tree_nc(const asn_value *, char *,
                                       asn_value const **);

te_errno asn_impl_write_value_field(asn_value *, const void *,
                                    size_t , char *);

te_errno asn_impl_read_value_field(const asn_value *, void *,
                                   size_t *, char *);

te_errno asn_impl_write_component_value(asn_value *, const asn_value *,
                                        char *);


/*
 * Implementation of static inline functions 
 */

static inline char *
asn_strdup(const char* src)
{
    if (src == NULL)
        return NULL;
    else
    {
        int   len = strlen(src);
        char *res = malloc(len + 1);

        if (res == NULL) return NULL;

        strcpy(res, src); 
        return res;
    }
}

/**
 * Wrapper over asn_impl_find_subvalue, for find in writable container
 * and get writable subvalue. All parameters are same. 
 */ 
static inline te_errno
asn_impl_find_subvalue_writable(asn_value *container, const char *label, 
                                asn_value **found_val)
{
    const asn_value *f_val;
    te_errno rc = asn_impl_find_subvalue(container, label, &f_val);

    *found_val = (asn_value *)f_val;
    return rc;
}


/**
 * Wrapper over asn_impl_fall_down_to_tree_nc, for find in writable container
 * and get writable subvalue. All parameters are same. 
 */ 
static inline te_errno 
asn_impl_fall_down_to_tree_writable(asn_value *container, 
                                    const char *field_labels,
                                    asn_value **found_value)
{
    te_errno rc;
    char *rest_labels = asn_strdup(field_labels); 
    const asn_value *f_val;

    rc = asn_impl_fall_down_to_tree_nc (container, rest_labels, &f_val); 
    free (rest_labels);

    /* Initial container was*/
    *found_value = (asn_value *)f_val;

    return rc;
}


/**
 * Compare two ASN tags.
 *
 * @param l  first argument;
 * @param r  second argument; 
 *
 * @return truth value of tag equality 
 * @retval 1 tags are equal;
 * @retval 0 tags are differ; 
 */
inline int 
asn_tag_equal(asn_tag_t l, asn_tag_t r)
{
    return ( (l.cl  == r.cl ) && (l.val == r.val) ); 
}


/**
 * Init empty ASN value of specified type.
 *
 * @param type       ASN type to which value should belong. 
 *
 * @return pointer to new ASN_value instance or NULL if error occurred. 
 */
asn_value *
asn_init_value(const asn_type * type)
{
    asn_value *new_value;
    int arr_len;

    if (type == NULL) return NULL;

    new_value = malloc(sizeof(asn_value));
    if (new_value == NULL)
        return NULL;

    memset(new_value, 0, sizeof(asn_value));

    new_value->asn_type = type;
    new_value->syntax   = type->syntax;
    new_value->tag      = type->tag;

    new_value->txt_len  = -1;

    arr_len = type->len; 

    switch (type->syntax)
    {
        case CHOICE:
        case TAGGED:
            arr_len = 1;
            /* fall through */
        case SEQUENCE:
        case SET:
            {
                size_t  sz = arr_len * sizeof(asn_value *);
                void   *ptr = malloc(sz);

                new_value->len = arr_len;
                new_value->data.array = ptr; 

                memset(ptr, 0, sz);
            }
            break;

        case INTEGER:
            new_value->txt_len = 1;
            /* fall through */
        default:
            new_value->data.integer = 0;
    }

    return new_value;
}

/**
 * Init empty ASN value of specified type with certain ASN tag.
 *
 * @param type       ASN type to which value should belong. 
 * @param tc         ASN tag class, see enum definition and ASN.1 standard.
 * @param tag        ASN tag value, may be arbitrary non-negative integer.  
 *
 * @return pointer to new ASN_value instance or NULL if error occurred. 
 */
asn_value *
asn_init_value_tagged(const asn_type *type,
                      asn_tag_class tc, asn_tag_value tag)
{
    asn_value *new_value = asn_init_value(type);

    if (new_value == NULL)
        return NULL;

    new_value->tag.cl  = tc;
    new_value->tag.val = tag;

    return new_value;
}


/* See description in asn_usr.h */
te_errno
asn_assign_value(asn_value *dst, const asn_value *src)
{
    int len;

    if (!src || !dst) 
        return TE_EWRONGPTR;

    if (dst->asn_type != src->asn_type)
        return TE_EASNWRONGTYPE;

    dst->syntax   = src->syntax;
#if 0
    dst->tag      = src->tag;
#endif
    dst->len      = src->len; 
    dst->txt_len  = src->txt_len; 

    len = src->len;

    if (src->syntax & COMPOUND)
    {
        int i;
        asn_value *src_elem;
        asn_value **arr;

        if (dst->data.array != NULL)
        {
            for (i = 0; i < (int)dst->len; i++)
                 asn_free_value(dst->data.array[i]);
            free(dst->data.array);
        }

        arr = dst->data.array = malloc(len * sizeof(asn_value *));

        if (arr==NULL)
            return ENOMEM;

        for (i = 0; i < len; i++, arr++)
        {
            if ((src_elem = src->data.array[i])!= NULL)
            {
                if ((*arr = asn_copy_value(src_elem))==NULL) 
                { /* ERROR! */
                    free(dst->data.array);
                    dst->data.array = NULL;
                    return ENOMEM;
                }
            }
            else /* data is not specified yet, value is incomplete.*/
                *arr = NULL;
        }
    }
    else if (src->syntax & PRIMITIVE_VAR_LEN)
    {
        if(src->syntax == OID) 
            len *= sizeof(int);

        if(src->syntax == BIT_STRING) 
            len = (len + 7) >> 3;

        free(dst->data.other);

        if ((src->data.other == NULL) || (len == 0))
        { /* data is not specified yet, value is incomplete.*/
            dst->data.other = NULL;
            return 0;
        }

        if ((dst->data.other = malloc (len))==NULL)
            return ENOMEM;
        
        memcpy (dst->data.other, src->data.other, len); 
    }
    else /* value is stored in data.integer */
    {
        dst->data.integer = src->data.integer;
    }

    dst->txt_len = src->txt_len;

    return 0; 
}


/**
 * Make a copy of ASN value instance.
 *
 * @param value       ASN value to be copied.  
 *
 * @return pointer to new ASN_value instance or NULL if error occurred. 
 */
asn_value * 
asn_copy_value(const asn_value *value)
{
    asn_value *new_value;
#if 1
    if (value == NULL)
        return NULL;

    new_value = asn_init_value(value->asn_type);

    if (asn_assign_value(new_value, value) != 0)
        return NULL;

    if (value->name)
        new_value->name = asn_strdup(value->name);
    else
        new_value->name = NULL;
    
    new_value->tag = value->tag;

    return new_value;
#else
    int len;

    if (!value) { /* ERROR! */ return NULL; } 

    new_value = malloc (sizeof (asn_value));
    if (!new_value) { /* ERROR! */ return NULL; } 

    new_value->asn_type = value->asn_type;
    new_value->syntax   = value->syntax;
    new_value->tag      = value->tag;
    new_value->len      = value->len; 
    new_value->txt_len  = value->txt_len; 

    if (value->name)
        new_value->name = asn_strdup(value->name);
    else
        new_value->name = NULL;

    len = value->len;

    if (value->syntax & COMPOUND)
    {
        int i;
        asn_value *src_elem;
        asn_value **arr = new_value->data.array = 
            malloc(len * sizeof(asn_value *));

        if (arr==NULL)
        { /* ERROR! */
            if (new_value->name) free(new_value->name);
            free(new_value);
            return NULL;
        }

        for (i = 0; i < len; i++, arr++)
        {
            if ((src_elem = value->data.array[i])!= NULL)
            {
                if ((*arr = asn_copy_value(src_elem))==NULL) 
                { /* ERROR! */
                    if (new_value->name) free(new_value->name);
                    free(new_value->data.array);
                    free(new_value);
                    return NULL;
                }
            }
            else /* data is not specified yet, value is incomplete.*/
                *arr = NULL;
        }
    }
    else if (value->syntax & PRIMITIVE_VAR_LEN)
    {
        if(value->syntax == OID) 
            len *= sizeof(int);

        if(value->syntax == BIT_STRING) 
            len = (len + 7) >> 3;

        if ((value->data.other == NULL) || (len == 0))
        { /* data is not specified yet, value is incomplete.*/
            new_value->data.other = NULL;
            return new_value;
        }

        if ((new_value->data.other = malloc (len))==NULL)
        { /* ERROR! */
            if (new_value->name) free(new_value->name);
            free(new_value);
            return NULL;
        }
        memcpy (new_value->data.other, value->data.other, len); 
    }
    else /* value is stored in data.integer */
    {
        new_value->data.integer = value->data.integer;
    }

    new_value->txt_len = value->txt_len;

    return new_value;
#endif
}

/**
 * Free memory allocalted by ASN value instance.
 *
 * @param value       ASN value to be destroyed. 
 *
 * @return nothing
 */
void 
asn_free_value(asn_value *value)
{
    if (!value) return;
    
    if (value->syntax & COMPOUND) 
    {
        unsigned int i;
        asn_value **arr = value->data.array; 
        for (i = 0; i < value->len; i++, arr++)
        {
            asn_free_value(*arr);
        }

        free(value->data.array);
    }
    else if (value->syntax & PRIMITIVE_VAR_LEN)
    {
        free(value->data.other);
    } 

    free(value->name);
    free(value);
}



te_errno
asn_free_child(asn_value *value,
               asn_tag_class tag_class, asn_tag_value tag_val)
{
    te_errno rc = 0;
    int index;

    if (value == NULL)
        return TE_EWRONGPTR;

    switch (value->syntax)
    {
        case SEQUENCE:
        case SET:
            if ((rc = asn_child_tag_index(value->asn_type, tag_class,
                                          tag_val, &index)) != 0)
                return rc;
            break;

        case CHOICE:
            index = 0;
            break;

        default:
            return TE_EASNWRONGTYPE; 
    }

    value->txt_len = -1;
    asn_free_value(value->data.array[index]);
    value->data.array[index] = NULL;

    return 0;
}

/* see description in asn_usr.h */
te_errno
asn_free_descendant(asn_value *value, const char *labels)
{
    int   len;
    char *low_label;
    char *up_labels = asn_strdup(labels);

    te_errno rc = 0;

    if (!value || !labels)
    {
        free(up_labels);
        return TE_EWRONGPTR; 
    }

    len = strlen(labels);
    low_label = up_labels + len - 1;
    while ((low_label > up_labels) && (*low_label != '.')) 
        low_label --;

    rc = 0;

    if (*low_label == '.') 
    {
        *low_label = '\0';
        low_label ++;
#if 0
        rc = asn_impl_fall_down_to_tree_writable(value, up_labels, &subvalue);
        value = subvalue; 
#else
        value = asn_find_descendant(value, &rc, up_labels);
#endif 
    } 
#if 0
    if (rc == 0)
        rc = asn_impl_find_subvalue_writable(value, low_label, &subvalue);
#endif
    if (rc == 0)
    {
        asn_put_child_value_by_label(value, NULL, low_label);

        value->txt_len = -1;
    }
    free(up_labels);

    return rc;
}


/* see description in asn_usr.h */
te_errno 
asn_free_subvalue(asn_value *value, const char* labels)
{
    return asn_free_descendant(value, labels);
}

/* see description in asn_usr.h */
te_errno
asn_free_child_value(asn_value *value, 
                     asn_tag_class tag_class, asn_tag_value tag_val)
{
    return asn_free_child(value, tag_class, tag_val);
}


/**
 * Obtain ASN type to which specified value belongs. 
 *
 * @param value       ASN value which type is interested. 
 *
 * @return pointer to asn_type instance or NULL if error occurred. 
 */
const asn_type * 
asn_get_type(const asn_value *value)
{
    if (!value) return NULL;
    return value->asn_type;
}

/* See description in asn_usr.h */
const char *
asn_get_type_name(const asn_type *type)
{
    if (!type) return NULL;
    return type->name;
}

/**
 * Get name of value;
 *
 * @param container     value which name is intereseted
 *
 * @return value's name or NULL.
 */ 
const char *
asn_get_name(const asn_value *value)
{
    if (!value) return NULL;
    return value->name;
}

te_errno 
asn_child_tag_index(const asn_type *type, asn_tag_class tag_class,
                    asn_tag_value tag_val, int *index)
{
    const asn_named_entry_t *n_en;
    unsigned i; 

    if(!type || !index)
        return TE_EWRONGPTR; 

    if( (type->syntax & COMPOUND) == 0 ||
        (type->syntax & ASN_SYN_NAMED) == 0)
    {
        return TE_EASNWRONGTYPE; 
    }

    n_en = type->sp.named_entries; 
        
    /* 
     * TODO: make something more efficient, then linear search, 
     * leafs in ASN type specification should be sorted by tag value.
     */
    for (i = 0; i < type->len; i++, n_en++)
        if(n_en->tag.cl == tag_class && n_en->tag.val == tag_val)
        {            
            *index = i;
            return 0;
        }

    return TE_EASNWRONGLABEL;
}

/* see description in asn_impl.h */
te_errno 
asn_child_named_index(const asn_type *type, const char *labels, 
                      int *index, const char **rest_labels)
{
    const asn_named_entry_t *n_en;
    unsigned i; 
    const char *p = labels, *q;

    if(!type || !labels || !index || !rest_labels)
        return TE_EWRONGPTR; 

    switch (type->syntax)
    {
        case CHOICE:
            if (*labels == '#') 
                labels++;
            /* pass through ... */ 
        case SEQUENCE:
        case SET:
            n_en = type->sp.named_entries; 
            for (i = 0; i < type->len; i++, n_en++)
            {
                for (p = labels, q = n_en->name; 
                     *p != 0 && *p != '.' && *p == *q; 
                     p++, q++);

                if ((*p == 0 || *p == '.') && *q == 0)
                {
                    *index = i;
                    break;
                }
            }
            break;

        case SEQUENCE_OF:
        case SET_OF:
            *index = strtol(labels, (char **)&p, 10);
            break;
            
        default:
            return TE_EASNWRONGTYPE; 
    } 

    if (*p == 0)
        *rest_labels = NULL;
    else if (*p == '.')
        *rest_labels = p + 1;
    else 
        return TE_EASNWRONGLABEL;

    return 0;
}

/* see description in asn_impl.h */
te_errno
asn_impl_find_subtype(const asn_type *type, const char *label, 
                      const asn_type **found_type)
{ 
    const char *rest = NULL;

    if(!label || !type || !found_type)
        return TE_EWRONGPTR; 

    if ((type->syntax & COMPOUND) == 0)
        return TE_EASNWRONGTYPE;

    if (type->syntax & ASN_SYN_NAMED)
    {
        te_errno rc;
        int index; 

        if ((rc = asn_child_named_index(type, label, &index, &rest)) != 0)
            return rc;

        if (rest != NULL)
            return TE_EASNWRONGLABEL;
        
        *found_type =type->sp.named_entries[index].type; 
    }
    else 
        *found_type = type->sp.subtype; 

    return 0; 
}












#define RETURN_NULL_WITH_ERROR(_error) \
    do {                                \
        if (status != NULL)             \
            *status = (_error);         \
        return NULL;                    \
    } while(0) 


/* see description in asn_usr.h */
asn_value *
asn_find_descendant(const asn_value *value, te_errno *status,
                    const char *labels_fmt, ...)
{
    va_list     list;
    te_errno    rc = 0;
    char        labels_buf[200]; 
    const char *rest_labels = labels_buf;
    asn_value  *tmp_value = (asn_value *)value;
    int         subval_index;

    va_start(list, labels_fmt);
    if (labels_fmt != NULL && labels_fmt[0] != 0) 
    {
        if (vsnprintf(labels_buf, sizeof(labels_buf), labels_fmt, list) >=
            (int)sizeof(labels_buf))
            rc = TE_E2BIG;
    }
    else
        labels_buf[0] = '\0';
    va_end(list);
    if (rc != 0)
        RETURN_NULL_WITH_ERROR(rc);

    if (status != NULL)
        *status = 0;

    if (!value)
        RETURN_NULL_WITH_ERROR(TE_EWRONGPTR);

    if (*labels_buf == '\0')
        return (asn_value *)value;

    while (rest_labels != NULL && (*rest_labels != '\0'))
    { 
        rc = asn_child_named_index(tmp_value->asn_type, rest_labels, 
                                   &subval_index, &rest_labels);
        if (rc != 0)
        {
            if ((rc == TE_EASNWRONGLABEL) &&
                (asn_get_syntax(tmp_value, NULL) == CHOICE))
            {
                if (rest_labels[0] == '#' && rest_labels[1] == '\001') 
                    rc = 0;
                else if ((rc = asn_get_choice_value(tmp_value, &tmp_value,
                                                    NULL, NULL)) == 0)
                    continue;
            }

            break;
        }

        rc = asn_get_child_by_index(tmp_value, &tmp_value, subval_index);
        if (rc != 0)
            break;
    }

    if (rc != 0 && status != NULL)
        *status = rc; 

    return (rc == 0) ? tmp_value : NULL;
}



asn_value *
asn_retrieve_descendant(asn_value *value, te_errno *status,
                        const char *labels_fmt, ...)
{
    va_list     list;
    te_errno    rc = 0;
    asn_value  *tmp_value;
    char        labels_buf[200]; 
    const char *rest_labels;
    int         subval_index;

    va_start(list, labels_fmt);
    if (labels_fmt != NULL && labels_fmt[0] != '\0') 
    {
        if (vsnprintf(labels_buf, sizeof(labels_buf), labels_fmt, list) >=
            (int)sizeof(labels_buf))
            rc = TE_E2BIG;
    }
    else
        labels_buf[0] = '\0';
    va_end(list);

    if (rc != 0)
        RETURN_NULL_WITH_ERROR(rc);

    if (value == NULL)
        RETURN_NULL_WITH_ERROR(TE_EWRONGPTR);

    tmp_value = value;
    asn_clean_count(tmp_value); 

    if (status != NULL)
        *status = 0;

    if (*labels_buf == '\0')
        goto end;

    rest_labels = labels_buf;
    while ((rest_labels != NULL) && (*rest_labels != '\0') && (rc == 0))
    { 
        asn_value *new_value;
        rc = asn_child_named_index(tmp_value->asn_type, rest_labels, 
                                   &subval_index, &rest_labels);
        if (rc != 0)
        {
            break;
        }

        rc = asn_get_child_by_index(tmp_value, &new_value, subval_index);
        if (rc == TE_EASNOTHERCHOICE)
            break;
        else if (rc == TE_EASNINCOMPLVAL)
        {
            const asn_type *new_type;

            rc = 0;
            switch (tmp_value->syntax)
            {
                case SEQUENCE:
                case SET:
                case CHOICE:
                    {
                    const asn_named_entry_t *ne = 
                            (tmp_value->asn_type->sp.named_entries) +
                            subval_index;
                    new_type = ne->type;
                    }
                    break;

                case SEQUENCE_OF:
                case SET_OF:
                    new_type = tmp_value->asn_type->sp.subtype;
                    break;

                default:
                    rc = TE_EASNGENERAL;
                    break;
            }
            if (rc != 0)
                break;

            new_value = asn_init_value(new_type); 
            rc = asn_put_child_by_index(tmp_value, new_value, subval_index); 
        }
        tmp_value = new_value;
    }

    if (rc != 0 && status != NULL)
        *status = rc; 

end:

    return (rc == 0) ? tmp_value : NULL;
}

#undef RETURN_NULL_WITH_ERROR





/* see description in asn_usr.h */
te_errno
asn_put_child_value(asn_value *container, asn_value *subvalue, 
                    asn_tag_class tag_class, asn_tag_value tag_val)
{
    te_errno rc;
    int index;

    if (container == NULL) 
        return TE_EWRONGPTR; 

    container->txt_len = -1;
    asn_clean_count(subvalue); 

    rc = asn_child_tag_index(container->asn_type, tag_class, tag_val,
                             &index);
    if (rc != 0)
        return rc; 

    return asn_put_child_by_index(container, subvalue, index);
}



/* see description in asn_usr.h */
te_errno
asn_put_child_value_by_label(asn_value *container, asn_value *subvalue,
                             const char *label)
{
    int       index;
    te_errno  rc;

    const char *rest_labels = NULL;


    if(label == NULL || container == NULL)
        return TE_EWRONGPTR; 

    rc = asn_child_named_index(container->asn_type, 
                               label, &index, &rest_labels); 
    if (rc != 0) 
        return rc;

    if (rest_labels != NULL)
        return TE_EASNWRONGLABEL;

    return asn_put_child_by_index(container, subvalue, index);
}



/* see description in asn_impl.h */
te_errno
asn_get_child_by_index(const asn_value *container, asn_value **child,
                       int index)
{
    const asn_named_entry_t *ne;

    int child_internal_offset = index;

    switch (container->syntax)
    {
        case CHOICE:
        case TAGGED:
            child_internal_offset = 0; /* fall through ... */
        case SEQUENCE:
        case SET:
            if ((index < 0) || ((unsigned)index > container->asn_type->len))
                return TE_EASNWRONGLABEL;

            *child = container->data.array[child_internal_offset];

            if (*child == NULL)
                return TE_EASNINCOMPLVAL;

            if (container->syntax == TAGGED)
                break;

            /* additional check of subtype for named children */
            ne = container->asn_type->sp.named_entries + index;
#if 0
            fprintf(stderr, "%s(index %d) bad child tag or name\n"
                    "container: tag %d.%d, '%s', type name '%s'\n"
                    "    child: tag %d.%d, '%s'\n"
                    "  in type: tag %d.%d, '%s'\n",
                    __FUNCTION__, index,
                    (int)((container)->tag.cl), 
                    (int)((container)->tag.val),
                    (container)->name, container->asn_type->name,
                    (int)((*child)->tag.cl), 
                    (int)((*child)->tag.val), (*child)->name,
                    (int)(ne->tag.cl), (int)(ne->tag.val), ne->name);
            fflush(stderr);
#endif

            if (!asn_tag_equal((*child)->tag,  ne->tag) || 
                 (strcmp((*child)->name, ne->name) != 0)  ) 
            { 
                if (container->syntax == CHOICE)
                    return TE_EASNOTHERCHOICE; 
                else 
                    return TE_EASNGENERAL;
            }
            break;

        case SEQUENCE_OF:
        case SET_OF:
            if (index < 0)
                index += container->len;

            if (index < 0)
                return TE_EINVAL;

            if ((unsigned)index >= container->len)
                return TE_EASNINCOMPLVAL;

            *child = container->data.array[index]; 
            break;

        default:
            return TE_EASNWRONGTYPE;
    }

    return 0;
}



/* see description in asn_impl.h */
te_errno
asn_put_child_by_index(asn_value *container, asn_value *new_value, 
                       int leaf_type_index)
{
    int     index = 0;
    int     new_len;
    te_bool named_value = TRUE;

    container->txt_len = -1;

    switch (container->syntax)
    {
        case SEQUENCE_OF:
        case SET_OF: 
            named_value = FALSE; 
            new_len = container->len;

            while (leaf_type_index < 0)
                leaf_type_index += new_len;

            if (leaf_type_index >= new_len)
            {
                if (new_value == NULL)
                    return TE_EASNWRONGLABEL;

                new_len = leaf_type_index + 1;
                if ((container->data.array = 
                      realloc(container->data.array, 
                              new_len * sizeof(asn_value *))) 
                     == NULL) 
                    return TE_ENOMEM; 

                memset((void *)(container->data.array + container->len),
                       0, sizeof(asn_value *) * (new_len - container->len));

                container->len = new_len;
            }
#if 1
            if (new_value == NULL)
            {
                unsigned int i = leaf_type_index;
                asn_free_value(container->data.array[i]);
                container->len--;

                for (; i < container->len; i++)
                    container->data.array[i] = container->data.array[i+1];

                container->data.array = 
                    realloc(container->data.array, 
                            container->len * sizeof(asn_value *));

                return 0;
            }
#endif

            /* pass through ... */
        case SEQUENCE:
        case SET:
            index = leaf_type_index;
            /* pass through ... */

        case CHOICE:

            if (container->data.array[index] != NULL)
                asn_free_value(container->data.array[index]);

            container->data.array[index] = new_value; 
            break;

        default:
            return TE_EASNWRONGTYPE; 
    }

    /* now set name of new subvalue, if it is. */
    if (new_value != NULL && named_value)
    {
        const asn_named_entry_t *ne =
            (container->asn_type->sp.named_entries) + leaf_type_index;

        if (new_value->syntax & COMPOUND)
            new_value->txt_len = -1;

        free(new_value->name); 
        new_value->name = asn_strdup(ne->name);
        new_value->tag  = ne->tag;
    }

    return 0;
}



te_errno
asn_put_choice(asn_value *container, asn_value *value)
{
    int i, n_subtypes = container->asn_type->len;
    const asn_named_entry_t * ne = 
                    container->asn_type->sp.named_entries;

    if (container->syntax != CHOICE)
        return TE_EASNWRONGTYPE;

    for (i = 0; i < n_subtypes; i++)
    {
        /* It is rather dangerous to compare pointers of 
         * asn_type structures, but in current implementation
         * it is equivalent to identity of ASN.1 types. 
         */
        if (ne[i].type == value->asn_type)
            break;
    }
    if (i == n_subtypes) 
        return TE_EASNWRONGTYPE;

    return asn_put_child_by_index(container, value, i);
}







/* see description in asn_usr.h */
te_errno
asn_get_descendent(const asn_value *container,
                   asn_value **found_value, 
                   const char *labels)
{
    te_errno rc = 0;
#if 1

    if (!container || !found_value)
        return TE_EWRONGPTR; 

    *found_value = asn_find_descendant(container, &rc, labels);

#else
    const char *rest_labels = labels;
    asn_value  *tmp_value = (asn_value *)container;

    int index;

    if (!container || !found_value)
        return TE_EWRONGPTR; 

    while (rest_labels != NULL && (*rest_labels != '\0'))
    { 
        rc = asn_child_named_index(tmp_value->asn_type, rest_labels, 
                                   &index, &rest_labels);
        if (rc != 0)
        {
            if ((asn_get_syntax(tmp_value, NULL) == CHOICE) && 
                ((rc = asn_get_choice_value(tmp_value, &tmp_value, 
                                               NULL, NULL)) == 0))
                continue;

            return rc;
        }

        rc = asn_get_child_by_index(tmp_value, &tmp_value, index);
        if (rc != 0)
            return rc; 
    }

    if (rc == 0)
        *found_value = tmp_value;
#endif
    return rc;
}


/* See description in asn_usr.h */
te_errno
asn_put_descendent(asn_value *container, asn_value *subval, 
                   const char *labels)
{
    const char  *rest_labels = labels;
    asn_value   *par_value = container;
    asn_value   *tmp;
    te_errno     rc = 0;

    int index;

    if (!container || !labels)
        return TE_EWRONGPTR; 

    while (rest_labels != NULL && (*rest_labels != '\0'))
    { 
        rc = asn_child_named_index(par_value->asn_type, rest_labels, 
                                   &index, &rest_labels);
        if (rc != 0)
            break;

        if (rest_labels == NULL)
            rc = asn_put_child_by_index(par_value, subval, index);
        else
        {
            rc = asn_get_child_by_index(par_value, &tmp,
                                        index);
            if (rc == TE_EASNINCOMPLVAL)
            {
                if (subval == NULL)
                    return 0;
                else
                {
                    const asn_type *new_type;

                    if (par_value->syntax & ASN_SYN_NAMED)
                        new_type = par_value->asn_type->
                                        sp.named_entries[index].type;
                    else
                        new_type = par_value->asn_type->sp.subtype;

                    tmp = asn_init_value(new_type); 

                    rc = asn_put_child_by_index(par_value, tmp, index); 
                    par_value = tmp;
                }
            }
            else
            {
                par_value = tmp;
            }
        }
        if (rc != 0)
            break;
    }
    return rc;
}

/* See description in asn_usr.h */
te_errno
asn_get_indexed(const asn_value *container, asn_value **subval, 
                int index, const char *labels)
{
    te_errno         rc;
    const asn_value *indexed_value = NULL;

    if (!container || !subval)
        return TE_EWRONGPTR; 

    if ((rc = asn_get_descendent(container, (asn_value **)&indexed_value,
                                 labels)) != 0)
        return rc;

    if (indexed_value->syntax != SEQUENCE_OF && 
        indexed_value->syntax != SET_OF)
        return TE_EASNWRONGTYPE;

    if (index < 0) 
        index += (int)indexed_value->len;

    if ((unsigned int)index >= indexed_value->len)
        return TE_EASNINCOMPLVAL; 

    *subval = indexed_value->data.array[index];

    return 0;
}



/* See description in asn_usr.h */
te_errno
asn_write_primitive(asn_value *value, const void *data, size_t d_len)
{
    unsigned int m_len = d_len; /* length of memory used */

    if (value  == NULL)
        return TE_EWRONGPTR; 

    value->txt_len = -1;

    switch(value->syntax)
    {
    case BOOL:
        if (d_len == 0) return TE_EINVAL;

        if (* (char*)data) /* TRUE */
        {
            value->data.integer = 0xff;
            value->txt_len = 4;
        } 
        else  /* FALSE */
        {
            value->data.integer = 0;
            value->txt_len = 5;
        }
        break;

    case INTEGER: 
    case ENUMERATED: 
        {
            long int val;
            switch (d_len)
            {
                case 0: 
                        return TE_EINVAL; 
                case sizeof(char) : 
                        val = *((const unsigned char *) data); 
                        break;
                case sizeof(short): 
                        val = *((const short *)data); 
                        break;
                case sizeof(long) : 
                        val = *((const long *) data); 
                        break;
                default: 
                        val = *((const int *) data);
            }
            if (value->syntax == INTEGER)
                value->txt_len = number_of_digits(val);
            value->data.integer = val;
        }
        break;

    case CHAR_STRING: 
        free(value->data.other);
        if (d_len == 0 || data == NULL)
        {
            value->data.other = NULL;
            value->len = 0;
            value->txt_len = 2;
        }
        else
        { 
            char *str = value->data.other = malloc(d_len + 1);
            strncpy(str, data, d_len);
            str[d_len] = '\0';
            value->len = d_len + 1; /* quantity of ALL used octets */
            value->txt_len = strlen(value->data.other) + 2;
            for (;*str != '\0'; str++)
                if (*str == '"') value->txt_len++;
        }
        break;

    case PR_ASN_NULL:
        break;

    case BIT_STRING: 
    case OID: 
        if (value->syntax == OID)
            m_len *= sizeof(int);
        else
            m_len = (m_len + 7) >> 3;
        /* fall through */
    case LONG_INT:
    case REAL:
    case OCT_STRING:
        if (d_len == 0 || data == NULL) 
        {
            value->data.other = NULL;
            value->len = 0;
        }
        else
        {
            void * val = malloc(m_len);

            if (value->asn_type->len > 0 &&
                value->asn_type->len != d_len)
            {
                return TE_EASNWRONGSIZE;
            }

            if (value->data.other)
                free (value->data.other);
            value->data.other = val;
            memcpy(val,  data, m_len);
            value->len = d_len; 
        }
        if (value->syntax == OCT_STRING)
            value->txt_len = d_len * 3 + 3; 

        break;

    default:
        return TE_EASNWRONGTYPE;
    }

    return 0;
}




/*
 * ========================= OLD ============================
 */


/* see description in asn_usr.h */
te_errno
asn_write_value_field(asn_value *container, const void *data, size_t d_len, 
                      const char *field_labels)
{
#if AVOID_STRSEP
    te_errno   rc;
    asn_value *subvalue = asn_retrieve_descendant(container, &rc, 
                                                  field_labels);
    if (subvalue == NULL)
        return rc;

    container->txt_len = -1;
    asn_clean_count(container); 
    rc = asn_write_primitive(subvalue, data, d_len);
#else
    char     *field_labels_int_copy = asn_strdup(field_labels); 
    te_errno  rc = asn_impl_write_value_field(container, data, d_len,
                                              field_labels_int_copy);
    free(field_labels_int_copy);

#endif
    return rc;
}


#if !AVOID_STRSEP
/**
 * Write data into primitive syntax leaf in specified ASN value, internal 
 * implemetation of this functionality.
 *
 * @param container     pointer to ASN value which leaf field is interested;
 * @param data          data to be written, should be in nature C format for
 *                      data type respective to leaf syntax;
 * @param d_len         length of data; 
 *                      Measured in octets for all types except OID and
 *                      BIT_STRING; for OID measured in sizeof(int),
 *                      for BIT_STRING measured in bits 
 * @param field_labels  string with dot-separated sequence of textual field
 *                      labels, specifying primitive-syntax leaf in ASN value 
 *                      tree with 'container' as a root. Label for 
 *                      'SEQUENCE OF' and 'SET OF' subvalues is decimal
 *                      notation of its integer index in array.
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno
asn_impl_write_value_field(asn_value *container, 
                           const void *data, size_t d_len, 
                           char *field_labels)
{
    unsigned int m_len = d_len; /* length of memory used */
    asn_value      *subvalue;
    const asn_type *subtype;

    if (container  == NULL|| 
        ((container->syntax & COMPOUND) == 0 &&
        container->syntax != PR_ASN_NULL && 
        data == NULL && d_len != 0))
        return TE_EWRONGPTR; 

    container->txt_len = -1;

    switch(container->syntax)
    {
    case BOOL:
        if (d_len == 0) return TE_EINVAL;

        if (* (char*)data) /* TRUE */
        {
            container->data.integer = 0xff;
            container->txt_len = 4;
        } 
        else  /* FALSE */
        {
            container->data.integer = 0;
            container->txt_len = 5;
        }
        break;

    case INTEGER: 
    case ENUMERATED: 
        {
            long int val;
            switch (d_len)
            {
                case 0: return TE_EINVAL; 
                case sizeof(char) : val = *((const unsigned char *) data); break;
                case sizeof(short): val = *((const short *)data); break;
                case sizeof(long) : val = *((const long *) data); break;
                default: val = *((const int *) data);
            }
            if (container->syntax == INTEGER)
                container->txt_len = number_of_digits(val);
            container->data.integer = val;
        }
        break;

    case CHAR_STRING:

        free(container->data.other);
        if (d_len == 0)
        {
            container->data.other = NULL;
            container->len = 0;
            container->txt_len = 2;
        }
        else
        { 
            char *str = container->data.other = malloc(d_len + 1);
            strncpy(str, data, d_len);
            str[d_len] = '\0';
            container->len = d_len + 1; /* quantity of ALL used octets */
            container->txt_len = strlen(container->data.other) + 2;
            for (;*str != '\0'; str++)
                if (*str == '"') container->txt_len++;
        }
        break;

    case PR_ASN_NULL:
        break;

    case BIT_STRING: 
    case OID: 
        if (container->syntax == OID)
            m_len *= sizeof(int);
        else
            m_len = (m_len + 7) >> 3;
        /* fall through */
    case LONG_INT:
    case REAL:
    case OCT_STRING:
        if (d_len == 0) 
        {
            container->data.other = NULL;
            container->len = 0;
        }
        else
        {
            void * val = malloc(m_len);

            if (container->asn_type->len > 0 &&
                container->asn_type->len != d_len)
            {
                return TE_EASNGENERAL;
            }

            if (container->data.other)
                free (container->data.other);
            container->data.other = val;
            memcpy(val,  data, m_len);
            container->len = d_len; 
        }
        if (container->syntax == OCT_STRING)
            container->txt_len = d_len * 3 + 3; 
        break;

    case SEQUENCE:
    case SET:
    case SEQUENCE_OF:
    case SET_OF: 
    case CHOICE:
    case TAGGED:
        {
            char       * rest_field_labels = field_labels;
            char       * cur_label = "";
            te_errno     rc;


            switch (container->syntax)
            {
                case TAGGED: break;
                case CHOICE: 
                    if (rest_field_labels && (*rest_field_labels == '#'))
                        rest_field_labels ++;
                    else 
                        break;
                default:
                    cur_label = strsep(&rest_field_labels, ".");
            }
            rc = asn_impl_find_subvalue_writable(container, cur_label, 
                                                 &subvalue);

            switch (rc)
            {
                case 0:
                    rc = asn_impl_write_value_field(subvalue, data, d_len,
                                                    rest_field_labels);
                break;

                case TE_EASNINCOMPLVAL:
                { 

                    rc = asn_impl_find_subtype(container->asn_type, 
                                               cur_label, &subtype);
                    if (rc) break;

                    subvalue = asn_init_value(subtype); 

                    rc = asn_impl_write_value_field(subvalue, data, d_len,
                                                    rest_field_labels);
                    if (rc) break; 

                    rc = asn_put_child_value_by_label(container, subvalue,
                                                      cur_label);
                } 
            } 

            return rc;
        }
    default: 
        return 0; /* nothing to do. */
    }

    return 0;
}
#endif

/**
 * Read data from primitive syntax leaf in specified ASN value.
 *
 * @param container     pointer to ASN value which leaf field is interested.
 * @param data          pointer to buffer for read data.
 * @param d_len         length of available buffer / read data (IN/OUT). 
 * @param field_labels  string with dot-separated sequence of textual field
 *                      labels, specifying primitive-syntax leaf in 
 *                      ASN value tree with 'container' as a root. 
 *                      Label for 'SEQUENCE OF' and 'SET OF' subvalues 
 *                      is decimalnotation of its integer index in array.
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno
asn_read_value_field(const asn_value *container, void *data, size_t *d_len,
                     const char *field_labels)
{
    char *field_labels_int_copy = asn_strdup(field_labels); 
    int   rc = asn_impl_read_value_field(container, data, d_len, 
                                         field_labels_int_copy);

    free (field_labels_int_copy);

    return rc;
} 

/**
 * Read data into primitive syntax leaf in specified ASN value, internal 
 * implemetation of this functionality.
 *
 * @param container     pointer to ASN value which leaf field is interested
 * @param data          pointer to buffer for read data (OUT)
 * @param d_len         length of available buffer / read data (IN/OUT); 
 *                      measured in octets for all types except OID and
 *                      BIT_STRING; for OID measured in sizeof(int),
 *                      for BIT_STRING measured in bits 
 * @param field_labels  string with dot-separated sequence of textual field
 *                      labels, specifying primitive-syntax leaf in 
 *                      ASN value tree with 'container' as a root. 
 *                      Label for 'SEQUENCE OF' and 'SET OF' subvalues 
 *                      is decimalnotation of its integer index in array.
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno
asn_impl_read_value_field(const asn_value *container,  void *data, 
                          size_t *d_len, char *field_labels)
{
    asn_value *value;
    size_t     m_len;
    te_errno   rc;

    if (!container) return TE_EWRONGPTR; 

    rc = asn_get_descendent(container, &value, field_labels); 
    if (rc) return rc;

    if (value->syntax != PR_ASN_NULL &&
        ((data == NULL) || (d_len == NULL)))
        return TE_EWRONGPTR; 

    m_len = value->len; 

    switch(value->syntax)
    {
    case BOOL:
#if 0
        if (*d_len)
        {
            *((char*)data) = (char) value->data.integer;
            *d_len = 1;
        }
        else 
            return TE_ESMALLBUF;
        break;
#endif
    case INTEGER: 
    case ENUMERATED: 
        {
            long int val = value->data.integer;

            if (*d_len > sizeof(long int))
                *d_len = sizeof(long int);

            switch (*d_len)
            {
                case sizeof(int8_t) : *((int8_t *) data) = val; break;
                case sizeof(int16_t): *((int16_t *)data) = val; break;
                case sizeof(int32_t): *((int32_t *)data) = val; break;
                case sizeof(int64_t): *((int64_t *)data) = val; break;
                default: 
                    return TE_EASNGENERAL;
            }
        }
        break;

    case OID:
    case BIT_STRING:
        if (value->syntax == OID)
            m_len *= sizeof(int);
        else
            m_len = (m_len + 7) >> 3;
        /* fall through */
    case CHAR_STRING:
    case LONG_INT:
    case OCT_STRING:
    case REAL:
        { 
            if (*d_len < value->len)
                return TE_ESMALLBUF;

            *d_len = value->len;
            memcpy(data, value->data.other, m_len);
        }
        break;

    case PR_ASN_NULL:
        break;

    case SEQUENCE:
    case SET:
    case SEQUENCE_OF:
    case SET_OF: 
        return TE_EASNNOTLEAF;

    case CHOICE:
    case TAGGED:
        {
            const asn_value *subval = value->data.array[0];

            if (subval == NULL)
                return TE_EASNINCOMPLVAL;
            else
                return asn_impl_read_value_field(subval, data, d_len, "");
        }
        break; /* unreachable */

    default: 
        return 0; /* nothing to do. */
    }

    return 0;
}


/* see description in asn_usr.h */
te_errno
asn_write_int32(asn_value *container, int32_t value, const char *labels)
{
    return asn_write_value_field(container, &value, sizeof(value), labels);
}

/* see description in asn_usr.h */
te_errno
asn_read_int32(const asn_value *container, int32_t *value,
               const char *labels)
{
    size_t len = sizeof(*value);
    return asn_read_value_field(container, value, &len, labels);
}

/* see description in asn_usr.h */
te_errno
asn_write_bool(asn_value *container, te_bool value, const char *labels)
{
    return asn_write_value_field(container, &value, sizeof(value), labels);
}

/* see description in asn_usr.h */
te_errno
asn_read_bool(const asn_value *container, te_bool *value,
              const char *labels)
{
    size_t len = sizeof(*value);
    return asn_read_value_field(container, value, &len, labels);
}

/* see description in asn_usr.h */
te_errno
asn_write_string(asn_value *container, const char *value,
                 const char *labels)
{
    const asn_type *leaf_type;
    te_errno rc;

    if (container == NULL || value == NULL)
        return TE_EWRONGPTR;

    if ((rc = asn_get_subtype(container->asn_type, &leaf_type,  labels))
         != 0)
        return rc;

    if (leaf_type->syntax != CHAR_STRING)
        return TE_EASNWRONGTYPE;

    return asn_write_value_field(container,
                                 value, strlen(value)+1, labels);
}

/* see description in asn_usr.h */
te_errno
asn_read_string(const asn_value *container, char **value,
                const char *labels)
{
    const asn_value *leaf_val;
    te_errno rc;

    if (container == NULL || value == NULL)
        return TE_EWRONGPTR;

    if ((rc = asn_get_subvalue(container, (asn_value **)&leaf_val,
                               labels)) != 0) 
        return rc;

    if (leaf_val->syntax != CHAR_STRING)
        return TE_EASNWRONGTYPE;

    if (leaf_val->data.other == NULL)
        *value = asn_strdup("");
    else
        *value = asn_strdup(leaf_val->data.other); 
 
    return (*value != NULL) ? 0 : TE_ENOMEM;
}


/**
 * Write component of COMPOUND subvalue in ASN value tree.
 *
 * @param container     Root of ASN value tree which subvalue to be changed.
 * @param elem_value    ASN value to be placed into the tree at place,
 *                      specified by subval_labels.
 * @param subval_labels string with dot-separated sequence of textual field
 *                      labels, specifying subvalue in ASN value 
 *                      tree with 'container' as a root. Label for 
 *                      'SEQUENCE OF' and 'SET OF' subvalues is decimal
 *                      notation of its integer index in array.  
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno
asn_write_component_value(asn_value *container, 
                          const asn_value *elem_value,
                          const char *subval_labels)
{
#if AVOID_STRSEP
    te_errno rc;
    asn_value *subvalue = asn_retrieve_descendant(container,
                                                  &rc, subval_labels); 

    if (rc != 0)
        return rc;

    rc = asn_assign_value(subvalue, elem_value);
#else
    char *field_labels_int_copy = asn_strdup(subval_labels); 

    te_errno   rc = asn_impl_write_component_value(container, elem_value,
                                              field_labels_int_copy);
    free (field_labels_int_copy);
#endif
    return rc;
}


#if !AVOID_STRSEP
/**
 * Write component of COMPOUND subvalue in ASN value tree,
 * internal implementation.
 * 
 *
 * @param container     Root of ASN value tree which subvalue to be changed.
 * @param elem_value    ASN value to be placed into the tree at place,
 *                      specified by subval_labels.
 * @param subval_labels string with dot-separated sequence of textual field
 *                      labels, specifying subvalue in ASN value 
 *                      tree with 'container' as a root. Label for 
 *                      'SEQUENCE OF' and 'SET OF' subvalues is decimal
 *                      notation of its integer index in array.  
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno
asn_impl_write_component_value(asn_value *container, 
                               const asn_value *elem_value,
                               char *subval_labels)
{
    char        *rest_field_labels = subval_labels;
    char        *cur_label = "";
    asn_value * subvalue;
    te_errno     rc;

                asn_value *new_value = NULL;

    if (!container || !elem_value || !subval_labels) return TE_EWRONGPTR; 

    if (!(container->syntax & COMPOUND))
        return TE_EASNWRONGLABEL;

    container->txt_len = -1;
    elem_value->txt_len = -1;

    switch (container->syntax)
    {
        case TAGGED: break;
        case CHOICE: 
            if (*rest_field_labels == '#')
                rest_field_labels ++;
            else 
                break;
        default:
            cur_label = strsep (&rest_field_labels, ".");
    } 

    rc = asn_impl_find_subvalue_writable(container, cur_label, &subvalue);
    if (rest_field_labels && (*rest_field_labels))
    { 
        /* There are more levels.. */
        if (rc) return rc;

        rc = asn_impl_write_component_value(subvalue, elem_value,
                                        rest_field_labels);
    }
    else
    { 
        /* no more labels, subvalue should be changed at this level */
        switch(rc)
        {
            case 0: 
                /* there is subvalue on that place, should be freed first.*/
                asn_put_child_value_by_label(container, NULL, cur_label);
                /* pass through ... */
            case TE_EASNINCOMPLVAL:
            {

                if ((container->syntax == CHOICE) && (*cur_label == '\0'))
                {
                    int i, n_subtypes = container->asn_type->len;
                    const asn_named_entry_t * ne = 
                                    container->asn_type->sp.named_entries;

                    for (i = 0; i < n_subtypes; i++)
                    {
                        /* It is rather dangerous to compare pointers of 
                         * asn_type structures, but in current implementation
                         * it is equivalent to identity of ASN.1 types. 
                         */
                        if (ne[i].type == elem_value->asn_type)
                        {
                            cur_label = ne[i].name;
                            break;
                        }
                    }
                    if (i == n_subtypes) 
                    {
                        return TE_EASNWRONGTYPE;
                    }
                }
                else
                { 
                    const asn_type *subtype = NULL;
                    rc = asn_impl_find_subtype(container->asn_type, 
                                               cur_label, &subtype);
                    if (rc) return rc;
                    
                    if (strcmp(subtype->name, elem_value->asn_type->name))
                    {
                        return TE_EASNWRONGTYPE;
                    }
                    if ((subtype->syntax == CHOICE) && 
                        (elem_value->syntax != CHOICE))
                    {
                        new_value = asn_init_value(subtype);
                        rc = asn_impl_write_component_value
                                        (new_value, elem_value, "");
                        if (rc)
                        {
                            asn_free_value(new_value);
                            return rc;
                        }
                    }
                }

                if (new_value == NULL)
                    new_value = asn_copy_value(elem_value);

                if (new_value == NULL) 
                {
                    return TE_EASNGENERAL;
                }

                rc = asn_put_child_value_by_label(container, new_value,
                                                  cur_label);

            }
            default:
                break;
        }
    }
    return rc; 
}
#endif



/**
 * See description in asn_usr.h
 */
te_errno 
asn_get_subvalue(const asn_value *container, asn_value **subval,
                 const char *subval_labels)
{
    return asn_get_descendent(container, subval,
                              subval_labels);
}


/**
 * See description in asn_usr.h
 */
te_errno
asn_get_child_type(const asn_type *type, const asn_type **subtype,
                   asn_tag_class tag_class, asn_tag_value tag_val)
{
    int index; 
    te_errno rc;

    if (type == NULL || subtype == NULL)
        return TE_EWRONGPTR; 

    rc = asn_child_tag_index(type, tag_class, tag_val, &index);
    if (rc != 0)
        return rc;
    
    *subtype = type->sp.named_entries[index].type;

    return 0;
}

/**
 * See description in asn_usr.h
 */
te_errno
asn_get_child_value(const asn_value *container, const asn_value **subval,
                    asn_tag_class tag_class, asn_tag_value tag_val)
{
    int index;
    te_errno rc;

    if (!container || !subval)
        return TE_EWRONGPTR; 

    if (container->syntax != SEQUENCE && 
        container->syntax != SET)
        return TE_EASNWRONGTYPE;

    rc = asn_child_tag_index(container->asn_type, tag_class, tag_val,
                             &index);
    if (rc != 0)
        return rc;

    if ((*subval = container->data.array[index]) == NULL)
        return TE_EASNINCOMPLVAL;

    return 0;
}


/**
 * See description in asn_usr.h
 */
te_errno
asn_get_choice_value(const asn_value *container, asn_value **subval,
                     asn_tag_class *tag_class, asn_tag_value *tag_val)
{
    asn_value *sv;
    if (!container)
        return TE_EWRONGPTR; 

    if (container->syntax != CHOICE)
        return TE_EINVAL; 

    if (container->len == 0 || container->data.array[0] == NULL)
        return TE_EASNINCOMPLVAL;

    sv = container->data.array[0];

    if (subval != NULL)
        *subval = sv;
    if (tag_class != NULL)
        *tag_class = sv->tag.cl;
    if (tag_val != NULL)
        *tag_val = sv->tag.val;

    return 0;
}


/**
 * See description in asn_usr.h
 */
te_errno 
asn_get_field_data(const asn_value *container, 
                   void *location, const char *subval_labels)
{
    const asn_value  *subval = container;
    const void      **data_ptr = (const void **)location;    

    te_errno rc;


    if (data_ptr == NULL || container == NULL)
    {
        return TE_EWRONGPTR;
    }

    if (container->syntax & COMPOUND)
    {
        if ((rc = asn_get_subvalue(container, (asn_value **)&subval,
                                   subval_labels))
            != 0)
            return rc;
    }
    else if (subval_labels && *subval_labels)
    {
        return TE_EASNWRONGLABEL;
    }

    switch (subval->syntax) 
    {
        case BOOL: 
        case INTEGER: 
        case ENUMERATED: 
            *data_ptr = &subval->data.integer; 
            break;

        case OID:
        case CHAR_STRING:
        case LONG_INT:
        case OCT_STRING:
        case BIT_STRING:
        case REAL:
            *data_ptr = subval->data.other;
            break;

        case PR_ASN_NULL:
            *data_ptr = NULL;
            break;

        case SEQUENCE:
        case SET:
        case SEQUENCE_OF:
        case SET_OF: 
        case CHOICE:
        case TAGGED:
            return TE_EASNNOTLEAF;

        default: 
            return 0; /* nothing to do. */
    }
    return 0;
}


/**
 * See description in asn_usr.h
 */ 
te_errno
asn_read_component_value (const asn_value *container, 
                asn_value ** elem_value, const char *subval_labels)
{
    const asn_value  *subvalue;
    te_errno     rc; 

    rc = asn_get_subvalue(container, (asn_value **)&subvalue, 
                          subval_labels); 
    if (rc) return rc;

    *elem_value = asn_copy_value(subvalue);
    if (*elem_value == NULL) 
        return TE_ENOMEM;

    /* fall into CHOICE and TAGGED */
    if ( (*elem_value)->syntax == CHOICE ||
         (*elem_value)->syntax == TAGGED   )
    {
        asn_value *transparent_val = *elem_value;
        *elem_value = asn_copy_value(transparent_val->data.array[0]);
        asn_free_value(transparent_val);
    }

    return 0; 
}


/**
 * Replace array element in indexed ('SEQUENCE OF' or 'SET OF') subvalue 
 * of root ASN value container. 
 *
 * @param container     Root of ASN value tree which subvalue is interested.
 * @param elem_value    ASN value to be placed into the array, specified 
 *                      by subval_labels at place, specified by index.
 * @param index         Array index of element to be replaced. 
 * @param subval_labels string with dot-separated sequence of textual field
 *                      labels, specifying indexed syntax subvalue in ASN value 
 *                      tree with 'container' as a root. 
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno
asn_write_indexed(asn_value *container, const asn_value *elem_value, 
                  int index, const char *subval_labels)
{
    asn_value *value = container;

    te_errno rc;

    rc = asn_impl_fall_down_to_tree_writable(container, subval_labels,
                                             &value);
    if (rc != 0)
        return rc;

    container->txt_len = -1;

    if (strcmp(elem_value->asn_type->name, value->asn_type->sp.subtype->name))
    {
        return TE_EASNWRONGTYPE; 
    }

    switch (value->syntax)
    {
        case SEQUENCE_OF:
        case SET_OF: 
            asn_free_value(value->data.array[index]); 
            value->data.array[index] = asn_copy_value(elem_value);
            break;
        
        default:
            return TE_EASNGENERAL;
    }

    return 0;
}

/**
 * Read array element in indexed ('SEQUENCE OF' or 'SET OF') subvalue 
 * of root ASN value 'container'. 
 *
 * @param container     Root of ASN value tree which subvalue is interested.
 * @param index         Array index of element to be read 
 * @param subval_labels string with dot-separated sequence of textual field
 *                      labels, specifying indexed syntax subvalue in ASN value 
 *                      tree with 'container' as a root. 
 *
 * @return pointer to new ASN_value instance or NULL if error occurred. 
 */ 
asn_value *
asn_read_indexed(const asn_value *container, int index,
                 const char *subval_labels)
{
    const asn_value *value = container;

    if (asn_get_subvalue(container, (asn_value **)&value, subval_labels)) 
        return NULL;


    if (index < 0) index += (int)value->len;

    if ( (index < 0) || (index >= (int)value->len) ) return NULL;

    if (value->syntax != SEQUENCE_OF &&
        value->syntax != SET_OF)
        return NULL;

    asn_value * subval = value->data.array[index];
    while ((subval->syntax == CHOICE ) || (subval->syntax == TAGGED ))
        if ((subval = subval->data.array[0]) == NULL) 
            return NULL;

    return asn_copy_value(subval); 
}

/* see description in asn_usr.h */
te_errno
asn_insert_indexed(asn_value *container, asn_value *elem_value, 
                   int index, const char *subval_labels)
{
    asn_value  *value;
    te_errno    r_c = 0; 
    int         new_len;

#if 1
    value = asn_retrieve_descendant(container, &r_c, subval_labels);
    
#else
    r_c = asn_impl_fall_down_to_tree_writable(container, subval_labels,
                                              &value); 
#endif

    if (r_c != 0)
        return r_c;

    container->txt_len = -1;
    asn_clean_count(elem_value); /* event it was calculated already,
                                 it not actual, because indent is 
                                 changed by this insert */

    if (strcmp(elem_value->asn_type->name,
               value->asn_type->sp.subtype->name))
    {
        return TE_EASNWRONGTYPE;
    }

    new_len = value->len + 1;

    if (index < 0)
        index += new_len;

    if ((index < 0) || (index >= new_len))
        return TE_EASNWRONGLABEL;

    if (value->syntax != SEQUENCE_OF &&
        value->syntax != SET_OF)
        return TE_EASNWRONGTYPE;

    {
        asn_value **arr = malloc(new_len * sizeof(asn_value *));

        unsigned int i;

        if (arr == NULL)
            return TE_ENOMEM;

        for (i = 0; i < (unsigned)index; i++)
            arr[i] = value->data.array[i];

        arr[index] = elem_value; 

        for (; i < value->len; i++)
            arr[i+1] = value->data.array[i];

        if (value->data.array)
            free(value->data.array);
        value->data.array = arr;
        value->len = new_len;
    }

    return 0;
}

/**
 * Remove array element from indexed syntax (i.e. 'SEQUENCE OF' or 'SET OF') 
 * subvalue of root ASN value container. 
 *
 * @param container     Root of ASN value tree which subvalue is interested.
 * @param index         Array index of element to be removed.
 * @param subval_labels string with dot-separated sequence of textual field
 *                      labels, specifying indexed syntax subvalue in ASN value 
 *                      tree with 'container' as a root. 
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno
asn_remove_indexed(asn_value * container, int index, const char *subval_labels)
{
    asn_value *value = container;
    te_errno   r_c = 0; 

    r_c = asn_impl_fall_down_to_tree_writable(container, subval_labels, &value); 

    if (r_c) return r_c;

    container->txt_len = -1;

    if (index < 0)
        index += (int)value->len;

    if ((index < 0) || (index >= (int)value->len))
        return TE_EASNWRONGLABEL;

    if (value->syntax != SEQUENCE_OF &&
        value->syntax != SET_OF)
        return TE_EASNWRONGTYPE;

    {
        asn_value * *arr = NULL;
        unsigned int i;
        
        if (value->len > 1)
        {
            arr = malloc((value->len - 1) * sizeof(asn_value *)); 
            if (arr == NULL) return TE_ENOMEM;
        }

        value->len --;

        for (i = 0; i < (unsigned)index; i++)
            arr[i] = value->data.array[i];

        asn_free_value(value->data.array[index]); 

        for (; i < value->len; i++)
            arr[i] = value->data.array[i+1];

        free(value->data.array);
        value->data.array = arr;
    }

    return 0;
}

/**
 * Get length of subvalue of root ASN value container. 
 *
 * @param container     Root of ASN value tree which subvalue is interested.
 * @param subval_labels string with dot-separated sequence of textual field
 *                      labels, specifying indexed syntax subvalue in ASN value 
 *                      tree with 'container' as a root. 
 *
 * @return number of elements in constractive subvalue, -1 if error occurred. 
 */ 
int
asn_get_length(const asn_value *container, const char *subval_labels)
{
    const asn_value *val;

    if (asn_get_subvalue(container, (asn_value **)&val, subval_labels)) 
        return -1;

    if (val->syntax == CHOICE)
    {
        val = val->data.array[0];
        if (val == NULL)
            return -1;
    }

    return val->len; 
}



/**
 * Fall down in tree according passed field labels.
 *
 * @param container     pointer to ASN value which leaf field is interested;
 * @param field_labels  textual field label, specifying subvalue of 'container'. 
 *                      Label for 'SEQUENCE OF' and 'SET OF' subvalues 
 *                      is decimal notation of its integer index in array.
 * @param found_value   found subvalue respecive to field_labels (OUT). 
 *
 * @return zero on success, otherwise error code.
 */
te_errno 
asn_impl_fall_down_to_tree_nc(const asn_value *container, char *field_labels,
                              const asn_value **found_value)
{
    te_errno   rc = 0; 
    const asn_value *value;

#if AVOID_STRSEP

    value = asn_find_descendant(container, &rc, field_labels);
#else

    char *rest_labels = field_labels; 
    char *cur_label;


    value = container;

    while ((rest_labels && (*rest_labels)) || (value->syntax == CHOICE))
    { 
        const asn_value *subvalue;
        cur_label = "";

        switch (value->syntax)
        {
            case TAGGED: break;
            case CHOICE: 
                if (rest_labels && (*rest_labels == '#'))
                    rest_labels ++;  
                    /* fall through to the 'default' label. */
                else 
                    break;
            default:
                cur_label = strsep (&rest_labels, ".");
        } 
        rc = asn_impl_find_subvalue(value, cur_label, &subvalue);
        value = subvalue; 

        if (rc) break;
    }


#endif
    if (rc == 0)
        *found_value = (asn_value *) value;

    return rc;
}




/**
 * Find one-depth subvalue in ASN value tree by its label.
 * This method is applicable only to values with COMPOUND syntax. 
 *
 * @param container  pointer to ASN value which leaf field is interested;
 * @param label      textual field label, specifying subvalue of 'container'. 
 *                   Label for 'SEQUENCE OF' and 'SET OF' subvalues 
 *                   is decimal notation of its integer index in array.
 * @param found_val  pointer to found subvalue (OUT).
 *
 * @return zero on success, otherwise error code. 
 */ 
te_errno
asn_impl_find_subvalue(const asn_value *container, const char *label, 
                       asn_value const **found_val)
{ 
    const char *rest = NULL;

    if (!container || !found_val)
        return TE_EWRONGPTR; 

    if (!(container->syntax & COMPOUND))
    {
        return TE_EASNGENERAL; 
    }

    if (container->syntax & ASN_SYN_NAMED)
    { /* we have current node constraint with named subtrees */
        int      index;
        te_errno rc;

        const asn_value **arr = (const asn_value **)container->data.array;

        switch (container->syntax)
        {
        case CHOICE:
            index = 0;

            if (arr[0] == NULL) 
                return TE_EASNINCOMPLVAL;

            if ((label == NULL) || 
                (*label == 0)   ||
                (strcmp(label, arr[0]->name) == 0))
                break;
            
            /* special label denote that no fall to choice should be performed*/
            if (*label == '\001') 
            {
                *found_val = container;
                return ASN_STOP_AT_CHOICE;
            } 

            return TE_EASNOTHERCHOICE;

        case SET: /*order of subvalues in SET syntax value may be arbitrary. */
            for (index = 0; index < (int)container->len; index++)
                if (strcmp(label, arr[index]->name) == 0)
                    break;

            if (index < (int)container->len) break;

            return TE_EASNINCOMPLVAL;

        default:
            rc = asn_child_named_index(container->asn_type, 
                                       label, &index, &rest); 
            if (rc) return rc;

            if (arr[index] == NULL)
                return TE_EASNINCOMPLVAL;
            break;
        }

        *found_val = arr[index];
        return 0;
    }
    else if (container->syntax == TAGGED)
    { 
        if ((*found_val = container->data.array[0]) == NULL)
            return TE_EASNINCOMPLVAL;

        return 0;
    } /* We have *_OF value */ 
    else 
    {
        int index;
        char *rest;
        index = strtol (label, &rest, 10);
        if (*rest) 
            return TE_EASNWRONGLABEL;
        
        if (index < 0)
            index += container->len;
        if ((index < 0 ) || (index >= (int)container->len))
            return TE_EASNINCOMPLVAL;

        *found_val = container->data.array[index];
        return 0;
    } 
    return TE_EASNGENERAL;
}

const char *
asn_get_choice_ptr(const asn_value *container)
{ 
    const asn_value *sval;

    if ((container->data.array == NULL) || 
        ((sval = container->data.array[0]) == NULL))
        return NULL; 


    return sval->name;

}


/**
 * Get choice in subvalue of root ASN value container. 
 *
 * @param container     Root of ASN value tree which subvalue is interested.
 * @param subval_labels string with dot-separated sequence of textual field
 *                      labels, specifying interested subvalue in ASN value 
 *                      tree with 'container' as a root. 
 *                      Subvalue should have ASN syntax CHOICE.
 * @param choice_label  string with label of choice in ASN value (OUT).
 * @param ch_lb_len     length of available buffer in choice_label.
 *
 * @return zero or error code.
 */ 
te_errno 
asn_get_choice(const asn_value *container, const char *subval_labels,
               char *choice_label, size_t ch_lb_len)
{
    const asn_value *val;
    const asn_value *sval;
    int rc;
    int len = 0;
    static char suffix [] = {'.', '#', '\1', '\0'};

    if (subval_labels != NULL)
        len = strlen (subval_labels);

    if (len)
    { 
        char *corrected_labels = malloc(len + sizeof(suffix));
        memcpy(corrected_labels, subval_labels, len); 
        memcpy(corrected_labels + len, suffix, sizeof(suffix)); 

        rc = asn_impl_fall_down_to_tree_nc(container, corrected_labels, &val); 
        free(corrected_labels);

        if (rc != 0 && rc != ASN_STOP_AT_CHOICE)
            return rc; 
    }
    else 
        val = container;

    if (val->syntax != CHOICE)
    {
        return TE_EASNWRONGTYPE;
    }

    if (val->data.array == NULL)
    {
        return TE_EASNGENERAL; 
    }

    if ((sval = val->data.array[0]) == NULL) 
        return TE_EASNINCOMPLVAL; 

    if (strlen(sval->name) >= ch_lb_len)
        return TE_ESMALLBUF;

    strncpy (choice_label, sval->name, ch_lb_len);

    return 0;
}


/**
 * Get tag of value;
 *
 * @param container     value which tag is intereseted
 *
 * @return tag value or -1 on error.
 */ 
asn_tag_value
asn_get_tag(const asn_value *container)
{
    if (container)
        return container->tag.val;
    return (asn_tag_value)-1;
}

/**
 * Obtain ASN syntax type;
 *
 * @param type          ASN value which leaf syntax is interested. 
 *
 * @return syntax of specified leaf in value.
 */
asn_syntax   
asn_get_syntax_of_type(const asn_type *type)
{
    if (type == NULL)
        return SYNTAX_UNDEFINED;

    return type->syntax;
}

/**
 * Obtain ASN syntax of specified field in value. 
 *
 * @param value          ASN value which leaf syntax is interested. 
 * @param subval_labels string with dot-separated sequence of textual field
 *                      labels, specifying interested subvalue in ASN value 
 *                      tree with 'value' as a root. 
 *
 * @return syntax of specified leaf in value.
 */
asn_syntax   
asn_get_syntax(const asn_value *value, const char *subval_labels)
{
    const asn_value *val;
    int rc;
    int len;
    static char suffix [] = {'.', '#', '\1', '\0'};

    if (value == NULL)
        return SYNTAX_UNDEFINED;

    if (subval_labels == NULL || *subval_labels == '\0')
        return value->syntax;

    len = strlen (subval_labels);
    if (len)
    { 
        char *corrected_labels = malloc (len + sizeof(suffix));

        memcpy (corrected_labels, subval_labels, len); 
        memcpy (corrected_labels + len, suffix, sizeof(suffix)); 

        rc = asn_impl_fall_down_to_tree_nc(value, corrected_labels, &val); 
        free (corrected_labels);

        if (rc && rc != ASN_STOP_AT_CHOICE)
            return SYNTAX_UNDEFINED; 
    }
    else
        val = value;

    return val->syntax;
}



/**
 * Get constant pointer to subtype of some ASN type.
 * 
 * @param container     ASN type. 
 * @param found_subtype Location for pointer to ASN sub-type (OUT). 
 * @param labels        String with dot-separated sequence of textual field
 *                      labels, specifying interested sub-type.
 *
 * @return zero or error code.
 */ 
te_errno 
asn_get_subtype(const asn_type *container, 
                const asn_type **found_subtype, const char *labels)
{
    const asn_type *type = container;
    const char *rest_labels = labels;
    int rc = 0; 

    type = container;

    while (rest_labels && (*rest_labels))
    { 
        int index;

        rc = asn_child_named_index(type, rest_labels, &index, &rest_labels);
        if (rc != 0)
            return rc;

        switch (type->syntax)
        {
            case SEQUENCE:
            case SET:
            case CHOICE:
                type = type->sp.named_entries[index].type;
                break;

            case SEQUENCE_OF:
            case SET_OF:
            case TAGGED:
                type = type->sp.subtype;
                break;

            default:
                return TE_EASNWRONGLABEL;
        }
    }

    *found_subtype = type;

    return 0;
}

/* See description in asn_usr.h */
te_errno
asn_label_to_tag(const asn_type *type, const char *label, asn_tag_t *tag)
{
    int rc = 0;
    int index;

    const char *rest_labels = NULL;

    if (tag == NULL)
        return TE_EWRONGPTR;

    rc = asn_child_named_index(type, label, &index, &rest_labels);
    if (rc != 0)
        return rc; 

    if (rest_labels != NULL)
        return TE_EASNWRONGLABEL;

    *tag = type->sp.named_entries[index].tag;

    return 0;
}


/* See description in asn_usr.h */
te_errno
asn_put_mark(asn_value *value, int mark)
{
    if (value == NULL)
        return TE_EINVAL;
    value->mark = mark;

    return 0;
}

/* See description in asn_usr.h */
te_errno
asn_get_mark(const asn_value *value, int *mark)
{
    if (value == NULL || mark == NULL )
        return TE_EINVAL;

    *mark = value->mark; 

    return 0;
}

/**
 * Get ASN path of value relative to container. This function must be used
 * only from inside callbacks of asn_walk_depth function.
 *
 * @param   value   ASN value
 *
 * @return  ASN path
 */
static const char*
asn_get_value_path(asn_value *value)
{
    if (value == NULL || value->path == NULL)
        return "";
    return value->path;
}

/**
 * Get label of a field by its index
 *
 * @param value      Root ASN value
 * @param index      Index of field
 * @param label      Buffer for name (OUT)
 * @param label_len  Length of buffer
 *
 * @return  0 - success
 *         -1 - error
 */
static int
asn_impl_get_label_by_index(asn_value *value, unsigned int index,
                            char *label, unsigned int label_len)
{
    const asn_type  *type = NULL;
    char            *name = NULL;
    asn_value       *child_value = NULL;
#define BUF_SIZE 20
    
    if (value == NULL || label == NULL || label_len == 0)
        return -1;

    type = asn_get_type(value);
    
    if (type->syntax & ASN_SYN_NAMED)
    {
        name = type->sp.named_entries[index].name;
        if (type->syntax == CHOICE)
        {
            if (0 != asn_get_child_by_index(value, &child_value, index))
            {
                return -1;
            }
            snprintf(label, label_len - 1, "#%s", child_value->name);
        }
        else
        {
            if (strlen(name) >= label_len)
            {
                return -1;
            }
            strncpy(label, name, label_len - 1);
        }
        label[label_len - 1] = '\0';
    }
    else /* Indexed entries */
    {
        name = malloc(BUF_SIZE * sizeof(char));
        if (name == NULL)
        {
            return -1;
        }
        snprintf(name, BUF_SIZE - 1, "%d", index);
        if (strlen(name) >= label_len)
        {
            return -1;
        }
        strncpy(label, name, label_len - 1);
        label[label_len - 1] = '\0';
    }

#undef BUF_SIZE
    return 0;
}

/**
 * Walks through ASN tree and calls func on each item
 *
 * See params description in asn_walk_depth function. Additional parameter
 * (path) contains ASN path to current value from the root node (container)
 * and could be obtained in the 'func' using asn_get_value_path function.
 */
static te_errno
asn_impl_walk_depth(asn_value *container, te_bool only_leafs, char *path,
                    te_errno *status, walk_method func, void *user_ptr)
{
    te_errno     rc = 0;
    unsigned int i;
    asn_value   *sv;
    char        *subpath = NULL;
    char        *valuename = NULL;
#define PATH_SIZE 512

    if (container == NULL || status == NULL || func == NULL)
        return TE_EINVAL;

    if (container->syntax & COMPOUND)
    {
        subpath = malloc(PATH_SIZE * sizeof(char));
        valuename = malloc(PATH_SIZE * sizeof(char));
        if (subpath == NULL || valuename == NULL)
        {
            return TE_ENOMEM;
        }
        for (i = 0; i < container->len; i++)
        {
            if ((sv = container->data.array[i]) != NULL)
            {
                valuename[0] = '\0';
                asn_impl_get_label_by_index(container->data.array[i], i,
                                            valuename, PATH_SIZE);
                snprintf(subpath, PATH_SIZE - 1, "%s.%s", path, valuename);
                rc = asn_impl_walk_depth(sv, only_leafs, subpath,
                                         status, func, user_ptr);
            }
            if (rc != 0 || (*status) != 0)
            {
                free(subpath);
                free(valuename);
                return rc;
            }
        }
        free(subpath);
        free(valuename);

        if (only_leafs && container->len > 0)
            return 0; /* nothing more to do with this node */
    }

    container->path = path;
    *status = func(container, user_ptr);
    container->path = NULL;

    return 0;
}

/* See description in asn_usr.h */
te_errno
asn_walk_depth(asn_value *container, te_bool only_leafs,
               te_errno *status, walk_method func, void *user_ptr)
{
    char    *path = NULL;
    te_errno rc;

    path = strdup("");
    rc = asn_impl_walk_depth(container, only_leafs, path, status,
                             func, user_ptr);
    free(path);

    return rc;
}

/* See description in asn_usr.h */
te_errno
asn_walk_breadth(asn_value *container, te_bool only_leafs,
                 te_errno *status, walk_method func, void *user_ptr)
{
    UNUSED(container);
    UNUSED(only_leafs);
    UNUSED(status);
    UNUSED(func);
    UNUSED(user_ptr);

    return TE_EOPNOTSUPP;
}

/* See description in asn_usr.h */
te_errno
asn_path_from_extended(const asn_value *node, const char *ext_path,
                       char *asn_path, unsigned int asn_path_len)
{
    char *search_start = NULL, *search_end = NULL;
    char *path = NULL;
    int prefix_len = 0;
    asn_value *container = NULL, *new_value = NULL;
    te_errno rc;
    const asn_type *value_type = NULL;
    const asn_type *subtype = NULL;
    const asn_type *search_type = NULL;
    asn_syntax value_syntax;
    char *search_name = NULL;
    char *search_value = NULL;
    int search_len = 0;
    char *p = NULL;
    int i, parsed_syms = 0, len;

    /* Check validity of arguments */
    if ((node == NULL) || (ext_path == NULL) ||
        (asn_path == NULL))
        return TE_EINVAL;

    /* Check if there are any searches */
    if ((search_start = strstr(ext_path, ".[")) == NULL)
    {
        /* Nothing to replace - return the path as is */
        if (asn_path_len < strlen(ext_path) + 1)
        {
            return TE_ENOBUFS;
        }
        memset(asn_path, '\0', asn_path_len);
        strncpy(asn_path, ext_path, asn_path_len);

        return 0;
    }

    /* Get ASN node which contents are to be searched */
    prefix_len = search_start - ext_path;
    path = malloc((prefix_len + 1) * sizeof(char));
    strncpy(path, ext_path, prefix_len);
    path[prefix_len] = '\0';
    rc = asn_get_descendent(node, &container, path);
    free(path);
    if (rc != 0)
    {
        RING("ext: asn_get_descendent failed");
        return rc;
    }

    /* Check type of container and type of members of container */
    value_type = asn_get_type(container);
    value_syntax = asn_get_syntax_of_type(value_type);
    if ((value_syntax != SET_OF) &&
        (value_syntax != SEQUENCE_OF))
    {
        RING("ext: wrong type of container");
        return TE_EASNWRONGTYPE;
    }
    rc = asn_get_subtype(value_type, &subtype, "0");
    if (rc != 0)
    {
        RING("ext: get_subtype failed");
        return rc;
    }
    value_syntax = asn_get_syntax_of_type(subtype);
    if ((value_syntax != SET) &&
        (value_syntax != SEQUENCE))
    {
        RING("ext: wrong subtype: %d", value_syntax);
        return TE_EASNWRONGTYPE;
    }

    /* Parse search expression - extract label and value */
    search_end = strstr(search_start, "].");
    if (search_end == NULL)
    {
        RING("ext: failed to find end of search");
        return TE_EINVAL;
    }

    search_start += 2;
    search_len = search_end - search_start;
    search_name = malloc((search_len + 1) * sizeof(char));
    search_value = malloc((search_len + 1) * sizeof(char));
    memset(search_name, '\0', search_len + 1);
    memset(search_value, '\0', search_len + 1);

    p = search_name;
    while (*search_start != ':')
    {
        if (isspace(*search_start))
            continue;
        *p++ = *search_start++;
    }

    search_start++;
    *search_value = '\"';
    p = search_value + 1;
    while (search_start != search_end)
    {
        if (isspace(*search_start))
            continue;
        *p++ = *search_start++;
    }
    *p = '\"';
    
    /* Check that type of member has corresponding label */
    for (i = 0; i < subtype->len; i++)
    {
        if (strcmp(subtype->sp.named_entries[i].name, search_name) == 0)
        {
            break;
        }
    }
    if (i == subtype->len && subtype->len != 0)
    {
        RING("ext: couldn't find label");
        rc = TE_EASNWRONGLABEL;
        goto cleanup;
    }
    search_type = subtype->sp.named_entries[i].type;

    /* Check that search type is CHAR_STRING */
    if (search_type->syntax != CHAR_STRING)
    {
        RING("ext: wrong syntax of search value = %d", search_type->syntax);
        rc = TE_EASNWRONGTYPE;
        goto cleanup;
    }

    /* Create asn_value from search value based on labeled member type */
    rc = asn_parse_value_text(search_value, search_type, &new_value, &parsed_syms);
    if (rc != 0)
    {
        RING("ext: failed to parse search value '%s'", search_value);
        goto cleanup;
    }

    /* Iterate through members of container comparing corrensponding field
     *   When found - remember index and stop iteration
     */
    for (i = 0; i < container->len; i++)
    {
        asn_value *subvalue, *value_to_compare;

        rc = asn_get_child_by_index(container, &subvalue, i);
        if (rc != 0)
        {
            RING("ext: Failed to get child by index");
            goto cleanup;
        }
        rc = asn_get_descendent(subvalue, &value_to_compare, search_name);
        if (rc != 0)
        {
            RING("ext: asn_get_descendent for value_to_compare failed");
            goto cleanup;
        }

        /* Compare value_to_compare with search_value - everything that is
         * mentioned in the new_value must present in value_to_compare
         */
        rc = asn_check_value_contains(value_to_compare, new_value);
        if (rc == 0)
            break;
    }
    if (i == container->len) /* Nothing was found */
    {
        RING("ext: nothing was found to match search");
        rc = TE_EASNDIFF;
        goto cleanup;
    }

    /*
     * Calculate necessary buffer length for ASN path and check
     * asn_path_len
     */
    path = malloc(20 * sizeof(char));
    len = snprintf(path, 20, ".%d.", i);
    if (prefix_len + len + strlen(search_end + 2) >= asn_path_len)
    {
        free(path);
        rc = TE_ENOBUFS;
        goto cleanup;
    }

    /* Create final asn_path */
    strncpy(asn_path, ext_path, prefix_len);
    asn_path[prefix_len] = '\0';
    strncat(asn_path, path, asn_path_len);
    strncat(asn_path, search_end + 2, asn_path_len);
    free(path);

    rc = 0;

cleanup:
    free(search_name);
    free(search_value);

    return rc;
}

/* See description in asn_usr.h */
int
asn_insert_value_extended_path(const asn_value *root_node,
                               const char *ext_path,
                               asn_value *value,
                               int *index)
{
    char *search_start = NULL, *search_end = NULL;
    char *prefix_path = NULL;
    int prefix_len = 0;
    asn_value *new_value = NULL;
    te_errno rc;
    const asn_type *value_type = NULL;
    const asn_type *subtype = NULL;
    const asn_type *search_type = NULL;
    asn_syntax value_syntax;
    char *search_name = NULL;
    char *search_value = NULL;
    int search_len = 0;
    char *p = NULL;
    int i, parsed_syms = 0;

    /* Check validity of arguments */
    if ((root_node == NULL) || (ext_path == NULL) ||
        (value == NULL))
        return TE_EINVAL;

    /* Check if there are any searches */
    if ((search_start = strstr(ext_path, ".[")) == NULL)
    {
        return TE_EOPNOTSUPP;
    }

    /* Get ASN node which contents are to be searched */
    prefix_len = search_start - ext_path;
    prefix_path = malloc((prefix_len + 1) * sizeof(char));
    strncpy(prefix_path, ext_path, prefix_len);
    prefix_path[prefix_len] = '\0';
    search_type = asn_get_type(root_node);
    rc = asn_get_subtype(search_type, &value_type, prefix_path);
    if (rc != 0)
    {
        RING("insert-extended: asn_get_descendent(%s) failed", prefix_path);
        goto cleanup;
    }

    /* Check type of container and type of members of container */
    value_syntax = asn_get_syntax_of_type(value_type);
    if ((value_syntax != SET_OF) &&
        (value_syntax != SEQUENCE_OF))
    {
        RING("insert-extended: incorrect type");
        rc = TE_EASNWRONGTYPE;
        goto cleanup;
    }
    rc = asn_get_subtype(value_type, &subtype, "0");
    if (rc != 0)
    {
        RING("insert-extended: failed to get subtype");
        goto cleanup;
    }
    value_syntax = asn_get_syntax_of_type(subtype);
    if (((value_syntax != SET) &&
         (value_syntax != SEQUENCE)) ||
        (value->syntax != value_syntax))
    {
        RING("insert-extended: wrong syntax of subtype (need: %d or %d, have: %d, value->syntax=%d", SET, SEQUENCE, value_syntax, value->syntax);
        rc = TE_EASNWRONGTYPE;
        goto cleanup;
    }

    /* Parse search expression - extract label and value */
    search_end = strstr(search_start, "]");
    if (search_end == NULL)
    {
        RING("insert-extended: failed to find search end");
        rc = TE_EINVAL;
        goto cleanup;
    }

    search_start += 2;
    search_len = search_end - search_start;
    search_name = malloc((search_len + 1) * sizeof(char));
    search_value = malloc((search_len + 1) * sizeof(char));
    memset(search_name, '\0', search_len + 1);
    memset(search_value, '\0', search_len + 1);
    
    p = search_name;
    while (*search_start != ':')
    {
        if (isspace(*search_start))
            continue;
        *p++ = *search_start++;
    }

    search_start++;
    search_value[0] = '\"';
    p = search_value + 1;
    while (search_start != search_end)
    {
        *p++ = *search_start++;
    }
    *p = '\"';
    
    /* Check that type of member has corresponding label */
    for (i = 0; i < subtype->len; i++)
    {
        if (strcmp(subtype->sp.named_entries[i].name, search_name) == 0)
        {
            break;
        }
    }
    if (i == subtype->len && subtype->len != 0)
    { 
        RING("insert-extended: subtype doesn't have necessary label");
        rc = TE_EASNWRONGLABEL;
        goto cleanup;
    }
    search_type = subtype->sp.named_entries[i].type;

    /* Check that search type is CHAR_STRING */
    if (search_type->syntax != CHAR_STRING)
    {
        rc = TE_EASNWRONGTYPE;
        goto cleanup;
    }

    /* Create asn_value from search value based on labeled member type */
    rc = asn_parse_value_text(search_value, search_type, &new_value, &parsed_syms);
    if (rc != 0)
    {
        RING("insert-extended: Failed to parse search_value");
        goto cleanup;
    }

    /*
     * Insert 'value' into the container and get index
     * For now always insert as index 0
     */
    rc = asn_insert_indexed(root_node, value, 0, prefix_path);
    if (rc != 0)
    {
        RING("insert-extended: Failed to insert value");
        goto cleanup;
    }

    if (index != NULL)
    {
        *index = 0;
    }

    /*
     * Set 'search_name' subvalue of newly inserted value to 'new_value'
     * value
     */
    rc = asn_put_descendent(value, new_value, search_name);

cleanup:
    free(prefix_path);
    free(search_name);
    free(search_value);

    return rc;
}

/*
 * Structure for asn_check_value_contains
 */
struct check_contains_s
{
    asn_value *container;
    asn_value *value;
};

/**
 * Compares two ASN values by comparing their textual representation
 *
 * TODO: Write more accurate comparison (consider order of items in
 * SEQUENCE_OF and don't consider it in SET_OF)
 *
 * @param a  value to compare
 * @param b  value to compare
 *
 * @return  0 - values are identical
 *          Status code - otherwise
 */
static te_errno
asn_impl_compare_values(asn_value *a, asn_value *b)
{
    char *text_a = NULL;
    char *text_b = NULL;
    int len_a = 0, len_b = 0;
    int req_a, req_b;
    te_errno rc = TE_EASNDIFF;

#if 0
    /* 
     * TODO: Understand why UniverstalString leafs has COMPOUND syntax, but
     * not CHAR_STRING. After that uncomment this type check.
     */
    if ((a->syntax != b->syntax) ||
        (a->syntax | COMPOUND) ||
        (b->syntax | COMPOUND))
    {
        RING("ASNCOMPARE: types are different: a(typename=%s)=%d, b(typename=%s)=%d", a->asn_type->name, a->asn_type->syntax, b->asn_type->name, b->asn_type->syntax);
        return TE_EASNWRONGTYPE;
    }
#endif

    len_a = len_b = 100;
    text_a = malloc(len_a * sizeof(char));
    text_b = malloc(len_b * sizeof(char));
    if (text_a == NULL || text_b == NULL)
    {
        RING("ASNCOMPARE: Failed to allocate %d and %d bytes of memory", len_a, len_b);
        return TE_ENOMEM;
    }

    req_a = asn_sprint_value(a, text_a, len_a, 0);
    req_b = asn_sprint_value(b, text_b, len_b, 0);

    if (req_a != len_a)
    {
        text_a = realloc(text_a, req_a + 1);
        len_a = req_a + 1;
    }
    if (req_b != len_b)
    {
        text_b = realloc(text_b, req_b + 1);
        len_b = req_b + 1;
    }
    req_a = asn_sprint_value(a, text_a, len_a, 0);
    req_b = asn_sprint_value(b, text_b, len_b, 0);

#if 0
    RING("ASNCOMPARE: req_a=%d, req_b=%d, text_a='%s', text_b='%s'", req_a, req_b, text_a, text_b);
#endif
    rc = TE_EASNDIFF;
    if (req_a == req_b)
    {
        if (0 == strncmp(text_a, text_b, req_a))
        {
            rc = 0;
        }
    }

    free(text_a);
    free(text_b);

    return rc;
}

/**
 * Callback for use with asn_walk_depth in asn_check_value_contains
 *
 * @param leaf      ASN value for leaf currently processed
 * @param user_ptr  Pointer to user-defined data
 *
 * @return  Status code
 */
static te_errno
asn_impl_check_contains_callback(asn_value *leaf, void *user_ptr)
{
    struct check_contains_s *cc = (struct check_contains_s *)user_ptr;
    const char *path;
    asn_value *cont_value;
    te_errno rc;

    path = asn_get_value_path(leaf);
#if 0
    RING("CHKCONTCALLBACK: got path='%s'", path);
#endif
    rc = asn_get_descendent(cc->container, &cont_value, path);
    if (rc != 0)
    {
        return rc;
    }
#if 0
    RING("CHKCONTCALLBACK: checking leaf(type=%s,%d) and cont_value(type=%s,%d)",
         leaf->asn_type->name, leaf->syntax,
         cont_value->asn_type->name, cont_value->syntax);
#endif

    return asn_impl_compare_values(leaf, cont_value);
}

/* See description in asn_usr.h */
te_errno
asn_check_value_contains(asn_value *container, asn_value *value)
{
    struct check_contains_s cc;
    te_errno status = 0, rc;

    cc.container = container;
    cc.value = value;
#if 0
    RING("CHKCONT: checking container(type=%s,%d) and value(type=%s,%d)",
         container->asn_type->name, container->syntax,
         value->asn_type->name, value->syntax);
#endif
    rc = asn_walk_depth(value, TRUE, &status,
                        asn_impl_check_contains_callback, (void *)&cc);

    if (rc == 0)
    {
        rc = status;
    }

    return rc;
}

/**
 * Definitions of ASN.1 base types.
 */ 

const asn_type asn_base_boolean_s = 
{ "BOOLEAN",           {UNIVERSAL, 1}, BOOL,        0, {NULL} };
const asn_type asn_base_integer_s = 
{ "INTEGER",           {UNIVERSAL, 2}, INTEGER,     0, {NULL} }; 
const asn_type asn_base_bitstring_s = 
{ "BIT STRING",        {UNIVERSAL, 3}, BIT_STRING,  0, {NULL} };
const asn_type asn_base_octstring_s = 
{ "OCTET STRING",      {UNIVERSAL, 4}, OCT_STRING,  0, {NULL} };
const asn_type asn_base_null_s = 
{ "NULL",              {UNIVERSAL, 5}, PR_ASN_NULL, 0, {NULL} };
const asn_type asn_base_objid_s = 
{ "OBJECT IDENTIFIER", {UNIVERSAL, 6}, OID,         0, {NULL} };
const asn_type asn_base_real_s = 
{ "REAL",              {UNIVERSAL, 9}, REAL,        0, {NULL} };
const asn_type asn_base_enum_s = 
{ "ENUMERATED",        {UNIVERSAL,10}, ENUMERATED,  0, {NULL} };
const asn_type asn_base_charstring_s = 
{ "UniversalString",   {UNIVERSAL,28}, CHAR_STRING, 0, {NULL} }; 


const asn_type  asn_base_int1_s = 
{ "INTEGER (0..1)",   {UNIVERSAL, 2}, INTEGER, 1, {0}};
const asn_type  asn_base_int2_s = 
{ "INTEGER (0..3)",   {UNIVERSAL, 2}, INTEGER, 2, {0}};
const asn_type  asn_base_int3_s = 
{ "INTEGER (0..7)",   {UNIVERSAL, 2}, INTEGER, 3, {0}};
const asn_type  asn_base_int4_s = 
{ "INTEGER (0..15)",   {UNIVERSAL, 2}, INTEGER, 4, {0}};
const asn_type  asn_base_int5_s = 
{ "INTEGER (0..31)",   {UNIVERSAL, 2}, INTEGER, 5, {0}};
const asn_type  asn_base_int6_s = 
{ "INTEGER (0..63)",   {UNIVERSAL, 2}, INTEGER, 6, {0}};
const asn_type  asn_base_int7_s = 
{ "INTEGER (0..127)",   {UNIVERSAL, 2}, INTEGER, 7, {0}};
const asn_type  asn_base_int8_s = 
{ "INTEGER (0..255)",   {UNIVERSAL, 2}, INTEGER, 8, {0}};
const asn_type  asn_base_int9_s = 
{ "INTEGER (0..511)",   {UNIVERSAL, 2}, INTEGER, 9, {0}};
const asn_type  asn_base_int12_s = 
{ "INTEGER (0..4095)",   {UNIVERSAL, 2}, INTEGER, 12, {0}};
const asn_type  asn_base_int16_s = 
{ "INTEGER (0..65535)",   {UNIVERSAL, 2}, INTEGER, 16, {0}};
const asn_type  asn_base_int24_s = 
{ "INTEGER (0..16777215)",   {UNIVERSAL, 2}, INTEGER, 24, {0}};
const asn_type  asn_base_int32_s = 
{ "INTEGER (0..4294967295)",   {UNIVERSAL, 2}, INTEGER, 32, {0}};


const asn_type * const asn_base_boolean     = &asn_base_boolean_s;
const asn_type * const asn_base_integer     = &asn_base_integer_s;
const asn_type * const asn_base_int4        = &asn_base_int4_s;
const asn_type * const asn_base_int8        = &asn_base_int8_s;
const asn_type * const asn_base_int16       = &asn_base_int16_s;
const asn_type * const asn_base_bitstring   = &asn_base_bitstring_s;
const asn_type * const asn_base_octstring   = &asn_base_octstring_s;
const asn_type * const asn_base_null        = &asn_base_null_s;
const asn_type * const asn_base_objid       = &asn_base_objid_s;
const asn_type * const asn_base_real        = &asn_base_real_s;
const asn_type * const asn_base_enum        = &asn_base_enum_s;
const asn_type * const asn_base_charstring  = &asn_base_charstring_s;

