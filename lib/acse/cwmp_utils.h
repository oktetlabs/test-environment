/** @file 
 * @brief ACSE 
 * 
 * CWMP data structures user utilities declarations. 
 *
 * Copyright (C) 2009-2010 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_CWMP_UTILS__H__
#define __TE_CWMP_UTILS__H__
#include "te_config.h"
#include "te_errno.h"
#include "cwmp_soapStub.h"

/** End of var-args list. */
extern void const * const va_end_list_ptr;
#define VA_END_LIST (va_end_list_ptr)



/*
 * Useful structs to some CWMP parameters processing.
 */

/**
 * Array of strings for user interface of some CWMP methods.
 */
typedef struct {
    char   **items; /* array of pointers to strings */
    size_t   size;  /* size of array, i.e. number of elements */
} string_array_t;

/**
 * Array of CWMP ParameterValue's for user interface of some CWMP methods.
 */
typedef struct {
    cwmp_parameter_value_struct_t **items;/* array of pointers to values */
    size_t   size; /* size of array, i.e. number of elements */
} cwmp_values_array_t;

/**
 * Construct string array for argument of UI for CWMP RPC.
 * First argument is base name, subsequent strings will 
 * be concatenated with the base name; 
 * last argument in list should be VA_END_LIST.
 * All strings in array are allocated by system malloc().
 *
 * @return new allocated value, ready to send.
 */
extern string_array_t *cwmp_str_array_alloc(const char *b_name,
                                            const char *f_name,
                                            ...);


/**
 * Add strings to string array. 
 * Parameters similar to funcion cwmp_str_array_alloc()
 *
 * @return status code.
 */
extern te_errno cwmp_str_array_add(string_array_t *a,
                                   const char *b_name,
                                   const char *f_name,
                                   ...);

/**
 * Add the same suffix to all strings in array. 
 *
 * @param a             array with strings
 * @param suffix        string which should be concatenated with all items.
 *
 * @return status code.
 */
extern te_errno cwmp_str_array_cat_tail(string_array_t *a,
                                        const char *suffix);


/**
 * Copy string array. 
 * 
 * @param a             array with strings to be copied
 */
extern string_array_t * cwmp_str_array_copy(string_array_t *a);

/**
 * Free string array. 
 * 
 * @param a             array with strings to be freed
 */
extern void cwmp_str_array_free(string_array_t *a);


/**
 * Construct value array for argument of UI for CWMP RPC.
 * All agruments, besides first 'base name' and last one,
 * should be separated to the triples <name>, <type>, <value>.
 * Last argument (instead of next <name>) in list should be VA_END_LIST.
 * Parameter <type> should be int value, <value> should be respective:
 *   <type>                          expected C type of <value>
 * SOAP_TYPE_boolean                    int       
 * SOAP_TYPE_int                        int       
 * SOAP_TYPE_byte                       int
 * SOAP_TYPE_string                     const char *
 * SOAP_TYPE_unsignedInt                uint32_t
 * SOAP_TYPE_unsignedByte               uint32_t
 * SOAP_TYPE_time                       time_t
 *
 * All memory blocks in array (for names and values) are allocated
 * by system malloc().
 *
 * @return new allocated value, ready to send.
 */
extern cwmp_values_array_t *cwmp_val_array_alloc(const char *b_name,
                                                 const char *f_name,
                                                 ...);


/**
 * Add values to the array. 
 * 
 * @param a             array with CWMP values
 *
 * Rest parameters similar to funcion cwmp_val_array_alloc()
 *
 * @return status code.
 */
extern te_errno cwmp_val_array_add(cwmp_values_array_t *a,
                                   const char *b_name,
                                   const char *f_name,
                                   ...);

/**
 * Find value with specified name in the array, check that
 * it has integer-like type, and obtain for user its exact type
 * and value. 
 *
 * @param a             array with CWMP values
 * @param name          name for search value
 * @param type          location for value type
 * @param value         location for value
 *
 * @return status code: 0 on success,
 *                      TE_ENOENT if value with asked name is not found,
 *                      TE_EBADTYPE if value has wrong type.
 */
extern te_errno cwmp_val_array_get_int(cwmp_values_array_t *a,
                                       const char *name, 
                                       int *type,
                                       int *value);

/**
 * Find value with specified name in the array, check that
 * it has string type, and obtain string for user. 
 *
 * @param a             array with CWMP values
 * @param name          name for search value
 * @param value         location for value; memory block for string 
 *                      itself will be allocated by malloc()
 *
 * @return status code: 0 on success,
 *                      TE_ENOENT if value with asked name is not found,
 *                      TE_EBADTYPE if value has wrong type.
 */
extern te_errno cwmp_val_array_get_str(cwmp_values_array_t *a,
                                       const char *name, 
                                       char **value);

/**
 * Check integer-like value in the CWMP values array. 
 * Passed <type> should be one of the following: 
 *
 * SOAP_TYPE_boolean
 * SOAP_TYPE_int   
 * SOAP_TYPE_byte 
 * SOAP_TYPE_unsignedInt
 * SOAP_TYPE_unsignedByte
 * 
 * @return    0 if all passed triples (<name>,<type>,<value>)
 *              are present in the array;
 *            TE_ENOENT if there is NO value with asked <name> in array;
 *            TE_EBADTYPE if there is value with asked <name> in array,
 *                        but it has type, differ then asked;
 *            TE_EFAULT if there is value with asked <name> in array,
 *                        but it wrong value;
 *            TE_EINVAL incoming data is not consistant.
 */
extern te_errno cwmp_val_array_check_int(cwmp_values_array_t *a,
                                         const char *name, 
                                         int type,
                                         int value);
/**
 * Check string value in the CWMP values array. 
 * Type of value with specifien name should be SOAP_TYPE_string.
 * 
 * @return    0 if all passed triples (<name>,<type>,<value>)
 *              are present in the array;
 *            TE_ENOENT if there is NO value with asked <name> in array;
 *            TE_EBADTYPE if there is value with asked <name> in array,
 *                        but it has type, differ then asked;
 *            TE_EFAULT if there is value with asked <name> in array,
 *                        but it wrong value;
 *            TE_EINVAL incoming data is not consistant.
 */
extern te_errno cwmp_val_array_check_str(cwmp_values_array_t *a,
                                         const char *name, 
                                         const char *value);


/**
 * Free CWMP values array. 
 */
extern void cwmp_val_array_free(cwmp_values_array_t *a);


static inline cwmp_parameter_value_struct_t *
cwmp_copy_par_value(cwmp_parameter_value_struct_t *src)
{
    size_t val_size = 0;
    cwmp_parameter_value_struct_t *ret = malloc(sizeof(*ret));
    ret->Name = strdup(src->Name);
    ret->__type = src->__type;
    switch(ret->__type)
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
            val_size = strlen(src->Value) + 1; break;
        default:
            RING("Copy CWMP ParValue, unsupported type %d", ret->__type);
            val_size = 0;
    }
    ret->Value = malloc(val_size);
    memcpy(ret->Value, src->Value, val_size);
    return ret;
}


/* ====================== OLD style API. ================== */



/**
 * Construct GetParameterValues argument.
 * First argument is base name, subsequent strings will 
 * be concatenated with the base name; 
 * last argument in list should be VA_END_LIST.
 *
 * @return new allocated value, ready to send.
 */
extern _cwmp__GetParameterValues *cwmp_get_values_alloc(const char *b_name,
                                                        const char *f_name,
                                                        ...);

/**
 * Construct GetParameterValues argument.
 * First argument is base name, subsequent strings will 
 * be concatenated with the base name; 
 * last argument in list should be VA_END_LIST.
 *
 * @return new allocated value, ready to send.
 */
extern _cwmp__GetParameterValues *cwmp_get_values_alloc(const char *b_name,
                                                        const char *f_name,
                                                        ...);

/**
 * Add name to GetParameterValues argument.
 * Last argument in list should be VA_END_LIST.
 */
extern te_errno cwmp_get_values_add(_cwmp__GetParameterValues *req,
                                    const char *b_name,
                                    const char *f_name, ...);

/**
 * Free GetParameterValues argument.
 */
extern void cwmp_get_values_free(_cwmp__GetParameterValues *req);

/**
 * Iterate over GetParameterValues response.
 */


/**
 * Find param in GetParameterValues response.
 */

/**
 * Free GetParameterValues response.
 */
extern void cwmp_get_values_resp_free(_cwmp__GetParameterValues *resp);


/**
 * Construct SetParameterValues argument.
 * All agruments, besides last one, should be separated 
 * to the triples <name>, <type>, <value>.
 * Last argument (instead of next <name>) in list should be VA_END_LIST.
 * Parameter <type> should be int value, <value> should be respective:
 *   <type>                          expected C type of <value>
 * SOAP_TYPE_boolean                    int       
 * SOAP_TYPE_int                        int       
 * SOAP_TYPE_byte                       int
 * SOAP_TYPE_string                     const char *
 * SOAP_TYPE_unsignedInt                uint32_t
 * SOAP_TYPE_unsignedByte               uint32_t
 * SOAP_TYPE_time                       time_t
 * SOAP_TYPE_SOAP_ENC__base64           const char * (??)
 * 
 */
extern _cwmp__SetParameterValues *cwmp_set_values_alloc(
                            const char *par_key,
                            const char *b_name,
                            const char *f_name,
                            ...);

/**
 * Print ParameterValueStruct to the string buffer, for human read.
 *
 * @return used buffer length.
 */
extern size_t snprint_ParamValueStruct(char *buf, size_t len, 
                                       cwmp__ParameterValueStruct *p_v);


/**
 * Put description of CWMP Fault to TE log. 
 */
extern void tapi_acse_log_fault(_cwmp__Fault *fault); 

#endif /* __TE_CWMP_UTILS__H__*/
