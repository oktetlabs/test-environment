/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */

/** @file
 * @brief TA core watcher
 *
 * A program which checks new core files and prints logs if
 * they are generated for TE binaries.
 */

/*
 * This is defined to make Linux-specific F_OFD* locks available.
 * With them file locks are inherited by child processes.
 */
#define _GNU_SOURCE

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

#include "te_errno.h"
#include "te_string.h"
#include "te_str.h"
#include "logger_api.h"
#include "logger_file.h"

/* Data retrieved from inotify descriptor */
typedef struct inotify_data {
    struct inotify_event event;
    char name[NAME_MAX];
} inotify_data;

/* Maximum length of core_pattern value */
#define CORE_PATTERN_LEN 1024

/* Storage for original core pattern */
static char orig_core_pattern[CORE_PATTERN_LEN] = "";
/* If @c true, core watcher should restore original core pattern at the end */
static bool restore_core_pattern = false;
/* File descriptor for te_core_pattern_lock file */
static int fd_lock = -1;

#define LOG_PREFIX "ta_core_watcher: "

/* The gdb command that gives the path to the binary file */
#define GDB_AUXV_CMD "info auxv"

/*
 * RegEx pattern for searching for a binary file from a string like this:
 * 31  AT_EXECFN   File name of executable   0x7ffc70ca2fe8 "/usr/sbin/nginx"
 */
#define GDB_AUXV_EXECFN_PATTERN ".*AT_EXECFN[ a-zA-Z0-9]*\"([^\"]*).*"

/* The match number containing the binary file (starting from 1) */
#define GDB_AUXV_EXECFN_MATCH_NUM 2

/*
 * Check value of an expression, goto to finish section if it
 * is negative.
 */
#define CHECK_RC(_expr) \
    do {                                                                    \
        rc = (_expr);                                                       \
        if (rc < 0)                                                         \
        {                                                                   \
            fprintf(stderr, LOG_PREFIX "'%s' failed, errno=%d (%s)\n",      \
                    #_expr, errno, strerror(errno));                        \
            goto finish;                                                    \
        }                                                                   \
    } while (0)

/*
 * Check value of an expression, if it is negative, set "rc"
 * variable to this value.
 */
#define FINISH_CHECK_RC(_expr) \
    do {                                                                  \
        int _rc = (_expr);                                                \
        if (_rc < 0)                                                      \
        {                                                                 \
            fprintf(stderr, LOG_PREFIX "'%s' on line %d failed, "         \
                    "errno=%d (%s)\n", #_expr, __LINE__,                  \
                    errno, strerror(errno));                              \
            rc = _rc;                                                     \
        }                                                                 \
    } while (0)

/* How long to wait if poll() returns unexpected results, in microseconds */
#define POLL_UNEXP_WAIT 20000

/* Acquire a lock via fcntl() */
static int
fcntl_lock(int fd, int cmd, int l_type)
{
    struct flock flk;

    memset(&flk, 0, sizeof(flk));
    flk.l_type = l_type;
    flk.l_whence = SEEK_SET;

    return fcntl(fd, cmd, &flk);
}

/*
 * Lock te_core_pattern_lock in a shared (read-only) way.
 * This function blocks until the lock is obtained.
 *
 * Every core watcher should hold at least a shared lock
 * until it terminates, to prevent other core watcher processes
 * from changing core pattern in unexpected way.
 */
static int
cp_shared_lock(int fd)
{
    return fcntl_lock(fd, F_OFD_SETLKW, F_RDLCK);
}

/*
 * Try to lock te_core_pattern_lock in an exclusive way. Return failure
 * immediately if this is not possible.
 */
static int
cp_try_exclusive_lock(int fd)
{
    return fcntl_lock(fd, F_OFD_SETLK, F_WRLCK);
}

/*
 * Lock te_core_pattern_lock in an exclusive way (blocks until the lock
 * is obtained).
 */
static int
cp_exclusive_lock(int fd)
{
    return fcntl_lock(fd, F_OFD_SETLKW, F_WRLCK);
}

/*
 * Unlock te_core_pattern_lock (should be called when core watcher
 * terminates).
 */
static int
cp_unlock(int fd)
{
    return fcntl_lock(fd, F_OFD_SETLKW, F_UNLCK);
}

/*
 * Check whether other core watchers are run by trying to obtain
 * exclusive lock on te_core_pattern_lock file. If it succeeds,
 * meaning that this core watcher is the only one, change system
 * core_pattern according to a parameter.
 *
 * Get a shared lock on te_core_pattern_lock file so that other
 * core watchers will not change core pattern in an unexpected way
 * while this one is running.
 *
 * If the provided core_pattern argument is empty string or this
 * program cannot change the current system core_pattern (because
 * another core watcher is running or this one is not run under root),
 * this function will retrieve the current core_pattern to the caller
 * via its argument.
 */
static int
lock_core_pattern(const char **core_pattern)
{
    int fd_cp = -1;
    ssize_t len;
    int rc = -1;
    bool change_pattern = false;
    bool root_user = (getuid() == 0);

    /*
     * This is done to make sure that lock file is accessible
     * to all users (i.e. S_IROTH and S_IWOTH are not masked out).
     */
    umask(0);

    fd_lock = open("/tmp/te_core_pattern_lock",
                   O_CREAT | O_RDWR,
                   S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
    if (fd_lock < 0 && errno == EACCES)
    {
        /*
         * I have no idea why this happens, but using O_CREAT flag may
         * lead to EACCES when file already exists but belongs to a
         * different user (whatever are file permissions) - even if
         * the current user is root.
         * This is really strange given that O_CREAT should only lead to
         * file creation if it does not exist.
         * I saw this issue on a host with 5.15.0-56-lowlatency kernel,
         * Ubuntu 22.04.2 LTS.
         */
        fd_lock = open("/tmp/te_core_pattern_lock",
                       O_RDWR, 0);
    }
    if (fd_lock < 0)
    {
        fprintf(stderr, "Failed to open core pattern lock file: %s\n",
                strerror(errno));
        goto finish;
    }

    /*
     * Firstly obtain shared lock to make sure that after that
     * core pattern will not change.
     *
     * If we firstly try to obtain exclusive lock and only then
     * obtain shared lock, there is a possibility that other
     * core watcher terminates and restores original core pattern
     * in between. Original core pattern may be a pipe with which
     * this program cannot work.
     */
    CHECK_RC(cp_shared_lock(fd_lock));

    if ((*core_pattern)[0] != '\0' && root_user)
    {
        rc = cp_try_exclusive_lock(fd_lock);
        if (rc >= 0)
            change_pattern = true;
    }

    CHECK_RC(fd_cp = open("/proc/sys/kernel/core_pattern",
                          root_user ? O_RDWR : O_RDONLY));
    CHECK_RC(len = read(fd_cp, orig_core_pattern,
                        sizeof(orig_core_pattern) - 1));
    orig_core_pattern[len] = '\0';

    if (change_pattern)
    {
        CHECK_RC(lseek(fd_cp, 0, SEEK_SET));
        CHECK_RC(ftruncate(fd_cp, 0));
        restore_core_pattern = true;
        CHECK_RC(write(fd_cp, *core_pattern, strlen(*core_pattern)));
    }
    else
    {
        *core_pattern = orig_core_pattern;
    }

    rc = 0;

finish:

    if (fd_cp >= 0)
        close(fd_cp);

    /*
     * If exclusive lock was obtained, convert it to shared now
     * so that other core watchers will not be blocked completely
     * but only forbidden to change core pattern.
     */
    if (change_pattern)
        FINISH_CHECK_RC(cp_shared_lock(fd_lock));

    return rc;
}

/*
 * If this core watcher is the one which changed original system
 * core pattern, make sure it is restored to original value.
 *
 * Unlock te_core_pattern_lock.
 */
static int
unlock_core_pattern(void)
{
    int fd_cp = -1;
    int rc = -1;

    if (fd_lock < 0)
        return 0;

    if (restore_core_pattern)
    {
        pid_t fork_rc = 0;

        /*
         * Try to obtain exclusive lock. If it fails, it means that
         * other core watchers are still running and core pattern
         * cannot be changed immediately.
         */
        rc = cp_try_exclusive_lock(fd_lock);
        if (rc < 0)
        {
            /*
             * Do fork() here if core pattern cannot be restored right now,
             * so that a child will keep waiting until other core watchers
             * terminate while the parent will terminate immediately, not
             * blocking RCF which started it.
             */
            fork_rc = fork();
        }

        if (rc >= 0 || fork_rc == 0)
        {
            if (rc < 0)
                CHECK_RC(cp_exclusive_lock(fd_lock));

            CHECK_RC(fd_cp = open("/proc/sys/kernel/core_pattern", O_RDWR));

            CHECK_RC(ftruncate(fd_cp, 0));
            CHECK_RC(write(fd_cp, orig_core_pattern,
                           strlen(orig_core_pattern)));
            CHECK_RC(close(fd_cp));
            fd_cp = -1;
        }
        else if (fork_rc < 0)
        {
            fprintf(stderr, "Failed to fork child process for restoring "
                    "core pattern: %s\n", strerror(errno));
        }
    }

    rc = 0;

finish:

    if (fd_cp >= 0)
        close(fd_cp);

    FINISH_CHECK_RC(cp_unlock(fd_lock));

    return rc;
}

/* Run a command, read its output and append it to TE string. */
ssize_t
read_from_cmd(te_string *dst, char *cmd, ...)
{
    FILE *f;
    size_t read_len;
    size_t total = 0;
    te_string cmd_str = TE_STRING_INIT;
    char buf[4096];

    va_list ap;

    va_start(ap, cmd);
    te_string_append_va(&cmd_str, cmd, ap);
    va_end(ap);

    f = popen(te_string_value(&cmd_str), "r");
    if (f == NULL)
    {
        fprintf(stderr, LOG_PREFIX "failed to run command with popen(): %s",
                te_string_value(&cmd_str));
        te_string_free(&cmd_str);
        return -1;
    }

    te_string_free(&cmd_str);

    while (1)
    {
        read_len = fread(buf, 1, sizeof(buf) - 1, f);
        if (read_len == 0)
            break;

        buf[read_len] = '\0';
        te_string_append(dst, "%s", buf);
        total += read_len;
    }

    pclose(f);

    return total;
}

int
main(int argc, const char *argv[])
{
    int inotify_fd = -1;
    int wd;
    int poll_num;
    struct pollfd fds[2];
    inotify_data data;

    const char *core_pattern = NULL;
    const char *ta_dir = NULL;
    const char *core_dir = NULL;

    char *p = NULL;
    char host_name[HOST_NAME_MAX];
    te_string out_str = TE_STRING_INIT;
    te_string log_str = TE_STRING_INIT;
    te_string core_dir_str = TE_STRING_INIT;
    char buf[1024];

    int rc = -1;

    regex_t execfn_re;
    regmatch_t pattern_match[GDB_AUXV_EXECFN_MATCH_NUM];
    regmatch_t *match = &pattern_match[GDB_AUXV_EXECFN_MATCH_NUM - 1];

    /*
     * This is done only for possible logs from tools library.
     * It is more convenient here to print to stdout/stderr than
     * to use logger macros; RCF will capture output and send it
     * to logger.
     */
    te_log_init("TA core watcher", te_log_message_file);

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s <core_pattern> [<binaries directory>]\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    core_pattern = argv[1];
    /*
     * If binaries directory is not passed, get cores dumped by binaries from
     * any directory.
     */
    if (argc == 3)
        ta_dir = argv[2];

    CHECK_RC(gethostname(host_name, sizeof(host_name)));

    CHECK_RC(lock_core_pattern(&core_pattern));

    if (core_pattern[0] == '|')
    {
        fprintf(stderr, "Cannot handle pipe in core pattern: %s\n",
                core_pattern);
        rc = -1;
        goto finish;
    }

    p = strrchr(core_pattern, '/');
    if (p == NULL)
    {
        /*
         * Unix TA changes its current directory to ./tmp inside its
         * directory soon after start. Create it here because core
         * watcher is started before TA; TA will ignore EEXIST
         * when trying to create this directory itself.
         */
        te_string_append(&core_dir_str, "%s/tmp/", ta_dir);
        core_dir = te_string_value(&core_dir_str);
        if (mkdir(core_dir, S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX) != 0 &&
                  errno != EEXIST)
        {
            fprintf(stderr, LOG_PREFIX "cannot create '%s': %d (%s)\n",
                    core_dir, errno, strerror(errno));
            rc = -1;
            goto finish;
        }
    }
    else
    {
        te_string_append(&core_dir_str, "%s", core_pattern);
        te_string_cut(&core_dir_str, core_dir_str.len - (p - core_pattern));
        core_dir = te_string_value(&core_dir_str);
    }

    CHECK_RC(inotify_fd = inotify_init());

    wd = inotify_add_watch(inotify_fd, core_dir, IN_CLOSE_WRITE);
    if (wd < 0)
    {
        fprintf(stderr, LOG_PREFIX "cannot watch '%s': %d (%s)\n",
                core_dir, errno, strerror(errno));
        rc = -1;
        goto finish;
    }

    fds[0].fd = inotify_fd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLERR | POLLHUP | POLLIN;

    rc = regcomp(&execfn_re, GDB_AUXV_EXECFN_PATTERN, REG_EXTENDED);
    if (rc != 0)
    {
        fprintf(stderr, LOG_PREFIX "failed to compile regular expression '%s' "
                "(err: %d)", GDB_AUXV_EXECFN_PATTERN, rc);
        goto finish;
    }

    /*
     * This log allows RCF to know that core watcher started
     * its work and now TA can be started.
     */
    printf(LOG_PREFIX "started; expect core files in %s\n",
           core_dir);
    fflush(stdout);

    /*
     * In this loop we poll inotify to know immediately about
     * any new files added in the directory determined by the current
     * core_pattern value. Every new file is checked by passing
     * it to "gdb -c". If GDB tells that this is a core file generated
     * by some binary inside TA directory, then we obtain and print
     * to stdout gdb backtrace for it.
     */
    while (1)
    {
        char cored_name[1024];
        char *p = NULL;
        ssize_t len;

        poll_num = poll(fds, 2, -1);
        if (poll_num < 1)
        {
            if (errno == EFAULT || errno == EINVAL)
            {
                fprintf(stderr, "poll() unexpectedly failed with "
                        "error %d (%s)\n", errno, strerror(errno));
                rc = -1;
                goto finish;
            }

            /*
             * Do not stop the program if there is a problem with
             * poll(), but sleep for a while to avoid CPU loading.
             */
            usleep(POLL_UNEXP_WAIT);
            continue;
        }

        if (fds[0].revents & POLLIN)
        {
            len = read(inotify_fd, &data, sizeof(data));
            if (len < 0)
            {
                if (errno == EINTR)
                    continue;

                fprintf(stderr,
                        LOG_PREFIX "read() from inotify FD failed: "
                        "%d (%s)\n", errno, strerror(errno));
                rc = -1;
                goto finish;
            }

            /*
             * Check that this is a core file generated by one
             * of the binaries in TA directory.
             */

            te_string_reset(&out_str);
            len = read_from_cmd(
                    &out_str,
                    "gdb --batch --ex '" GDB_AUXV_CMD "' -c %s/%s 2>/dev/null",
                    core_dir, data.event.name);

            if (len > 0)
            {
                /*
                 * If binaries directory is not passed to Core watcher, ignore
                 * TA directory binaries check and get binaries directory name
                 * from GDB output.
                 */

                rc = regexec(&execfn_re, out_str.ptr,
                             GDB_AUXV_EXECFN_MATCH_NUM, pattern_match, 0);
                if (rc == 0 && match->rm_so > 0 && match->rm_eo > 0)
                {
                    /*
                     * Get path to the core generating binary from
                     * GDB output and obtain backtrace for it.
                     */

                    out_str.ptr[match->rm_eo] = '\0';
                    TE_SPRINTF(cored_name, "%s", out_str.ptr + match->rm_so);

                    if (*cored_name != '/')
                    {
                        /*
                         * If gdb does not return absolute path, try
                         * to find it in core dump. gdb must get
                         * path to the binary which was actually run
                         * to produce correct backtrace for a core dump.
                         * And we need absolute path to check whether
                         * cored binary was located in TA directory if
                         * that directory was provided.
                         */

                        te_string_reset(&out_str);
                        len = read_from_cmd(
                                &out_str,
                                "strings '%s/%s' | grep '/%s$' -m 1",
                                core_dir, data.event.name, cored_name);
                        if (len > 0)
                        {
                            TE_SPRINTF(cored_name, "%s",
                                       te_string_value(&out_str));
                            for (p = cored_name; *p != '\0'; p++)
                            {
                                if (isspace(*p))
                                {
                                    *p = '\0';
                                    break;
                                }
                            }
                        }
                    }

                    if (ta_dir != NULL)
                    {
                        if (strstr(cored_name, ta_dir) != cored_name)
                            continue;
                    }

                    te_string_reset(&log_str);
                    te_string_append(
                               &log_str,
                               "On host %s '%s' terminated dumping a core "
                               "file '%s/%s'\n",
                               host_name, cored_name, core_dir,
                               data.event.name);

                    te_string_append(&log_str, "\nGDB output:\n");
                    len = read_from_cmd(
                            &log_str,
                            "gdb -batch -ex \"echo Backtrace:\n\" "
                            "-ex \"thread apply all bt\" -c %s/%s %s 2>&1",
                            core_dir, data.event.name, cored_name);

                    printf("%s\n", te_string_value(&log_str));
                    fflush(stdout);
                }
            }
        }

        if (fds[1].revents != 0)
        {
            /*
             * This allows RCF to terminate this program
             * gracefully collecting any last logs.
             */

            if (fds[1].revents != POLLIN)
                break;

            len = read(STDIN_FILENO, buf, sizeof(buf));
            if (len == 0 || (len > 0 && buf[0] == 'q'))
                break;
        }
    }

    rc = 0;

finish:

    FINISH_CHECK_RC(unlock_core_pattern());

    if (inotify_fd >= 0)
        FINISH_CHECK_RC(close(inotify_fd));

    if (fd_lock >= 0)
        FINISH_CHECK_RC(close(fd_lock));

    te_string_free(&out_str);
    te_string_free(&log_str);
    te_string_free(&core_dir_str);

    if (rc < 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
