/** @file
 * @brief RCF RPC encoding/decoding routines
 *
 * Implementation of routines used by RCF RPC to encode/decode RPC data.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Renata Sayakhova <Renata.Sayakhova@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef RPC_XML
 
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_FEATURES_H
#include <features.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_RPC_TYPES_H
#include <rpc/types.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_RPC_XDR_H
#include <rpc/xdr.h>
#endif
#ifdef HAVE_EXPAT_H
#include <expat.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include "te_defs.h"
#include "xml_xdr.h"


#define LASTUNSIGNED    ((u_int)0-1)

/**
 * Find attribute 'value' and extract it's value
 */ 
#define EXTRACT_VALUE(atts, value) \
    do                                                                  \
    {                                                                   \
        for (; ((atts) != NULL) && (*(atts) != NULL); (atts)++)         \
        {                                                               \
            if (strncmp(*atts++, "value", strlen("value")) == 0)        \
            {                                                           \
                value = (((atts) != NULL) && (*(atts) != NULL)) ?       \
                        (char *)(*(atts)) : NULL;                       \
                break;                                                  \
            }                                                           \
        }                                                               \
    }  while (0)   

/**
 * Find attribute 'rc' and extract it's value
 */ 
#define EXTRACT_RC(atts, value) \
    do                                                                  \
    {                                                                   \
        for (; ((atts) != NULL) && (*(atts) != NULL); (atts)++)         \
        {                                                               \
            if (strncmp(*atts++, "rc", strlen("rc")) == 0)              \
            {                                                           \
                value = (((atts) != NULL) && (*(atts) != NULL)) ?       \
                        (char *)(*(atts)) : NULL;                       \
                break;                                                  \
            }                                                           \
        }                                                               \
    }  while (0)   


/**
 * Forward declarations of string handlers.
 * TODO: must be removed after removing 'char' from tarpc.m4 header
 */ 
static void start_string(void *data, const XML_Char *elem, const XML_Char **atts);
static void end_string(void *data, const XML_Char *elem);
   
/**
 * Structure for uint8_t buffer.
 */ 
typedef struct hex_buf
{
    uint8_t *buf;
    u_int    size;
} hex_buf;

/**
 * Start handler for compound data (structure)
 */ 
void
start_compound_data(void *data, const XML_Char *elem, const XML_Char **atts)
{
    bool_t rc;
    char  *rc_string = NULL;

    UNUSED(elem);
    
    EXTRACT_RC(atts, rc_string);
    rc = (rc_string != NULL && 
          strncmp(rc_string, "FALSE", strlen("FALSE")) == 0) ?
         FALSE : TRUE;
    ((xml_app_data *)data)->rc = rc;
    
    return;
}

/**
 * End handler for compound data (structure)
 */ 
void
end_compound_data(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Start handler for int type
 */ 
static void
start_int(void *data, const XML_Char *elem, const XML_Char **atts)
{
    int  value;
    char *endptr = NULL;
    char *value_str = NULL;
    
    value = 0;

    UNUSED(elem);
    
    EXTRACT_VALUE(atts, value_str);
    if (value_str != NULL)
    {    
        value = strtol(value_str, &endptr, 0);
        if (value == LONG_MIN || value == LONG_MAX || endptr == NULL)
            value = 0;
    }    
    *(int *)data = value;
    return;
}

/**
 * End handler for int type
 */ 
static void
end_int(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Convertion procedure for int type
 */ 
bool_t
xmlxdr_int(XDR *xdrs, int *ip)
{
    char         *buf;
    u_int        *buflen = &(xdrs->x_handy);
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    char         *format;
    int           printed;
    
    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            const char *name = data->name;
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;

            format = (name == NULL) ? "<param value=\"%d\"/>\n" :
                                      "<param name=\"%s\" value=\"%d\"/>\n";
            
            if ((u_int)(printed = snprintf(buf, *buflen,
                                           format,
                                           (name == NULL) ? "" : name, *ip))
                >= *buflen || printed < 0)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;    
            }    
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;
            
            XML_Parser parser = data->parser;
            XML_SetUserData(parser, (void *)ip);
            XML_SetElementHandler(parser, start_int, end_int);

            buf = xdrs->x_public + (strlen(xdrs->x_public) + 1 - *buflen);
            end = strchr(buf, '>');
            end++;
            if (end == NULL)
            {
                printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                return FALSE;
            }        
            else
            {
                printed = end - buf;
                if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                {
                    printf("%s %d: %s at line %d\n",
                           __FUNCTION__, __LINE__,
                           XML_ErrorString(XML_GetErrorCode(parser)),
                           XML_GetCurrentLineNumber(parser));
                    return FALSE;
                }
                *buflen -= printed;
            }
            break;
        }
        case XDR_FREE:
        {
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Start handler for uint8_t type
 */ 
static void
start_uint8_t(void *data, const XML_Char *elem, const XML_Char **atts)
{
    uint32_t  value;
    char     *value_str = NULL;
    char     *endptr = NULL;
    
    value = 0;

    UNUSED(elem);
    
    EXTRACT_VALUE(atts, value_str);
    if (value_str != NULL) 
    {    
        value = strtoul(value_str, &endptr, 0);
        if (value == ULONG_MAX || endptr == NULL)
            value = 0;
    }    
    *(uint8_t *)data = (uint8_t)value;
    return;
}

/**
 * End handler for uint8_t type
 */ 
static void
end_uint8_t(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Converion procedure for uint8_t type
 */ 
bool_t
xmlxdr_uint8_t(XDR *xdrs, uint8_t *ip)
{
    char         *buf;
    u_int        *buflen = &(xdrs->x_handy);
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    char         *format;
    int           printed;
    
    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            const char *name = data->name;
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;
   
            format = (name == NULL) ? "<param value=\"%u\"/>\n" :
                                      "<param name=\"%s\" value=\"%d\"/>\n";
            
            if ((u_int)(printed = snprintf(buf, *buflen,
                                           format,
                                           (name == NULL) ? "" : name, *ip)) 
                >= *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;    
            }    
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;
            
            XML_Parser parser = data->parser;
            XML_SetUserData(parser, (void *)ip);
            XML_SetElementHandler(parser, start_uint8_t, end_uint8_t);

            buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
            end = strchr(buf, '>');
            end++;
            if (end == NULL)
            {
                printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                return FALSE;
            }        
            else
            {
                printed = end - buf;
                if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                {
                    printf("%s %d: %s at line %d\n",
                           __FUNCTION__, __LINE__,
                           XML_ErrorString(XML_GetErrorCode(parser)),
                           XML_GetCurrentLineNumber(parser));
                    return FALSE;
                }
                *buflen -= printed;
            }
            break;
        }
        case XDR_FREE:
        {
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Start handler for uint16_t type
 */ 
static void
start_uint16_t(void *data, const XML_Char *elem, const XML_Char **atts)
{
    uint32_t  value;
    char     *value_str = NULL;
    char     *endptr = NULL;
    
    value = 0;

    UNUSED(elem);
    
    EXTRACT_VALUE(atts, value_str);
    if (value_str != NULL) 
    {    
        value = strtoul(value_str, &endptr, 0);
        if (value == ULONG_MAX || endptr == NULL)
            value = 0;
    }    
    *(uint16_t *)data = (uint16_t)value;
    return;
}

/**
 * End handler for uint16_t type
 */ 
static void
end_uint16_t(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Converion procedure for uint16_t type
 */ 
bool_t
xmlxdr_uint16_t(XDR *xdrs, uint16_t *ip)
{
    char         *buf;
    u_int        *buflen = &(xdrs->x_handy);
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    char         *format;
    int           printed;
    
    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            const char *name = data->name;
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;
   
            format = (name == NULL) ? "<param value=\"%u\"/>\n" :
                                      "<param name=\"%s\" value=\"%d\"/>\n";
            
            if ((u_int)(printed = snprintf(buf, *buflen,
                                           format,
                                           (name == NULL) ? "" : name, *ip)) 
                >= *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;    
            }    
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;
            
            XML_Parser parser = data->parser;
            XML_SetUserData(parser, (void *)ip);
            XML_SetElementHandler(parser, start_uint16_t, end_uint16_t);

            buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
            end = strchr(buf, '>');
            end++;
            if (end == NULL)
            {
                printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                return FALSE;
            }        
            else
            {
                printed = end - buf;
                if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                {
                    printf("%s %d: %s at line %d\n",
                           __FUNCTION__, __LINE__,
                           XML_ErrorString(XML_GetErrorCode(parser)),
                           XML_GetCurrentLineNumber(parser));
                    return FALSE;
                }
                *buflen -= printed;
            }
            break;
        }
        case XDR_FREE:
        {
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Start handler for int16_t type
 */ 
static void
start_int16_t(void *data, const XML_Char *elem, const XML_Char **atts)
{
    int32_t   value;
    char     *value_str = NULL;
    char     *endptr = NULL;
    
    value = 0;

    UNUSED(elem);
    
    EXTRACT_VALUE(atts, value_str);
    if (value_str != NULL) 
    {    
        value = strtol(value_str, &endptr, 0);
        if (value == LONG_MIN || value == LONG_MAX || endptr == NULL)
            value = 0;
    }    
    *(int16_t *)data = (int16_t)value;
    return;
}

/**
 * End handler for int16_t type
 */ 
static void
end_int16_t(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Converion procedure for int16_t type
 */ 
bool_t
xmlxdr_int16_t(XDR *xdrs, int16_t *ip)
{
    char         *buf;
    u_int        *buflen = &(xdrs->x_handy);
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    char         *format;
    int           printed;
    
    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            const char *name = data->name;
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;
   
            format = (name == NULL) ? "<param value=\"%d\"/>\n" :
                                      "<param name=\"%s\" value=\"%d\"/>\n";
            
            if ((u_int)(printed = snprintf(buf, *buflen,
                                           format,
                                           (name == NULL) ? "" : name, *ip)) 
                >= *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;    
            }    
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;
            
            XML_Parser parser = data->parser;
            XML_SetUserData(parser, (void *)ip);
            XML_SetElementHandler(parser, start_int16_t, end_int16_t);

            buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
            end = strchr(buf, '>');
            end++;
            if (end == NULL)
            {
                printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                return FALSE;
            }        
            else
            {
                printed = end - buf;
                if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                {
                    printf("%s %d: %s at line %d\n",
                           __FUNCTION__, __LINE__,
                           XML_ErrorString(XML_GetErrorCode(parser)),
                           XML_GetCurrentLineNumber(parser));
                    return FALSE;
                }
                *buflen -= printed;
            }
            break;
        }
        case XDR_FREE:
        {
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Start handler for int32_t type
 */ 
static void
start_int32_t(void *data, const XML_Char *elem, const XML_Char **atts)
{
    int32_t   value;
    char     *value_str = NULL;
    char     *endptr = NULL;
    
    value = 0;

    UNUSED(elem);
    
    EXTRACT_VALUE(atts, value_str);
    if (value_str != NULL) 
    {    
        value = strtol(value_str, &endptr, 0);
        if (value == LONG_MIN || value == LONG_MAX || endptr == NULL)
            value = 0;
    }    
    *(int32_t *)data = value;
    return;
}

/**
 * End handler for int32_t type
 */ 
static void
end_int32_t(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Converion procedure for int32_t type
 */ 
bool_t
xmlxdr_int32_t(XDR *xdrs, int32_t *ip)
{
    char  *buf;
    u_int *buflen = &(xdrs->x_handy);
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    char  *format;
    int  printed;
    
    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            const char *name = data->name;
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;
   
            format = (name == NULL) ? "<param value=\"%ld\"/>\n" :
                                      "<param name=\"%s\" value=\"%d\"/>\n";
            
            if ((u_int)(printed = snprintf(buf, *buflen,
                                           format,
                                           (name == NULL) ? "" : name, *ip)) 
                >= *buflen)
            {
                printf("%s %d: small biffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;    
            }    
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;
            
            XML_Parser parser = data->parser;
            XML_SetUserData(parser, (void *)ip);
            XML_SetElementHandler(parser, start_int32_t, end_int32_t);

            buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
            end = strchr(buf, '>');
            end++;
            if (end == NULL)
            {
                printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                return FALSE;
            }        
            else
            {
                printed = end - buf;
                if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                {
                    printf("%s %d: %s at line %d\n",
                           __FUNCTION__, __LINE__,
                           XML_ErrorString(XML_GetErrorCode(parser)),
                           XML_GetCurrentLineNumber(parser));
                    return FALSE;
                }
                *buflen -= printed;
            }
            break;
        }
        case XDR_FREE:
        {
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}


/**
 * Start handler for uint32_t type
 */ 
static void
start_uint32_t(void *data, const XML_Char *elem, const XML_Char **atts)
{
    uint32_t  value;
    char     *value_str = NULL;
    char     *endptr = NULL;
    
    value = 0;

    UNUSED(elem);
    
    EXTRACT_VALUE(atts, value_str);
    if (value_str != NULL) 
    {    
        value = strtoul(value_str, &endptr, 0);
        if (value == ULONG_MAX || endptr == NULL)
            value = 0;
    }    
    *(uint32_t *)data = value;
    return;
}

/**
 * End handler for uint32_t type
 */ 
static void
end_uint32_t(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Converion procedure for uint32_t type
 */ 
bool_t
xmlxdr_uint32_t(XDR *xdrs, uint32_t *ip)
{
    char         *buf;
    u_int        *buflen = &(xdrs->x_handy);
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    char         *format;
    int           printed;
    
    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            const char *name = data->name;
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;
   
            format = (name == NULL) ? "<param value=\"%lu\"/>\n" :
                                      "<param name=\"%s\" value=\"%d\"/>\n";
            
            if ((u_int)(printed = snprintf(buf, *buflen,
                                           format,
                                           (name == NULL) ? "" : name, *ip)) 
                >= *buflen)
            {
                printf("%s %d: small biffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;    
            }    
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;
            
            XML_Parser parser = data->parser;
            XML_SetUserData(parser, (void *)ip);
            XML_SetElementHandler(parser, start_uint32_t, end_uint32_t);

            buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
            end = strchr(buf, '>');
            end++;
            if (end == NULL)
            {
                printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                return FALSE;
            }        
            else
            {
                printed = end - buf;
                if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                {
                    printf("%s %d: %s at line %d\n",
                           __FUNCTION__, __LINE__,
                           XML_ErrorString(XML_GetErrorCode(parser)),
                           XML_GetCurrentLineNumber(parser));
                    return FALSE;
                }
                *buflen -= printed;
            }
            break;
        }
        case XDR_FREE:
        {
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Start handler for uint64_t type
 */ 
static void
start_uint64_t(void *data, const XML_Char *elem, const XML_Char **atts)
{
    uint64_t  value;
    char     *value_str = NULL;
    char     *endptr = NULL;
    
    value = 0;

    UNUSED(elem);
    
    EXTRACT_VALUE(atts, value_str);
    
    if (value_str != NULL) 
    {    
        value = strtoull(value_str, &endptr, 0);
        if (value == ULONG_MAX || endptr == NULL)
            value = 0;
    }    
    *(uint64_t *)data = value;
    return;
}

/**
 * End handler for uint64_t type
 */ 
static void
end_uint64_t(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Convertion procedure for uint64_t type
 */ 
bool_t
xmlxdr_uint64_t(XDR *xdrs, uint64_t *ip)
{
    char  *buf;
    u_int *buflen = &(xdrs->x_handy);
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    char  *format;
    u_int  printed;
    
    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            const char *name = data->name;
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
          
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;
            
            format = (name == NULL) ? "<param value=\"%llu\"/>\n" :
                                      "<param name=\"%s\" value=\"%d\"/>\n";
            if ((printed = snprintf(buf, *buflen,
                                    format,
                                    (name == NULL) ? "" : name, *ip)) >= *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;    
            }    
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;
            
            XML_Parser parser = data->parser;
            XML_SetUserData(parser, (void *)ip);
            XML_SetElementHandler(parser, start_uint64_t, end_uint64_t);

            buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
            end = strchr(buf, '>');
            end++;
            
            if (end == NULL)
            {
                printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                return FALSE;
            }        
            else
            {
                printed = end - buf;
                if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                {
                    printf("%s %d: %s at line %d\n",
                           __FUNCTION__, __LINE__,
                           XML_ErrorString(XML_GetErrorCode(parser)),
                           XML_GetCurrentLineNumber(parser));
                    return FALSE;
                }
                *buflen -= printed;
            }
            break;
        }
        case XDR_FREE:
        {
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Start handler for uint8_t array type
 */ 
static void
start_array(void *data, const XML_Char *elem, const XML_Char **atts)
{
    hex_buf   *value = (hex_buf *)data;
    XML_Char  *current;
    char      *value_str = NULL;
    int        i = 0;
    int        j = 0;
    char      *endptr = NULL; 

    UNUSED(elem);

    value->buf = NULL;
    value->size = 0;

    EXTRACT_VALUE(atts, value_str);

    if (value_str != NULL) 
    {
        i = strlen(value_str);
   
        if (i != 0)
            i = (i + 1)/3;
        
        value->buf = (uint8_t *)calloc(i, sizeof(uint8_t));
        if (value->buf == NULL)
            return;

        current = value_str;
        while (j < i)
        {
            value->buf[j] = (uint8_t)strtoul(current, &endptr, 16);
            j++;
            if (*endptr != ' ')
                break;
            current = endptr + 1;
        }
        if (j != i)
        {
            free(value->buf);
            return;
        }
        value->size = i;
    }
    return;
}

/**
 * End handler for uint8_t array type
 */
static void
end_array(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Convertion procedure for array type
 * TODO: code related to 'char' type must be removed after
 * removing 'char' from tarpc.m4 header
 */
bool_t
xmlxdr_array(XDR *xdrs, caddr_t *addrp, u_int *sizep,
             u_int maxsize, u_int elsize, xdrproc_t elproc)
{
    char  *buf;
    u_int *buflen = &(xdrs->x_handy);
    
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    
    const char   *name = data->name;
    const char   *type = data->type;
    char          format[MAXBUFSIZE + 1] = {0,};
    u_int         printed;
    bool_t        uint8_t_type;

    bool_t        char_type;
    
    u_int    i;

    UNUSED(maxsize);

    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            void       *elem = *addrp;
            
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);

            uint8_t_type = (strcmp(type, "uint8_t") == 0);
            char_type = (strcmp(type, "char") == 0);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;

            snprintf(format, MAXBUFSIZE - strlen(format), "<param");
            if (name != NULL)
                snprintf(format + strlen(format), MAXBUFSIZE - strlen(format), " name=\"%%s\"");
            if (uint8_t_type || char_type)
                snprintf(format + strlen(format), MAXBUFSIZE - strlen(format), " value=\"");
            else
                snprintf(format + strlen(format), MAXBUFSIZE - strlen(format), ">\n");
            if ((printed = snprintf(buf, *buflen,
                                    format,
                                    (name == NULL) ? "" : name)) >= *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }    
            buf += printed;
            *buflen -= printed;
            printed = 0;

            if (char_type)
            {
                
                if ((printed = snprintf(buf, *buflen, "%s\"/>\n", *addrp)) >= *buflen)
                {
                    printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }
                buf += printed;
                *buflen -= printed;
                break;
            }
            else
            for (i = 0; i < *sizep; i++)
            {
                if (uint8_t_type)
                {
                    if ((printed = snprintf(buf, *buflen, "%02x ", *(uint8_t *)elem)) >= *buflen)
                    {
                        printf("%d: printed %d, buflen %d\n", __LINE__, printed, *buflen);
                        return FALSE;
                    }
                    buf += printed;
                    *buflen -= printed; 
                }
                else
                {
                    ((xml_app_data *)(xdrs->x_private))->name = NULL;
                    ((xml_app_data *)(xdrs->x_private))->depth++;
                    if (!elproc(xdrs, (void *)elem, LASTUNSIGNED))
                        return FALSE;
                    ((xml_app_data *)(xdrs->x_private))->depth--;
                }
                elem += elsize;
            }
            /* Get rid off last space */
            if (uint8_t_type && printed > 0)
            {
                buf -= 1;
                *buflen -= 1;
            }
            
            if (uint8_t_type)
            {    
                if ((printed = snprintf(buf, *buflen,
                                        "\"/>\n")) >= *buflen)
                {
                    printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }
            }
            else
            {
                buf = xdrs->x_public + strlen(xdrs->x_public);
                memset(buf, ' ', indent);
                *buflen -= indent;
                buf += indent;

                if ((printed = snprintf(buf, *buflen,
                                        "</param>\n")) >= *buflen)
                {
                    printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }
            }
            buf += printed;
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;
            XML_Parser parser = data->parser;

            uint8_t_type = (strcmp(type, "uint8_t") == 0);
            char_type = (strcmp(type, "char") == 0);
            
            if (uint8_t_type)
            {
                hex_buf array;

                XML_SetUserData(parser, (void *)&array);
                XML_SetElementHandler(parser, start_array, end_array);

                buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
                end = strchr(buf, '>');
                end++;
            
                if (end == NULL)
                {
                    printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }        
                else
                {
                    printed = end - buf;
                    if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                    {
                        printf("%s %d: %s at line %d\n",
                               __FUNCTION__, __LINE__,
                               XML_ErrorString(XML_GetErrorCode(parser)),
                               XML_GetCurrentLineNumber(parser));
                        return FALSE;
                    }
                    *buflen -= printed;
                }
                *addrp = array.buf;
                *sizep = array.size;
            }
            else if (char_type)
            {
                char *end;

                XML_Parser parser = data->parser;
                XML_SetUserData(parser, (void *)addrp);
                XML_SetElementHandler(parser, start_string, end_string);

                buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
                end = strchr(buf, '>');
                end++;
            
                if (end == NULL)
                {
                    printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }        
                else
                {
                    printed = end - buf;
                    if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                    {
                        printf("%s %d: %s at line %d\n",
                               __FUNCTION__, __LINE__,
                               XML_ErrorString(XML_GetErrorCode(parser)),
                               XML_GetCurrentLineNumber(parser));
                        return FALSE;
                    }
                    *buflen -= printed;
                }
                break;
            }
            else
            {
                void       *array = NULL;
                uint32_t    array_size = 0;      
               
                XML_SetUserData(parser, (void *)data);
                XML_SetElementHandler(parser, start_compound_data, end_compound_data);
                buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
                end = strchr(buf, '>');
                end++;
                if (end == NULL)
                {
                    printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }        
                else
                {
                    printed = end - buf;
                    if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                    {
                        printf("%s %d: %s at line %d\n",
                               __FUNCTION__, __LINE__,
                               XML_ErrorString(XML_GetErrorCode(parser)),
                               XML_GetCurrentLineNumber(parser));
                        return FALSE;
                    }
                    *buflen -= printed;
                    buf += printed;
                }
                while (TRUE)
                {
                    buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
                    for (end = buf; *end == ' ' || *end == '\n'; end++);
                    
                    if (strncmp(end, "</param>", strlen("</param>")) == 0)
                        break;
                    array = realloc(array, (++array_size) * elsize);
                    if (array == NULL)
                    {
                        printf("%s %d: memory allocation failed", __FUNCTION__, __LINE__);
                        return FALSE;
                    }
                    if (!elproc(xdrs, (void *)(array + (array_size - 1) * elsize), LASTUNSIGNED))
                        return FALSE;
                    
                }
                XML_SetUserData(parser, (void *)data);
                XML_SetElementHandler(parser, start_compound_data, end_compound_data);
                buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
                end = strchr(buf, '>');
                end++;
                if (end == NULL)
                {
                    printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }        
                else
                {
                    printed = end - buf;
                    if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                    {
                        printf("%s %d: %s at line %d\n",
                               __FUNCTION__, __LINE__,
                               XML_ErrorString(XML_GetErrorCode(parser)),
                               XML_GetCurrentLineNumber(parser));
                        return FALSE;
                    }
                    *buflen -= printed;
                    buf += printed;
                }
                *addrp = array;
                *sizep = array_size;
            }
            break;
        }
        case XDR_FREE:
        {
            free(*addrp);
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Start handler for uint8_t vector type
 */ 
static void
start_vector(void *data, const XML_Char *elem, const XML_Char **atts)
{
    hex_buf   *value = (hex_buf *)data;
    XML_Char  *current;
    char      *value_str = NULL;
    u_int      i = 0;
    u_int      j = 0;
    char      *endptr = NULL; 

    UNUSED(elem);

    EXTRACT_VALUE(atts, value_str);

    memset(value->buf, 0, value->size);

    if (value_str != NULL) 
    {
        i = strlen(value_str);
   
        if (i != 0)
            i = (i + 1)/3;

        if (i != value->size)
            return;
        
        current = value_str;
        while (j < i)
        {
            value->buf[j] = strtoul(current, &endptr, 16);
            j++;
            if (*endptr != ' ')
                break;
            current = endptr + 1;
        }
        if (j != i)
        {
            return;
        }
    }
    return;
}

/**
 * End handler for uint8_t array type
 */
static void
end_vector(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Convertion procedure for vector type
 */
bool_t
xmlxdr_vector(XDR *xdrs, char *basep, u_int nelem,
              u_int elsize, xdrproc_t elproc)
{
    char  *buf;
    u_int *buflen = &(xdrs->x_handy);
    
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    
    const char   *name = data->name;
    const char   *type = data->type;
    char          format[MAXBUFSIZE + 1] = {0,};
    u_int         printed;
    bool_t        uint8_t_type;
   
    u_int         i;
    
    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            void       *elem = basep;
            
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);

            uint8_t_type = (strcmp(type, "uint8_t") == 0);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;

            snprintf(format, MAXBUFSIZE - strlen(format), "<param");
            if (name != NULL)
                snprintf(format + strlen(format), MAXBUFSIZE - strlen(format), " name=\"%%s\"");
            if (uint8_t_type)
                snprintf(format + strlen(format), MAXBUFSIZE - strlen(format), " value=\"");
            else
                snprintf(format + strlen(format), MAXBUFSIZE - strlen(format), ">\n");
            if ((printed = snprintf(buf, *buflen,
                                    format,
                                    (name == NULL) ? "" : name)) >= *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }    
            buf += printed;
            *buflen -= printed;
            printed = 0;
            
            for (i = 0; i < nelem; i++)
            {
                if (uint8_t_type)
                {
                    if ((printed = snprintf(buf, *buflen, "%02x ", *(uint8_t *)elem)) >= *buflen)
                    {
                        printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                        return FALSE;
                    }
                    buf += printed;
                    *buflen -= printed; 
                }
                else
                {
                    ((xml_app_data *)(xdrs->x_private))->name = NULL;
                    ((xml_app_data *)(xdrs->x_private))->depth++;
                    if (!elproc(xdrs, (void *)elem, LASTUNSIGNED))
                        return FALSE;
                    ((xml_app_data *)(xdrs->x_private))->depth--;
                }
                elem += elsize;
            }
            /* Get rid off last space */
            if (uint8_t_type && printed > 0)
            {
                buf -= 1;
                *buflen -= 1;
            }
            
            if (uint8_t_type)
            {    
                if ((printed = snprintf(buf, *buflen,
                                        "\"/>\n")) >= *buflen)
                {
                    printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }
            }
            else
            {
                buf = xdrs->x_public + strlen(xdrs->x_public);
                memset(buf, ' ', indent);
                *buflen -= indent;
                buf += indent;

                if ((printed = snprintf(buf, *buflen,
                                        "</param>\n")) >= *buflen)
                {
                    printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }
            }
            buf += printed;
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;
            XML_Parser parser = data->parser;

            uint8_t_type = (strcmp(type, "uint8_t") == 0);
            
            if (uint8_t_type)
            {
                hex_buf array;

                array.buf = basep;
                array.size = nelem;     

                XML_SetUserData(parser, (void *)&array);
                XML_SetElementHandler(parser, start_vector, end_vector);

                buf = xdrs->x_public + (strlen(xdrs->x_public) + 1  - *buflen);
                end = strchr(buf, '>');
                end++;
            
                if (end == NULL)
                {
                    printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }        
                else
                {
                    printed = end - buf;
                    if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                    {
                        printf("%s %d: %s at line %d\n",
                               __FUNCTION__, __LINE__,
                               XML_ErrorString(XML_GetErrorCode(parser)),
                               XML_GetCurrentLineNumber(parser));
                        return FALSE;
                    }
                    *buflen -= printed;
                }
            }
            else
            {
                void       *array = basep;
                uint32_t    array_size = 0;      
               
                XML_SetUserData(parser, (void *)data);
                XML_SetElementHandler(parser, start_compound_data, end_compound_data);
                buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
                end = strchr(buf, '>');
                end++;
                if (end == NULL)
                {
                    printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }        
                else
                {
                    printed = end - buf;
                    if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                    {
                        printf("%s %d: %s at line %d\n",
                               __FUNCTION__, __LINE__,
                               XML_ErrorString(XML_GetErrorCode(parser)),
                               XML_GetCurrentLineNumber(parser));
                        return FALSE;
                    }
                    *buflen -= printed;
                    buf += printed;
                }
                while (TRUE)
                {
                    buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
                    for (end = buf; *end == ' ' || *end == '\n'; end++);
                    
                    if (strncmp(end, "</param>", strlen("</param>")) == 0)
                        break;
                    if (++array_size > nelem)
                    {
                        printf("%s %d: parse error", __FUNCTION__, __LINE__);
                        return FALSE;
                    }
                    if (!elproc(xdrs, (void *)(array + (array_size - 1) * elsize), LASTUNSIGNED))
                        return FALSE;
                }
                if (array_size < nelem)
                {
                    printf("%s %d: parse error", __FUNCTION__, __LINE__);
                    return FALSE;
                }
               
                XML_SetUserData(parser, (void *)data);
                XML_SetElementHandler(parser, start_compound_data, end_compound_data);
                buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
                end = strchr(buf, '>');
                end++;
                if (end == NULL)
                {
                    printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                    return FALSE;
                }        
                else
                {
                    printed = end - buf;
                    if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                    {
                        printf("%s %d: %s at line %d\n",
                               __FUNCTION__, __LINE__,
                               XML_ErrorString(XML_GetErrorCode(parser)),
                               XML_GetCurrentLineNumber(parser));
                        return FALSE;
                    }
                    *buflen -= printed;
                    buf += printed;
                }
            }
            break;
        }
        case XDR_FREE:
        {
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}


/**
 * Start handler for string type
 */
static void
start_string(void *data, const XML_Char *elem, const XML_Char **atts)
{
    char      *value_str = NULL;
    caddr_t    value;

    UNUSED(elem);
    
    EXTRACT_VALUE(atts, value_str);
    if (value_str != NULL)
    {
        value = (caddr_t)calloc(1, strlen(value_str));
        if (value == NULL)
            return;
        strncpy(value, value_str, strlen(value_str));
        *(caddr_t *)data = value;
    }
    return;
}

/**
 * End handler for string type
 */ 
static void
end_string(void *data, const XML_Char *elem)
{
    UNUSED(data);
    UNUSED(elem);
    return;
}

/**
 * Convertion procedure for string type
 */ 
bool_t
xmlxdr_string(XDR *xdrs, caddr_t *addrp, u_int maxsize)
{
    char  *buf;
    u_int *buflen = &(xdrs->x_handy);
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);
    char  *format;
    u_int  printed;

    UNUSED(maxsize);

    switch (xdrs->x_op)
    {
        case XDR_ENCODE:
        {
            const char *name = data->name;
            u_int indent = (data->depth) * INDENT;

            buf = xdrs->x_public + strlen(xdrs->x_public);
            
            if (indent > *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            
            memset(buf, ' ', indent);
            *buflen -= indent;
            buf += indent;
            
            format = (name == NULL) ? "<param value=\"%d\"/>\n" :
                                      "<param name=\"%s\" value=\"%s\"/>\n";
            if ((printed = snprintf(buf, *buflen,
                                    format,
                                    (name == NULL) ? "" : name, *addrp)) >= *buflen)
            {
                printf("%s %d: small buffer, exit\n", __FUNCTION__, __LINE__);
                return FALSE;
            }    
            buf += printed;
            *buflen -= printed;
            break;
        }
        case XDR_DECODE:
        {
            char *end;

            XML_Parser parser = data->parser;
            XML_SetUserData(parser, (void *)addrp);
            XML_SetElementHandler(parser, start_string, end_string);

            buf = xdrs->x_public + (strlen(xdrs->x_public)  + 1 - *buflen);
            end = strchr(buf, '>');
            end++;
            
            if (end == NULL)
            {
                printf("%s: %d: Cannot find \">\"\n", __FUNCTION__, __LINE__);
                return FALSE;
            }        
            else
            {
                printed = end - buf;
                if (XML_Parse(parser, buf, printed, 0) == XML_STATUS_ERROR)
                {
                    printf("%s %d: %s at line %d\n",
                           __FUNCTION__, __LINE__,
                           XML_ErrorString(XML_GetErrorCode(parser)),
                           XML_GetCurrentLineNumber(parser));
                    return FALSE;
                }
                *buflen -= printed;
            }
            break;
        }
        case XDR_FREE:
        {
            free(*addrp);
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Dummy function for 'char' type. 
 * TODO: must be removed after removing 'char' from
 * tarpc.m4 header
 */ 
bool_t
xmlxdr_char(XDR *xdrs, char *ip)
{
    UNUSED(xdrs);
    UNUSED(ip);
    return FALSE;
}

/**
 * Initializes XDR structure to perform
 * further convertions.
 *
 * @param xdrs      XDR structure
 * @param buf       buffer to contain xml document representing
 *                  transmitting data
 * @param buflen    buffer size
 * @param op        rpc_xml_call or rpc_xml_result
 * @param rc        TRUE of FALSE, used only when op is rpc_xml_result
 * @param name      call/result name
 * @param x_op      XDR_ENCODE, XDR_DECODE of XDR_FREE
 *
 * @return TRUE of FALSE (in case of memory allocation failure)
 */ 
bool_t
xdrxml_create(XDR *xdrs, caddr_t buf, u_int buflen, 
              rpc_xml_op op, bool_t rc,
              const char *name, enum xdr_op x_op)
{
    xml_app_data *data;

    data = (xml_app_data *)calloc(1, sizeof(xml_app_data));
    if (data == NULL)
    {
        printf("%s %d: memory allocation failed", __FUNCTION__, __LINE__);
        return FALSE;
    }

    data->op = op;
    data->rc = rc;
    data->type = "";
    data->name = name;
    data->depth = 0;
    data->parser = XML_ParserCreate(NULL);

    if (x_op == XDR_ENCODE)
        memset(buf, 0, buflen);

    xdrs->x_public = buf;
    xdrs->x_handy = buflen;
    xdrs->x_private = (caddr_t)data;

    xdrs->x_op = x_op;
    return TRUE;
}

/**
 * Free memory allocated during XDR structure initializing in
 * xdrxml_create()
 *
 * @param xdrs  XDR structure
 */ 
bool_t
xdrxml_free(XDR *xdrs)
{
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);

    XML_ParserFree(data->parser);
    free(data);
    return TRUE;
}

/**
 * Return rc containing in xml document
 * attached to XDR structure
 *
 * @param xdrs  XDR structure
 */ 
bool_t
xdrxml_return_code(XDR *xdrs)
{
    xml_app_data *data = (xml_app_data *)(xdrs->x_private);

    return data->rc;
}

#endif
