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
#include <winioctl.h>
#include <setupapi.h>
/* In file "wingdi.h" ERROR is defined to 0 */
#undef ERROR

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logfork.h"

#include <sys/types.h>
#include <pthread.h>
#include <regex.h>

#include "iscsi_initiator.h"

#define DEFAULT_INITIAL_R2T_WIN32    1
#define DEFAULT_IMMEDIATE_DATA_WIN32 1

void
iscsi_win32_report_error(const char *function, int line, 
                         unsigned long previous_error)
{
    static char buffer[256];
    unsigned long win_error = 
        (previous_error != 0 ? previous_error : GetLastError());
    
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

static int                 cli_started;
static HANDLE              host_input, cli_output;
static HANDLE              host_output, cli_input;
static PROCESS_INFORMATION process_info;
static HANDLE              cli_timeout_timer = INVALID_HANDLE_VALUE;
static HKEY                driver_parameters = INVALID_HANDLE_VALUE;
static HDEVINFO            scsi_adapters     = INVALID_HANDLE_VALUE;
static SP_DEVINFO_DATA     iscsi_dev_info;

static te_errno iscsi_win32_set_default_parameters(void);

static te_errno
iscsi_win32_find_initiator_registry(void)
{
    GUID            scsi_class_guid;
    unsigned        index;
    long            result;

    static char   buffer[1024];
    unsigned long buf_size;
    unsigned long value_type;

    if (driver_parameters != INVALID_HANDLE_VALUE)
        return 0;

    if (!SetupDiClassGuidsFromName("SCSIAdapter", &scsi_class_guid, 1, &buf_size))
    {
        ISCSI_WIN32_REPORT_ERROR();
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }

    scsi_adapters = SetupDiGetClassDevs(&scsi_class_guid, NULL, NULL, 0);
    if (scsi_adapters == INVALID_HANDLE_VALUE)
    {
        ISCSI_WIN32_REPORT_ERROR();
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    for (index = 0;; index++)
    {
        iscsi_dev_info.cbSize = sizeof(iscsi_dev_info);
        if (!SetupDiEnumDeviceInfo(scsi_adapters, index, &iscsi_dev_info))
        {
            ISCSI_WIN32_REPORT_ERROR();
            return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }
        buf_size = sizeof(buffer);
        memset(buffer, 0, buf_size);
        if (!SetupDiGetDeviceRegistryProperty(scsi_adapters, &iscsi_dev_info,
                                              SPDRP_HARDWAREID, &value_type,
                                              buffer, buf_size, &buf_size))
        {
            ISCSI_WIN32_REPORT_ERROR();
            return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }
        if (value_type != REG_MULTI_SZ)
        {
            ERROR("Registry seems to be corrupted, very bad");
            return TE_RC(ISCSI_AGENT_TYPE, TE_ECORRUPTED);
        }
        if (strcasecmp(buffer, "Root\\iSCSIPrt") == 0)
            break;
    }

    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "SYSTEM\\CurrentControlSet\\Control\\Class\\");
    buf_size = sizeof(buffer) - strlen(buffer);
    if (!SetupDiGetDeviceRegistryProperty(scsi_adapters, &iscsi_dev_info,
                                          SPDRP_DRIVER, &value_type,
                                          buffer + strlen(buffer), 
                                          buf_size, &buf_size))
    {
        ISCSI_WIN32_REPORT_ERROR();
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    if (value_type != REG_SZ)
    {
            ERROR("Registry seems to be corrupted, very bad");
            return TE_RC(ISCSI_AGENT_TYPE, TE_ECORRUPTED);
    }
    strcat(buffer, "\\Parameters");
    if ((result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer,
                               0, KEY_ALL_ACCESS, &driver_parameters)) != 0)
    {
        ISCSI_WIN32_REPORT_RESULT(result);
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    return iscsi_win32_set_default_parameters();
}

static te_errno
iscsi_win32_run_cli(const char *cmdline)
{
    STARTUPINFO startup;
    SECURITY_ATTRIBUTES attr;

    if (cli_started)
    {
        iscsi_win32_finish_cli();
    } 

    if (cli_timeout_timer == INVALID_HANDLE_VALUE)
    {
        cli_timeout_timer = CreateWaitableTimer(NULL, TRUE, NULL);
        if (cli_timeout_timer == NULL)
        {
            ISCSI_WIN32_REPORT_ERROR();
            return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }
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
        ISCSI_WIN32_REPORT_ERROR();
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    SetHandleInformation(host_input, HANDLE_FLAG_INHERIT, 0);

    if (!CreatePipe(&cli_input, &host_output, &attr, 0))
    {
        ISCSI_WIN32_REPORT_ERROR();
        CloseHandle(host_input);
        CloseHandle(cli_output);
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
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
        ISCSI_WIN32_REPORT_ERROR();
        CloseHandle(host_input);
        CloseHandle(cli_output);
        CloseHandle(host_output);
        CloseHandle(cli_input);
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    cli_started = TRUE;
    return 0;
}

typedef struct iscsi_win32_registry_parameter
{
    int offer;
    int offset;
    char *name;
    unsigned long (*transform)(void *);
    unsigned long constant;
} iscsi_win32_registry_parameter;

static unsigned long
iscsi_win32_bool2int(void *data)
{
    return strcasecmp((char *)data, "Yes") == 0;
}

static te_errno
iscsi_win32_set_registry_parameter(iscsi_win32_registry_parameter *parm, 
                                   void *data)
{
    unsigned long value;
    long          result;

    if (parm->offset < 0)
        value = parm->constant;
    else
    {
        data = (char *)data + parm->offset;
        if (parm->transform != NULL)
            value = parm->transform(data);
        else
            value = *(int *)data;
    }
    
    RING("Setting %s to %u via registry", parm->name, (unsigned)value);
    if ((result = RegSetValueEx(driver_parameters, 
                                parm->name, 0, REG_DWORD, 
                                (void *)&value, sizeof(value))) != 0)
    {
        ISCSI_WIN32_REPORT_RESULT(result);
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    return 0;
}

static te_errno
iscsi_win32_set_default_parameters(void)
{
#define RPARAMETER(name, value) {0, -1, name, NULL, value}
    static iscsi_win32_registry_parameter rparams[] =
        {
            RPARAMETER("InitialR2T", DEFAULT_INITIAL_R2T_WIN32),
            RPARAMETER("ImmediateData", DEFAULT_IMMEDIATE_DATA_WIN32),
            RPARAMETER("MaxRecvDataSegmentLength",
                       DEFAULT_MAX_RECV_DATA_SEGMENT_LENGTH),
            RPARAMETER("FirstBurstLength", DEFAULT_FIRST_BURST_LENGTH),
            RPARAMETER("MaxBurstLength", DEFAULT_MAX_BURST_LENGTH),
            RPARAMETER("ErrorRecoveryLevel", 
                       DEFAULT_ERROR_RECOVERY_LEVEL),
            {0, 0, NULL, NULL, 0}
        };
    iscsi_win32_registry_parameter *rp;

    for (rp = rparams; rp->name != NULL; rp++)
    {
        if (iscsi_win32_set_registry_parameter(rp, NULL) != 0)
        {
            ERROR("Cannot set default for '%s'", rp->name);
            return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }
    }
    return 0;
#undef RPARAMETER
}

static te_errno
iscsi_win32_restart_iscsi_service(void)
{
    SP_PROPCHANGE_PARAMS params;
    
    params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    params.StateChange = DICS_PROPCHANGE;
    params.Scope       = DICS_FLAG_CONFIGSPECIFIC;
    params.HwProfile   = 0;
    if (!SetupDiSetClassInstallParams(scsi_adapters, &iscsi_dev_info,
                                      (SP_CLASSINSTALL_HEADER *)&params,
                                      sizeof(params)))
    {
        ISCSI_WIN32_REPORT_ERROR();
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    if (!SetupDiChangeState(scsi_adapters, &iscsi_dev_info))
    {
        ISCSI_WIN32_REPORT_ERROR();
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
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

static void CALLBACK
iscsi_cli_timeout(void *state, unsigned long low_timer, unsigned long high_timer)
{
    *(te_bool *)state = TRUE;
}

static char cli_buffer[2048];
static char *new_line = NULL;
static unsigned long residual = 0;

te_errno
iscsi_win32_wait_for(regex_t *pattern, 
                     regex_t *abort_pattern,
                     regex_t *terminal_pattern,
                     int first_part, int nparts, int maxsize, char **buffers)
{
    
    char *free_ptr = cli_buffer;
    unsigned long read_size = sizeof(cli_buffer) - 1;
    unsigned long read_bytes;
    unsigned long available;
    int re_code;
    int i;
    int j;
    te_bool timeout = FALSE;
    LARGE_INTEGER timeout_value;
    
    regmatch_t matches[10];


    timeout_value.QuadPart = -200000000LL; /* 10 sec */
    for (;;)
    {
        if (residual != 0)
        {
            memmove(cli_buffer, new_line, residual);
        }
        free_ptr = cli_buffer;
        read_size = sizeof(cli_buffer) - 1;
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
                return TE_RC(ISCSI_AGENT_TYPE, TE_ENOSPC);
            }

            RING("Waiting for %d bytes from iSCSI CLI", read_size);
            available = 0;
            timeout = FALSE;

            if (!SetWaitableTimer(cli_timeout_timer, &timeout_value, 0,
                                  iscsi_cli_timeout, &timeout, FALSE))
            {
                ISCSI_WIN32_REPORT_ERROR();
                return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
            }
            while (available == 0 && !timeout)
            {
                if (!PeekNamedPipe(host_input, NULL, 0, NULL, &available, NULL))
                {
                    ISCSI_WIN32_REPORT_ERROR();
                    return TE_RC(ISCSI_AGENT_TYPE, TE_EIO);
                }
                SleepEx(0, TRUE);
            }
            if (timeout)
            {
                ERROR("iSCSI CLI timed out...");
                return TE_RC(ISCSI_AGENT_TYPE, TE_ETIMEDOUT);
            }
                               
            if (!ReadFile(host_input, free_ptr, read_size, &read_bytes, NULL))
            {
                ISCSI_WIN32_REPORT_ERROR();
                return TE_RC(ISCSI_AGENT_TYPE, TE_EIO);
            }
            free_ptr[read_bytes] = '\0';
        }
        RING("Probing line '%s'", cli_buffer);
        re_code = regexec(pattern, cli_buffer, 10, matches, 0);
        if (re_code == 0)
            break;
        if (re_code != REG_NOMATCH)
        {
            char err_buf[64] = "";
            regerror(re_code, pattern, err_buf, sizeof(err_buf) - 1);
            ERROR("Matching error: %s", err_buf);
            return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }
        if (terminal_pattern != NULL)
        {
            re_code = regexec(terminal_pattern, cli_buffer, 0, NULL, 0);
            if (re_code == 0)
                return TE_RC(ISCSI_AGENT_TYPE, TE_ENODATA);
        }
        if (abort_pattern != NULL)
        {
            re_code = regexec(abort_pattern, cli_buffer, 0, NULL, 0);
            if (re_code == 0)
            {
                ERROR("iSCSI CLI reported an error: '%s'", cli_buffer);
                return TE_RC(ISCSI_AGENT_TYPE, TE_ESHCMD);
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
            memcpy(buffers[j], cli_buffer + matches[i].rm_so, size);
            buffers[j][size] = '\0';
        }
    }
    return 0;
}

te_errno
iscsi_win32_disable_readahead(const char *devname)
{
    HANDLE dev_handle = CreateFile(devname, 
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);
    DISK_CACHE_INFORMATION cache_info;
    unsigned long result_size;

    if (dev_handle == NULL)
    {
        ISCSI_WIN32_REPORT_ERROR();
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    if (!DeviceIoControl(dev_handle,
                         IOCTL_DISK_GET_CACHE_INFORMATION,
                         NULL,
                         0,
                         &cache_info,
                         sizeof(cache_info),
                         &result_size,
                         NULL))
    {
        ISCSI_WIN32_REPORT_ERROR();
        CloseHandle(dev_handle);
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    cache_info.ReadCacheEnabled = FALSE;
    cache_info.WriteCacheEnabled = FALSE;
    cache_info.DisablePrefetchTransferLength = 0;
    if (!DeviceIoControl(dev_handle,
                         IOCTL_DISK_SET_CACHE_INFORMATION,
                         &cache_info,
                         sizeof(cache_info),
                         &cache_info,
                         sizeof(cache_info),
                         &result_size,
                         NULL))
    {
        ISCSI_WIN32_REPORT_ERROR();
        CloseHandle(dev_handle);
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    CloseHandle(dev_handle);
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
    memset(cli_buffer, 0, sizeof(cli_buffer));
    residual = 0;
    
    return success ? 0 : TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
}

static int
iscsi_win32_write_target_params(iscsi_target_data_t *target,
                                iscsi_connection_data_t *connection,
                                te_bool is_connection)
{
    iscsi_target_param_descr_t     *p;
    
#define PARAMETER(field, offer, type) \
    {OFFER_##offer, #field, type, ISCSI_OPER_PARAM, offsetof(iscsi_connection_data_t, field), NULL, NULL}
#define XPARAMETER(field, offer, type, fmt) \
    {OFFER_##offer, #field, type, ISCSI_OPER_PARAM, offsetof(iscsi_connection_data_t, field), fmt, NULL}
#define GPARAMETER(field, type) \
    {0, #field, type, ISCSI_GLOBAL_PARAM, offsetof(iscsi_target_data_t, field), NULL, NULL}
#define AUTH_PARAM(field, type) \
    {0, #field, type, ISCSI_SECURITY_PARAM, offsetof(iscsi_tgt_chap_data_t, field), NULL, NULL}
#define XAUTH_PARAM(field, type, fmt) \
    {0, #field, type, ISCSI_SECURITY_PARAM, offsetof(iscsi_tgt_chap_data_t, field), fmt, NULL}
#define CONSTANT(field, type) \
    {0, "", type, ISCSI_FIXED_PARAM, offsetof(iscsi_constant_t, field), NULL, NULL}
#define RPARAMETER(field, name, offer, transform) \
    {OFFER_##offer, offsetof(iscsi_connection_data_t, field), name, transform, 0}

    static iscsi_win32_registry_parameter rparams[] =
        {
            RPARAMETER(first_burst_length, "FirstBurstLength", 
                       FIRST_BURST_LENGTH, NULL),
            RPARAMETER(max_burst_length, "MaxBurstLength", 
                       FIRST_BURST_LENGTH, NULL),
            RPARAMETER(max_recv_data_segment_length, "MaxRecvDataSegmentLength", 
                       MAX_RECV_DATA_SEGMENT_LENGTH, NULL),
            RPARAMETER(initial_r2t, "InitialR2T", 
                       INITIAL_R2T, iscsi_win32_bool2int),
            RPARAMETER(immediate_data, "ImmediateData",
                       IMMEDIATE_DATA, iscsi_win32_bool2int),
            RPARAMETER(error_recovery_level, "ErrorRecoveryLevel",
                       ERROR_RECOVERY_LEVEL, NULL),
            {0, NULL, 0, NULL}
        };

    static iscsi_target_param_descr_t params[] =
        {
            GPARAMETER(target_name, TRUE),
            CONSTANT(true, TRUE),
            GPARAMETER(target_addr, TRUE),
            GPARAMETER(target_port, FALSE),
            CONSTANT(wildcard, TRUE),
            CONSTANT(wildcard, TRUE),
            CONSTANT(zero, FALSE),
            CONSTANT(zero, FALSE),
            XPARAMETER(header_digest, HEADER_DIGEST, TRUE, iscsi_not_none),
            XPARAMETER(data_digest, DATA_DIGEST, TRUE, iscsi_not_none),
            PARAMETER(max_connections, MAX_CONNECTIONS,  FALSE),
            PARAMETER(default_time2wait, DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain, DEFAULT_TIME2RETAIN,  FALSE),
            AUTH_PARAM(peer_name, TRUE),
            AUTH_PARAM(peer_secret, TRUE),
            XAUTH_PARAM(chap, TRUE, iscsi_not_none),
            CONSTANT(wildcard, TRUE),
            CONSTANT(zero, FALSE),
            ISCSI_END_PARAM_TABLE
        };
    static iscsi_target_param_descr_t conn_params[] =
        {
            GPARAMETER(session_id, TRUE),
            CONSTANT(wildcard, TRUE),
            CONSTANT(wildcard, TRUE),
            GPARAMETER(target_addr, TRUE),
            GPARAMETER(target_port, FALSE),
            CONSTANT(zero, FALSE),
            CONSTANT(zero, FALSE),
            XPARAMETER(header_digest, HEADER_DIGEST, TRUE, iscsi_not_none),
            XPARAMETER(data_digest, DATA_DIGEST, TRUE, iscsi_not_none),
            PARAMETER(max_connections, MAX_CONNECTIONS,  FALSE),
            PARAMETER(default_time2wait, DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain, DEFAULT_TIME2RETAIN,  FALSE),
            AUTH_PARAM(peer_name, TRUE),
            AUTH_PARAM(peer_secret, TRUE),
            XAUTH_PARAM(chap, TRUE, iscsi_not_none),
            CONSTANT(wildcard, TRUE),
            ISCSI_END_PARAM_TABLE
        };
    static char buffer[2048];
    
    if (iscsi_win32_find_initiator_registry() != 0)
    {
        ERROR("Unable to find registry branch for iSCSI parameters");
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }

    if (!is_connection)
    {
        iscsi_win32_registry_parameter *rp;

        for (rp = rparams; rp->name != NULL; rp++)
        {
            if (rp->offer == 0 || 
                (connection->conf_params & rp->offer) == rp->offer)
            {
                if (iscsi_win32_set_registry_parameter(rp, connection) != 0)
                {
                    ERROR("Unable to set '%s'", rp->name);
                    return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
                }
            }
        }
        if (iscsi_win32_restart_iscsi_service() != 0)
        {
            ERROR("Unable to restart iSCSI service");
            return 0;
        }
    }

    *buffer = '\0';
    for (p = is_connection ? conn_params : params; p->offset >= 0; p++)
    {
        if ((p->offer == 0 || (connection->conf_params & p->offer) == p->offer) &&
            iscsi_is_param_needed(p, target, connection, 
                                  &connection->chap))
        {
            strcat(buffer, " ");
            iscsi_write_param(iscsi_append_to_buf, buffer,
                              p, target, connection,
                              &connection->chap);
        }
        else
        {
            strcat(buffer, " *");
        }
    }
    iscsi_send_to_win32_iscsicli("%s %s", 
                                 is_connection ? "AddConnection" : "LoginTarget", 
                                 buffer);
    return 0;
#undef RPARAMETER
#undef PARAMETER
#undef XPARAMETER
#undef AUTH_PARAM
#undef XAUTH_PARAM
#undef CONSTANT
}


static char   *iscsi_conditions[] = {
    "^Session Id is (0x[a-f0-9]*-0x[a-f0-9]*)", 
    "^Connection Id is (0x[a-f0-9]*-0x[a-f0-9]*)", 
    "^The operation completed successfully.", 
    "Error:|The target has already been logged|[Ff]ailed|cannot|invalid",
    "Connection Id[[:space:]]*:[[:space:]]*([a-f0-9]*)-([a-f0-9]*)",
    "Device Number[[:space:]]*:[[:space:]]*(-?[0-9]*)"
};

static regex_t iscsi_regexps[TE_ARRAY_LEN(iscsi_conditions)];

#define session_id_regexp (iscsi_regexps[0])
#define connection_id_regexp (iscsi_regexps[1])
#define success_regexp (iscsi_regexps[2])
#define error_regexp (iscsi_regexps[3])
#define existing_conn_regexp (iscsi_regexps[4])
#define dev_number_regexp (iscsi_regexps[5])

te_errno
iscsi_initiator_win32_set(iscsi_connection_req *req)
{
    int                  rc = 0;
    iscsi_target_data_t *target = iscsi_configuration()->targets + req->target_id;
    iscsi_connection_data_t *conn = target->conns + req->cid;

    switch (req->status)
    {
        case ISCSI_CONNECTION_DOWN:
        case ISCSI_CONNECTION_REMOVED:
            if (strcmp(conn->session_type, "Discovery") != 0)
            {
                te_bool do_logout = FALSE;
                
                rc = 0;
                if (req->cid > 0)
                {
                    if (conn->connection_id[0] != '\0')
                    {
                        rc = iscsi_send_to_win32_iscsicli("RemoveConnection %s %s",
                                                          target->session_id,
                                                          conn->connection_id);
                        do_logout = TRUE;
                    }
                }
                else
                {
                    if (target->session_id[0] != '\0')
                    {
                        rc = iscsi_send_to_win32_iscsicli("LogoutTarget %s",
                                                          target->session_id);
                        do_logout = TRUE;
                    }
                }
                if (rc != 0)
                {
                    ERROR("Unable to stop connection %d, %d: %r", req->target_id, req->cid);
                }
                else if (do_logout)
                {
                    rc = iscsi_win32_wait_for(&success_regexp, &error_regexp, 
                                              NULL, 0, 0, 0, NULL);
                    if (rc != 0)
                    {
                        ERROR("Unable to stop connection %d, %d: %r", req->target_id, req->cid);
                    }
                }
                
                iscsi_win32_finish_cli();

                if (rc == 0)
                {
                    conn->connection_id[0] = '\0';
                    if (req->cid == 0)
                        target->session_id[0] = '\0';
                }
            }
            break;
        case ISCSI_CONNECTION_UP:
            if (strcmp(conn->session_type, "Discovery") == 0)
            {
                ERROR("Discovery is not yet supported for Windows iSCSI");
                rc = TE_RC(ISCSI_AGENT_TYPE, TE_ENOSYS);
            }
            else
            {
                char *buffers[1];

                rc = iscsi_win32_write_target_params(target, 
                                                     &target->conns[req->cid],
                                                     req->cid != 0);
                if (rc != 0)
                {
                    ERROR("Unable to set iSCSI parameters: %r", rc);
                    return rc;
                }
                RING("Waiting for session and connection IDs");
                
                rc = 0;
                if (req->cid == 0)
                {
                    buffers[0] = target->session_id;
                    rc = iscsi_win32_wait_for(&session_id_regexp, 
                                              &error_regexp,
                                              &success_regexp,
                                              1, 1, 
                                              sizeof(target->session_id) - 1, buffers);
                }
                if (rc == 0)
                {
                    RING("Got Session Id = %s", target->session_id);
                    buffers[0] = conn->connection_id;
                    rc = iscsi_win32_wait_for(&connection_id_regexp, 
                                              &error_regexp,
                                              &success_regexp,
                                              1, 1, 
                                              sizeof(conn->connection_id) - 1, 
                                              buffers);
                }
                if (rc == 0)
                {
                    RING("Got Connection Id = %s", conn->connection_id);
                    rc = iscsi_win32_wait_for(&success_regexp, &error_regexp, NULL, 0, 0, 0, NULL);
                }
                iscsi_win32_finish_cli();
            }
            if (rc != 0)
            {
                ERROR("Unable to start initiator connection %d, %d: %r",
                      req->target_id, req->cid, rc);
                return TE_RC(ISCSI_AGENT_TYPE, rc);
            }
            break;
        default:
            ERROR("Invalid operational code %d", req->status);
            return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);
    }
    return rc;
}

te_errno
iscsi_win32_prepare_device(iscsi_connection_data_t *conn)
{
    char *buffers[2];
    char  conn_id_first[SESSION_ID_LENGTH] = "0x";
    char  conn_id_second[SESSION_ID_LENGTH] = "-0x";
    char  drive_id[64];
    int   drive_no;
    int   rc;
    
    rc = iscsi_send_to_win32_iscsicli("SessionList");
    if (rc != 0)
    {
        ERROR("Unable to obtain session list: %r", rc);
        return rc;
    }
    buffers[0] = conn_id_first + 2;
    buffers[1] = conn_id_second + 3;

    RING("Looking for Connection ID %s", conn->connection_id);
    for (;;)
    {
        rc = iscsi_win32_wait_for(&existing_conn_regexp, 
                                  &error_regexp,
                                  &success_regexp, 
                                  1, 2, SESSION_ID_LENGTH - 1, 
                                  buffers);
        if (rc == 0)
        {
            strcat(conn_id_first, conn_id_second);
            RING("Got connection ID %s", conn_id_first);
            if (strcmp(conn_id_first, conn->connection_id) == 0)
                break;
        }
        else
        {
            if (TE_RC_GET_ERROR(rc) == TE_ENODATA)
                return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
            else
                return rc;
        }
    }
    buffers[0] = drive_id;
    RING("Waiting for device number");
    rc = iscsi_win32_wait_for(&dev_number_regexp, 
                              &error_regexp,
                              &success_regexp,
                              1, 1, sizeof(drive_id) - 1,
                              buffers);
                              
    iscsi_win32_finish_cli();
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENODATA)
            return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
        ERROR("Unable to find drive number: %r", rc);
        return rc;
    }
    drive_no = strtol(drive_id, NULL, 10);
    if (drive_no < 0)
        return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);

    pthread_mutex_lock(&conn->status_mutex);
    /* will not work on EBCDIC-based systems :P */
    strcpy(conn->device_name, "\\\\.\\PHYSICALDRIVE0");
    conn->device_name[strlen(conn->device_name) - 1] += drive_no;
    pthread_mutex_unlock(&conn->status_mutex);

    iscsi_win32_disable_readahead(conn->device_name);
    return 0;
}

te_bool
iscsi_win32_init_regexps(void)
{
    unsigned i;
    int status;
    char err_buf[64];
    
    for (i = 0 ; i < TE_ARRAY_LEN(iscsi_conditions); i++)
    {
        status = regcomp(&iscsi_regexps[i], iscsi_conditions[i], REG_EXTENDED);
        if (status != 0)
        {
            regerror(status, NULL, err_buf, sizeof(err_buf) - 1);
            ERROR("Cannot compile regexp '%s': %s", iscsi_conditions[i], err_buf);
            return FALSE;
        }
    }
    return TRUE;
}

#endif
