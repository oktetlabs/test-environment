/** @file 
 * @brief CWMP data common methods implementation
 * 
 * CWMP data exchange common methods, useful for transfer CWMP message
 * structures, declared in cwmp_soapStub.h, between processes with 
 * different address spaces.
 *
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution). 
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version. 
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details. 
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
*/


#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "cwmp_data.h"
#include "acse_soapH.h"
#include "cwmp_utils.h"

#include <string.h>
#include <strings.h>

int va_end_list_var = 10;
void const * const va_end_list_ptr = &va_end_list_ptr;

#define CWMP_VAL_ARR_QUANTUM 8

static inline te_errno
cwmp_str_array_add_va(string_array_t *a,
                      const char *b_name,
                      const char *first_name,
                      va_list ap)
{
    size_t      real_arr_len = a->size, i = a->size;
    size_t      b_len, n_len;
    const char *v_name = first_name;
    char      **array = a->items;

    b_len = strlen(b_name);

    do {
        n_len = (NULL != v_name ? strlen(v_name) : 0);

        if (real_arr_len <= i)
        {
            real_arr_len += CWMP_VAL_ARR_QUANTUM;
            a->items = array = realloc(array,
                                       real_arr_len * sizeof(char*));
        }
        if (NULL == (array[i] = malloc(b_len + n_len + 1)))
            return TE_ENOMEM;

        strcpy(array[i], b_name);
        if (v_name != NULL)
            strcpy(array[i] + b_len, v_name);

        a->size = (++i);
        v_name = va_arg(ap, const char *);
    } while (VA_END_LIST != v_name);

    return 0;
}

/* see description in cwmp_utils.h */
string_array_t *
cwmp_str_array_copy(string_array_t *src)
{
    unsigned i;
    string_array_t *res = malloc(sizeof(*res));
    if (NULL == src)
        return NULL;
    res->size = src->size;
    if (NULL == (res->items = calloc(src->size, sizeof(char *))))
        return NULL;
    for (i = 0; i < src->size; i++)
        if (src->items[i])
            if (NULL == (res->items[i] = strdup(src->items[i])))
                return NULL;
    return res;
}

/* see description in cwmp_utils.h */
string_array_t *
cwmp_str_array_alloc(const char *b_name, const char *first_name, ...)
{
    va_list         ap;
    te_errno        rc = 0; 
    string_array_t *ret;

    if (NULL == (ret = malloc(sizeof(*ret))))
        return NULL;
    ret->items = NULL;
    ret->size = 0;


    va_start(ap, first_name);
    if (NULL != b_name && NULL != first_name)
        rc = cwmp_str_array_add_va(ret, b_name, first_name, ap);
    va_end(ap);
    if (rc != 0)
    {
        WARN("%s(): alloc string array failed %r", __FUNCTION__, rc);
        cwmp_str_array_free(ret);
        return NULL;
    } 

    return ret;
}



/* see description in cwmp_utils.h */
te_errno
cwmp_str_array_add(string_array_t *a,
                   const char *b_name, const char *first_name, ...)
{
    va_list     ap;
    te_errno    rc;

    if (NULL == a || NULL == b_name || NULL == first_name)
        return TE_EINVAL;

    va_start(ap, first_name);
    rc = cwmp_str_array_add_va(a, b_name, first_name, ap);
    va_end(ap);

    return rc;
}


/* see description in cwmp_utils.h */
te_errno
cwmp_str_array_cat_tail(string_array_t *a, const char *suffix)
{
    size_t s_len;
    unsigned i;

    if (NULL == a || NULL == suffix)
        return TE_EINVAL;

    s_len = strlen(suffix);
    for (i = 0; i < a->size; i++)
    {
        size_t item_len;
        if (NULL == a->items[i])
            continue;
        item_len = strlen(a->items[i]);
        a->items[i] = realloc(a->items[i], item_len + s_len + 1);
        if (NULL == a->items[i]) 
            /* out of memory, it seems unnecessary to leave consistancy */
            return TE_ENOMEM;
        memcpy(a->items[i] + item_len, suffix, s_len + 1);
    }
    return 0;
}


/* see description in cwmp_utils.h */
void
cwmp_str_array_free(string_array_t *a)
{
    if (NULL == a)
        return;
    if (a->size > 0)
    {
        unsigned i;
        for (i = 0; i < a->size; i++)
            free(a->items[i]);
    }
    free(a->items);
    free(a);
}

/* */
#define STR_LOG_MAX 256

te_errno
cwmp_str_array_log(unsigned log_level, const char *intro, string_array_t *a)
{
    size_t log_buf_size = STR_LOG_MAX * (a->size + 1);
    char *log_buf = malloc(log_buf_size);
    char *s = log_buf;
    size_t p, total_p = 0, i;
    if (NULL == log_buf) return TE_ENOMEM;

    if (NULL == intro) intro = "CWMP_UTILS, array of string";
    p = snprintf(s, log_buf_size - total_p, "%s:\n", intro);
    s += p; total_p += p;

    for (i = 0; (i < a->size) && (total_p < log_buf_size); i++)
    {
        p = snprintf(s, log_buf_size - total_p, "   %s\n", a->items[i]);
        s += p; total_p += p;
    }

    LGR_MESSAGE(log_level, TE_LGR_USER, log_buf);

    free(log_buf);

    return 0;
}

/* Internal common method */
static inline te_errno
cwmp_val_array_add_va(cwmp_values_array_t *a,
                      const char *base_name, const char *first_name,
                      va_list ap)
{
    size_t      real_arr_len = a->size, i = a->size;
    const char *v_name = first_name;
    size_t      b_len, v_len;

    cwmp_parameter_value_struct_t **array = a->items;

    b_len = strlen(base_name);

    VERB("add vals to val_array. b_len %d; first_name '%s'", 
         b_len, first_name);

    do {
        if (real_arr_len <= i)
        {
            real_arr_len += CWMP_VAL_ARR_QUANTUM;
            a->items = array = realloc(array,
                                     real_arr_len * sizeof(array[0]));
        }
        v_len = (NULL != v_name ? strlen(v_name) : 0);
        array[i] = malloc(sizeof(cwmp_parameter_value_struct_t));
        if (NULL == array[i])
            return TE_ENOMEM;
        array[i]->Name = malloc(b_len + v_len + 1);
        if (NULL == array[i]->Name)
            return TE_ENOMEM;

        memcpy(array[i]->Name, base_name, b_len);
        memcpy(array[i]->Name + b_len, v_name, v_len + 1);

        VERB("add val to val_array[%d]: v_len %d; sfx '%s', Name '%s'", 
             i, v_len, v_name, array[i]->Name);

        array[i]->__type = va_arg(ap, int);
        switch (array[i]->__type)
        {
#define SET_SOAP_TYPE(type_, arg_type_) \
            do { \
                type_ val = va_arg(ap, arg_type_ ); \
                array[i]->Value = malloc(sizeof(type_)); \
                *((type_ *)array[i]->Value) = val; \
            } while (0)

            case SOAP_TYPE_boolean:         
            case SOAP_TYPE_int:         
                SET_SOAP_TYPE(int, int);  break;
            case SOAP_TYPE_byte:
                SET_SOAP_TYPE(char, int); break;
            case SOAP_TYPE_unsignedInt:
                SET_SOAP_TYPE(uint32_t, int); break;
            case SOAP_TYPE_unsignedByte:
                SET_SOAP_TYPE(uint8_t, int); break;
            case SOAP_TYPE_time:
                SET_SOAP_TYPE(time_t, time_t); break;
#undef SET_SOAP_TYPE
            case SOAP_TYPE_string:
            case SOAP_TYPE_SOAP_ENC__base64:
                {
                    const char * val = va_arg(ap, const char *);
                    array[i]->Value = strdup(NULL != val ? val : "");
                }
                break;
            default:
                RING("%s(): unsupported type %d",
                     __FUNCTION__, array[i]->__type);
            return TE_EINVAL;
        }

        v_name = va_arg(ap, const char *);
        a->size = (++i);
    } while (VA_END_LIST != v_name);

    return 0;
}


/* see description in cwmp_utils.h */
cwmp_values_array_t *
cwmp_val_array_alloc(const char *b_name, const char *first_name, ...)
{
    va_list   ap;
    te_errno  rc = 0; 

    cwmp_values_array_t *ret;

    if (NULL == (ret = malloc(sizeof(*ret))))
        return NULL;
    ret->items = NULL;
    ret->size = 0;

    if (NULL == b_name || NULL == first_name)
        return ret;

    va_start(ap, first_name);
    rc = cwmp_val_array_add_va(ret, b_name, first_name, ap);
    va_end(ap);
    if (rc != 0)
    {
        WARN("%s(): alloc string array failed %r", __FUNCTION__, rc);
        cwmp_val_array_free(ret);
        return NULL;
    } 

    return ret;
}


/* see description in cwmp_utils.h */
te_errno
cwmp_val_array_add(cwmp_values_array_t *a,
                   const char *b_name, const char *first_name, ...)
{
    va_list     ap;
    te_errno    rc;

    if (NULL == a || NULL == b_name || NULL == first_name)
        return TE_EINVAL;

    va_start(ap, first_name);
    rc = cwmp_val_array_add_va(a, b_name, first_name, ap);
    va_end(ap);

    return rc;
}

/* see description in cwmp_utils.h */
cwmp_values_array_t *
cwmp_copy_par_value_list(cwmp_parameter_value_list_t *src)
{
    cwmp_values_array_t           *ret = NULL;
    cwmp_parameter_value_struct_t *pval_src, 
                                  *pval_dst;

    size_t      val_size = 0;
    unsigned    i;

    if (NULL == src)
        return NULL;

    ret = malloc(sizeof(cwmp_values_array_t));
    ret->size = src->__size;
    ret->items = calloc(ret->size, sizeof(void*));
    for (i = 0; i < ret->size; i++)
    {
        pval_src = src->__ptrParameterValueStruct[i];
        ret->items[i] = pval_dst = 
            malloc(sizeof(cwmp_parameter_value_struct_t));

        pval_dst->Name = strdup(pval_src->Name);
        pval_dst->__type = pval_src->__type;
        switch(pval_dst->__type)
        {
            case SOAP_TYPE_boolean:
                val_size = sizeof(enum xsd__boolean); break;
            case SOAP_TYPE_int:
                val_size = sizeof(int); break;
            case SOAP_TYPE_byte:
                val_size = sizeof(char); break;
            case SOAP_TYPE_unsignedInt:
                val_size = sizeof(unsigned int); break;
            case SOAP_TYPE_unsignedByte:
                val_size = sizeof(unsigned char); break;
            case SOAP_TYPE_time:
                val_size = sizeof(time_t); break;
            case SOAP_TYPE_string:
                val_size = strlen(pval_src->Value) + 1; break;
            default:
                RING("Copy CWMP ParValue, unsupported type %d",
                    pval_dst->__type);
                val_size = 0;
        }
        pval_dst->Value = malloc(val_size);
        memcpy(pval_dst->Value, pval_src->Value, val_size);
    }
    return ret;
}


/* see description in cwmp_utils.h */
void
cwmp_val_array_free(cwmp_values_array_t *a)
{
    if (NULL == a)
        return;
    if (a->size > 0)
    {
        unsigned i;
        for (i = 0; i < a->size; i++)
        {
            free(a->items[i]->Name);
            free(a->items[i]->Value);
            free(a->items[i]);
        }
    }
    free(a->items);
    free(a);
}



/* see description in cwmp_utils.h */
te_errno
cwmp_val_array_get_int(cwmp_values_array_t *a, const char *name, 
                       int *type, int *value)
{
    unsigned i;

    if (NULL == a || NULL == value)
        return TE_EINVAL;

    for (i = 0; i < a->size; i++)
    {
        char *suffix = rindex(a->items[i]->Name, '.');
        if (NULL == suffix)
            continue;
        if (NULL == name || strcmp(suffix + 1, name) == 0)
        {
            switch (a->items[i]->__type)
            {
                case SOAP_TYPE_xsd__boolean:         
                case SOAP_TYPE_int:         
                case SOAP_TYPE_unsignedInt:
                    *value = *((int *)a->items[i]->Value);
                    break;
                case SOAP_TYPE_byte:
                case SOAP_TYPE_unsignedByte:
                    *value = *((int8_t *)a->items[i]->Value);
                    break;
                    
                case SOAP_TYPE_time:
                case SOAP_TYPE_string:
                case SOAP_TYPE_SOAP_ENC__base64:
                    return TE_EBADTYPE;
            }
            if (NULL != type)
                *type = a->items[i]->__type;
            return 0;
        }
    }
    return TE_ENOENT;
}


te_errno
cwmp_val_array_check_int(cwmp_values_array_t *a,
                         const char *name, int type, int value)
{
    int r_type, r_value;
    te_errno rc;

    rc = cwmp_val_array_get_int(a, name, &r_type, &r_value);
    if (0 != rc)
        return rc;
    if (r_type != type)
        return TE_EBADTYPE;
    if (r_value != value)
        return TE_EFAULT;
    return 0;
}



te_errno
cwmp_val_array_get_str(cwmp_values_array_t *a,
                       const char *name, char **value)
{
    unsigned i;

    if (NULL == a || NULL == value)
        return TE_EINVAL;

    for (i = 0; i < a->size; i++)
    {
        char *suffix = rindex(a->items[i]->Name, '.');
        if (NULL == suffix)
            continue;
        if (NULL == name || strcmp(suffix + 1, name) == 0)
        {
            switch (a->items[i]->__type)
            {
                case SOAP_TYPE_string:
                    *value = strdup((char *)a->items[i]->Value);
                    break;

                case SOAP_TYPE_xsd__boolean:         
                case SOAP_TYPE_int:         
                case SOAP_TYPE_unsignedInt:
                case SOAP_TYPE_byte:
                case SOAP_TYPE_unsignedByte:
                case SOAP_TYPE_time:
                case SOAP_TYPE_SOAP_ENC__base64:
                    return TE_EBADTYPE;
            }
            return 0;
        }
    }
    return TE_ENOENT;
}

#define VAL_LOG_MAX 512

te_errno
cwmp_val_array_log(unsigned log_level, const char *intro,
                   cwmp_values_array_t *a)
{
    size_t log_buf_size = VAL_LOG_MAX * (a->size + 1);
    char *log_buf = malloc(log_buf_size);
    char *s = log_buf;
    size_t p, total_p = 0, i;
    if (NULL == log_buf) return TE_ENOMEM;

    if (NULL == intro) intro = "CWMP_UTILS, array of values";
    p = snprintf(s, log_buf_size - total_p, "%s:\n    ", intro);
    s += p; total_p += p;

    for (i = 0; (i < a->size) && (total_p < log_buf_size); i++)
    {
        p = snprint_ParamValueStruct(s, log_buf_size-total_p, a->items[i]);
        s += p; total_p += p;
        p = snprintf(s, log_buf_size-total_p, "\n    ");
        s += p; total_p += p;
    }

    LGR_MESSAGE(log_level, TE_LGR_USER, log_buf);

    free(log_buf);

    return 0;
}


/*
 * =========== Utils for CWMP OID =================
 */

/* see description in cwmp_utils.h */
cwmp_oid_t *
cwmp_name_to_oid(const char *name)
{
#define MAX_OID_LEN 256
    char   *loc_label[MAX_OID_LEN] = {NULL, }; 
    size_t  idx = 0, i;
    cwmp_oid_t *new_oid = malloc(sizeof(cwmp_oid_t));

    char *next_label = strdup(name);

    assert(name != NULL);

    do {
        loc_label[idx] = strsep(&next_label, ".");
        idx++;
    } while(next_label != NULL);

    if (loc_label[idx-1][0] == '\0')
        new_oid->size = idx - 1; /* After last '.' no token, object name */
    else
        new_oid->size = idx; /* After last '.' is token, leaf name */

    new_oid->label = calloc(new_oid->size + 1, sizeof(char *));

    for (i = 0; i < new_oid->size; i++)
        new_oid->label[i] = loc_label[i];

    if (loc_label[idx-1][0] == '\0')
        new_oid->label[new_oid->size] = loc_label[new_oid->size];
    else
        new_oid->label[new_oid->size] = NULL;

    return new_oid;
#undef MAX_OID_LEN
}

/* see description in cwmp_utils.h */
te_errno
cwmp_oid_add_str(cwmp_oid_t *oid, ...)
{
    /* TODO */
    UNUSED(oid);
    return TE_EFAIL;
}

/* see description in cwmp_utils.h */
size_t
cwmp_oid_to_string(cwmp_oid_t *oid, char *buf, size_t buf_size)
{
    size_t i, total = 0;

    for (i = 0; (i < oid->size) && (total < buf_size); i++)
    {
        total += snprintf(buf + total, buf_size - total, "%s",
                          oid->label[i]);
        if (oid->label[i+1] != NULL)
            total += snprintf(buf + total, buf_size - total, ".");
    }
    return total;
}

/* see description in cwmp_utils.h */
void
cwmp_oid_free(cwmp_oid_t *oid)
{
    if (oid == NULL) return;

    free(oid->label[0]);
    free(oid->label);
    free(oid);
}


/*
 * =========== Utils for particular CWMP RPCs =================
 */


cwmp_set_parameter_attributes_t *
cwmp_set_attrs_alloc(const char *par_name, int notification, 
                     string_array_t *access_list)
{
    cwmp_set_parameter_attributes_t *req = malloc(sizeof(*req));
    te_errno rc;

    req->ParameterList = calloc(1, sizeof(*(req->ParameterList)));

    rc = cwmp_set_attrs_add(req, par_name, notification, access_list);
    if (rc != 0)
    {
        ERROR("alloc SetParamAttr failed %r", rc);
        free(req->ParameterList);
        free(req);
        return NULL;
    }

    return req;
}

te_errno
cwmp_set_attrs_add(cwmp_set_parameter_attributes_t *request,
                   const char *par_name, int notification, 
                   string_array_t *user_access_list)
{
    cwmp_set_parameter_attributes_list_t     *pa_list;
    cwmp_set_parameter_attributes_struct_t  **array;

    string_array_t  *access_list = NULL;
    size_t           last;

    if (NULL == request || NULL == request->ParameterList || 
        NULL == par_name)
        return TE_EINVAL;

    pa_list = request->ParameterList;
    array = pa_list->__ptrSetParameterAttributesStruct;

    last = pa_list->__size;
    pa_list->__size++;
    pa_list->__ptrSetParameterAttributesStruct = array =
                    realloc(array, pa_list->__size * sizeof(array[0]));
    array[last] = calloc(1, sizeof(*(array[0])));
    array[last]->Name = calloc(1, sizeof(char *));
    *(array[last]->Name) = strdup(par_name);
    if (notification >= 0)
    {
        array[last]->NotificationChange = 1;
        array[last]->Notification = notification;
    }
    else
        array[last]->NotificationChange = 0;

    access_list = cwmp_str_array_copy(user_access_list);

    array[last]->AccessList_ = malloc(sizeof(AccessList));
    if (access_list != NULL)
    {
        array[last]->AccessListChange = 1;
        array[last]->AccessList_->__size = access_list->size;
        if (access_list->size > 0)
            array[last]->AccessList_->__ptrstring = access_list->items;
        else /* Init by any legal pointer, necessary hack for gSOAP */
            array[last]->AccessList_->__ptrstring = malloc(sizeof(char*));
    }
    else
    {   
        /* AccessList should present in message mandatory */
        array[last]->AccessListChange = 0;
        array[last]->AccessList_->__size = 0;
        /* Init by any legal pointer, necessary hack for gSOAP */
        array[last]->AccessList_->__ptrstring = malloc(sizeof(char*));
    }

    return 0;
}


static inline const char *
soap_simple_type_string(int type)
{
    static char buf[10];
    switch (type)
    {
        case SOAP_TYPE_int:          return "SOAP_TYPE_int";
        case SOAP_TYPE_xsd__boolean: return "SOAP_TYPE_boolean";
        case SOAP_TYPE_byte:         return "SOAP_TYPE_byte";
        case SOAP_TYPE_string:       return "SOAP_TYPE_string";
        case SOAP_TYPE_unsignedInt:  return "SOAP_TYPE_unsignedInt";
        case SOAP_TYPE_unsignedByte: return "SOAP_TYPE_unsignedByte";
        case SOAP_TYPE_time:         return "SOAP_TYPE_time";
        case SOAP_TYPE_SOAP_ENC__base64: return "SOAP_TYPE_base64";
    }
    snprintf(buf, sizeof(buf), "<unknown: %d>", type);
    return buf;
}

size_t
snprint_ParamValueStruct(char *buf, size_t len, 
                         cwmp__ParameterValueStruct *p_v)
{
    size_t rest_len = len;
    size_t used;

    char *p = buf;
    void *v  = p_v->Value;
    int  type = p_v->__type;

    used = sprintf(p, "%s (type %s) = ", p_v->Name, 
            soap_simple_type_string(p_v->__type));
    p+= used; rest_len -= used;
    switch(type)
    {
        case SOAP_TYPE_string:
        case SOAP_TYPE_SOAP_ENC__base64:
            p+= snprintf(p, rest_len, "'%s'", (char *)v);
            break;
        case SOAP_TYPE_time:
            p+= snprintf(p, rest_len, "time %dsec", (int)(*((time_t *)v)));
            break;
        case SOAP_TYPE_byte:
            p+= snprintf(p, rest_len, "%d", (int)(*((char *)v)));
            break;
        case SOAP_TYPE_int:    
            p+= snprintf(p, rest_len, "%d", *((int *)v));
            break;
        case SOAP_TYPE_unsignedInt:
            p+= snprintf(p, rest_len, "%u", *((uint32_t *)v));
            break;
        case SOAP_TYPE_unsignedByte:
            p+= snprintf(p, rest_len, "%u", (uint32_t)(*((uint8_t *)v)));
            break;
        case SOAP_TYPE_xsd__boolean:
            p+= snprintf(p, rest_len, *((int *)v) ? "True" : "False");
            break;
    }
    return p - buf;
}

size_t
snprint_cwmpFault(char *buf, size_t len, _cwmp__Fault *f)
{
    char *p = buf;

    p += snprintf(p, len - (p - buf),
                  "CWMP Fault: %s (%s)", f->FaultCode, f->FaultString);
    if (f->__sizeSetParameterValuesFault > 0)
    {
        int i;
        p += snprintf(p, len - (p - buf), "; Set details:");
        for (i = 0; i < f->__sizeSetParameterValuesFault; i++)
        {
            struct _cwmp__Fault_SetParameterValuesFault *p_v_f = 
                                        &(f->SetParameterValuesFault[i]);
            p += snprintf(p, len - (p - buf), 
                          "\n\tparam[%d], name %s, fault %s(%s);",
                          i, p_v_f->ParameterName,
                          p_v_f->FaultCode,
                          p_v_f->FaultString);
        }
    }
    p += snprintf(p, len - (p - buf), "\n");
    return p - buf;
}

#define BUF_LOG_SIZE 32768
static char buf_log[BUF_LOG_SIZE];

void
tapi_acse_log_fault(_cwmp__Fault *f)
{
    snprint_cwmpFault(buf_log, BUF_LOG_SIZE, f);
    WARN(buf_log);
    buf_log[0] = '\0';
}


te_bool
cwmp_check_set_fault(cwmp_fault_t *f, unsigned idx,
                     const char *param_name, const char *fault_code)
{
    assert(f != NULL);
    assert(f->SetParameterValuesFault != NULL);
    assert(param_name != NULL);
    assert(fault_code != NULL);

    return 
        (strcmp(f->SetParameterValuesFault[idx].ParameterName,
                param_name) == 0) &&
        (strcmp(f->SetParameterValuesFault[idx].FaultCode,
                fault_code) == 0);
}


te_bool
cwmp_check_event(cwmp_event_list_t *ev_list, 
                 const char *event_code, const char *command_key)
{
    cwmp_event_struct_t *ev;
    int i;

    /* TODO: make match with regex */
    for (i = 0; i < ev_list->__size; i++)
    {
        ev = ev_list->__ptrEventStruct[i];
        if (strcmp(event_code,  ev->EventCode) == 0 && 
            (NULL == command_key || 
                strcmp(command_key, ev->CommandKey) == 0))
            return TRUE;
    }
    return FALSE;
}


size_t
snprint_cwmpEvents(char *buf, size_t len, cwmp_event_list_t *ev_list)
{
    cwmp_event_struct_t *ev;
    char *p = buf;
    int i;

    for (i = 0; i < ev_list->__size; i++)
    {
        ev = ev_list->__ptrEventStruct[i];
        p += snprintf(p, len - (p-buf),
                      "Event[%u]: code '%s', ComKey '%s'\n",
                      i, ev->EventCode, ev->CommandKey);
    }
    return p - buf;
}


te_errno
tapi_acse_log_cwmpEvents(unsigned log_level, cwmp_event_list_t *ev_list)
{
    char *log_buf;
    size_t buflen;
    if (NULL == ev_list)
        return TE_EINVAL;

    if (0 == ev_list->__size)
    {
        LGR_MESSAGE(log_level, TE_LGR_USER, "Empty EventList.");
        return 0;
    }
    log_buf = malloc(buflen = ev_list->__size * 128);

    snprint_cwmpEvents(log_buf, buflen, ev_list);

    LGR_MESSAGE(log_level, TE_LGR_USER, log_buf);

    free(log_buf);
    return 0;
}


#define URL_BUF_SIZE 1024

static char url_buf[URL_BUF_SIZE];

cwmp_download_t *
cwmp_download_alloc(const char *command_key, cwmp_file_type_t ftype,
                    size_t fsize, const char *url_fmt, ...)
{
    va_list ap;
    int     url_len;

    cwmp_download_t *dl = NULL;

    va_start(ap, url_fmt);
    url_len = vsnprintf(url_buf, URL_BUF_SIZE, url_fmt, ap);
    va_end(ap);

    if (url_len < 0)
    {
        printf("error on vsnprintf()");
        return NULL;
    }
    dl = calloc(1, sizeof(cwmp_download_t));

    dl->CommandKey = strdup(command_key);
    dl->FileType = strdup(cwmp_file_type_to_str(ftype)); 
    dl->URL = strdup(url_buf);
    dl->Username = "";
    dl->Password = "";
    dl->FileSize = fsize;
    dl->TargetFileName = basename(dl->URL);
    dl->SuccessURL = "";
    dl->FailureURL = "";

    return dl;
}




