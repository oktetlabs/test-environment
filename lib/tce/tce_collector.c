/** @file
 * @brief Tester Subsystem
 *
 * TCE data collector
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#define _FILE_OFFSET_BITS 64
#include <te_config.h>
#include <te_defs.h>

#include <limits.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/select.h>
#include <sys/wait.h>
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>
#include <te_errno.h>
#include "posix_tar.h"
#include "gcov-io.h"

static char tar_file_prefix[PATH_MAX + 1];

typedef struct bb_function_info 
{
    long checksum;
    int arc_count;
    const char *name;
    long long *counts;
    struct bb_function_info *next;
} bb_function_info;

typedef struct bb_object_info
{
    int peer_id;
    const char *filename;
    long long object_max;
    long long object_sum;
    long object_functions;
    long long program_sum;
    long long program_max;
    long program_arcs;
    long ncounts;
    struct bb_object_info *next;
    struct bb_function_info *function_infos;
} bb_object_info;


typedef struct channel_data channel_data;
struct channel_data
{
    int fd;
    void (*state)(channel_data *me);
    char buffer[128];
    char *bufptr;
    int remaining;
    int peer_id;
    bb_object_info *object;
    long long *counter;
    int counter_guard;
    channel_data *next;
};

static sig_atomic_t caught_signo;
static void signal_handler(int no)
{
    caught_signo = no;
}


static fd_set active_channels;
static te_bool already_dumped = FALSE;

/* forward declarations */
static void function_header_state(channel_data *ch);
static void object_header_state(channel_data *ch);
static void read_data(int channel);
static void collect_line(channel_data *ch);
static bb_object_info * get_object_info(int peer_id, const char *filename);
static bb_function_info *get_function_info(bb_object_info *oi,
                                           const char *name, 
                                           long arc_count, long checksum);
static void dump_data(void);
static void dump_object(bb_object_info *oi);
static te_bool dump_object_data(bb_object_info *oi, FILE *tar_file);
static void clear_data(void);

static void get_kernel_coverage(void);
struct bb;
static void process_gcov_syms(FILE *symfile, int core_file,
                              void (*functor)(int, struct bb *, void *), 
                              void *extra);
static void do_gcov_sum(int core_file, struct bb *object, void *extra);
static void get_kernel_gcov_data(int core_file, 
                                 struct bb *object, void *extra);



static char **collector_args;
static int data_lock = -1;
static char *ksymtable = "/proc/kallsyms";

te_bool tce_standalone = FALSE;

int 
init_tce_collector(int argc, char **argv)
{
    char buf[PATH_MAX + 1];
    char **ptr;
    strcpy(tar_file_prefix, *argv);
    collector_args = malloc(sizeof(*collector_args) * argc);
    for (argv++, ptr = collector_args; *argv != NULL; argv++, ptr++)
    {
        *ptr = strdup(*argv);
    }
    *ptr = NULL;
    sprintf(buf, "%s.lock", tar_file_prefix);
    data_lock = open(buf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    remove(buf);
    return 0;
}


const char *
obtain_principal_tce_connect(void)
{
    return collector_args ? collector_args[0] : NULL;
}

static
void report_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fputs("tce_collector: ", stderr);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
}

#define report_notice report_error

int 
tce_collector(void)
{
    fd_set sockets;
    fd_set current;
    int listen_on = -1;
    int max_fd = -1;
    te_bool is_socket;
    char **args;
    static struct sigaction on_signals;

    report_notice("Starting TCE collector, pid = %u", (unsigned)getpid());
    if (collector_args == NULL)
    {
        report_error("init_tce_collector has not been called");
        return EXIT_FAILURE;
    }


    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    on_signals.sa_handler = signal_handler;
    on_signals.sa_flags = 0;
    sigemptyset(&on_signals.sa_mask);
    sigaction(SIGUSR1, &on_signals, NULL);
    sigaction(SIGTERM, &on_signals, NULL);
    sigaction(SIGHUP, &on_signals, NULL);

    FD_ZERO(&active_channels);
    FD_ZERO(&sockets);
    for (args = collector_args; *args != NULL; args++)
    {
        is_socket = FALSE;
        listen_on = -1;
        if (strncmp(*args, "fifo:", 5) == 0)
        {
            report_notice("opening %s", *args + 5);
            mkfifo(*args + 5, S_IRUSR | S_IWUSR);
            listen_on = open(*args + 5, O_RDONLY | O_NONBLOCK, 0);
            if (listen_on < 0)
            {
                report_error("can't open '%s' (%s), skipping", 
                      *args + 5, strerror(errno));
            }
        }
#ifdef HAVE_SYS_SOCKET_H
#ifdef HAVE_SYS_UN_H
        else if (strncmp(*args, "unix:", 5) == 0)
        {
            struct sockaddr_un addr;
            is_socket = TRUE;
            listen_on = socket(PF_UNIX, SOCK_STREAM, 0);
            if (listen_on < 0)
            {
                report_error("can't create local socket (%s)", 
                        strerror(errno));
            }
            else
            {
                addr.sun_family = AF_UNIX;
                strcpy(addr.sun_path, *args + 5);
                if (bind(listen_on, (struct sockaddr *)&addr, sizeof(addr)))
                {
                    report_error("can't bind to local socket %s (%s)",
                            *args + 5, strerror(errno));
                    close(listen_on);
                    listen_on = -1;
                }
            }
        }
#endif
#ifdef HAVE_NETINET_TCP_H
        else if (strncmp(*args, "tcp:", 4) == 0)
        {
            char *tmp;
            struct sockaddr_in addr;
            is_socket = TRUE;
            listen_on = socket(PF_INET, SOCK_STREAM, 0);
            if (listen_on < 0)
            {
                report_error("can't create TCP socket (%s)", 
                        strerror(errno));
            }
            else
            {
                addr.sin_family = AF_INET;
                addr.sin_port = htons(strtoul(*args + 4, &tmp, 0));
                
                if (addr.sin_port == 0)
                {
                    report_error("no port specified at '%'", *args);
                    return EXIT_FAILURE;
                }

                if (*tmp)
                {
                    addr.sin_addr.s_addr = 0;
                }
                else
                {
                    inet_pton(AF_INET, tmp + 1, &addr.sin_addr);
                }
                setsockopt(listen_on, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
                if (bind(listen_on, (struct sockaddr *)&addr, sizeof(addr)))
                {
                    report_error("can't bind to TCP socket %d:%s (%s)",
                            ntohs(addr.sin_port),
                            *tmp ? tmp + 1 : "*", strerror(errno));
                    close(listen_on);
                    listen_on = -1;
                }
            }
        }
#endif
#endif /* HAVE_SYS_SOCKET_H */
        else if (strncmp(*args, "kallsyms:", 9) == 0)
        {
            ksymtable = *args + 9;
        }
        else
        {
            report_error("invalid argument '%s'", *args);
            return EXIT_FAILURE;
        }
        if (listen_on >= 0)
        {
            if (is_socket)
            {
                fcntl(listen_on, F_SETFL, 
                      O_NONBLOCK | fcntl(listen_on, F_GETFL));
                if (listen(listen_on, 5))
                {
                    report_error("can't listen at '%s'", *args);
                    close(listen_on);
                    continue;
                }
                FD_SET(listen_on, &sockets);
            }
            FD_SET(listen_on, &active_channels);
            if (listen_on > max_fd)
                max_fd = listen_on;
        }
    }

    if (max_fd < 0)
    {
        report_error("no channels specified");
        return EXIT_FAILURE;
    }
    report_notice("TCE collector started");
    for (;;)
    {
        int result;
        memcpy(&current, &active_channels, sizeof(current));
        result = select(max_fd + 1, &current, NULL, NULL, NULL);
        if (result < 0)
        {
            if (errno == EINTR)
            {
                int signo = caught_signo;
                report_notice("TCE collector caught signal %d", signo);
                caught_signo = 0;
                if (signo == SIGHUP)
                    dump_data();
                else if (signo == SIGUSR1)
                    clear_data();
                else if (signo == SIGTERM)
                {
                    if (!tce_standalone)
                        clear_data();
                    break;
                }
            }
            else
            {
                report_error("select error %s", strerror(errno));
            }
        }
        else
        {
            int fd;
            for (fd = 0; fd <= max_fd && result; fd++)
            {
                if (FD_ISSET(fd, &current))
                {
                    result--;
                    if (FD_ISSET(fd, &sockets))
                    {
                        listen_on = accept(fd, NULL, NULL);
                        if (listen_on < 0)
                        {
                            report_error("accept error %s", 
                                         strerror(errno));
                        }
                        else
                        {
                            fcntl(listen_on, F_SETFL, 
                                  O_NONBLOCK | fcntl(listen_on, F_GETFL));
                            FD_SET(listen_on, &active_channels);
                            if (listen_on > max_fd)
                                max_fd = listen_on;
                        }
                    }
                    else
                    {
                        read_data(fd);
                    }
                }
            }
        }
    }
    return 0;
}


static pid_t tce_collector_pid;

int
run_tce_collector(int argc, char *argv[])
{
    int rc;
    if (tce_collector_pid != 0)
        return TE_RC(TE_TA_LINUX, EALREADY);
    rc = init_tce_collector(argc, argv);
    if (rc != 0)
        return rc;
    tce_collector_pid = fork();
    if (tce_collector_pid == (pid_t)-1)
        return TE_RC(TE_TA_LINUX, errno);
    else if (tce_collector_pid == 0)
    {
        exit(tce_collector());
    }
    return 0;
}


int
dump_tce_collector(void)
{
    struct flock lock;
    if (tce_collector_pid == 0)
        return 0;

    if (kill(tce_collector_pid, SIGHUP) != 0)
        return TE_RC(TE_TA_LINUX, errno);
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(data_lock, F_SETLKW, &lock);
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(data_lock, F_SETLK, &lock);
    return 0;
}

int
stop_tce_collector(void)
{
    int tce_status;
    
    if (tce_collector_pid == 0)
        return 0;
    if (kill(tce_collector_pid, SIGTERM) != 0)
        return TE_RC(TE_TA_LINUX, errno);
    if (waitpid(tce_collector_pid, &tce_status, 0) < 0)
        return TE_RC(TE_TA_LINUX, errno);
    tce_collector_pid = 0;
    return WIFEXITED(tce_status) && WEXITSTATUS(tce_status) == 0 ? 0 : 
        TE_RC(TE_TA_LINUX, ETESHCMD);
}


static void
counter_state(channel_data *ch)
{
    if (*ch->buffer != '+')
    {
        ch->state = function_header_state;
        ch->state(ch);
    }
    else
    {
        char *tmp;
        long long count = strtoll(ch->buffer, &tmp, 10);
        if (*tmp != '\0')
        {
            report_error("peer id %d, error near '%s'",
                    ch->peer_id, ch->buffer);
            ch->state = NULL;
        }
        else if (ch->counter_guard == 0)
        {
            report_error("too many arcs for peer %d", ch->peer_id);
            ch->state = NULL;
        }
        else
        {
            *ch->counter += count;
            ch->counter++;
            ch->counter_guard--;
        }
    }
}

static void
function_header_state(channel_data *ch)
{
    if (*ch->buffer != '*')
    {
        ch->state = object_header_state;
        ch->state(ch);
    }
    else
    {
        char *space;
        space = strchr(ch->buffer,  ' ');
        if (space == NULL )
        {
            report_error("peer %d, error near '%s'", 
                    ch->peer_id, space);
            ch->state = NULL;
        }
        else
        {
            long arc_count, checksum;
            bb_function_info *fi;

            *space++ = '\0';
            sscanf(space, "%ld %ld", &checksum, &arc_count);
            fi = get_function_info(ch->object, ch->buffer + 1, 
                                   arc_count, checksum);
            if (fi == NULL)
            {
                ch->state = NULL;
                return;
            }
            if (fi->arc_count != 0)
            {
                ch->counter = fi->counts;
                ch->counter_guard = fi->arc_count;
                ch->state = counter_state;
            }
        }
    }
}

static void
object_header_state(channel_data *ch)
{
    char *space;
    struct flock lock;

    if (strcmp(ch->buffer, "end") == 0)
    {
        ch->state = NULL;
        return;
    }

    already_dumped = FALSE;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(data_lock, F_SETLKW, &lock);

    space = strchr(ch->buffer,  ' ');
    if (space == NULL )
    {
        report_error("peer %d, error near '%s'", 
                ch->peer_id, ch->buffer);
        ch->state = NULL;
    }
    else
    {
        long long program_sum = 0;
        long long program_max = 0;
        long program_arcs = 0;
        long long object_max = 0;
        long long object_sum = 0;
        long object_functions = 0;
        long ncounts;
        bb_object_info *oi;

        *space++ = '\0';
        oi = get_object_info(ch->peer_id, ch->buffer);
        if(sscanf(space, "%ld %ld %Ld %Ld %ld %Ld %Ld",
                  &object_functions, 
                  &program_arcs, 
                  &program_sum, 
                  &program_max, 
                  &ncounts, 
                  &object_sum, 
                  &object_max) != 7)
        {
            report_error("error parsing '%s' for peer %d", 
                         space, ch->peer_id);
            ch->state = NULL;
            return;
        }
        
        if (oi->ncounts && oi->ncounts != ncounts)
        {
            report_error("peer %d, error near '%s'", 
                ch->peer_id, space);
            ch->state = NULL;
        }
        else
        {
            oi->ncounts = ncounts;
            oi->program_arcs = program_arcs;
            oi->object_functions = object_functions;
            oi->object_sum = object_sum;
            oi->program_sum += program_sum;
            if (object_max > oi->object_max)
                oi->object_max = object_max;
            if (program_max > oi->program_max)
                oi->program_max = program_max;
            if (ncounts != 0)
            {
                ch->state = function_header_state;
                ch->object = oi;
            }
        }
    }
}

static void
auth_state(channel_data *ch)
{
    ch->peer_id = strtoul(ch->buffer, NULL, 10);
    ch->state = (ch->peer_id != 0 ? object_header_state : NULL);
}

static void
read_data(int channel)
{
    static channel_data *channels;
    channel_data *found;
    
    for (found = channels; found != NULL; found = found->next)
    {
        if (found->fd == channel)
            break;
    }
    if (found == NULL)
    {
        found = calloc(1, sizeof(*found));
        found->fd = channel;
        found->next = channels;
        channels = found;
        found->bufptr = found->buffer;
        found->remaining = sizeof(found->buffer) - 1;
    }

    if (found->state == NULL)
        found->state = auth_state;
     
    collect_line(found);
    if (found->state == NULL)
    {
        FD_CLR(found->fd, &active_channels);
        close(found->fd);
    }
}

static void
collect_line(channel_data *ch)
{
    int len;
    len = read(ch->fd, ch->bufptr, ch->remaining);
    if (len <= 0)
    {
        if ((len != 0 && errno != EPIPE) || 
            (ch->state != object_header_state))
        {
            report_error("read error on %d: %s", ch->fd,
                    strerror(errno));
        }
        ch->state = NULL;
    }
    else
    {
        char *found_newline; 

        ch->bufptr += len;
        ch->remaining -= len;
        do
        {
            found_newline = memchr(ch->buffer, '\n', 
                                   ch->bufptr - ch->buffer);
            if (found_newline != NULL)
            {
                *found_newline = '\0';
#if 0
                report_notice("got %s", ch->buffer);
#endif
                ch->state(ch);
#if 0
                report_notice("processed");
#endif
                len = ch->bufptr - found_newline - 1;
                memmove(ch->buffer, found_newline + 1, len);
                ch->bufptr = ch->buffer + len;
                ch->remaining = sizeof(ch->buffer) - 1 - len;
            }
            else if (ch->remaining == 0)
            {
                report_error("too long line on %d", ch->fd);
                ch->state = NULL;
            }
        } while (found_newline != NULL && ch->state != NULL);
    }
}

#define BB_HASH_SIZE 11113
static bb_object_info *bb_hash_table[BB_HASH_SIZE];

static unsigned 
hash(int peer_id, const char *str)
{
    unsigned int ret = peer_id;
    unsigned int ctr = 0;


    while (*str)
    {
        ret ^= (*str++ & 0xFF) << ctr;
        ctr = (ctr + 1) % sizeof (void *);
    }
    return ret % BB_HASH_SIZE;
}


static bb_object_info *
get_object_info(int peer_id, const char *filename)
{
    unsigned key = hash(peer_id, filename);
    bb_object_info *found;
    
    for (found = bb_hash_table[key]; found != NULL; found = found->next)
    {
        if (found->peer_id == peer_id && 
            strcmp(found->filename, filename) == 0)
        {
            return found;
        }
    }
    found = malloc(sizeof(*found));
    found->peer_id = peer_id;
    found->filename = strdup(filename);
    found->ncounts = 0;
    found->function_infos = NULL;
    found->next = bb_hash_table[key];
    bb_hash_table[key] = found;
    return found;
}

static bb_function_info *
get_function_info(bb_object_info *oi, const char *name, 
                  long arc_count, long checksum)
{
    bb_function_info *fi;
    for (fi = oi->function_infos; fi != NULL; fi = fi->next)
    {
        if (strcmp(fi->name, name) == 0)
            break;
    }
    if (fi == NULL)
    {
        fi = malloc(sizeof(*fi));
        fi->name = strdup(name);
        fi->arc_count = arc_count;
        fi->checksum = checksum;
        fi->next = oi->function_infos;
        fi->counts = calloc(fi->arc_count, sizeof(*fi->counts));
        oi->function_infos = fi;
    }
    else
    {
        if (fi->arc_count != arc_count)
        {
            report_error("arc count mismatch for function %s", 
                         fi->name);
            return NULL;
        }
        if (fi->checksum != checksum)
        {
            report_error("arc count mismatch for function %s", 
                         fi->name);
            return NULL;
        }
    }
    return fi;
}


static void
dump_data(void)
{
    bb_object_info *iter;
    unsigned idx;
    struct flock lock;

    if (already_dumped)
        return;

    clear_data();
    report_notice("Dumping TCE data");
    get_kernel_coverage();
    for (idx = 0; idx < BB_HASH_SIZE; idx++)
    {
        for (iter = bb_hash_table[idx]; iter != NULL; iter = iter->next)
            dump_object(iter);
    }

    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(data_lock, F_SETLK, &lock);
    report_notice("TCE data dumped");
    already_dumped = TRUE;
}


static void
dump_object(bb_object_info *oi)
{
    static char tar_name[PATH_MAX + 1];
    static char tar_header[512];
    FILE *tar_file;
    long pos, len;
    unsigned long checksum;
    unsigned idx;

    sprintf(tar_name, "%s%d.tar", tar_file_prefix, oi->peer_id);
    report_notice("dumping to %s", tar_name);
    tar_file = fopen(tar_name, "r+");
    if (tar_file == NULL)
    {
        tar_file = fopen(tar_name, "w+");
        if (tar_file == NULL)
        {
            report_error("cannot open %s: %s", tar_name, strerror(errno));
            return;
        }
    }

    fseek(tar_file, 0, SEEK_END);
    if (strlen(oi->filename) <= TAR_NAME_LENGTH)
        strncpy(tar_header + TAR_NAME, oi->filename, TAR_NAME_LENGTH);
    else
    {
        const char *last_slash = strrchr(oi->filename, '/');
        strncpy(tar_header + TAR_NAME, last_slash + 1, 100);
        memcpy(tar_header + TAR_PREFIX, oi->filename, 
               last_slash - oi->filename);
    }
    sprintf(tar_header + TAR_MODE, "%#7.7o", TUREAD | TGREAD | TOREAD);
    memcpy(tar_header + TAR_UID, "0000000", 7);
    memcpy(tar_header + TAR_GID, "0000000", 7);
    sprintf(tar_header + TAR_MTIME, "%#11.11lo", (long)time(NULL));
    tar_header[TAR_TYPE] = REGTYPE;
    memcpy(tar_header + TAR_MAGIC, TMAGIC, TMAGLEN);
    memcpy(tar_header + TAR_VERSION, TVERSION, TVERSLEN);
    memset(tar_header + TAR_CHKSUM, ' ', 8);
    memcpy(tar_header + TAR_UNAME, "root", 4);
    memcpy(tar_header + TAR_GNAME, "root", 4);
    fwrite(tar_header, sizeof(tar_header), 1, tar_file);
    pos = ftell(tar_file);

    if(dump_object_data(oi, tar_file))
    {
        len = ftell(tar_file) - pos;
        sprintf(tar_header + TAR_SIZE, "%#11.11lo", len);
        for(idx = 0, checksum = 0; idx < sizeof(tar_header); idx++)
        {
            checksum += (unsigned char)tar_header[idx];
        }        
        sprintf(tar_header + TAR_CHKSUM, "%#7.7lo", checksum);
        fseek(tar_file, pos - sizeof(tar_header), SEEK_SET);
        fwrite(tar_header, sizeof(tar_header), 1, tar_file);
        fseek(tar_file, 0, SEEK_END);
        memset(tar_header, 0, sizeof(tar_header));
        if (len % 512)
            fwrite(tar_header, 512 - len % 512, 1, tar_file);
        /* two empty blocks at a possible archive end */
        fclose(tar_file);
    }
    else
    {
        report_error("error writing to %s: %s", tar_name, strerror(errno));
        fclose(tar_file);
        remove(tar_name);
    }
}

static te_bool
dump_object_data(bb_object_info *oi, FILE *tar_file)
{
    bb_function_info *fi;
    long long *ci;
    long arc_count;
    
    if (/* magic */
        __write_long (-123, tar_file, 4)
        /* number of functions in object file.  */
        || __write_long (oi->object_functions, tar_file, 4)
        /* length of extra data in bytes.  */
        || __write_long ((4 + 8 + 8) + (4 + 8 + 8), tar_file, 4)
        
        /* whole program statistics. If merging write per-object
           now, rewrite later */
        /* number of instrumented arcs.  */
        || __write_long (oi->program_arcs, tar_file, 4)
        /* sum of counters.  */
        || __write_gcov_type (oi->program_sum, tar_file, 8)
        /* maximal counter.  */
        || __write_gcov_type (oi->program_max, tar_file, 8)
        
        /* per-object statistics.  */
        /* number of counters.  */
        || __write_long (oi->ncounts, tar_file, 4)
        /* sum of counters.  */
        || __write_gcov_type (oi->object_sum, tar_file, 8)
        /* maximal counter.  */
        || __write_gcov_type (oi->object_max, tar_file, 8))
    {
        report_error("error writing output file");
        return FALSE;
    }

    for (fi = oi->function_infos; fi != NULL; fi = fi->next)
    {
        if (__write_gcov_string(fi->name,
                                strlen(fi->name), tar_file, -1)
            || __write_long(fi->checksum, tar_file, 4)
            || __write_long(fi->arc_count, tar_file, 4))
        {
            report_error("error writing output file");
            return FALSE;
        }
        for (arc_count = fi->arc_count, ci = fi->counts; 
             arc_count != 0; 
             arc_count--, ci++)
        {
            if (__write_gcov_type (*ci, tar_file, 8))
            {
                report_error("error writing output file");
                return FALSE;
            }
        }
    }
    return TRUE;
}


static void
clear_data(void)
{
    int idx;
    bb_object_info *iter;
    
    char buffer[PATH_MAX + 1];
    for (idx = 0; idx < BB_HASH_SIZE; idx++)
    {
        for (iter = bb_hash_table[idx]; iter != NULL; iter = iter->next)
        {
            sprintf(buffer, "%s%d.tar", tar_file_prefix, iter->peer_id);
            remove(buffer);
        }
    }

}


struct bb_raw_function_info 
{
    long checksum;
    int arc_count;
    const char *name;
};

/* Structure emitted by --profile-arcs  */
struct bb
{
    long zero_word;
    const char *filename;
    long long *counts;
    long ncounts;
    struct bb *next;
    
    /* Older GCC's did not emit these fields.  */
    long sizeof_bb;
    struct bb_raw_function_info *function_infos;
};


typedef struct summary_data
{
    long long sum;
    long long max;
    long arcs;
} summary_data;

static void
get_kernel_coverage(void)
{
    FILE *symfile = fopen(ksymtable, "r");
    int core_file;
    summary_data summary = {0, 0, 0};
    
    if (symfile == NULL)
    {
        report_error("Cannot open kernel symtable file %s: %s", 
                     ksymtable, strerror(errno));
        return;
    }
    core_file = open("/dev/kmem", O_RDONLY);
    if (core_file < 0)
    {
        report_error("Cannot open kernel memory file: %s", strerror(errno));
        fclose(symfile);
        return;
    }

    process_gcov_syms(symfile, core_file, do_gcov_sum, &summary);
    process_gcov_syms(symfile, core_file, get_kernel_gcov_data, &summary);
    close(core_file);
    fclose(symfile);
}


static ssize_t
read_at(int fildes, off_t offset, void *buffer, size_t size)
{
    if(lseek(fildes, offset, SEEK_SET) == (off_t)-1)
        return (ssize_t)-1;
    return read(fildes, buffer, size);
}

static int
read_str(int fildes, off_t offset, char *buffer, size_t maxlen)
{
    if(lseek(fildes, offset, SEEK_SET) == (off_t)-1)
        return -1;
    while(--maxlen)
    {
        if(read(fildes, buffer, 1) <= 0)
            return -1;
        if(*buffer++ == '\0')
            return 0;
    }
    *buffer = '\0';
    errno = ENAMETOOLONG;
    return -1;
}

static void
process_gcov_syms(FILE *symfile, int core_file,
                  void (*functor)(int, struct bb *, void *), void *extra)
{
    size_t offset;
    static char symbuf[256];
    char *token;
    
    rewind(symfile);
    while (fgets(symbuf, sizeof(symbuf) - 1, symfile) != NULL)
    {
        offset = strtoul(symbuf, &token, 16);
        strtok(token, " \t\n");
        token = strtok(NULL, "\t\n");
        if (strstr(token, "GCOV") != NULL)
        {
            /* This is a very crude hack relying on internals of GCC */
            /* It is necessary because GCOV symbols are pointing to
               constructors, rather than actual data. 
               On x86, the pointer to the actual data happen to be
               4 bytes later (an immediate argument to mov).
               Most probably, this won't work on x86-64 :(
            */
            unsigned long tmp;
            struct bb bb_buf;
            
            report_notice("offset is %lu", (unsigned long)offset);
            if(read_at(core_file, offset + 4, &tmp, sizeof(tmp)) < 
               (ssize_t)sizeof(tmp))
            {
                report_error("read error on /dev/kmem: %s", 
                             strerror(errno));
                continue;
            }
            report_notice("new offset is %lu", tmp);
            if(read_at(core_file, tmp, &bb_buf, sizeof(bb_buf)) < 
               (ssize_t)sizeof(bb_buf))
            {
                report_error("read error on /dev/kmem: %s", 
                             strerror(errno));
                continue;
            }
            
            functor(core_file, &bb_buf, extra);
        }
    }

}

static void
do_gcov_sum(int core_file, struct bb *object, void *extra)
{
    summary_data *summary = extra;
    int i;
    long long counter;

    if(lseek(core_file, (size_t)object->counts, SEEK_SET) == (off_t)-1)
    {
        report_error("seeking error on /dev/kmem: %s", strerror(errno));
        return;
    }
    for (i = 0; i < object->ncounts; i++)
    {
        if(read(core_file, &counter, sizeof(counter)) < 
           (ssize_t)sizeof(counter))
        {
            report_error("error reading from /dev/kmem: %s", 
                         strerror(errno));
            return;
        }
        summary->sum += counter;
        
        if (counter > summary->max)
            summary->max = counter;
    }
    summary->arcs += object->ncounts;
}


static void
get_kernel_gcov_data(int core_file, struct bb *object, void *extra)
{
    summary_data *summary = extra;
    summary_data object_summary = {0, 0, 0};
    long object_functions = 0;
    struct bb_raw_function_info fn_info;
    bb_function_info *fi;
    bb_object_info *oi;
    long long *target_counters;
    long long counter;
    int i;
    static char name_buffer[PATH_MAX + 1];
    char *real_start;
        

    if (lseek(core_file, (size_t)object->function_infos, SEEK_SET) == 
        (off_t)-1)
    {
        report_error("seeking error on /dev/kmem: %s", strerror(errno));
        return;
    }
    for (;;)
    {
        if(read(core_file, &fn_info, sizeof(fn_info)) < 
           (ssize_t)sizeof(fn_info))
        {
            report_error("error reading from /dev/kmem: %s", 
                         strerror(errno));
            return;
        }
        if (fn_info.arc_count < 0)
            break;
        
        object_functions++;
    }

    do_gcov_sum(core_file, object, &object_summary);
    if (read_str(core_file, (size_t)object->filename, 
                 name_buffer, sizeof(name_buffer)) != 0)
    {
        report_error("error reading from /dev/kmem: %s", strerror(errno));
        return;
    }
    real_start = strstr(name_buffer, "//");
    oi = get_object_info(getpid(), 
                         real_start ? real_start + 1 : name_buffer);
    oi->ncounts = object->ncounts;
    oi->object_functions = object_functions;
    oi->object_sum += object_summary.sum;
    oi->program_sum += summary->sum;
    oi->program_arcs += summary->arcs;
    if (object_summary.max > oi->object_max)
        oi->object_max = object_summary.max;
    if (summary->max > oi->program_max)
        oi->program_max = summary->max;

    for(;;)
    {          
        if(read_at(core_file, (size_t)object->function_infos++, 
                   &fn_info, sizeof(fn_info)) < (ssize_t)sizeof(fn_info))
        {
            report_error("error reading from /dev/kmem: %s", 
                         strerror(errno));
            return;
        }
        if (fn_info.arc_count < 0)
            break;
        if (read_str(core_file, (size_t)fn_info.name, 
                     name_buffer, sizeof(name_buffer)) != 0)
        {
            report_error("error reading from /dev/kmem: %s", 
                         strerror(errno));
            return;
        }
        fi = get_function_info(oi, name_buffer, 
                               fn_info.arc_count, fn_info.checksum);
        if (fi != NULL)
        {
            for (i = fn_info.arc_count, target_counters = fi->counts;
                 i > 0; 
                 i--, object->counts++, target_counters++)
            {
                read_at(core_file, (size_t)object->counts, 
                        &counter, sizeof(counter));
                *target_counters += counter;
            }
        }
    }

}

