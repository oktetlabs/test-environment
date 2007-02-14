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
#include "te_iscsi.h"

#include <sys/types.h>
#include <pthread.h>
#include <regex.h>

#include "iscsi_initiator.h"

/* This is adapted from ddk/ntddstor.h, since that file cannot be
 * included in a userland program, and there is _no_ other way to
 * get that GUID. Blame Microsoft, not me.
 */
static GUID guid_devinterface_disk = {0x53f56307L, 
                                      0xb6bf, 
                                      0x11d0, 
                                      {0x94, 0xf2, 0x00, 0xa0, 
                                       0xc9, 0x1e, 0xfb, 0x8b}};

#define DEFAULT_INITIAL_R2T_WIN32    1
#define DEFAULT_IMMEDIATE_DATA_WIN32 1

/**
 * See description in iscsi_initiator.h
 */
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

/** TRUE if `iscsicli' process is running */
static te_bool             cli_started;

/** Handles for pipes communicating with `iscsicli' process */
static HANDLE              host_input, cli_output;
static HANDLE              host_output, cli_input;

/** Process info structure for `iscsicli' process */
static PROCESS_INFORMATION process_info;

/** Timer used to wait for `iscsicli' response */
static HANDLE              cli_timeout_timer = INVALID_HANDLE_VALUE;

/** Win32 registry branch handle which holds `hidden' iSCSI parameters */
static HKEY                driver_parameters = INVALID_HANDLE_VALUE;

/** So called `device information set' for SCSI adapters */
static HDEVINFO            scsi_adapters     = INVALID_HANDLE_VALUE;

/** Device info for a SCSI device associated with the Initiator */
static SP_DEVINFO_DATA     iscsi_dev_info;

/** iSCSI Initiator instance name */
static char iscsi_initiator_instance[256];

/** Empirically discovered iscscli output patterns:
 *   -# New session ID 
 *   -# New connection ID 
 *   -# Last line of output
 *   -# Error messages
 *   -# Existing connection ID
 *   -# SCSI device number
 *   -# Initiator instance name
 */
static char   *iscsi_conditions[] = {
    "^Session Id is (0x[a-f0-9]*-0x[a-f0-9]*)", 
    "^Connection Id is (0x[a-f0-9]*-0x[a-f0-9]*)", 
    "^The operation completed successfully.", 
    "Error:|The target has already been logged|[Ff]ailed|cannot|invalid|not found",
    "Connection Id[[:space:]]*:[[:space:]]*([a-f0-9]*)-([a-f0-9]*)",
    "Device Interface Name[[:space:]]*:[[:space:]]*([^[:space:]]+)",
    "Legacy Device Name[[:space:]]*:[[:space:]]*([^[:space:]]+)",
    "^Session Id[[:space:]]*:[[:space:]]*([a-f0-9]*-[a-f0-9]*)", 
    "^Total of ([0-9]*) sessions",
};

/** Compiled form of iscsi_conditions */
static regex_t iscsi_regexps[TE_ARRAY_LEN(iscsi_conditions)];

#define session_id_regexp (iscsi_regexps[0])
#define connection_id_regexp (iscsi_regexps[1])
#define success_regexp (iscsi_regexps[2])
#define error_regexp (iscsi_regexps[3])
#define existing_conn_regexp (iscsi_regexps[4])
#define dev_name_regexp (iscsi_regexps[5])
#define legacy_dev_name_regexp (iscsi_regexps[6])
#define session_list_id_regexp (iscsi_regexps[7])
#define total_sessions_regexp (iscsi_regexps[8])

static te_errno iscsi_win32_set_default_parameters(void);

/**
 * Formatting function for iscsi_write_param().
 *
 * @return "0" if @p val is "None", "1" otherwise
 * @param val   String value
 */
static char *
iscsi_not_none (void *val)
{
    static char buf[2];
    *buf = (strcmp(val, "None") == 0 ? '0' : '1');
    return buf;
}


/**
 * See description in iscsi_initiator.h
 */
te_errno
iscsi_win32_write_to_device(iscsi_connection_data_t *conn)
{
    te_errno rc         = 0;
    HANDLE   dev_handle = CreateFile(conn->device_name,
                                     GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL);

    if (dev_handle == NULL)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            RING("Device %s is not ready :(", conn->device_name);
            pthread_mutex_lock(&conn->status_mutex);
            *conn->device_name = '\0';
            pthread_mutex_unlock(&conn->status_mutex);
            rc = TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
        }
        else
        {
            ISCSI_WIN32_REPORT_ERROR();
            return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }
    }
    else
    {
        unsigned long bytes_written = 0;
        const char    buf[ISCSI_SCSI_BLOCKSIZE] = "testing";

        if (!WriteFile(dev_handle, buf, sizeof(buf), &bytes_written, NULL))
        {
            ISCSI_WIN32_REPORT_ERROR();
            rc = TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
            CloseHandle(dev_handle);
        }
        else
        {
            if (bytes_written != sizeof(buf))
            {
                rc = TE_RC(ISCSI_AGENT_TYPE, TE_ENOSPC);
            }
            if (!CloseHandle(dev_handle))
            {
                ISCSI_WIN32_REPORT_ERROR();
                rc = TE_OS_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
                ERROR("Error syncing data to %s",
                      conn->device_name);
            }
        }
    }
    return rc;
}

/**
 *  Detect Initiator instance name for the current iSCSI device
 *
 *  @return TRUE if the detected instance name is in the list of Initiators
 */
static te_bool
iscsi_win32_detect_initiator_name(void)
{
    char          service_name[128] = "";
    unsigned long buf_size;
    unsigned long value_type;
    unsigned long result;
    HKEY          all_services;
    HKEY          iscsi_service;

    buf_size = sizeof(service_name);
    if (!SetupDiGetDeviceRegistryProperty(scsi_adapters, &iscsi_dev_info,
                                          SPDRP_SERVICE, &value_type,
                                          service_name, 
                                          buf_size, &buf_size))
    {
        if (GetLastError() != ERROR_INVALID_DATA)
            ISCSI_WIN32_REPORT_ERROR();
        return FALSE;
    }

    if ((result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                               "SYSTEM\\CurrentControlSet\\Services",
                               0, KEY_ALL_ACCESS, &all_services)) != 0)
    {
        ISCSI_WIN32_REPORT_RESULT(result);
        return FALSE;
    }
    strcat(service_name, "\\Enum");
    if ((result = RegOpenKeyEx(all_services, service_name,
                               0, KEY_ALL_ACCESS, &iscsi_service)) != 0)
    {
        RegCloseKey(all_services);
        ISCSI_WIN32_REPORT_RESULT(result);
        return FALSE;
    }
    RegCloseKey(all_services);
    
    memset(iscsi_initiator_instance, 0, sizeof(iscsi_initiator_instance));
    buf_size = sizeof(iscsi_initiator_instance) - 1;
    result = RegQueryValueEx(iscsi_service, "0", NULL, &value_type, 
                             iscsi_initiator_instance, &buf_size);
    RegCloseKey(iscsi_service);
    
    if (result != 0)
    {
        ISCSI_WIN32_REPORT_RESULT(result);
        return FALSE;
    }
    strcat(iscsi_initiator_instance, "_0");
    return TRUE;
}

/**
 * Name of the Microsoft Initiator vendor.
 */
#define ISCSI_MICROSOFT_MANUFACTURER_NAME "Microsoft"

/**
 * Name of the SF Initiator vendor.
 */
#define ISCSI_SF_MANUFACTURER_NAME "Solarflare Communications"

/**
 * Location of the MS & L5 Initiator configuration parameters
 * in the registry.
 */
#define ISCSI_MS_REG_PATH \
            "SYSTEM\\CurrentControlSet\\Control\\Class\\"

/**
 * Location of the SF Initiator configuration parameters
 * in the registry.
 */
#define ISCSI_SF_REG_PATH \
            "SYSTEM\\CurrentControlSet\\Services\\SFCISCSI\\"

/**
 * Find a registry branch holding `hidden' iSCSI parameters,
 * using Win32 SetupAPI.
 * This function is called once per agent run, and it resets
 * all the `hidden' parameters to their default values.
 *
 * @return Status code
 */
static te_errno
iscsi_win32_find_initiator_registry(void)
{
    GUID            scsi_class_guid;
    unsigned        index;
    long            result;

    static char   buffer[1024];
    static char   registry_path_name[1024];
    
    unsigned long buf_size;
    unsigned long value_type;

    const char *manufacturer;

    switch (iscsi_configuration()->init_type)
    {
        case ISCSI_MICROSOFT:
            manufacturer = ISCSI_MICROSOFT_MANUFACTURER_NAME;
            strcpy(registry_path_name, 
                   ISCSI_MS_REG_PATH);
            break;
        case ISCSI_L5_WIN32:
            manufacturer = ISCSI_SF_MANUFACTURER_NAME;
            strcpy(registry_path_name, 
                   ISCSI_SF_REG_PATH);
            break;
        default:
            ERROR("Unsupported iSCSI initiator");
            return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);
    }

    if (driver_parameters != INVALID_HANDLE_VALUE)
        return 0;

    if (!SetupDiClassGuidsFromName("SCSIAdapter", 
                                   &scsi_class_guid, 1, &buf_size))
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
        if (!SetupDiGetDeviceRegistryProperty(scsi_adapters, 
                                              &iscsi_dev_info,
                                              SPDRP_MFG, &value_type,
                                              buffer, buf_size, &buf_size))
        {
            if (GetLastError() == ERROR_INVALID_DATA)
                continue;
            ISCSI_WIN32_REPORT_ERROR();
            return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }
        if (value_type != REG_SZ)
        {
            ERROR("Registry seems to be corrupted, very bad");
            return TE_RC(ISCSI_AGENT_TYPE, TE_ECORRUPTED);
        }
        RING("Manufacturer is %s, looking for %s", buffer, manufacturer);
        if (strstr(buffer, manufacturer) != NULL)
        {
            if (iscsi_win32_detect_initiator_name())
                break;
        }
    }

    /*
     * The idea is that we search for MS Initiator in the correct
     * place, but the L5 Initiator place is 'hardcoded'.
     * In fact, this code is useless, because all configuration
     * keys are hardcoded by both vendors.
     */
    if (iscsi_configuration()->init_type == ISCSI_MICROSOFT)
    {
        /* 
         * For microsoft Initiator the place is:
         * registry_path_name\\DeviceID\\
         */
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, registry_path_name);
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
    }
    else if (iscsi_configuration()->init_type == ISCSI_L5_WIN32)
    {
        sprintf(buffer, registry_path_name);
    }
    else
    {
        /* doublecheck */
        ERROR("Unsupported Initiator type");
    }
    
    strcat(buffer, "\\Parameters");
    
    RING("Trying to open %s", buffer);
    if ((result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer,
                               0, KEY_ALL_ACCESS, &driver_parameters)) != 0)
    {
        if (result == ERROR_FILE_NOT_FOUND)
        {
            WARN("The Initiator does not support extended configuration");
            driver_parameters = INVALID_HANDLE_VALUE;
            return 0;
        }
        ISCSI_WIN32_REPORT_RESULT(result);
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }
    return iscsi_win32_set_default_parameters();
}

/**
 * Starts a `iscsicli' process using @p cmdline.
 * If the process is already running, it is killed.
 *
 * @return              Status code
 * @param  cmdline      Command line (including the command name)
 */
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
    if (!CreateProcess(NULL, (char *)cmdline, 
                       NULL, NULL, TRUE,
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

/**
 * Description of MS iSCSI `hidden' parameters 
 * (i.e. those which are only configurable via the registry)
 */
typedef struct iscsi_win32_registry_parameter
{
    int offer;  /**< OFFER_XXX mask */
    int offset; /**< Offset in the data structure */  
    char *name; /**< Registry value name */
    unsigned long (*transform)(void *); 
                /**< Function to translate our value to
                 *   the one needed by MS iSCSI
                 */
    unsigned long constant; /**< If @p offset is less than 0,
                             *   this value is taken instead of
                             *   a structure field
                             */
} iscsi_win32_registry_parameter;

/** 
 * Transformation function for iscsi_win32_set_registry_parameter()
 */
static unsigned long
iscsi_win32_bool2int(void *data)
{
    return strcasecmp((char *)data, "Yes") == 0;
}

/**
 * Sets an iSCSI parameter described by @p parm in the Win32 registry.
 *
 * @return      Status code
 * @param parm  Parameter description
 * @param data  Target/connection data
 */
static te_errno
iscsi_win32_set_registry_parameter(iscsi_win32_registry_parameter *parm, 
                                   void *data)
{
    unsigned long value;
    long          result;

    if (driver_parameters == INVALID_HANDLE_VALUE)
    {
        WARN("Setting %s is not supported by the Initiator",
             parm->name);
        return 0;
    }

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

/**
 * Resets iSCSI `hidden' parameters to their default values
 *
 * @return Status code
 */
static te_errno
iscsi_win32_set_default_parameters(void)
{
#define RPARAMETER(name, value) {0, -1, name, NULL, value}
    /** 
     * Registry-configurable operational parameters, 
     *  RFC3720 default values 
     */
    static iscsi_win32_registry_parameter rparams[] =
        {
            RPARAMETER("InitialR2T", DEFAULT_INITIAL_R2T_WIN32),
            RPARAMETER("ImmediateData", DEFAULT_IMMEDIATE_DATA_WIN32),
            RPARAMETER("MaxRecvDataSegmentLength",
                       ISCSI_DEFAULT_MAX_RECV_DATA_SEGMENT_LENGTH),
            RPARAMETER("FirstBurstLength", ISCSI_DEFAULT_FIRST_BURST_LENGTH),
            RPARAMETER("MaxBurstLength", ISCSI_DEFAULT_MAX_BURST_LENGTH),
            RPARAMETER("ErrorRecoveryLevel", 
                       ISCSI_DEFAULT_ERROR_RECOVERY_LEVEL),
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
iscsi_win32_cleanup_stalled_sessions(void)
{
    te_errno rc;
    char session_id[ISCSI_SESSION_ID_LENGTH];
    char *buffers[1];
    char sessions_count_str[10];
    int  sessions_count;

    for (;;)
    {

        RING("Call 'iscsicli.exe SessionList'");
        iscsi_send_to_win32_iscsicli("SessionList");

        buffers[0] = sessions_count_str;
        rc = iscsi_win32_wait_for(&total_sessions_regexp, 
                                  &error_regexp,
                                  &success_regexp,
                                  1, 1, 
                                  sizeof(sessions_count_str) \
                                  - 1, 
                                  buffers);
        if (rc != 0)
        {
            ERROR("Failed to get amount of iSCSI sessions, rc=%r", rc);
            return rc;
        }

        sessions_count = atoi(sessions_count_str);
        RING("Total of %d sessions", sessions_count);

        if (sessions_count == 0)
        {
            RING("No sessions opened");
            break;
        }

        RING("Waiting for session IDs");

        buffers[0] = session_id;
        rc = iscsi_win32_wait_for(&session_list_id_regexp, 
                                  &error_regexp,
                                  &success_regexp,
                                  1, 1, 
                                  sizeof(session_id) \
                                  - 1, 
                                  buffers);
        if (rc != 0)
        {
            ERROR("Failed to get session ID, rc=%r", rc);
            return rc;
        }
        RING("Found opened session \"%s\"", session_id);
    
        iscsi_send_to_win32_iscsicli("LogoutTarget %s", session_id);
        iscsi_win32_finish_cli();
    }

    return 0;
}

/**
 * Reloads MS iSCSI device driver service
 *
 * @return Status code
 */
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

/**
 * See description in iscsi_initiator.h
 */
te_errno
iscsi_send_to_win32_iscsicli(const char *fmt, ...)
{
    static char buffer[2048];
    va_list args;
    int len;

    va_start(args, fmt);
    len = strlen("iscsicli.exe ");
    memcpy(buffer, "iscsicli.exe ", len);
    vsnprintf(buffer + len, sizeof(buffer) - 1 - len, fmt, args);
    va_end(args);

    return iscsi_win32_run_cli(buffer);
}

/**
 * A callback used when cli_timeout_timer fires
 */
static void CALLBACK
iscsi_cli_timeout(void *state, 
                  unsigned long low_timer, unsigned long high_timer)
{
    UNUSED(low_timer);
    UNUSED(high_timer);
    *(te_bool *)state = TRUE;
}

/** 
 * Buffer holding output from `iscsicli' process
 */
static char cli_buffer[2048];

/**
 * A pointer to the first new line in @p cli_buffer
 */
static char *new_line = NULL;

/**
 * Number of bytes forming an incomplete line in @p cli_buffer
 */
static unsigned long residual = 0;

/**
 * See description in iscsi_initiator.h
 *
 * This function reads output from `iscsicli' until a full line is read.
 * It then matches it agains one of three patterns (@p pattern, 
 * @p abort_pattern, @p terminal_pattern). If a match is found, the function
 * returns filling @p buffers as necessary and leaving all remaining output
 * (i.e. a beginning of the next line of the output) in the @p cli_buffer 
 * for further examination.
 */
te_errno
iscsi_win32_wait_for(regex_t *pattern, 
                     regex_t *abort_pattern,
                     regex_t *terminal_pattern,
                     int first_part, int nparts, 
                     int maxsize, char **buffers)
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
                if (!PeekNamedPipe(host_input, NULL, 0, NULL, 
                                   &available, NULL))
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
                               
            if (!ReadFile(host_input, free_ptr, read_size, 
                          &read_bytes, NULL))
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

/**
 * See description in iscsi_initiator.h
 *
 * NOTE: This function seems to have no useful effect with MS iSCSI :(
 */
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

/**
 * See description in iscsi_initiator.h
 *
 * The function terminates a running `iscsicli' process and 
 * clears all pending data in @p cli_buffer.
 */
te_errno
iscsi_win32_finish_cli(void)
{
    te_bool success;
    
    CloseHandle(host_output);
    CloseHandle(host_input);
    success = (WaitForSingleObject(process_info.hProcess, 100) == 
               WAIT_OBJECT_0);
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

/**
 * Format command line parameters for `iscsicli' process
 * 
 * @return A pointer to a static buffer holding the command line
 * @param table         iSCSI parameter descriptions
 * @param target        Target-wide data
 * @param connection    Connection-wide data
 */
static const char *
iscsi_win32_format_params(iscsi_target_param_descr_t *table,
                          iscsi_target_data_t *target, 
                          iscsi_connection_data_t *connection)
{
    static char buffer[2048];

    *buffer = '\0';
    for (; table->offset >= 0; table++)
    {
        if ((table->offer == 0 || 
             (connection->conf_params & table->offer) == table->offer) &&
            iscsi_is_param_needed(table, target, connection, 
                                  &connection->chap))
        {
            strcat(buffer, " ");
            iscsi_write_param(iscsi_append_to_buf, buffer,
                              table, target, connection,
                              &connection->chap);
        }
        else
        {
            strcat(buffer, " *");
        }
    }
    return buffer;
}

/** Macros to facilitate writing parameter descriptions */

/** Operational parameter template */
#define PARAMETER(field, offer, type)                       \
    {OFFER_##offer, #field, type, ISCSI_OPER_PARAM,         \
     offsetof(iscsi_connection_data_t, field), NULL, NULL}

/** Operational parameter template with custom formatter */
#define XPARAMETER(field, offer, type, fmt)                 \
    {OFFER_##offer, #field, type, ISCSI_OPER_PARAM,         \
     offsetof(iscsi_connection_data_t, field), fmt, NULL}

/** Target-wide parameter template */
#define GPARAMETER(field, type)                         \
    {0, #field, type, ISCSI_GLOBAL_PARAM,               \
    offsetof(iscsi_target_data_t, field), NULL, NULL}

/** Security parameter template */
#define AUTH_PARAM(field, type)                             \
    {0, #field, type, ISCSI_SECURITY_PARAM,                 \
     offsetof(iscsi_tgt_chap_data_t, field), NULL, NULL}

/** Security parameter template with custom formatter */
#define XAUTH_PARAM(field, type, fmt)                   \
    {0, #field, type, ISCSI_SECURITY_PARAM,             \
    offsetof(iscsi_tgt_chap_data_t, field), fmt, NULL}

/** Constant value template */
#define CONSTANT(name)                                              \
    {0, #name, 0, ISCSI_FIXED_PARAM, 0, iscsi_constant_##name, NULL}

/** `Hidden' parameter template */
#define RPARAMETER(field, name, offer, transform)               \
    {OFFER_##offer, offsetof(iscsi_connection_data_t, field),   \
     name, transform, 0}

static char *
iscsi_constant_zero(void *null)
{
    UNUSED(null);
    return "0";
}

static char *
iscsi_constant_true(void *null)
{
    UNUSED(null);
    return "T";
}

static char *
iscsi_constant_wildcard(void *null)
{
    UNUSED(null);
    return "*";
}

static char *
iscsi_constant_instance(void *null)
{
    UNUSED(null);
    return iscsi_initiator_instance;
}

/**
 * This function actually initiates a login procedure for Win initiator.
 * It first modifies registry values and restarts the iSCSI service 
 * (when necessary) and then calls `iscsicli' process.
 *
 * @return Status code
 * @param target        Target-wide data
 * @param connection    Connection data
 * @param is_connection TRUE if it's not the leading connection
 */
static te_errno
iscsi_win32_write_target_params(iscsi_target_data_t *target,
                                iscsi_connection_data_t *connection,
                                te_bool is_connection)
{
    /** Registry configurable operation parameters */
    static iscsi_win32_registry_parameter rparams[] =
        {
            RPARAMETER(first_burst_length, "FirstBurstLength", 
                       FIRST_BURST_LENGTH, NULL),
            RPARAMETER(max_burst_length, "MaxBurstLength", 
                       FIRST_BURST_LENGTH, NULL),
            RPARAMETER(max_recv_data_segment_length, 
                       "MaxRecvDataSegmentLength", 
                       MAX_RECV_DATA_SEGMENT_LENGTH, NULL),
            RPARAMETER(initial_r2t, "InitialR2T", 
                       INITIAL_R2T, iscsi_win32_bool2int),
            RPARAMETER(immediate_data, "ImmediateData",
                       IMMEDIATE_DATA, iscsi_win32_bool2int),
            RPARAMETER(error_recovery_level, "ErrorRecoveryLevel",
                       ERROR_RECOVERY_LEVEL, NULL),
            {0, 0, NULL, NULL, 0}
        };

    /** CLI-configurable session-wide parameters */
    static iscsi_target_param_descr_t params[] =
        {
            GPARAMETER(target_name, TRUE),
            CONSTANT(true),
            GPARAMETER(target_addr, TRUE),
            GPARAMETER(target_port, FALSE),
            CONSTANT(instance),
            CONSTANT(wildcard),
            CONSTANT(zero),
            CONSTANT(zero),
            XPARAMETER(header_digest, HEADER_DIGEST, TRUE, iscsi_not_none),
            XPARAMETER(data_digest, DATA_DIGEST, TRUE, iscsi_not_none),
            PARAMETER(max_connections, MAX_CONNECTIONS,  FALSE),
            PARAMETER(default_time2wait, DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain, DEFAULT_TIME2RETAIN,  FALSE),
            AUTH_PARAM(peer_name, TRUE),
            AUTH_PARAM(peer_secret, TRUE),
            XAUTH_PARAM(chap, TRUE, iscsi_not_none),
            CONSTANT(wildcard),
            CONSTANT(zero),
            ISCSI_END_PARAM_TABLE
        };

    /** CLI-configurable connection-wide parameters */
    static iscsi_target_param_descr_t conn_params[] =
        {
            GPARAMETER(session_id, TRUE),
            CONSTANT(wildcard),
            CONSTANT(wildcard),
            GPARAMETER(target_addr, TRUE),
            GPARAMETER(target_port, FALSE),
            CONSTANT(zero),
            CONSTANT(zero),
            XPARAMETER(header_digest, HEADER_DIGEST, TRUE, iscsi_not_none),
            XPARAMETER(data_digest, DATA_DIGEST, TRUE, iscsi_not_none),
            PARAMETER(max_connections, MAX_CONNECTIONS,  FALSE),
            PARAMETER(default_time2wait, DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain, DEFAULT_TIME2RETAIN,  FALSE),
            AUTH_PARAM(peer_name, TRUE),
            AUTH_PARAM(peer_secret, TRUE),
            XAUTH_PARAM(chap, TRUE, iscsi_not_none),
            CONSTANT(wildcard),
            ISCSI_END_PARAM_TABLE
        };
    
    if (iscsi_win32_find_initiator_registry() != 0)
    {
        ERROR("Unable to find registry branch for iSCSI parameters");
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }

    if (!is_connection && driver_parameters != INVALID_HANDLE_VALUE)
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
        if (iscsi_configuration()->win32_service_restart != 0)
        {
            RING("Restart Win32 iSCSI Initiator Service");
            if (iscsi_win32_restart_iscsi_service() != 0)
            {
                ERROR("Unable to restart iSCSI service");
                return 0;
            }
        }
        else
        {
            RING("Close all remaining sessions if any");
            if (iscsi_win32_cleanup_stalled_sessions() != 0)
            {
                ERROR("Failed to close all remaining sessions");
                return 0;
            }
        }
    }
    iscsi_send_to_win32_iscsicli("%s %s", 
                                 is_connection ? 
                                 "AddConnection" : 
                                 "LoginTarget", 
                                 iscsi_win32_format_params((is_connection ? 
                                                            conn_params : 
                                                            params),
                                                           target, 
                                                           connection));
    return 0;
}

/**
 * Initiates a discovery session for MS Initiator.
 *
 * @return Status code
 * @param target        Target-wide data
 * @param connection    Conncection data
 */
static te_errno
iscsi_win32_do_discovery(iscsi_target_data_t *target,
                         iscsi_connection_data_t *connection)
{
    /** CLI-configurable parameters for Discovery sessions */
    static iscsi_target_param_descr_t params[] =
        {
            GPARAMETER(target_addr, TRUE),
            GPARAMETER(target_port, FALSE),
            CONSTANT(instance),
            CONSTANT(wildcard),
            CONSTANT(zero),
            CONSTANT(zero),
            XPARAMETER(header_digest, HEADER_DIGEST, TRUE, iscsi_not_none),
            XPARAMETER(data_digest, DATA_DIGEST, TRUE, iscsi_not_none),
            PARAMETER(max_connections, MAX_CONNECTIONS,  FALSE),
            PARAMETER(default_time2wait, DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain, DEFAULT_TIME2RETAIN,  FALSE),
            AUTH_PARAM(peer_name, TRUE),
            AUTH_PARAM(peer_secret, TRUE),
            XAUTH_PARAM(chap, TRUE, iscsi_not_none),
            ISCSI_END_PARAM_TABLE
        };
    int rc;

    if (iscsi_win32_find_initiator_registry() != 0)
    {
        ERROR("Unable to find registry branch for iSCSI parameters");
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }

    iscsi_send_to_win32_iscsicli("AddTargetPortal %s", 
                                 iscsi_win32_format_params(params,
                                                           target, 
                                                           connection));
    rc = iscsi_win32_wait_for(&success_regexp, &error_regexp, 
                              NULL, 0, 0, 0, NULL);
    if (rc != 0)
    {
        ERROR("Unable to add target portal for discovery: %r", rc);
        return rc;
    }
#if 0
    iscsi_send_to_win32_iscsicli("RefreshTargetPortal %s %d * *", 
                                 target->target_addr, target->target_port);
    rc = iscsi_win32_wait_for(&success_regexp, &error_regexp, 
                              NULL, 0, 0, 0, NULL);
    if (rc != 0)
    {
        ERROR("Unable to refresh target portal: %r", rc);
        return rc;
    }
#endif
    iscsi_send_to_win32_iscsicli("RemoveTargetPortal %s %d * *", 
                                 target->target_addr, target->target_port);
    rc = iscsi_win32_wait_for(&success_regexp, &error_regexp, NULL, 
                              0, 0, 0, NULL);
    if (rc != 0)
    {
        ERROR("Unable to refresh target portal: %r", rc);
        return rc;
    }
    iscsi_win32_finish_cli();
    return 0;
}

#undef RPARAMETER
#undef PARAMETER
#undef XPARAMETER
#undef AUTH_PARAM
#undef XAUTH_PARAM
#undef CONSTANT


/**
 * See iscsi_initiator.h and iscsi_initator_conn_request_thread()
 * for a complete description of the state machine involved.
 */
te_errno
iscsi_initiator_win32_set(iscsi_connection_req *req)
{
    int                  rc = 0;
    iscsi_target_data_t *target = iscsi_configuration()->targets + 
        req->target_id;
    iscsi_connection_data_t *conn = target->conns + req->cid;

    switch (req->status)
    {
        case ISCSI_CONNECTION_DOWN:
        case ISCSI_CONNECTION_REMOVED:
            RING("Connection Down");
            if (strcmp(conn->session_type, "Discovery") != 0)
            {
                te_bool do_logout = FALSE;
                
                rc = 0;
                if (req->cid > 0)
                {
                    RING("Remove the connection, session=%s, cid=%d",
                         target->session_id, req->cid);
                    if ((target->session_id[0] != '\0') &&
                        (conn->connection_id[0] != '\0'))
                    {
                        rc = iscsi_send_to_win32_iscsicli \
                            ("RemoveConnection %s %s",
                             target->session_id,
                             conn->connection_id);
                        do_logout = TRUE;
                    }
                    else
                    {
                        ERROR("The connection does not exist");
                    }
                }
                else
                {
                    RING("Remove the connection, session=%s, cid=%d",
                         target->session_id, req->cid);
                    if (target->session_id[0] != '\0')
                    {
                        rc = iscsi_send_to_win32_iscsicli \
                            ("LogoutTarget %s",
                             target->session_id);
                        do_logout = TRUE;
                    }
                    else
                    {
                        ERROR("The connection does not exist");
                    }
                }
                if (rc != 0)
                {
                    ERROR("Unable to stop connection %d, %d: %r", 
                          req->target_id, req->cid);
                }
                else if (do_logout)
                {
                    rc = iscsi_win32_wait_for(&success_regexp, 
                                              &error_regexp, 
                                              NULL, 0, 0, 0, NULL);
                    if (rc != 0)
                    {
                        ERROR("Unable to stop connection %d, %d: %r", 
                              req->target_id, req->cid);
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
            RING("Connection Up");
            if (target->conns[req->cid].status == 
                ISCSI_CONNECTION_DISCOVERING)
            {
                rc = iscsi_win32_do_discovery(target, 
                                              &target->conns[req->cid]);
            }
            else
            {
                char *buffers[1];

                rc = iscsi_win32_write_target_params(target, 
                                                     &target->conns \
                                                     [req->cid],
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
                                              sizeof(target->session_id) \
                                              - 1, 
                                              buffers);
                }
                if (rc == 0)
                {
                    RING("Got Session Id = %s", target->session_id);
                    buffers[0] = conn->connection_id;
                    rc = iscsi_win32_wait_for(&connection_id_regexp, 
                                              &error_regexp,
                                              &success_regexp,
                                              1, 1, 
                                              sizeof(conn->connection_id) \
                                              - 1, 
                                              buffers);
                }
                if (rc == 0)
                {
                    RING("Got Connection Id = %s", conn->connection_id);
                    rc = iscsi_win32_wait_for(&success_regexp, 
                                              &error_regexp, 
                                              NULL, 0, 0, 0, NULL);
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

/**
 * Detect iSCSI device name w/o using iscsicli.exe.
 *
 * @param target_id     Target ID
 * @param device_name   Buffer to hold iSCSI device name (OUT)
 * @return              Status code
 */
static te_errno
iscsi_win32_detect_device_interface_name(int target_id, char *device_name)
{
    static char                      buffer[1024];
    unsigned long                    buf_size;
    unsigned                         dev_index   = 0;
    unsigned                         index       = 0;
    unsigned long                    value_type;
    SP_DEVINFO_DATA                  drive_dev_info;
    SP_DEVICE_INTERFACE_DATA         intf;
    SP_DEVICE_INTERFACE_DETAIL_DATA *details;
    HDEVINFO                         disk_drives = INVALID_HANDLE_VALUE;

    RING("Trying to detect disk interfaces");

    disk_drives = SetupDiGetClassDevs(&guid_devinterface_disk, NULL, NULL, 
                                      DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (disk_drives == INVALID_HANDLE_VALUE)
    {
        ISCSI_WIN32_REPORT_ERROR();
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
    }


    for (dev_index = 0; target_id >= 0; dev_index++)
    {
        drive_dev_info.cbSize = sizeof(drive_dev_info);
        if (!SetupDiEnumDeviceInfo(disk_drives, dev_index, &drive_dev_info))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break;
            ISCSI_WIN32_REPORT_ERROR();
            return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }

        buf_size = sizeof(buffer);
        memset(buffer, 0, buf_size);
        if (!SetupDiGetDeviceRegistryProperty(disk_drives,
                                              &drive_dev_info,
                                              SPDRP_FRIENDLYNAME,
                                              &value_type,
                                              buffer, buf_size, &buf_size))
        {
            if (GetLastError() == ERROR_INVALID_DATA)
                continue;
            ISCSI_WIN32_REPORT_ERROR();
            return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
        }
        if (value_type != REG_SZ)
        {
            ERROR("Registry seems to be corrupted, very bad");
            return TE_RC(ISCSI_AGENT_TYPE, TE_ECORRUPTED);
        }

        if (strstr(buffer, "UNH") == NULL)
            continue;
        
        for (index = 0; target_id >= 0; index++, target_id--)
        {
            intf.cbSize = sizeof(intf);
            if (!SetupDiEnumDeviceInterfaces(disk_drives, &drive_dev_info,
                                             &guid_devinterface_disk, index, &intf))
            {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
                ISCSI_WIN32_REPORT_ERROR();
                return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
            }
        }
    }
    if (target_id >= 0)
    {
        RING("No devices detected yet");
        return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
    }

    SetupDiGetDeviceInterfaceDetail(disk_drives, &intf,
                                    NULL, 0, &buf_size, NULL);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        ISCSI_WIN32_REPORT_ERROR();
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);  
    }
    details = malloc(buf_size);
    if (details == NULL)
    {
        ERROR("Unable to allocate details buffer of length %u",
              (unsigned)buf_size);
        return TE_RC(ISCSI_AGENT_TYPE, TE_ENOMEM);
    }
    details->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    if (!SetupDiGetDeviceInterfaceDetail(disk_drives, &intf,
                                         details, buf_size, &buf_size, NULL))
    {
        ISCSI_WIN32_REPORT_ERROR();
        free(details);
        return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);            
    }
    RING("Detected interface name is %s", details->DevicePath);
    strncpy(device_name, details->DevicePath, ISCSI_MAX_DEVICE_NAME_LEN - 1);
    free(details);
    return 0;
}

/**
 * Probe for a Win32 SCSI device readiness and obtain its name.
 *
 * @return              Status code
 * @param conn          Connection data
 * @param target_id     Target ID
 */
te_errno
iscsi_win32_prepare_device(iscsi_connection_data_t *conn, int target_id)
{
    char *buffers[2];
    char  conn_id_first[ISCSI_SESSION_ID_LENGTH] = "0x";
    char  conn_id_second[ISCSI_SESSION_ID_LENGTH] = "-0x";
    char  drive_id[ISCSI_MAX_DEVICE_NAME_LEN];
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
                                  1, 2, ISCSI_SESSION_ID_LENGTH - 1, 
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
#if 0
    RING("Waiting for device name");
    rc = iscsi_win32_wait_for(&dev_name_regexp, 
                              &error_regexp,
                              &success_regexp,
                              1, 1, sizeof(drive_id) - 1,
                              buffers);
#endif
    RING("Waiting for legacy name");
    rc = iscsi_win32_wait_for(&legacy_dev_name_regexp, 
                              &error_regexp,
                              &success_regexp,
                              1, 1, sizeof(drive_id) - 1,
                              buffers);
    RING("iscsi_win32_wait_for() returns rc=%r (%d)", rc, rc);
                              
    iscsi_win32_finish_cli();
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENODATA)
        {
            RING("Call iscsi_win32_detect_device_interface_name()");
            rc = iscsi_win32_detect_device_interface_name(target_id, 
                                                          drive_id);
            if (rc != 0)
            {
                RING("iscsi_win32_detect_device_interface_name() "
                     "returns rc=%r (%d)", rc, rc);
                return rc;
            }
        }
        else
        {
            ERROR("Unable to find drive number: %r", rc);
            return rc;
        }
    }
    if (*drive_id == '\0')
    {
        ERROR("Device ID is not found");
        return TE_RC(ISCSI_AGENT_TYPE, TE_EAGAIN);
    }

    pthread_mutex_lock(&conn->status_mutex);
    strcpy(conn->device_name, drive_id);
    pthread_mutex_unlock(&conn->status_mutex);

    iscsi_win32_disable_readahead(conn->device_name);
    return 0;
}

/**
 * Compile iscsi_conditions into iscsi_regexps
 *
 * @return TRUE if successfully compiled, FALSE otherwise
 */
te_errno
iscsi_win32_init_regexps(void)
{
    unsigned i;
    int status;
    char err_buf[64];
    
    for (i = 0 ; i < TE_ARRAY_LEN(iscsi_conditions); i++)
    {
        status = regcomp(&iscsi_regexps[i], iscsi_conditions[i], 
                         REG_EXTENDED);
        if (status != 0)
        {
            regerror(status, NULL, err_buf, sizeof(err_buf) - 1);
            ERROR("Cannot compile regexp '%s': %s", 
                  iscsi_conditions[i], err_buf);
            return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);
        }
    }
    return 0;
}

#else

#include "te_config.h"
#include "package.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_iscsi.h"
#include "iscsi_initiator.h"

te_errno
iscsi_initiator_win32_set(iscsi_connection_req *req)
{
    UNUSED(req);
    return TE_RC(ISCSI_AGENT_TYPE, TE_ENOSYS);
}

#endif
