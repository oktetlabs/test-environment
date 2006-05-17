/** @file
 * @brief Win32-specific stuff
 *
 * iSCSI Initiator configuration
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 */

#ifdef __CYGWIN__

#define TE_LGR_USER      "Configure iSCSI"

#include "te_config.h"
#include "package.h"

#include <windows.h>
/* In file "wingdi.h" ERROR is defined to 0 */
#undef ERROR

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logfork.h"

#include <sys/types.h>
#include <regex.h>

#include "iscsi_initiator.h"

static void
win32_report_error(const char *function, int line)
{
    static char buffer[256];
    unsigned long win_error = GetLastError();
    
    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 
                      win_error, 0, buffer,
                      sizeof(buffer) - 1, NULL) == 0)
    {
        unsigned long fmt_error = GetLastError();
        ERROR("%s():%d: Win32 reported an error %x", function, line, 
              win_error);
        ERROR("Unable to format message string: %x", fmt_error);
    }
    else
    {
        ERROR("%s():%d: Win32 error: %s (%x)", function, line,
              buffer, win_error);
    }
}

#define WIN32_REPORT_ERROR() win32_report_error(__FUNCTION__, __LINE__)

static int cli_started;
static HANDLE host_input, cli_output;
static HANDLE host_output, cli_input;
static PROCESS_INFORMATION process_info;

static te_errno
iscsi_win32_run_cli(const char *cmdline)
{
    STARTUPINFO startup;
    SECURITY_ATTRIBUTES attr;

    if (cli_started)
    {
        iscsi_win32_finish_cli();
    }

    startup.cb = sizeof(startup);
    startup.lpReserved = NULL;
    startup.lpDesktop  = NULL;
    startup.lpTitle    = NULL;
    startup.dwFlags    = STARTF_USESTDHANDLES;
    startup.cbReserved2 = 0;
    startup.lpReserved2 = NULL;

    attr.nLength = sizeof(attr);
    attr.lpSecurityDescriptor = NULL;
    attr.bInheritHandle = TRUE;

    if (!CreatePipe(&host_input, &cli_output, &attr, 0))
    {
        WIN32_REPORT_ERROR();
        return TE_RC(TE_TA_WIN32, TE_EFAIL);
    }
    SetHandleInformation(host_input, HANDLE_FLAG_INHERIT, 0);

    if (!CreatePipe(&cli_input, &host_output, &attr, 0))
    {
        WIN32_REPORT_ERROR();
        CloseHandle(host_input);
        CloseHandle(cli_output);
        return TE_RC(TE_TA_WIN32, TE_EFAIL);
    }
    SetHandleInformation(host_output, HANDLE_FLAG_INHERIT, 0);

    startup.hStdInput  = GetStdHandle(STD_INPUT_HANDLE); // cli_input;
    startup.hStdOutput = cli_output;
    startup.hStdError  = cli_output;

    RING("Running iSCSI CLI as '%s'", cmdline);
    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE,
                       CREATE_NO_WINDOW, NULL, NULL, 
                       &startup, &process_info))
    {
        WIN32_REPORT_ERROR();
        CloseHandle(host_input);
        CloseHandle(cli_output);
        CloseHandle(host_output);
        CloseHandle(cli_input);
        return TE_RC(TE_TA_WIN32, TE_EFAIL);
    }
    cli_started = TRUE;
    return 0;
}

te_errno
iscsi_send_to_win32_iscsicli(const char *fmt, ...)
{
    static char buffer[2048];
    va_list args;
    int rc;
    int len;

    va_start(args, fmt);
    len = strlen("iscsicli.exe ");
    memcpy(buffer, "iscsicli.exe ", len);
    vsnprintf(buffer + len, sizeof(buffer) - 1 - len, fmt, args);
    va_end(args);

    return iscsi_win32_run_cli(buffer);
}


te_errno
iscsi_win32_wait_for(regex_t *pattern, 
                     regex_t *abort_pattern,
                     regex_t *terminal_pattern,
                     int first_part, int nparts, int maxsize, char **buffers)
{
    static char buffer[2048];
    static char *new_line = NULL;
    static unsigned long residual = 0;
    
    char *free_ptr = buffer;
    unsigned long read_size = sizeof(buffer) - 1;
    unsigned long read_bytes;
    int re_code;
    int i;
    int j;
    
    regmatch_t matches[10];

    for (;;)
    {
        if (residual != 0)
        {
            memmove(buffer, new_line, residual);
        }
        free_ptr = buffer;
        read_size = sizeof(buffer) - 1;
        read_bytes = residual;

        for (;;)
        {
            new_line = memchr(free_ptr, '\n', read_bytes);
            if (new_line != NULL)
            {
                *new_line++ = '\0';
                residual = read_bytes - (new_line - free_ptr);
                break;
            }
            free_ptr += read_bytes;
            read_size -= read_bytes;
            if (read_size == 0)
            {
                ERROR("%s(): The input line is too long", __FUNCTION__);
                return TE_RC(TE_TA_WIN32, TE_ENOSPC);
            }
            RING("Waiting for %d bytes from iSCSI CLI", read_size);
            if (!ReadFile(host_input, free_ptr, read_size, &read_bytes, NULL))
            {
                WIN32_REPORT_ERROR();
                return TE_RC(TE_TA_WIN32, TE_EIO);
            }
            free_ptr[read_bytes] = '\0';
        }
        INFO("Probing line '%s'", buffer);
        re_code = regexec(pattern, buffer, 10, matches, 0);
        if (re_code == 0)
            break;
        if (re_code != REG_NOMATCH)
        {
            char err_buf[64] = "";
            regerror(re_code, pattern, err_buf, sizeof(err_buf) - 1);
            ERROR("Matching error: %s", err_buf);
            return TE_RC(TE_TA_WIN32, TE_EFAIL);
        }
        if (terminal_pattern != NULL)
        {
            re_code = regexec(terminal_pattern, buffer, 0, NULL, 0);
            if (re_code == 0)
                return TE_RC(TE_TA_WIN32, TE_ENODATA);
        }
        if (abort_pattern != NULL)
        {
            re_code = regexec(abort_pattern, buffer, 0, NULL, 0);
            if (re_code == 0)
            {
                ERROR("iSCSI CLI reported an error: '%s'", buffer);
                return TE_RC(TE_TA_WIN32, TE_ESHCMD);
            }
        }
    }
    for (i = first_part, j = 0; i < first_part + nparts; i++, j++)
    {
        int size;
        if (matches[i].rm_so == -1)
            buffers[j][0] = '\0';
        else
        {
            size = matches[i].rm_eo - matches[i].rm_so;
            if (size > maxsize)
                size = maxsize;
            memcpy(buffers[j], buffer + matches[i].rm_so, size);
            buffers[j][size] = '\0';
        }
    }
    return 0;
}

te_errno
iscsi_win32_finish_cli(void)
{
    te_bool success;
    
    CloseHandle(host_output);
    CloseHandle(host_input);
    success = (WaitForSingleObject(process_info.hProcess, 100) != WAIT_OBJECT_0);
    if (!success)
    {
        WARN("Killing iSCSI CLI process");
        TerminateProcess(process_info.hProcess, (unsigned long)-1);
    }
    
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    cli_started = FALSE;
    
    return success ? 0 : TE_RC(TE_TA_WIN32, TE_EFAIL);
}

#endif
