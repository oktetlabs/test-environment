/** @file
 * @brief Simple stdout/stderr redirection in two files
 *
 * Redirection of stdout/stderr in two files: stdout merged with stderr,
 * stderr only.
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_FCNTL_H
#define __USE_GNU
#include <fcntl.h>
#endif

#if 1 /* HAVE_LOGGER_API_H */
#define TE_LGR_USER     "StdOutErr"
#include "logger_api.h"
DEFINE_LGR_ENTITY("Builder");
#else
#define ERROR(x...)     fprintf(stderr, x)
#endif


/** Index of pipe-in file descriptor in pipe fd's array */
#define PIPE_IN     1
/** Index of pipe-out file descriptor in pipe fd's array */
#define PIPE_OUT    0

/** 
 * Macro to check expression and return with EXIT_FAILURE,
 * if it's not true.
 */
#define CHECK(x) \
    do {                                            \
        if (!(x))                                   \
        {                                           \
            ERROR(#x ": %s\n", strerror(errno));    \
            return EXIT_FAILURE;                    \
        }                                           \
    } while (0)


/** Is child run? */
static int child_run = 0;
/** Exit status of the child */
static int status = EXIT_FAILURE;
/** Temporary buffer */
static char buf[4096];

/**
 * SIGCHLD handler.
 *
 * @param sig       Signal number (unused)
 */
static void
sigchld_handler(int sig)
{
    pid_t   pid;

    (void)sig;

    child_run = 0;

    pid = wait(&status);
}

/**
 * Entry point.
 *
 * @param argc      Number of arguments including program name
 * @param argv      Array with arguments
 *
 * @return Exist status of run child or EXIT_FAILURE.
 */
int
main(int argc, char *const argv[])
{
    const char     *out_err_file;
    const char     *err_file;
    const char     *cmd;
    int             rc;
    int             outpipe[2];
    int             errpipe[2];
    fd_set          fds;
    struct timeval  tv;
    int             out_err_fd, err_fd;
    ssize_t         r;
    
    
    if (argc < 4)
    {
        ERROR("USAGE: te_stdouterr <stdout+stderr file> <stderr file> "
              "<cmd> <args> ...\n");
        return EXIT_FAILURE;
    }
    out_err_file = argv[1];
    err_file     = argv[2];
    cmd          = argv[3];
    argc -= 3;
    argv += 3;


    /* Open file for stdout+stderr and set close-on-exec flag */
    CHECK((out_err_fd = open(out_err_file, O_CREAT | O_TRUNC | O_WRONLY,
                                           S_IRUSR | S_IWUSR)) >= 0);
    CHECK(fcntl(out_err_fd, F_SETFD, FD_CLOEXEC) == 0);

    /* Open file for stderr and set close-on-exec flag */
    CHECK((err_fd = open(err_file, O_CREAT | O_TRUNC | O_WRONLY,
                                   S_IRUSR | S_IWUSR)) >= 0);
    CHECK(fcntl(err_fd, F_SETFD, FD_CLOEXEC) == 0);

    /* Create a pipe for stdout */
    CHECK(pipe(outpipe) == 0);
    /* Set close-on-exec flag for pipe-out */
    CHECK(fcntl(outpipe[PIPE_OUT], F_SETFD, FD_CLOEXEC) == 0);
    /* Duplicate stdout as the pipe-in */
    CHECK(dup2(outpipe[PIPE_IN], STDOUT_FILENO) == STDOUT_FILENO);
    /* Close initial pipe-in file descriptor */
    CHECK(close(outpipe[PIPE_IN]) == 0);

    /* Create a pipe for stdout */
    CHECK(pipe(errpipe) == 0);
    /* Set close-on-exec flag for pipe-out */
    CHECK(fcntl(errpipe[PIPE_OUT], F_SETFD, FD_CLOEXEC) == 0);
    /* Duplicate stderr as the pipe-in */
    CHECK(dup2(errpipe[PIPE_IN], STDERR_FILENO) == STDERR_FILENO);
    /* Close initial pipe-in file descriptor */
    CHECK(close(errpipe[PIPE_IN]) == 0);

    signal(SIGCHLD, sigchld_handler);

    child_run = 1;
    if (fork() == 0)
    {
        rc = execvp(cmd, argv);
        if (rc != 0)
        {
            ERROR("execvp(): %s\n", strerror(errno));
            child_run = 0;
            return EXIT_FAILURE;
        }
    }

    do {
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        FD_ZERO(&fds);
        FD_SET(outpipe[0], &fds);
        FD_SET(errpipe[0], &fds);
        
        rc = select(10, &fds, NULL, NULL, &tv);
        if (rc == -1)
        {
            if (errno != EINTR)
            {
                ERROR("select(): %s\n", strerror(errno));
                return EXIT_FAILURE;
            }
        }
        else if (rc > 0)
        {
            if (FD_ISSET(outpipe[PIPE_OUT], &fds))
            {
                r = read(outpipe[PIPE_OUT], buf, sizeof(buf));
                if (r > 0)
                {
                    CHECK(write(out_err_fd, buf, r) == r);
                }
            }
            if (FD_ISSET(errpipe[PIPE_OUT], &fds))
            {
                r = read(errpipe[PIPE_OUT], buf, sizeof(buf));
                if (r > 0)
                {
                    CHECK(write(out_err_fd, buf, r) == r);
                    CHECK(write(err_fd,     buf, r) == r);
                }
            }
        }
    } while (child_run);

    if (WIFSIGNALED(status))
    {
        ERROR("'%s' was killed by signal %d", cmd, WTERMSIG(status));
    }
#ifdef WCOREDUMP
    if (WCOREDUMP(status))
    {
        ERROR("'%s' dumped core", cmd);
    }
#endif

    return WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE;
}
