/** @file
 * @brief RGT core: I/O functions
 *
 * Implementation of auxiliary functions for I/O operations.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "io.h"
#include "rgt_common.h"
#include "log_msg.h"

/* See the description in io.h */
size_t
universal_read(FILE *fd, void *buf, size_t count, rgt_io_mode_t io_mode)
{
    size_t r_count;

    do {
        r_count = fread(buf, 1, count, fd);

        if ((r_count == count) || (io_mode == RGT_IO_MODE_NBLK))
        {
            /*
             * All required number of bytes have been read, or
             * the number of bytes available for read is less than required
             * and the function was called with RGT_IO_MODE_NBLK mode.
             */

            /* Return number of butes successfully read */
            return r_count;
        }

        count -= r_count;

        /* Wait for a while may be some data comes */
        sleep(1);
    } while (1);

    /* This code is never reached: just for fixing Compiler Warning */
    return 0;
}

/* See description in io.h */
void
write_xml_string(struct obstack *obstk, const char *str, te_bool attr_val)
{
    te_bool br_cntrl_start = TRUE;
    int     i = 0;

    while (str[i] != '\0')
    {
        switch (str[i])
        {
            case '\r':
                /* FALLTHROUGH */
            case '\n':
                /*
                 * Skip \r after \n (or \n after \r) because it does not
                 * bring any formating, but just follows after \r (\n)
                 * on some systems.
                 *
                 * So that if we meet \r\n or \r\n we should
                 * output single <br/>
                 *
                 * In case of \r\r\r or \n\n\n -> <br/><br/><br/>
                 * In case of \r\n\r or \n\r\r -> <br/><br/>
                 * In case of \n\r\r\n or \r\n\n\r -> <br/><br/>
                 *
                 * If br_cntrl_start is TRUE, we output </BR>
                 * just after we meet '\n' ('\r') checking that there
                 * was no '\r' ('\n') as the previous character.
                 */
                if (i > 0 && str[i - 1] != '\n' && str[i - 1] != '\r')
                    br_cntrl_start = TRUE;

/* Returns '\r' ('\n') when ch_ is '\n' ('\r') */
#define BR_CTRL_INVERT(ch_) (((ch_) == '\n') ? '\r' : '\n')

                if (br_cntrl_start && i > 0 &&
                    str[i - 1] == BR_CTRL_INVERT(str[i]))
                {
                    /* Skip this character */
                    br_cntrl_start = FALSE;
                    break;
                }
#undef BR_CTRL_INVERT

                br_cntrl_start = TRUE;

                if (attr_val)
                {
                    /*
                     * We want to pass new-lines through XML log report,
                     * so we need to emphasise this explicitly, otherwise
                     * XML parser will interpret '\n' as a "space" character
                     * and change it to universal ' ' (space) character.
                     */
                    if (obstk != NULL)
                        obstack_grow(obstk, "&#10;", 5);
                    else
                        fputs("&#10;", rgt_ctx.out_fd);
                }
                else
                {
                    if (obstk != NULL)
                        obstack_grow(obstk, "<br/>", 5);
                    else
                        fputs("<br/>", rgt_ctx.out_fd);
                }
                break;

            case '<':
                if (obstk != NULL)
                    obstack_grow(obstk, "&lt;", 4);
                else
                    fputs("&lt;", rgt_ctx.out_fd);
                break;

            case '>':
                if (obstk != NULL)
                    obstack_grow(obstk, "&gt;", 4);
                else
                    fputs("&gt;", rgt_ctx.out_fd);
                break;

            case '&':
                if (obstk != NULL)
                    obstack_grow(obstk, "&amp;", 5);
                else
                    fputs("&amp;", rgt_ctx.out_fd);
                break;

            case '\'':
            case '\"':
                if (attr_val)
                {
                    const char *val = str[i] == '\'' ? "&apos;" : "&quot;";
                    /*
                     * Quote character only when we output
                     * attribute value.
                     */
                    if (obstk != NULL)
                        obstack_grow(obstk, val, strlen(val));
                    else
                        fputs(val, rgt_ctx.out_fd);
                    break;
                }
                /* FALLTHROUGH */
                /*
                 * For non attribute output - process as ordinary
                 * prinable character
                 */

            default:
                if (str[i] == '\t' || isprint(str[i]))
                {
                    if (obstk != NULL)
                        obstack_1grow(obstk, str[i]);
                    else
                        fputc(str[i], rgt_ctx.out_fd);
                }
                else
                {
                    if (obstk != NULL)
                        obstack_printf(obstk, "&lt;0x%02x&gt;",
                                       (unsigned char)str[i]);
                    else
                        fprintf(rgt_ctx.out_fd, "&lt;0x%02x&gt;",
                                (unsigned char)str[i]);
                }
                break;
        }
        i++;
    }
}
