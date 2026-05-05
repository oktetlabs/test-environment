/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Agent library
 *
 * User management routines
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER      "Agent library"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <grp.h>
#include <pwd.h>

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "agentlib.h"
#include "te_alloc.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_exec_child.h"
#include "te_str.h"
#include "te_string.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logger_ta_lock.h"

#define DEFAULT_GETPW_R_SIZE_MAX 16384

/* PAM (Pluggable Authentication Modules) support */
#if defined(HAVE_SECURITY_PAM_APPL_H) && defined(HAVE_LIBPAM)
#include <security/pam_appl.h>

#define TA_USE_PAM  1

/** Data passed between 'set_change_passwd' and 'conv_fun' callback fun */
typedef struct {
    char const *passwd;                    /**< Password string pointer */
    char        err_msg[PAM_MAX_MSG_SIZE]; /**< Error message storage   */

} appdata_t;

typedef struct pam_response pam_response_t;

/** Avoid slight differences between UNIX'es over typedef */
#if defined __linux__
#define PAM_FLAGS 0
typedef struct pam_message const pam_message_t;
#elif defined __sun__
#define PAM_FLAGS (PAM_NO_AUTHTOK_CHECK | PAM_SILENT)
typedef struct pam_message pam_message_t;
#elif defined __FreeBSD__ || defined __NetBSD__
#define PAM_FLAGS PAM_SILENT
typedef struct pam_message const pam_message_t;
#endif

#else

#define TA_USE_PAM  0

#endif /* HAVE_SECURITY_PAM_APPL_H && HAVE_LIBPAM */

static pthread_mutex_t pwent_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Check, if user with the specified name exists.
 *
 * @param user          user name
 *
 * @return              @c true if user exists, @c false if does not
 */
static bool
user_exists(const char *user)
{
    return getpwnam(user) != NULL ? true : false;
}

/**
 * Check PID returned by @b te_exec_child().
 *
 * @param pid           pid
 * @param cmdline       commandline
 * @param check_result  Should we check the result
 *
 * @return              Status code
 */
static te_errno
check_pid(pid_t pid, const char *cmdline, bool check_result)
{
    te_errno rc = 0;
    int status;

    if (pid < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Cannot start %s: %r", cmdline, rc);
        return rc;
    }

    if (ta_waitpid(pid, &status, 0) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Error waiting for %s: %r", cmdline, rc);
        return rc;
    }

    if (WIFEXITED(status))
    {
        if (check_result && WEXITSTATUS(status) != 0)
        {
            ERROR("%s terminated abnormally with status = %d",
                  cmdline, WEXITSTATUS(status));
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
    }
    else
    {
        assert(WIFSIGNALED(status));
        ERROR("%s killed by signal %d", cmdline,
              WTERMSIG(status));
    }

    return 0;
}

/* See description in agentlib.h */
te_errno
ta_te_username_is_numeric(const char *username, bool *is_num)
{
    const char *tmp;
    bool is_numeric = false;

    if (is_num == NULL)
        is_num = &is_numeric;

    tmp = te_str_strip_prefix(username, TE_USER_PREFIX);
    if (tmp == NULL || *tmp == '\0')
        return TE_RC(TE_TAPI, TE_EINVAL);

    for (*is_num = true; *tmp != '\0'; tmp++)
    {
        if (!isdigit(*tmp))
        {
            *is_num = false;
            break;
        }
    }

    if (*is_num)
        return 0;

    tmp = te_str_strip_prefix(tmp, TE_USER_NONNUM_AFFIX);
    if (tmp == NULL || *tmp == '\0')
        return TE_RC(TE_TAPI, TE_EINVAL);

    for (; *tmp != '\0'; tmp++)
    {
        if (!isalnum(*tmp) && *tmp != '_')
            return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}

#if TA_USE_PAM
/**
 * Callback function provided by user and called from within PAM library.
 *
 * @param num_msg       number of messages
 * @param msg           array of 'num_msg' pointers to messages
 * @param resp          address of pointer to returned array of responses
 * @param data          pointer passed to PAM library pam_start function
 *
 * @return              Return code (PAM_SUCCESS on success,
 *                      PAM_BUF_ERR when it is insufficient memory)
 *
 * @sa                  PAM library expects that response array
 *                      itself and each its .resp member are allocated
 *                      by malloc (calloc, realloc).
 *                      PAM library is responsible for freeing them.
 */
static int
conv_fun(int num_msg, pam_message_t **msg, pam_response_t **resp,
         void *data)
{
    /* Try to allocate responses array to be returned */
    struct pam_response *resp_array = calloc(num_msg, sizeof(*resp));
    appdata_t           *appdata    = data;

    int      i;
    unsigned full_len = strlen(appdata->passwd) + 1; /**< Password
                                                       *  length + 1
                                                       */

    /** If responses array is allocated successfully */
    if (resp_array != NULL)
    {
        for (i = 0; i < num_msg; i++) /* Process each message */
        {
            /** PAM prompts for password */
            if (msg[i]->msg_style == PAM_PROMPT_ECHO_ON ||
                msg[i]->msg_style == PAM_PROMPT_ECHO_OFF)
            {
                /** Allocate memory for password and supply it to PAM */
                if ((resp_array[i].resp = malloc(full_len)) != NULL)
                    memcpy(resp_array[i].resp, appdata->passwd, full_len);
                else
                {
                   /* Rollback allocation already
                    * been done at the moment
                    */
                    while (i-- > 0)
                        free(resp_array[i].resp);

                    free(resp_array);
                    return PAM_BUF_ERR;
                }
            }
            else
                /** PAM assumes user should read this error message */
                if (msg[i]->msg_style == PAM_ERROR_MSG)
                {
                    WARN("%s", msg[i]->msg);

                   /* Save message in order to have opportunity
                    * to display it later by main execution flow
                    * (set_change_passwd) in case of a real error
                    */
                    strcpy(appdata->err_msg, msg[i]->msg);
                }
        }

        *resp = resp_array; /* Assign responses array pointer for PAM */
    }
    else
        return PAM_BUF_ERR;

    return PAM_SUCCESS;
}

/**
 * Set (change) user password over PAM (i. e. portably across UNIX'es).
 *
 * @param user          user name
 * @param passwd        user password
 *
 * @return              Return code (0 on success, -1 on error)
 */
static int
set_change_passwd(char const *user, char const *passwd)
{
    pam_handle_t       *handle;
    appdata_t           appdata;  /**< Data passed to callback and back */
    struct pam_conv     conv;     /**< Callback structure */

    int pam_rc;
    int rc = -1;

    appdata.passwd     = passwd;
    appdata.err_msg[0] = '\0';

    conv.conv        = &conv_fun; /**< callback function */
    conv.appdata_ptr = &appdata;  /**< data been passed to callback fun */

    /** Check user existence */
    if(getpwnam(user) != NULL)
    {
        /** Initialize PAM library */
        if ((pam_rc = pam_start("passwd", user, &conv, &handle))
            == PAM_SUCCESS)
        {
            uid_t euid = geteuid(); /**< Save current effective user id */

            if (euid == 0 || setuid(0) == 0)     /**< Get 'root' */
            {
                /** Try to set/change password */
                if ((pam_rc = pam_chauthtok(handle, PAM_FLAGS))
                    == PAM_SUCCESS)
                    rc = 0;
                else
                {
                    ERROR("pam_chauthtok, user: '%s', passwd: '%s': %s",
                          user, passwd, pam_strerror(handle, pam_rc));

                   /* If callback function received error message string
                    * then type it too
                    */
                    if (appdata.err_msg[0])
                        ERROR("%s", appdata.err_msg);
                }

                if (euid != 0)
                    setuid(euid);   /* Restore saved previously user id */
            }
            else
                ERROR("setuid: %s", strerror(errno));

            /** Terminate PAM library */
            if ((pam_rc = pam_end(handle, pam_rc)) != PAM_SUCCESS)
                ERROR("pam_end: %s", pam_strerror(handle, pam_rc));
        }
        else
            ERROR("pam_start, user: '%s', passwd: '%s': %s", user, passwd,
                 pam_strerror(handle, pam_rc));
    }
    else
        ERROR("getpwnam, user '%s': %s",
              user, errno ? strerror(errno) : "User does not exist");

    return rc;
}
#endif /* TA_USE_PAM */

/* See description in agentlib.h */
te_errno
ta_user_list(char **list)
{
    struct passwd pwd;
    struct passwd *pwd_result;
    long pwd_bufsize;
    char *pwd_buf;
    int getpwent_res;
    te_string str = TE_STRING_INIT;

    pwd_bufsize= sysconf(_SC_GETPW_R_SIZE_MAX);
    if (pwd_bufsize == -1)
        pwd_bufsize = DEFAULT_GETPW_R_SIZE_MAX;

    pwd_buf =  TE_ALLOC(pwd_bufsize);

    pthread_mutex_lock(&pwent_mutex);

    setpwent();

    while (1)
    {
        while ((getpwent_res = getpwent_r(&pwd, pwd_buf, pwd_bufsize,
                                          &pwd_result)) == ERANGE)
        {
            pwd_bufsize *= 2;
            TE_REALLOC(pwd_buf, pwd_bufsize);
        }

        if (pwd_result == NULL)
        {
            /* sometimes getpwent_r() finishes with ENOENT */
            if (getpwent_res == 0 || getpwent_res == ENOENT)
            {
                break;
            }
            else
            {
                ERROR("Failed to get info using getpwent_r(): %s",
                      strerror(getpwent_res));
                break;
            }
        }

        if (ta_te_username_is_numeric(pwd.pw_name, NULL) != 0)
            continue;

        if (str.len != 0)
            te_string_append(&str, " ");

        te_string_append(&str, pwd.pw_name);
    }

    endpwent();
    pthread_mutex_unlock(&pwent_mutex);

    free(pwd_buf);
    *list = str.ptr;

    return 0;
}

/* See description in agentlib.h */
te_errno
ta_user_gid_uid_get(gid_t *gid, uid_t *uid, const char *user)
{
    struct passwd pwd;
    struct passwd *pwd_result;
    long pwd_bufsize;
    char *pwd_buf;
    int getpwnam_res;
    bool user_is_num;
    gid_t gid_temp;
    uid_t uid_temp;

    if (gid == NULL)
        gid = &gid_temp;

    if (uid == NULL)
        uid = &uid_temp;

    if (ta_te_username_is_numeric(user, &user_is_num) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    pwd_bufsize= sysconf(_SC_GETPW_R_SIZE_MAX);
    if (pwd_bufsize == -1)
        pwd_bufsize = DEFAULT_GETPW_R_SIZE_MAX;

    pwd_buf =  TE_ALLOC(pwd_bufsize);

    while ((getpwnam_res = getpwnam_r(user, &pwd, pwd_buf, pwd_bufsize,
                                      &pwd_result)) == ERANGE)
    {
        pwd_bufsize *= 2;
        TE_REALLOC(pwd_buf, pwd_bufsize);
    }
    if (getpwnam_res != 0)
    {
        ERROR("User %s is not found using getpwnam_r(): %s", user,
              strerror(getpwnam_res));
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    *gid = pwd.pw_gid;
    *uid = pwd.pw_uid;

    free(pwd_buf);

    if (user_is_num)
    {
        const char *id_str;
        unsigned int id_from_username;

        id_str = te_str_strip_prefix(user, TE_USER_PREFIX);
        if (te_strtoui(id_str, 10, &id_from_username) != 0 ||
            *gid != (gid_t)id_from_username ||
            *uid != (uid_t)id_from_username)
        {
            WARN("User %s unexpectedly has GID=%u, UID=%u", user,
                 (unsigned int)*gid, (unsigned int)*uid);
        }
    }

    return 0;
}

/* See description in agentlib.h */
te_errno
ta_user_add(const char *user)
{
    const char *uid_str;
    char * const *argv;
    char buf[4096];
#define MAX_HOMEDIR_LEN 256
    char homedir[MAX_HOMEDIR_LEN];
#undef MAX_HOMEDIR_LEN
    pid_t pid;

    te_errno     rc;
    bool user_is_num;

    if (ta_te_username_is_numeric(user, &user_is_num) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (user_exists(user))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (user_is_num)
    {
        uid_str = te_str_strip_prefix(user, TE_USER_PREFIX);

        /*
         * We manually add group to be independent from system settings
         * (one group for all users / each user with its group)
         * "-f" is used in order not to fail if such group already exists (bug 11813)
         */
        argv = (char * const[]){"/usr/sbin/groupadd",
                                "-f",
                                "-g", TE_CONST_PTR_CAST(char, uid_str),
                                TE_CONST_PTR_CAST(char, user),
                                NULL};
        pid = te_exec_child("/usr/sbin/groupadd", argv, NULL, -1,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            NULL);
        rc = check_pid(pid, "/usr/sbin/groupadd", true);
        if (rc != 0)
            return rc;

        TE_SPRINTF(homedir, "/tmp/%s", user);
        argv = (char *const[]){"/usr/sbin/useradd",
                               "-d", homedir,
                               "-g", TE_CONST_PTR_CAST(char, uid_str),
                               "-u", TE_CONST_PTR_CAST(char, uid_str),
                               "-m",
                               TE_CONST_PTR_CAST(char, user),
                               NULL};
        pid = te_exec_child("/usr/sbin/useradd", argv, NULL, -1,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            NULL);
        rc = check_pid(pid, "/usr/sbin/useradd", true);
        if (rc != 0)
            return rc;
    }
    else
    {
#if ADDUSER_IS_DEBIAN_LIKE
        TE_SPRINTF(homedir, "/tmp/%s", user);

        argv = (char *const[]){"/usr/sbin/adduser",
                               "--home", homedir,
                               "--disabled-password",
                               "--gecos", "\"\"",
                               TE_CONST_PTR_CAST(char, user),
                               NULL};
        pid = te_exec_child("/usr/sbin/adduser", argv, NULL, -1,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            NULL);

        rc = check_pid(pid, "/usr/sbin/useradd", true);
        if (rc != 0)
            return rc;
#else
        struct group gr;
        struct group *gr_result = NULL;
        int getgrnam_res;
        char *gr_buf = NULL;
        size_t gr_bufsize;
#define MAX_USER_GID_LEN 16
        char gid_str[MAX_USER_GID_LEN];
#undef MAX_USER_GID_LEN

        argv = (char *const[]){"/usr/sbin/groupadd",
                                "-f",
                                TE_CONST_PTR_CAST(char, user),
                                NULL};
        pid = te_exec_child("/usr/sbin/groupadd", argv, NULL, -1,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            NULL);

        rc = check_pid(pid, "/usr/sbin/groupadd", true);
        if (rc != 0)
            return rc;

        gr_bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
        if (gr_bufsize == -1)
#define DEFAULT_GETGR_R_SIZE_MAX 1024
            gr_bufsize = DEFAULT_GETGR_R_SIZE_MAX;
#undef DEFAULT_GETGR_R_SIZE_MAX

        gr_buf =  TE_ALLOC(gr_bufsize);
        getgrnam_res = getgrnam_r(user, &gr, gr_buf, gr_bufsize, &gr_result);
        while (getgrnam_res != 0)
        {
            gr_bufsize *= 2;
            TE_REALLOC(gr_buf, gr_bufsize);
            getgrnam_res = getgrnam_r(user, &gr, gr_buf, gr_bufsize, &gr_result);
        }

        if (gr_result == NULL)
        {
            ERROR("Failed to get info of group %s using getgrnam_r()", user);
            free(gr_buf);
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
        else
        {
            TE_SPRINTF(gid_str, "%u", (unsigned int)gr.gr_gid);
        }
        free(gr_buf);

        TE_SPRINTF(homedir, "/tmp/%s", user);

        argv = (char *const[]){"/usr/sbin/useradd",
                               "-d", homedir,
                               "-g", gid_str,
                               "-m",
                               TE_CONST_PTR_CAST(char, user),
                               NULL};
        pid = te_exec_child("/usr/sbin/useradd", argv, NULL, -1,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            TE_EXEC_CHILD_DEV_NULL_FD,
                            NULL);

        rc = check_pid(pid, "/usr/sbin/useradd", true);
        if (rc != 0)
            return rc;
#endif
    }

#if TA_USE_PAM
    /** Set (change) password for just added user */
    if (set_change_passwd(user, user) != 0)
#else
    TE_SPRINTF(buf, "echo %s:%s | /usr/sbin/chpasswd", user, user);
    if ((rc = ta_system(buf)) != 0)
#endif
    {
        ERROR("change_passwd failed");
        ta_user_del(user);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    TE_SPRINTF(buf, "/tmp/%s/.ssh/id_ed25519", user);
    rc = agent_key_generate(AGENT_KEY_MANAGER_SSH, "ed25519", 1024, user, buf);
    if (rc != 0)
    {
        ERROR("Cannot create ssh key: %r", rc);
        ta_user_del(user);
        return rc;
    }

    return 0;
}

/* See description in agentlib.h */
te_errno
ta_user_del(const char *user)
{
    te_errno rc = 0;
    pid_t pid;
    char * const *argv;

    if (ta_te_username_is_numeric(user, NULL) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (!user_exists(user))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    argv = (char * const[]){"/usr/bin/killall",
                            "-u", TE_CONST_PTR_CAST(char, user),
                            NULL};
    pid = te_exec_child("/usr/bin/killall", argv, NULL, -1,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        NULL);
    /* Ignore result */
    rc = check_pid(pid, "/usr/bin/killall", false);
    if (rc != 0)
        return rc;

    argv = (char * const[]){"/usr/sbin/userdel",
                            "-r", TE_CONST_PTR_CAST(char, user),
                            NULL};
    pid = te_exec_child("/usr/sbin/userdel", argv, NULL, -1,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        NULL);
    rc = check_pid(pid, "/usr/sbin/userdel", true);
    if (rc != 0)
        return rc;

    argv = (char * const[]){"/usr/sbin/groupdel",
                            TE_CONST_PTR_CAST(char, user),
                            NULL};
    pid = te_exec_child("/usr/sbin/groupdel", argv, NULL, -1,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        NULL);
    /* we ignore result, as group may be deleted by userdel */
    rc = check_pid(pid, "/usr/sbin/groupdel", false);

    return rc;
}
