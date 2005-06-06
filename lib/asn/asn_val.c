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

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 

#include "asn_impl.h" 
#include "te_errno.h"
#include "te_defs.h"


#define ASN_STOP_AT_CHOICE 1

/* this function defined in asn_text.c */
extern int number_of_digits(int value);

/*
 * Declarations of local methods, for descriptions see their definition.
 */ 

int asn_impl_named_subvalue_index(const asn_type * type, const char *label,
                                  int *index);

int asn_child_tag_index(const asn_type *type, asn_tag_class tag_class,
                        uint16_t tag_val, int *index);

int asn_impl_fall_down_to_tree_nc(const asn_value *, char *,
                                  asn_value const **);

int asn_impl_write_value_field(asn_value_p , const void *, size_t , char *);

int asn_impl_read_value_field(const asn_value *, void *, unsigned int *, char *);

int asn_impl_write_component_value(asn_value_p , const asn_value *,  char *);

int asn_impl_insert_subvalue(asn_value_p container, const char *label, 
                             asn_value_p new_value);

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
static inline int
asn_impl_find_subvalue_writable(asn_value *container, const char *label, 
                                asn_value **found_val)
{
    const asn_value *f_val;
    int rc = asn_impl_find_subvalue(container, label, &f_val);

    *found_val = (asn_value *)f_val;
    return rc;
}


/**
 * Wrapper over asn_impl_fall_down_to_tree_nc, for find in writable container
 * and get writable subvalue. All parameters are same. 
 */ 
static inline int 
asn_impl_fall_down_to_tree_writable (asn_value *container, 
                    const char *field_labels, asn_value **found_value)
{
    int rc;
    char *rest_labels = asn_strdup(field_labels); 
    const asn_value *f_val;

    rc = asn_impl_fall_down_to_tree_nc (container, rest_labels, &f_val); 
    free (rest_labels);

    /* Initial container was*/
    *found_value = (asn_value *)f_val;

    return rc;
}

#if 0
extern char *strsep (char **, char *);
#endif

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
asn_value_p 
asn_init_value(const asn_type * type)
{
    asn_value_p new_value;
    int arr_len;

    if (type == NULL) return NULL;

    new_value = malloc (sizeof (asn_value));
    if (!new_value) { return NULL; } 
    memset (new_value, 0, sizeof(asn_value));

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
    case SEQUENCE:
    case SET:
        {
            int sz    = arr_len * sizeof(asn_value_p);
            void *ptr = malloc(sz);

            new_value->len = arr_len;
            new_value->data.array = ptr; 

            memset(ptr, 0, sz);
        }
        break;

    case INTEGER:
        new_value->txt_len = 1;
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
asn_value_p 
asn_init_value_tagged(const asn_type * type, asn_tag_class tc, uint16_t tag)
{
    asn_value_p new_value = asn_init_value(type);

    if (new_value == NULL)
        return NULL;

    new_value->tag.cl  = tc;
    new_value->tag.val = tag;

    return new_value;
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
    asn_value_p new_value;
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

    if (value->syntax & CONSTRAINT)
    {
        int i;
        asn_value_p src_elem;
        asn_value_p *arr = new_value->data.array = 
            malloc(len * sizeof(asn_value_p));

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
}

/**
 * Free memory allocalted by ASN value instance.
 *
 * @param value       ASN value to be destroyed. 
 *
 * @return nothing
 */
void 
asn_free_value(asn_value_p value)
{
    if (!value) return;
    
    if (value->syntax & CONSTRAINT) 
    {
        unsigned int i;
        asn_value_p *arr = value->data.array; 
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



/**
 * Free subvalue of constraint ASN value instance.
 *
 * @param container   ASN value which subvalue should be destroyed.
 * @param labels      string with dot-separated sequence of textual field
 *                    labels, specifying subvalue in ASN value tree with 
 *                    'container' as a root. Label for 'SEQUENCE OF' and 
 *                    'SET OF' subvalues is decimal notation of its integer 
 *                    index in array.
 *
 * @return zero on success, otherwise error code.
 */
int 
asn_free_subvalue(asn_value_p value, const char* labels)
{
    int   len = strlen (labels);
    char *low_label;
    char *up_labels = asn_strdup(labels);
    int   rc = 0;

    asn_value_p subvalue;

    if (!value || !labels)
    {
        free(up_labels);
        return ETEWRONGPTR; 
    }

    low_label = up_labels + len - 1;
    while ((low_label> up_labels) && (*low_label != '.')) 
        low_label --;

    rc = 0;

    if (*low_label == '.') 
    {
        *low_label = '\0';
        low_label ++;
        rc = asn_impl_fall_down_to_tree_writable(value, up_labels, &subvalue);

        value = subvalue; 
    } 

    if (rc == 0)
        rc = asn_impl_find_subvalue_writable(value, low_label, &subvalue);

    if (rc == 0)
    {
        asn_free_value(subvalue);
        asn_impl_insert_subvalue(value, low_label, NULL);

        value->txt_len = -1;
    }
    free(up_labels);

    return rc;
}

int
asn_free_child_value(asn_value_p value, 
                     asn_tag_class tag_class, uint16_t tag_val)
{
    int rc;
    int index;

    if (value == NULL)
        return ETEWRONGPTR;

    rc = asn_child_tag_index(value->asn_type, tag_class, tag_val,
                             &index);
    if (rc != 0)
        return rc;

    value->txt_len = -1;
    asn_free_value(value->data.array[index]);
    value->data.array[index] = NULL;

    return 0;
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

/**
 * Determine numeric index of field in structure presenting ASN.1 type
 * by tag of subvalue.
 * This method is applicable only to values with CONSTRAINT syntax with
 * named components: 'SEQUENCE', 'SET' and 'CHOICE'. 
 *
 * @param type       ASN type which subvalue is interested. 
 * @param tag_class  class of ASN tag
 * @param tag_val    value of ASN tag
 * @param index      found index, unchanged if error occurred (OUT).
 *
 * @return zero on success, otherwise error code. 
 */
int 
asn_child_tag_index(const asn_type *type, asn_tag_class tag_class,
                    uint16_t tag_val, int *index)
{
    const asn_named_entry_t *n_en;
    unsigned i; 

    if(!type || !index)
        return ETEWRONGPTR; 

    if( !(type->syntax & CONSTRAINT) || (type->syntax & 1))
        return EASNGENERAL; 

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

    return EASNWRONGLABEL;
}

/**
 * Determine numeric index of field label in structure presenting ASN.1 type.
 * This method is applicable only to values with CONSTRAINT syntax with named
 * components: 'SEQUENCE', 'SET' and 'CHOICE'. 
 *
 * @param type       ASN type which subvalue is interested. 
 * @param label      textual field label, specifying subvalue in type. 
 * @param index      found index, unchanged if error occurred (OUT).
 *
 * @return zero on success, otherwise error code. 
 */
int 
asn_impl_named_subvalue_index(const asn_type *type, const char *label,
                              int *index)
{
    const asn_named_entry_t *n_en;
    unsigned i; 

    if(!type || !label || !index)
        return ETEWRONGPTR; 

    if( !(type->syntax & CONSTRAINT) || (type->syntax & 1))
        return EASNGENERAL; 

    n_en = type->sp.named_entries; 
        
    for (i = 0; i < type->len; i++, n_en++)
        if(strcmp(label, n_en->name) == 0)
        {            
            *index = i;
            return 0;
        }
    return EASNWRONGLABEL;
}

/**
 * Find one-depth sub-type for passed ASN type tree by its label.
 * This function is applicable only for ASN types with CONSTRAINT syntax.
 *
 * @param type       pointer to ASN value which leaf field is interested;
 * @param label      textual field label, specifying subvalue of 'type',
 *                   for syntaxes "*_OF" and "TAGGED" this parameter is ignored. 
 * @param found_type pointer to found ASN type (OUT).
 *
 * @return zero on success, otherwise error code. 
 */ 
int
asn_impl_find_subtype(const asn_type * type, const char *label, 
                      const asn_type ** found_type)
{ 

    if(!label || !type || !found_type)
        return ETEWRONGPTR; 

    if( !(type->syntax & CONSTRAINT) )
        return EASNGENERAL;

    if (!(type->syntax & 1))
    {
        int rc;
        int index; 

        rc = asn_impl_named_subvalue_index(type, label, &index);
        if (rc) return rc;
        
        *found_type =type->sp.named_entries[index].type; 
        return 0;
    }

    *found_type = type->sp.subtype; 
    return 0; 
}





/* see description in asn_usr.h */
int
asn_put_child_value(asn_value *container, asn_value *new_value, 
                    asn_tag_class tag_class, uint16_t tag_val)
{
    int rc;
    int index = 0, leaf_type_index;

    if (container == NULL) 
        return ETEWRONGPTR; 

    container->txt_len = -1;

    rc = asn_child_tag_index(container->asn_type, tag_class, tag_val,
                             &leaf_type_index);
    if (rc != 0)
        return rc; 

    switch (container->syntax)
    {
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
            return EASNGENERAL; 
    }

    /* now set name of new subvalue, if it is. */
    if (new_value)
    {
        if (new_value->syntax & CONSTRAINT)
            new_value->txt_len = -1;

        if (new_value->name) 
            free(new_value->name); 

        new_value->name = asn_strdup(container->asn_type->
                                sp.named_entries[leaf_type_index].name);
    }

    return 0;
}


/**
 * Insert one-depth subvalue into ASN value tree by its label (if applicable).
 * This method is applicable only to values with CONSTRAINT syntax with named
 * components: 'SEQUENCE', 'SET',  'CHOICE' and 'TAGGED'. 
 * Passed value is not copied, but inserted into ASN tree of 'container' as is.
 * Passed value may be NULL, this means that specified subvalue is allready
 * destroyed and pointer to it should be removed from 'container'. 
 *
 * @param container  pointer to ASN value which leaf field is interested;
 * @param label      textual field label, specifying subvalue of 'container'. 
 * @param new_value  value to be inserted. 
 *
 * @return zero on success, otherwise error code. 
 */ 
int
asn_impl_insert_subvalue(asn_value_p container, const char *label, 
                         asn_value_p new_value)
{ 
    if(!label || !container)
        return ETEWRONGPTR; 

    container->txt_len = -1;

    switch (container->syntax)
    {
        case SEQUENCE:
        case SET:
            {
                int index;
                int rc;

                rc = asn_impl_named_subvalue_index(container->asn_type, 
                                                   label, &index); 
                if (rc) 
                    return rc;

                container->data.array[index] = new_value; 
            }
            break;

        case CHOICE:
            if (container->data.array[0] && new_value)
                return EASNOTHERCHOICE;
            /* pass through ... */
        case TAGGED:
            container->data.array[0] = new_value;
            break;

        default:
            return EASNGENERAL; 
    }

    /* now set name of new subvalue, if it is. */
    if (new_value)
    {
        if (new_value->syntax & CONSTRAINT)
            new_value->txt_len = -1;

        if (new_value->name) 
            free(new_value->name); 

        if (label && *label)  
            new_value->name = asn_strdup(label);
        else                  
            new_value->name = NULL;
    }

    return 0;
}



/**
 * Write data into primitive syntax leaf in specified ASN value, wrapper over
 * internal function "asn_impl_write_value_field".
 *
 * @param container     pointer to ASN value which leaf field is interested;
 * @param data          data to be written, should be in nature C format for
 *                      data type respective to leaf syntax;
 * @param d_len         length of data; 
 * @param field_labels  string with dot-separated sequence of textual field
 *                      labels, specifying primitive-syntax leaf in ASN value 
 *                      tree with 'container' as a root. Label for 
 *                      'SEQUENCE OF' and 'SET OF' subvalues is decimal
 *                      notation of its integer index in array.
 *
 * @return zero on success, otherwise error code.
 */ 
int
asn_write_value_field(asn_value_p container, const void *data, int d_len, 
                      const char *field_labels)
{
    char *field_labels_int_copy = asn_strdup(field_labels); 
    int   rc = asn_impl_write_value_field(container, data, d_len, 
                                          field_labels_int_copy);
    free(field_labels_int_copy);

    return rc;
}


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
int
asn_impl_write_value_field(asn_value_p container, 
                           const void *data, size_t d_len, 
                           char *field_labels)
{
    unsigned int m_len = d_len; /* length of memory used */

    if (!container || 
        (! (container->syntax & CONSTRAINT) &&
        container->syntax != PR_ASN_NULL && !data && d_len))
        return ETEWRONGPTR; 

    container->txt_len = -1;

    switch(container->syntax)
    {
    case BOOL:
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
                case sizeof(char) : val = *((const unsigned char *) data); break;
                case sizeof(short): val = *((const short *)data); break;
                case sizeof(long) : val = *((const long *) data); break;
                default: val = *((int *) data);
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
        {
            void * val = malloc(m_len);

            if (container->asn_type->len && (container->asn_type->len != d_len))
                return EASNGENERAL;

            if (container->data.other)
                free (container->data.other);
            container->data.other = val;
            memcpy(val,  data, m_len);
            container->len = d_len; 
        }
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
            asn_value_p  subvalue;
            int          rc;


            switch (container->syntax)
            {
                case TAGGED: break;
                case CHOICE: 
                    if (rest_field_labels && (*rest_field_labels == '#'))
                        rest_field_labels ++;
                    else 
                        break;
                default:
                    cur_label = strsep (&rest_field_labels, ".");
            }
            rc = asn_impl_find_subvalue_writable(container, cur_label, 
                                                 &subvalue);

            switch (rc)
            {
                case 0:
                    rc = asn_impl_write_value_field(subvalue, data, d_len,
                                                    rest_field_labels);
                break;

                case EASNINCOMPLVAL:
                { 
                    const asn_type * subtype;

                    rc = asn_impl_find_subtype(container->asn_type, 
                                                     cur_label, &subtype);
                    if (rc) break;

                    subvalue = asn_init_value(subtype);

                    if (container->syntax != TAGGED)
                        subvalue->name = asn_strdup(cur_label); 

                    rc = asn_impl_write_value_field(subvalue, data, d_len,
                                                    rest_field_labels);
                    if (rc) break; 

                    rc = asn_impl_insert_subvalue(container, cur_label, 
                                                  subvalue);
                } 
            } 

            return rc;
        }
    default: 
        return 0; /* nothing to do. */
    }

    return 0;
}


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
int
asn_read_value_field(const asn_value *container, void *data, int *d_len,
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
int
asn_impl_read_value_field(const asn_value *container,  void *data, 
                          unsigned int *d_len, char *field_labels)
{
    const asn_value *value;
    int m_len;
    int rc;

    if (!container) return ETEWRONGPTR; 

    rc = asn_get_subvalue(container, &value, field_labels); 
    if (rc) return rc;

    if (value->syntax != PR_ASN_NULL && (!data || !d_len))
        return ETEWRONGPTR; 

    if (d_len && *d_len < value->len)
        return ETESMALLBUF;

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
            return ETESMALLBUF;
        break;
#endif
    case INTEGER: 
    case ENUMERATED: 
        {
            long int val = value->data.integer;

            if (*d_len > sizeof (long int))
                *d_len = sizeof (long int);

            switch (*d_len)
            {
                case sizeof(char) : *((char *) data) = val; break;
                case sizeof(short): *((short *)data) = val; break;
                case sizeof(long) : *((long *) data) = val; break;
                default: 
                    return EASNGENERAL;
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
            *d_len = value->len;
            memcpy (data, value->data.other, m_len);
        }
        break;

    case PR_ASN_NULL:
        break;

    case SEQUENCE:
    case SET:
    case SEQUENCE_OF:
    case SET_OF: 
    case CHOICE:
    case TAGGED:
        return EASNNOTLEAF;

    default: 
        return 0; /* nothing to do. */
    }

    return 0;
}


/**
 * Write component of CONSTRAINT subvalue in ASN value tree.
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
int
asn_write_component_value(asn_value_p container, 
                          const asn_value *elem_value,
                          const char *subval_labels)
{
    char *field_labels_int_copy = asn_strdup(subval_labels); 

    int   rc = asn_impl_write_component_value(container, elem_value,
                                              field_labels_int_copy);
    free (field_labels_int_copy);

    return rc;
}


/**
 * Write component of CONSTRAINT subvalue in ASN value tree,
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
int
asn_impl_write_component_value(asn_value_p container, 
                               const asn_value *elem_value,
                               char *subval_labels)
{
    char        *rest_field_labels = subval_labels;
    char        *cur_label = "";
    asn_value_p  subvalue;
    int          rc;

    if (!container || !elem_value || !subval_labels) return ETEWRONGPTR; 

    if (!(container->syntax & CONSTRAINT))
        return EASNWRONGLABEL;

    container->txt_len = -1;

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
    { /* There are more levels.. */
        if (rc) return rc;

        rc = asn_impl_write_component_value(subvalue, elem_value,
                                        rest_field_labels);
    }
    else
    { /* no more labels, subvalue should be changed at this level */
        switch(rc)
        {
            case 0: /* there is subvalue on that place, should be freed first.*/
                asn_free_value(subvalue);
                asn_impl_insert_subvalue(container, cur_label, NULL);
                /* pass through ... */
            case EASNINCOMPLVAL:
            {
                asn_value_p new_value = NULL;

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
                        return EASNWRONGTYPE;
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
                        return EASNWRONGTYPE;
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
                    return EASNGENERAL;
                rc = asn_impl_insert_subvalue(container, cur_label, 
                                                         new_value);

            }
            default:
                break;
        }
    }
    return rc; 
}

/**
 * See description in asn_usr.h
 */
int 
asn_get_subvalue(const asn_value *container, const asn_value **subval,
                 const char *subval_labels)
{
    int rc;
    char *rest_labels;

    if (!container || !subval)
        return ETEWRONGPTR; 

    if (subval_labels == NULL || *subval_labels == '\0')
    {
        if (container->syntax == CHOICE)
        { 
            if ((*subval = container->data.array[0]) == NULL)
                return EASNINCOMPLVAL; 
        }
        else
            *subval = container;
        return 0;
    }

    if (!(container->syntax & CONSTRAINT))
        return EASNWRONGTYPE;

    rest_labels = asn_strdup(subval_labels); 

    rc = asn_impl_fall_down_to_tree_nc (container, rest_labels, subval); 
    free (rest_labels); 
    return rc;
}

/**
 * See description in asn_usr.h
 */
int
asn_get_indexed(const asn_value *container, const asn_value **subval, 
                int index)
{
    if (!container || !subval)
        return ETEWRONGPTR; 

    if (container->syntax != SEQUENCE_OF && 
        container->syntax != SET_OF)
        return EINVAL;

    if (index < 0)
        return EINVAL; 

    if ((unsigned int)index >= container->len)
        return EASNINCOMPLVAL; 

    *subval = container->data.array[index];

    return 0;
}

/**
 * See description in asn_usr.h
 */
int
asn_get_child_type(const asn_type *type, const asn_type **subtype,
                   asn_tag_class tag_class, uint16_t tag_val)
{
    int index, rc;

    if (type == NULL || subtype == NULL)
        return ETEWRONGPTR; 

    rc = asn_child_tag_index(type, tag_class, tag_val, &index);
    if (rc != 0)
        return rc;
    
    *subtype = type->sp.named_entries[index].type;

    return 0;
}

/**
 * See description in asn_usr.h
 */
int
asn_get_child_value(const asn_value *container, const asn_value **subval,
                    asn_tag_class tag_class, uint16_t tag_val)
{
    int index, rc;

    if (!container || !subval)
        return ETEWRONGPTR; 

    if (container->syntax != SEQUENCE && 
        container->syntax != SET)
        return EINVAL;

    rc = asn_child_tag_index(container->asn_type, tag_class, tag_val,
                             &index);
    if (rc != 0)
        return rc;

    if ((*subval = container->data.array[index]) == NULL)
        return EASNINCOMPLVAL;

    return 0;
}


/**
 * See description in asn_usr.h
 */
int
asn_get_choice_value(const asn_value *container, const asn_value **subval,
                     asn_tag_class *tag_class, uint16_t *tag_val)
{
    UNUSED(tag_class);
    UNUSED(tag_val);

    if (!container || !subval)
        return ETEWRONGPTR; 

    if (container->syntax != CHOICE)
        return EINVAL; 

    if (container->len == 0 || container->data.array[0] == NULL)
        return EASNINCOMPLVAL;

    *subval = container->data.array[0];

    return 0;
}


/**
 * See description in asn_usr.h
 */
int 
asn_get_field_data(const asn_value *container, 
                   const uint8_t ** data_ptr, const char *subval_labels)
{
    const asn_value *subval = container;
    int rc;


    if (!data_ptr || !container)
    {
        return ETEWRONGPTR;
    }

    if(container->syntax & CONSTRAINT)
    {
        rc = asn_get_subvalue (container, &subval, subval_labels);
        if (rc)
            return rc;
    }
    else if (subval_labels && *subval_labels)
    {
        return EASNWRONGLABEL;
    }

    switch (subval->syntax) 
    {
        case BOOL: 
        case INTEGER: 
        case ENUMERATED: 
            *data_ptr = (uint8_t *)&subval->data.integer; 
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
            return EASNNOTLEAF;

        default: 
            return 0; /* nothing to do. */
    }
    return 0;
}


/**
 * See description in asn_usr.h
 */ 
int
asn_read_component_value (const asn_value *container, 
                asn_value ** elem_value, const char *subval_labels)
{
    const asn_value  *subvalue;
    int          rc; 

    rc = asn_get_subvalue(container, &subvalue, subval_labels); 
    if (rc) return rc;

    *elem_value = asn_copy_value(subvalue);
    if (*elem_value == NULL) 
        return ENOMEM;

    /* fall into CHOICE and TAGGED */
    if ( (*elem_value)->syntax == CHOICE ||
         (*elem_value)->syntax == TAGGED   )
    {
        asn_value_p transparent_val = *elem_value;
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
int
asn_write_indexed(asn_value_p container, const asn_value_p elem_value, 
                  int index, const char *subval_labels)
{
    asn_value_p value = container;

    int rc;

    rc = asn_impl_fall_down_to_tree_writable(container, subval_labels, &value); 
    if (rc)
        return rc;

    container->txt_len = -1;

    if (strcmp(elem_value->asn_type->name, value->asn_type->sp.subtype->name))
    {
        return EASNWRONGTYPE; 
    }

    switch (value->syntax)
    {
        case SEQUENCE_OF:
        case SET_OF: 
            asn_free_value(value->data.array[index]); 
            value->data.array[index] = asn_copy_value(elem_value);
            break;
        
        default:
            return EASNGENERAL;
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
asn_value_p
asn_read_indexed(const asn_value *container, int index,
                 const char *subval_labels)
{
    const asn_value *value = container;

    if (asn_get_subvalue(container, &value, subval_labels)) 
        return NULL;


    if (index < 0) index += (int)value->len;

    if ( (index < 0) || (index >= (int)value->len) ) return NULL;

    switch (value->syntax)
    {
        case SEQUENCE_OF:
        case SET_OF:
        {
            asn_value_p subval = value->data.array[index];
            while ((subval->syntax == CHOICE ) || (subval->syntax == TAGGED ))
                if ((subval = subval->data.array[0]) == NULL) 
                    return NULL;

            return asn_copy_value(subval); 
        }
        default:
            return NULL;
    }

    return NULL;
}

/**
 * Insert array element in indexed syntax (i.e. 'SEQUENCE OF' or 'SET OF') 
 * subvalue of root ASN value container. 
 *
 * @param container     Root of ASN value tree which subvalue is interested.
 * @param elem_value    ASN value to be placed into the array, specified 
 *                      by subval_labels at place, specified by index.
 * @param index         Array index of place to which element should be
 *                      inserted. 
 * @param subval_labels string with dot-separated sequence of textual field
 *                      labels, specifying indexed syntax subvalue in ASN value 
 *                      tree with 'container' as a root. 
 *
 * @return zero on success, otherwise error code.
 */ 
int
asn_insert_indexed(asn_value_p container, const asn_value_p elem_value, 
                              int index, const char *subval_labels )
{
    asn_value_p value;
    int r_c; 
    int new_len;

    r_c = asn_impl_fall_down_to_tree_writable(container, subval_labels, &value); 

    if (r_c) return r_c;

    container->txt_len = -1;

    if (strcmp(elem_value->asn_type->name, value->asn_type->sp.subtype->name))
    {
        return EASNWRONGTYPE;
    }

    new_len = value->len + 1;

    if (index < 0) index += new_len;

    if ( (index < 0) || (index >= new_len) ) return EASNWRONGLABEL;

    switch (value->syntax)
    {
        case SEQUENCE_OF:
        case SET_OF:
        {
            asn_value_p *arr = malloc(new_len * sizeof(asn_value_p));
            unsigned int i;
            if (arr == NULL) return ENOMEM;

            for (i = 0; i < (unsigned)index; i++)
                arr[i] = value->data.array[i];

            arr[index] = asn_copy_value(elem_value); 

            for (; i < value->len; i++)
                arr[i+1] = value->data.array[i];

            if (value->data.array)
                free(value->data.array);
            value->data.array = arr;
            value->len = new_len;
        }

            break;
        default:
            return EASNWRONGTYPE;
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
int
asn_remove_indexed(asn_value_p container, int index, const char *subval_labels)
{
    asn_value_p value = container;
    int r_c; 

    r_c = asn_impl_fall_down_to_tree_writable(container, subval_labels, &value); 

    if (r_c) return r_c;

    container->txt_len = -1;

    if (index < 0) index += (int)value->len;

    if ( (index < 0) || (index >= (int)value->len) ) return EASNWRONGLABEL;

    switch (value->syntax)
    {
        case SEQUENCE_OF:
        case SET_OF:
        {
            asn_value_p *arr = NULL;
            unsigned int i;
            
            if (value->len > 1)
            {
                arr = malloc((value->len - 1) * sizeof(asn_value_p)); 
                if (arr == NULL) return ENOMEM;
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

            break;
        default:
            return EASNWRONGTYPE;
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

    if (asn_get_subvalue(container, &val, subval_labels)) 
        return -1;

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
int 
asn_impl_fall_down_to_tree_nc (const asn_value *container, char *field_labels, 
                            asn_value const **found_value)
{
    const asn_value *value;

    char *rest_labels = field_labels; 
    char *cur_label;
    int   r_c = 0; 

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
        r_c = asn_impl_find_subvalue(value, cur_label, &subvalue);
        value = subvalue; 

        if (r_c) break;
    }

    *found_value = (asn_value_p) value;

    return r_c;
}




/**
 * Find one-depth subvalue in ASN value tree by its label.
 * This method is applicable only to values with CONSTRAINT syntax. 
 *
 * @param container  pointer to ASN value which leaf field is interested;
 * @param label      textual field label, specifying subvalue of 'container'. 
 *                   Label for 'SEQUENCE OF' and 'SET OF' subvalues 
 *                   is decimal notation of its integer index in array.
 * @param found_val  pointer to found subvalue (OUT).
 *
 * @return zero on success, otherwise error code. 
 */ 
int
asn_impl_find_subvalue(const asn_value *container, const char *label, 
                       asn_value const **found_val)
{ 
    if( !container || !found_val)
        return ETEWRONGPTR; 

    if( !(container->syntax & CONSTRAINT))
        return EASNGENERAL; 

    if (!(container->syntax & 1))
    { /* we have current node constraint with named subtrees */
        int index;
        int rc;
        const asn_value_p * arr = container->data.array; 

        switch (container->syntax)
        {
        case CHOICE:
            index = 0;

            if (arr[0] == NULL) 
                return EASNINCOMPLVAL;

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

            return EASNOTHERCHOICE;

        case SET: /*order of subvalues in SET syntax value may be arbitrary. */
            for (index = 0; index < (int)container->len; index++)
                if (strcmp(label, arr[index]->name) == 0)
                    break;

            if (index < (int)container->len) break;

            return EASNINCOMPLVAL;

        default:
            rc = asn_impl_named_subvalue_index(container->asn_type, 
                                               label, &index); 
            if (rc) return rc;

            if (arr[index] == NULL)
                return EASNINCOMPLVAL;
            break;
        }

        *found_val = arr[index];
        return 0;
    }
    else if (container->syntax == TAGGED)
    { 
        if ((*found_val = container->data.array[0]) == NULL)
            return EASNINCOMPLVAL;

        return 0;
    } /* We have *_OF value */ 
    else 
    {
        int index;
        char *rest;
        index = strtol (label, &rest, 10);
        if (*rest) 
            return EASNWRONGLABEL;
        
        if (index < 0)
            index += container->len;
        if ((index < 0 ) || (index >= (int)container->len))
            return EASNINCOMPLVAL;

        *found_val = container->data.array[index];
        return 0;
    } 
    return EASNGENERAL;
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
int 
asn_get_choice(const asn_value *container, const char *subval_labels,
               char *choice_label, size_t ch_lb_len)
{
    const asn_value *val;
    const asn_value *sval;
    int rc;
    int len = strlen (subval_labels);
    static char suffix [] = {'.', '#', '\1', '\0'};
    char *corrected_labels = malloc (len + sizeof(suffix));

    if (len)
    { 
        memcpy (corrected_labels, subval_labels, len); 
        memcpy (corrected_labels + len, suffix, sizeof(suffix)); 

        rc = asn_impl_fall_down_to_tree_nc(container, corrected_labels, &val); 
        free (corrected_labels);

        if (rc != 0 && rc != ASN_STOP_AT_CHOICE)
            return rc; 
    }
    else 
        val = container;

    if (val->syntax != CHOICE)
    {
        return EASNWRONGTYPE;
    }

    if (val->data.array == NULL)
    {
        printf ("data array is null\n");
        return EASNGENERAL; 
    }

    if ((sval = val->data.array[0]) == NULL) 
        return EASNINCOMPLVAL; 

    if (strlen(sval->name) >= ch_lb_len)
        return ETESMALLBUF;

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
unsigned short 
asn_get_tag (const asn_value_p container )
{
    if (container)
        return container->tag.val;
    return -1;
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
    char *corrected_labels;

    if (value == NULL)
        return SYNTAX_UNDEFINED;

    if (subval_labels == NULL || *subval_labels == '\0')
        return value->syntax;

    len = strlen (subval_labels);
    if (len)
    { 
        corrected_labels = malloc (len + sizeof(suffix));

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
int 
asn_get_subtype(const asn_type *container, 
                const asn_type **found_subtype, const char *labels)
{
    const asn_type *type;
    char *rl_store_ptr;
    char *rest_labels = rl_store_ptr = asn_strdup(labels); 
    char *cur_label;
    int r_c = 0; 

    type = container;

    while (rest_labels && (*rest_labels))
    { 
        const asn_type *subtype;
        cur_label = "";

        switch (type->syntax)
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
        r_c = asn_impl_find_subtype(type, cur_label, &subtype);
        type = subtype; 

        if (r_c) break;
    }

    *found_subtype = type;

    free(rl_store_ptr);

    return r_c;
}

/* See description in asn_usr.h */
int
asn_label_to_tag(const asn_type *type, const char *label, asn_tag_t *tag)
{
    int rc = 0;
    int index;

    if (tag == NULL)
        return ETEWRONGPTR;

    rc = asn_impl_named_subvalue_index(type, label, &index);
    if (rc != 0)
        return rc; 

    *tag = type->sp.named_entries[index].tag;

    return 0;
}



/**
 * Definitions of ASN.1 base types.
 */ 
asn_type asn_base_boolean_s = 
{ "BOOLEAN",           {UNIVERSAL, 1}, BOOL,        0, {NULL} };
asn_type asn_base_integer_s = 
{ "INTEGER",           {UNIVERSAL, 2}, INTEGER,     0, {NULL} }; 
asn_type asn_base_bitstring_s = 
{ "BIT STRING",        {UNIVERSAL, 3}, BIT_STRING,  0, {NULL} };
asn_type asn_base_octstring_s = 
{ "OCTET STRING",      {UNIVERSAL, 4}, OCT_STRING,  0, {NULL} };
asn_type asn_base_null_s = 
{ "NULL",              {UNIVERSAL, 5}, PR_ASN_NULL, 0, {NULL} };
asn_type asn_base_objid_s = 
{ "OBJECT IDENTIFIER", {UNIVERSAL, 6}, OID,         0, {NULL} };
asn_type asn_base_real_s = 
{ "REAL",              {UNIVERSAL, 9}, REAL,        0, {NULL} };
asn_type asn_base_enum_s = 
{ "ENUMERATED",        {UNIVERSAL,10}, ENUMERATED,  0, {NULL} };
asn_type asn_base_charstring_s = 
{ "UniversalString",   {UNIVERSAL,28}, CHAR_STRING, 0, {NULL} }; 

asn_type asn_base_int4_s = 
{ "INTEGER (0..15)",   {UNIVERSAL, 2}, INTEGER, 4, {0}}; 
asn_type asn_base_int8_s = 
{ "INTEGER (0..255)",  {UNIVERSAL, 2}, INTEGER, 8, {0}}; 
asn_type asn_base_int16_s = 
{ "INTEGER (0..65535)",{UNIVERSAL, 2}, INTEGER, 16, {0}};



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

