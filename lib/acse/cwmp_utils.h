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
 * $Id: $
 */
#ifndef __TE_CWMP_UTILS__H__
#define __TE_CWMP_UTILS__H__
#include "te_config.h"
#include "te_errno.h"
#include "cwmp_soapStub.h"

/** End of var-args list. */
extern void * const va_end_list_ptr;
#define VA_END_LIST (va_end_list_ptr)

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
