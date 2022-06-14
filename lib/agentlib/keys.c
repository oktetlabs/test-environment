/** @file
 * @brief Agent library
 *
 * Key management routines
 *
 *
 * Copyright (C) 2003-2022 OKTET Labs. All rights reserved.
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 */

#define TE_LGR_USER      "Agent library"

#include "agentlib.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_str.h"
#include "te_exec_child.h"

#include "logger_api.h"

#define SSH_KEYGEN_NAME "ssh-keygen"

static uid_t
get_user_uid(const char *name)
{
#ifndef HAVE_PWD_H
    UNUSED(name);
    return (uid_t)-1;
#else
#ifdef HAVE_PWNAM_R
    struct passwd pwd_storage;
    /*
     * The right way implies using sysconf() and
     * reallocating if needed, but this looks like
     * a sufficient value for all reasonable cases
     */
    char strbuf[256];
#endif
    struct passwd *pwd;

    if (name == NULL)
        return (uid_t)-1;

#ifdef HAVE_PWNAM_R
    errno = getpwnam_r(name, &pwd_storage, strbuf, sizeof(strbuf), &pwd);
#else
    pwd = getpwnam(name);
#endif
    if (pwd == NULL)
    {
        ERROR("User '%s' not found: %r, using the current one",
              TE_OS_RC(TE_TA_UNIX, errno));
        return (uid_t)-1;
    }

    return pwd->pw_uid;
#endif
}

te_errno
agent_key_generate(agent_key_manager manager, const char *type,
                   unsigned bitsize, const char *user,
                   const char *private_key_file)
{
    uid_t uid;
    pid_t pid;
    te_errno rc;
    int status;
    char bitsize_fmt[16];

    if (type == NULL || private_key_file == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (manager != AGENT_KEY_MANAGER_SSH)
        return TE_RC(TE_TA_UNIX, TE_EPROTONOSUPPORT);

    uid = get_user_uid(user);
    TE_SPRINTF(bitsize_fmt, "%u", bitsize);

    /*
     * We need to remove an old key file if it exists,
     * because there is no simple way not to ask ssh-keygen
     * for overwrite confirmation
     */
    remove(private_key_file);

    /*
     * The key is generated with an empty passphrase,
     * because it is going to be used non-interactively.
     * There is no real security issue here, as we're
     * basically working in a controlled environment
     */
    pid = te_exec_child(SSH_KEYGEN_NAME,
                        (char *const[]){
                            SSH_KEYGEN_NAME,
                            "-t", TE_CONST_PTR_CAST(char, type),
                            "-b", bitsize_fmt,
                            "-N", "", "-q",
                            "-f", TE_CONST_PTR_CAST(char, private_key_file),
                            NULL
                        }, NULL,
                        uid,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        TE_EXEC_CHILD_DEV_NULL_FD,
                        NULL);
    if (pid < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Cannot start %s: %r", SSH_KEYGEN_NAME, rc);
        return rc;
    }

    if (ta_waitpid(pid, &status, 0) < 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Error waiting for %s: %r", SSH_KEYGEN_NAME, rc);
        return rc;
    }

    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) != 0)
        {
            ERROR("%s terminated abnormally with status = %d",
                  SSH_KEYGEN_NAME, WEXITSTATUS(status));
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);
        }
    }
    else
    {
        assert(WIFSIGNALED(status));
        ERROR("%s killed by signal %d", SSH_KEYGEN_NAME,
              WTERMSIG(status));
    }

    if (access(private_key_file, F_OK) != 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("%s was successful, but %s does not exist: %r",
              SSH_KEYGEN_NAME, private_key_file, rc);
    }

    return 0;
}
