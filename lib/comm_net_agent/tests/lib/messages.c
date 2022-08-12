/** @file
 * @brief Test Environment
 * Network Communication Library Tests - Test Agent side - Library
 * Operations with Messages
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#include <stdio.h>
#include <unistd.h>
#ifdef STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif

#include "messages.h"
#include "debug.h"

/**
 * Compares binary contents of two buffers.
 *
 * @param buffer1   first buffer
 * @param len1      length of the first buffer
 * @param buffer2   second buffer
 * @param len2      length of the second buffer
 *
 * @retval 0        buffers are equal
 * @retval 1        buffers are not equal
 */
int
compare_buffers(char *buffer1, int len1, char *buffer2, int len2)
{
    return ((len1 != len2) ? 1 :
            (memcmp(buffer1, buffer2, len1) != 0 ? 1 : 0));
}

/**
 * Generates a command/answer of size 'cmd_size' plus 'attachment_size'.
 * Note that if attachment_size is not zero, cmd_size cannot be less
 * than 20.
 *
 * @param buffer          Buffer to be filled with the command
 * @param cmd_size        Size of the command part of the message including
 *                        the trailing separator
 * @param attachment_size Size of the attachment to be generated
 *
 * @return              n/a
 *
 */
void
generate_command(char *buffer, int cmd_size, int attachment_size)
{
    char         buf[BUFSIZ];
    int          att_str_len, len;
    static const char  letters[] = "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "0123456789_/ ";
    char        *end_ptr, *p;

    if (attachment_size != 0 && cmd_size < 20)
    {
       fprintf(stderr, "generate_command: cmd_size %d too small\n", cmd_size);
       exit(2);
    }

    /* compute the size of the ' attach <size>' string */
    if (attachment_size != 0)
    {
       sprintf(buf, " attach %d", attachment_size);
       att_str_len = strlen(buf);
    }
    else
    {
       att_str_len = 0;
    }

    /* randomly fill the command */
    len = cmd_size - att_str_len - 1; /* don't forget the separator */
    end_ptr = buffer + len;
    for (p = buffer; p < end_ptr; p++)
       *p = letters[(int)(random() % (sizeof(letters) - 1))];

    /* now put the 'attach' string */
    if (attachment_size != 0)
       strcpy(end_ptr, buf);
    end_ptr += att_str_len;

    /* XXXX: here the actual separator must be set */
    *end_ptr++ = '\0';

    /* now generate the binary attachment */
    if (attachment_size != 0)
    {
       p = end_ptr;
       end_ptr += attachment_size;

       while (p < end_ptr)
       {
           *p = (char)(((unsigned long int)(random())) % 0x100);
#if 0
           *p = 'A';
#endif
           p++;
       }
    }
}
