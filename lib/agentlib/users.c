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
    struct pam_response *resp_array = TE_ALLOC(num_msg * sizeof(*resp));
    appdata_t           *appdata    = data;

    int      i;
    unsigned full_len = strlen(appdata->passwd) + 1; /**< Password
                                                       *  length + 1
                                                       */

    for (i = 0; i < num_msg; i++) /* Process each message */
    {
        /** PAM prompts for password */
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_ON ||
                msg[i]->msg_style == PAM_PROMPT_ECHO_OFF)
        {
            /** Allocate memory for password and supply it to PAM */
            resp_array[i].resp = TE_ALLOC(full_len);
            memcpy(resp_array[i].resp, appdata->passwd, full_len);
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
    FILE *f;
    char buf[4096];
    char trash[128];
    char *s = buf;

    if ((f = fopen("/etc/passwd", "r")) == NULL)
    {
        te_errno rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Failed to open file /etc/passwd; errno %r", rc);
        return rc;
    }

    buf[0] = 0;

    while (fgets(trash, sizeof(trash), f) != NULL)
    {
        char *tmp = strstr(trash, TE_USER_PREFIX);
        char *tmp1;

        unsigned int uid;

        if (tmp == NULL)
            continue;

        tmp += strlen(TE_USER_PREFIX);
        uid = strtol(tmp, &tmp1, 10);
        if (tmp1 == tmp || *tmp1 != ':')
            continue;
        s += sprintf(s, TE_USER_PREFIX "%u", uid);
    }
    fclose(f);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/* See description in agentlib.h */
te_errno
ta_user_add(const char *user)
{
    char buf[4096];
#if TA_USE_PAM || defined(__linux__)
    char *tmp;
    char *tmp1;

    unsigned int uid;

    te_errno     rc;
#endif

#if !TA_USE_PAM && !defined(__linux__)
    UNUSED(user);
    ERROR("user_add failed (no user management facilities available)");
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#else
    if (user_exists(user))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (strncmp(user, TE_USER_PREFIX, strlen(TE_USER_PREFIX)) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    tmp = (char *)user + strlen(TE_USER_PREFIX);
    uid = strtol(tmp, &tmp1, 10);
    if (tmp == tmp1 || *tmp1 != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    /*
     * We manually add group to be independent from system settings
     * (one group for all users / each user with its group)
     * "-f" is used in order not to fail if such group already exists (bug 11813)
     */
    sprintf(buf, "/usr/sbin/groupadd -f -g %u %s ", uid, user);
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("\"%s\" command failed with %d", buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    sprintf(buf, "/usr/sbin/useradd -d /tmp/%s -g %u -u %u -m %s ",
            user, uid, uid, user);
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("\"%s\" command failed with %d", buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

#if 0
    /* Fedora has very aggressive nscd cache */
    /* https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=134323 */
    ta_system("/usr/sbin/nscd -i group && /usr/sbin/nscd -i passwd");
#endif

#if TA_USE_PAM
    /** Set (change) password for just added user */
    if (set_change_passwd(user, user) != 0)
#else
    sprintf(buf, "echo %s:%s | /usr/sbin/chpasswd", user, user);
    if ((rc = ta_system(buf)) != 0)
#endif
    {
        ERROR("change_passwd failed");
        ta_user_del(user);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

#if 0
    /* Fedora has very aggressive nscd cache */
    /* https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=134323 */
    ta_system("/usr/sbin/nscd -i group && /usr/sbin/nscd -i passwd");
#endif


    TE_SPRINTF(buf, "/tmp/%s/.ssh/id_ed25519", user);
    rc = agent_key_generate(AGENT_KEY_MANAGER_SSH, "ed25519", 1024, user, buf);
    if (rc != 0)
    {
        ERROR("Cannot create ssh key: %r", rc);
        ta_user_del(user);
        return rc;
    }

    return 0;
#endif /* !TA_USE_PAM */
}

/* See description in agentlib.h */
te_errno
ta_user_del(const char *user)
{
    char buf[4096];
    te_errno rc;

    if (!user_exists(user))
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    sprintf(buf, "/usr/bin/killall -u %s", user);
    ta_system(buf); /* Ignore rc */
    sprintf(buf, "/usr/sbin/userdel -r %s", user);
    if ((rc = ta_system(buf)) != 0)
    {
        ERROR("\"%s\" command failed with %d", buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    sprintf(buf, "/usr/sbin/groupdel %s", user);
    if ((rc = ta_system(buf)) != 0)
    {
        /* Yes, we ignore rc, as group may be deleted by userdel */
        VERB("\"%s\" command failed with %d", buf, rc);
    }

    /* Fedora has very aggressive nscd cache */
    /* https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=134323 */
    ta_system("/usr/sbin/nscd -i group && /usr/sbin/nscd -i passwd");

    return 0;
}
