/** @file
 * @brief Logger subsystem
 * 
 * TEN side Logger library. 
 *
 *  
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "logger_api.h"
#include "logger_prc_internal.h"

/*   Raw log file format. Max length of the raw log message 
 *   is a sum of:
 *      NFL(Entity_name) - 1 byte
 *      Entity_name - 255 bytes max
 *      Log_ver - 1 byte
 *      Timestamp - 8 byte
 *      Level - 2 bytes
 *      Log_message_length - 2 bytes
 *      NFL(User_name) - 1 byte
 *      User_name - 255 bytes max
 *      NFL(of form_str) - 1 byte
 *      Log_data_format_str - 255 bytes max
 *      NFL(arg1) - 1 byte
 *      arg1 - 255 bytes max
 *         ...
 *      NFL(arg12) - 1 byte
 *      arg12 - 255 bytes max
 *
 *      TOTAL = 3853 bytes
 */


/**
 * This macro fills in NFL(Next_Field_Length) field and corresponding 
 * string field as following:
 *     NFL (1 byte) - actual next string length without trailing null;
 *     Argument field (1 .. 255 bytes) - logged string.
 *
 * @param _log_str  - logged string
 * @param _len      - (OUT) logged string length
 */
#define LGR_FILL_STR(_log_str, _len) \
    do {                                        \
        (_len) = strlen((_log_str));            \
        if ((_len) > LGR_FIELD_MAX)             \
            (_len) = LGR_FIELD_MAX;             \
        *mess_buf = (_len);                     \
        ++mess_buf;                             \
        memcpy(mess_buf, (_log_str), (_len));   \
        mess_buf += (_len);                     \
    } while (0)
    

/** Path to log file */
extern char *te_log_path;


/* This function definition is placed in logger process location */
void lgr_register_message(char *buf_mess, uint32_t buf_len);


/** 
 * Create message and register it in the raw log file.
 *
 * @param level         log level of the message
 * @param entity_name   Entity name whose user generates this message;
 * @param user_name     Arbitrary "user name";
 * @param form_str      Raw log format string. This string should contain
 *                      conversion specifiers if some arguments following;
 * @param ...           Arguments passed into the function according to 
 *                      raw log format string description.
 *
 * @bugs Should be processed test result value for %te specifier?
 */
void
log_message(uint16_t level, const char *entity_name, 
            const char *user_name, const char *form_str, ...)
{
    va_list     ap;
    struct      timeval tv;
    uint32_t    tvsec;
    uint32_t    tvusec;    
    const char *p_fs;
    int         narg = 0;
    int8_t      raw_mess[LGR_MAX_BUF];
    int8_t     *mess_buf = raw_mess;
    uint16_t    tmp_length = 0;    
    uint16_t   *mess_length_field;
    uint16_t    mess_length;


    memset(raw_mess, 0, LGR_MAX_BUF);
    
    /* Fill in Entity_name field and corresponding Next_filed_length one */
    LGR_FILL_STR(entity_name, tmp_length);

    /* Fill in Log_ver and timestamp fields */
    *mess_buf = LGR_LOG_VERSION;
    ++mess_buf;
    
    gettimeofday(&tv, NULL);
    LGR_32_TO_NET(tv.tv_sec, &tvsec);
    LGR_32_TO_NET(tv.tv_usec, &tvusec);
    *((uint32_t *)mess_buf) = tvsec;
    mess_buf += sizeof(int);
    *((uint32_t *)mess_buf) = tvusec;    
    mess_buf += sizeof(int);

    /* Fill in Log level to be passed to raw log */
    *((uint16_t *)mess_buf) = htons(level);
    mess_buf += sizeof(uint16_t);
    
    /* Fix Log_message_length field and initiate one */
    mess_length_field = (uint16_t *)mess_buf;
    *mess_length_field = 0;
    ++mess_buf; ++mess_buf;
    
    /* Fill in User_name field and corresponding Next_field_length one */
    LGR_FILL_STR(user_name, tmp_length);
    *mess_length_field = *mess_length_field + LGR_NFL_FLD + tmp_length;

    /* 
     * Fill in Log_data_format_string field and corresponding 
     * Next_filed_length one 
     */    
    LGR_FILL_STR(form_str, tmp_length);
    *mess_length_field = *mess_length_field + LGR_NFL_FLD + tmp_length;
    
    va_start(ap, form_str);
    for (p_fs = form_str; *p_fs; p_fs++)
    {
        if (*p_fs != '%')
            continue;
        
        if ((++narg) > LGR_MAX_ARGS)
        {
            log_message(0x02, "LOGGER", "WARNING", 
                        "Too many arguments: "
                        "Max Number: %d, Processed: %d\n",
                        LGR_MAX_ARGS, narg);
            return;
        }
        p_fs++;
        switch (*p_fs)
        {
            case '%':
                break;
                
            case 'd':
            case 'i':
            case 'o':
            case 'x':
            case 'X':
            case 'u':
            case 'p':
                {
                    int32_t val;
                    *mess_buf = (unsigned char)sizeof(int);
                    ++mess_buf;
                    val = va_arg(ap, int);
                    LGR_32_TO_NET(val, mess_buf);
                    mess_buf += sizeof(int);
                    *mess_length_field = *mess_length_field + 
                                 LGR_NFL_FLD + sizeof(int);
                }                     
                break;
            
            case 'c':
                *mess_buf = (unsigned char)sizeof(char);
                ++mess_buf;
                *mess_buf = (int8_t)va_arg(ap, int);
                ++mess_buf;
                *mess_length_field = *mess_length_field + 
                                     LGR_NFL_FLD + sizeof(char);
                break;       
               
            case 's':
                {
                    char *tmp = va_arg(ap, char*);
                    LGR_FILL_STR(tmp, tmp_length);
                    *mess_length_field = *mess_length_field + 
                                         LGR_NFL_FLD + tmp_length;
                    break;
                }
                
            case 't': /* %tm, %tb, %te, %tf */
                if (*++p_fs != 0 )
                {
                    if (*p_fs == 'm')
                    {  
                        /* 
                         * Args order should be:
                         *  - start address of dumped memory;  
                         *  - size of memory piece;
                         */
                        char *dump = (char *)va_arg(ap, char*);
                        tmp_length = (uint8_t)va_arg(ap, int);
                        if (tmp_length > LGR_FIELD_MAX)
                            tmp_length = LGR_FIELD_MAX;                       
                        *mess_buf = tmp_length;
                        ++mess_buf;

                        memcpy(mess_buf, dump, tmp_length);
                        *mess_length_field = *mess_length_field + 
                                             LGR_NFL_FLD + tmp_length;
                        break;
                    }
                    else if (*p_fs == 'b' || *p_fs == 'e') 
                    {    /* ??? test result for 'e' case */
                        char *tmp = va_arg(ap, char *);
                        LGR_FILL_STR(tmp, tmp_length);
                        *mess_length_field = *mess_length_field + 
                                             LGR_NFL_FLD + tmp_length;
                        break;
                    }
                    else if (*p_fs == 'f')
                    {
                        char new_path[LGR_FIELD_MAX] = "";
                        char templ[LGR_FILE_MAX] = "XXXXXX";
                        char *tmp = va_arg(ap, char *);
                        int  fd;

                        fd =  mkstemp(templ);
                        if (fd < 0)
                        {
                            log_message(0x02, "LOGGER", "WARNING", 
                                        "mkstemp() failure.\n");
                            return;         
                        }
                       
                        if (close(fd) != 0)
                        {
                            log_message(0x02, "LOGGER", "WARNING", 
                                        "close() failure.\n");
                            return;         
                        }

                        strcat(new_path, te_log_path);
                        strcat(new_path, templ);
                        if (rename(tmp, new_path) != 0)
                        {
                            log_message(0x02, "LOGGER", "WARNING", 
                                        "File %s copying error\n", tmp);
                            return;         
                        }
                        
                        LGR_FILL_STR(templ, tmp_length);                  
                        *mess_length_field = *mess_length_field + 
                                             LGR_NFL_FLD + tmp_length;
                        break;
                    }
                }
            
                log_message(0x02, "LOGGER", "WARNING", 
                            "Illegal specifier - t%c\n", *p_fs);
                return;
        
            default:
                log_message(0x02, "LOGGER", "WARNING", 
                            "Illegal specifier - %c\n", *p_fs);
                return;
        }
    }    
    va_end(ap);
    
    mess_length = *mess_length_field;
    *mess_length_field = htons(mess_length);   
    /* message length is a mess_length_field value plus uncounted fields */
    lgr_register_message(raw_mess, mess_length + raw_mess[0] + 14);
}
