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
#include <rcf_common.h>
#include "posix_tar.h"
#include "gcov-io.h"
#include "tce_internal.h"

te_bool tce_debugging = FALSE;

static char tar_file_prefix[RCF_MAX_PATH + 1];

static tce_channel_data *channels;


static sig_atomic_t caught_signo = 0;
static void signal_handler(int no)
{
    caught_signo = no;
}


static fd_set active_channels;
static te_bool already_dumped = TRUE;
static te_bool dump_request = FALSE;

/* forward declarations */
static void function_header_state(tce_channel_data *ch);
static void object_header_state(tce_channel_data *ch);
static void read_data(int channel);
static void collect_line(tce_channel_data *ch);
static void dump_data(void);
static void dump_object(tce_object_info *oi);
static te_bool dump_object_data(tce_object_info *oi, FILE *tar_file);
static void clear_data(void);



static char **collector_args;
static int data_lock = -1;
static int peers_counter;

te_bool tce_standalone = FALSE;

int 
tce_init_collector(int argc, char **argv)
{
    char buf[RCF_MAX_PATH + 16];
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
    if (!tce_standalone)
        remove(buf);
    return 0;
}


const char *
tce_obtain_principal_connect(void)
{
    return collector_args ? collector_args[0] : NULL;
}

int
tce_obtain_principal_peer_id(void)
{
    static int peer_id;
    
    if (peer_id == 0)
    {
        peer_id = getpid();
    }
    return peer_id;
}

static void 
tce_generic_report(const char *level, const char *fmt, va_list args)
{
    fprintf(stderr, "tce_collector: %s%s", 
            level != NULL ? level : "", 
            level != NULL ? ":" : "");
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
}

void
tce_report_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    tce_generic_report("ERROR", fmt, args);
    va_end(args);
}

void
tce_report_notice(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    tce_generic_report(NULL, fmt, args);
    va_end(args);
}

void
tce_print_debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (tce_debugging)
        tce_generic_report("DEBUG", fmt, args);
    va_end(args);
}


static int
lock_data(void)
{
    if (already_dumped)
    {
        struct flock lock;

        already_dumped = FALSE;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;
        if (fcntl(data_lock, F_SETLKW, &lock) != 0)
        {
            int rc = errno;
            tce_report_error("Cannot obtain data lock: %s",
                             strerror(rc));
            return rc;
        }
    }
    return 0;
}

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

    tce_report_notice("Starting TCE collector, pid = %u", 
                      (unsigned)getpid());
    if (collector_args == NULL)
    {
        tce_report_error("tce_init_collector has not been called");
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
            remove(*args + 5);
            tce_print_debug("opening %s", *args + 5);
            mkfifo(*args + 5, S_IRUSR | S_IWUSR);
            listen_on = open(*args + 5, O_RDONLY | O_NONBLOCK, 0);
            if (listen_on < 0)
            {
                tce_report_error("can't open '%s' (%s), skipping", 
                                 *args + 5, strerror(errno));
            }
        }
#ifdef HAVE_SYS_SOCKET_H
#ifdef HAVE_SYS_UN_H
        else if (strncmp(*args, "unix:", 5) == 0)
        {
            struct sockaddr_un addr;
            is_socket = TRUE;
            remove(*args + 5);            
            listen_on = socket(PF_UNIX, SOCK_STREAM, 0);
            if (listen_on < 0)
            {
                tce_report_error("can't create local socket (%s)", 
                        strerror(errno));
            }
            else
            {
                addr.sun_family = AF_UNIX;
                strcpy(addr.sun_path, *args + 5);
                if (bind(listen_on, (struct sockaddr *)&addr, sizeof(addr)))
                {
                    tce_report_error("can't bind to local socket %s (%s)",
                            *args + 5, strerror(errno));
                    close(listen_on);
                    listen_on = -1;
                }
                if (chmod(addr.sun_path, 0666) != 0)
                {
                    tce_report_notice("can't change permissions for %s: %s",
                                  addr.sun_path, strerror(errno));
                }
            }
        }
        else if (strncmp(*args, "abstract:", 9) == 0)
        {
            struct sockaddr_un addr;
            is_socket = TRUE;
            listen_on = socket(PF_UNIX, SOCK_STREAM, 0);
            if (listen_on < 0)
            {
                tce_report_error("can't create local socket (%s)", 
                        strerror(errno));
            }
            else
            {
                addr.sun_family = AF_UNIX;
                memset(addr.sun_path, 0, sizeof(addr.sun_path));
                strcpy(addr.sun_path + 1, *args + 9);
                if (bind(listen_on, (struct sockaddr *)&addr, sizeof(addr)))
                {
                    tce_report_error("can't bind to abstract socket " 
                                     "%s (%s)",
                                     *args + 9, strerror(errno));
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
                tce_report_error("can't create TCP socket (%s)", 
                        strerror(errno));
            }
            else
            {
                addr.sin_family = AF_INET;
                addr.sin_port = htons(strtoul(*args + 4, &tmp, 0));
                
                if (addr.sin_port == 0)
                {
                    tce_report_error("no port specified at '%'", *args);
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
                    tce_report_error("can't bind to TCP socket %d:%s (%s)",
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
            lock_data();
            tce_set_ksymtable(*args + 9);
        }
        else if (strcmp(*args, "--debug") == 0)
        {
            tce_debugging = TRUE;
        }
        else
        {
            tce_report_error("invalid argument '%s'", *args);
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
                    tce_report_error("can't listen at '%s'", *args);
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
        tce_report_error("no channels specified");
        return EXIT_FAILURE;
    }
    tce_report_notice("TCE collector started");
    for (;;)
    {
        int result = 0;
        memcpy(&current, &active_channels, sizeof(current));
        errno = 0;
        if (caught_signo == 0)
        {
            result = select(max_fd + 1, &current, NULL, NULL, NULL);
        }
        if (caught_signo != 0)
        {
            int signo = caught_signo;
            tce_print_debug("TCE collector caught signal %d", signo);
            caught_signo = 0;
            if (signo == SIGHUP)
                dump_data();
            else if (signo == SIGUSR1)
            {
                lock_data();
                peers_counter++;
            }
            else if (signo == SIGTERM)
            {
                if (peers_counter > 0)
                {
                    tce_report_error("%d peers have not dumped data",
                                 peers_counter);
                }
                if (!tce_standalone)
                    clear_data();
                break;
            }
            continue;
        }
        if (result <= 0)
        {            
            if (errno != 0)
            {
                tce_report_error("select error %s", strerror(errno));
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
                            tce_report_error("accept error %s", 
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


pid_t tce_collector_pid = 0;

int
tce_run_collector(int argc, char *argv[])
{
    int rc;
    if (tce_collector_pid != 0)
        return TE_RC(TE_TA_LINUX, EALREADY);
    tce_obtain_principal_peer_id();
    rc = tce_init_collector(argc, argv);
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
tce_dump_collector(void)
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
    if (fcntl(data_lock, F_SETLKW, &lock) != 0)
    {
        int rc = errno;
        tce_report_error("Unable to obtain data lock: %s",
                         strerror(rc));
        return rc;
    }
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(data_lock, F_SETLK, &lock);
    return 0;
}

int
tce_stop_collector(void)
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

int 
tce_notify_collector(void)
{
    if (tce_collector_pid == 0)
        return 0;
    if (kill(tce_collector_pid, SIGUSR1) != 0)
        return TE_RC(TE_TA_LINUX, errno);
    return 0;
}

static void counter_group_state(tce_channel_data *ch);

static void
counter_state(tce_channel_data *ch)
{
    if (*ch->buffer != '+' && *ch->buffer != '~')
    {
        ch->state = function_header_state;
        ch->state(ch);
    }
    else if (*ch->buffer == '~')
    {
        ch->state = counter_group_state;
        ch->state(ch);
    }
    else
    {
        char *tmp;
        long long value = strtoll(ch->buffer, &tmp, 10);
        if (ch->counter_guard <= 0)
        {
            tce_report_error("too many arcs for peer %d", ch->peer_id);
            ch->state = NULL;
        }
        else
        {
            switch(ch->function->groups[ch->the_group].mode)
            {
                case TCE_MERGE_ADD:
                    *ch->counter++ += value;
                    ch->counter_guard--;
                    break;
                case TCE_MERGE_SINGLE:
                {
                    long long counter, all;
                    
                    counter = strtoll(tmp, &tmp, 10);
                    all     = strtoll(tmp, &tmp, 10);
                    if (ch->counter[0] == value)
                        ch->counter[1] += counter;
                    else if (counter > ch->counter[1])
                    {
                        ch->counter[0] = value;
                        ch->counter[1] = counter - ch->counter[1];
                    }
                    else
                        ch->counter[1] -= counter;
                    ch->counter[2] += all;
                    ch->counter += 3;
                    ch->counter_guard -= 3;
                    break;
                }
                case TCE_MERGE_DELTA:
                {
                    long long last, counter, all;

                    last = value;
                    value = strtoll(tmp, &tmp, 10);
                    counter = strtoll(tmp, &tmp, 10);
                    all = strtoll(tmp, &tmp, 10);
                    
                    if (ch->counter[1] == value)
                        ch->counter[2] += counter;
                    else if (counter > ch->counter[2])
                    {
                        ch->counter[1] = value;
                        ch->counter[2] = counter - ch->counter[2];
                    }
                    else
                        ch->counter[2] -= counter;
                    ch->counter[3] += all;
                    ch->counter += 4;
                    ch->counter_guard -= 4;
                    break;
                }
                default:
                    tce_report_error("internal error: unknown merge mode");
                    ch->state = NULL;
                    return;
            }
        }
    }
}

static void
counter_group_state(tce_channel_data *ch)
{
    if (*ch->buffer != '~')
    {
        ch->state = function_header_state;
        ch->state(ch);
    }
    else
    {
        char word[16];
        enum tce_merge_mode mode;
        unsigned count;
        
        if (sscanf(ch->buffer + 1, "%15s %u", word, &count) != 2)
        {
            tce_report_error("error parsing '%s' for peer %d",
                             ch->buffer, ch->peer_id);
            ch->state = NULL;
            return;
        }
        for (ch->the_group++; 
             !((1 << ch->the_group) & ch->object->ctr_mask);
             ch->the_group++)
            ;
        if (ch->the_group >= GCOV_COUNTER_GROUPS)
        {
            tce_report_error("too many counter groups for peer %d", 
                             ch->peer_id);
            ch->state = NULL;
            return;
        }
        if (strcmp(word, "add") == 0)
            mode = TCE_MERGE_ADD;
        else if (strcmp(word, "single") == 0)
            mode = TCE_MERGE_SINGLE;
        else if (strcmp(word, "delta") == 0)
            mode = TCE_MERGE_DELTA;
        else
        {
            tce_report_error("unknown merge mode '%s' for peer %d", 
                             word, ch->peer_id);
            ch->state = NULL;
            return;
        }
        if (ch->function->groups[ch->the_group].number != 0 &&
            ch->function->groups[ch->the_group].number != count)
        {
            tce_report_error("number of counters in a group "
                             "mismatch for peer %d", 
                      ch->peer_id);
            ch->state = NULL;
            return;
        }
        if (ch->function->groups[ch->the_group].mode != 
            TCE_MERGE_UNDEFINED && 
            ch->function->groups[ch->the_group].mode != mode)
        {
            tce_report_error("merge mode mismatch for peer %d", 
                      ch->peer_id);
            ch->state = NULL;
            return;
        }
        ch->function->groups[ch->the_group].number = count;
        ch->function->groups[ch->the_group].mode = mode;
        ch->state = counter_state;
    }
}

static void
function_header_state(tce_channel_data *ch)
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
            tce_report_error("peer %d, error near '%s'", 
                    ch->peer_id, space);
            ch->state = NULL;
        }
        else
        {
            unsigned arc_count, checksum;
            tce_function_info *fi;

            *space++ = '\0';
            if (sscanf(space, "%u %u", &checksum, &arc_count) != 2)
            {
                tce_report_error("parse error near '%s'", space);
                ch->state = NULL;
                return;
            }
            tce_print_debug("at %s %ld", space, checksum);
            
            fi = tce_get_function_info(ch->object, ch->buffer + 1, 
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
                ch->function = fi;
                ch->the_group = -1;
                ch->state = counter_group_state;
            }
        }
    }
}

static void
summary_state(tce_channel_data *ch)
{
    if (*ch->buffer != '>')
    {
        ch->state = function_header_state;
        ch->state(ch);
    }
    else
    {
        unsigned n_counters;
        unsigned program_n_counters;
        unsigned object_runs;
        unsigned program_runs;
        long long program_max;
        long long program_sum;
        long long program_sum_max;
        long long object_sum;
        long long object_max;
        long long object_sum_max;

        
        if (sscanf(ch->buffer + 1, 
                   "%u %u %Ld %Ld %Ld %u %u %Ld %Ld %Ld",
                   &n_counters, &object_runs, 
                   &object_sum, &object_max, &object_sum_max,
                   &program_n_counters, &program_runs,
                   &program_sum, &program_max, &program_sum_max) != 10)
        {
            tce_report_error("error parsing '%s' for peer %d",
                      ch->buffer, ch->peer_id);
            ch->state = NULL;
            return;
        }
        if (ch->object->ncounts != 0 &&
            (ch->object->ncounts != (long)n_counters ||
             ch->object->program_ncounts != program_n_counters))
        {
            tce_report_error("counters number mismatch for '%s'", 
                             ch->object->filename);
            ch->state = NULL;
            return;
        }
        ch->object->ncounts = n_counters;
        ch->object->program_ncounts = program_n_counters;
        ch->object->object_runs  += object_runs;
        ch->object->program_runs += program_runs;
        ch->object->program_sum  += program_sum;
        ch->object->object_sum   += object_sum;
        if (program_max > ch->object->program_max)
            ch->object->program_max = program_max;
        if (object_max > ch->object->object_max)
            ch->object->object_max = object_max;
        ch->object->program_sum_max += program_sum_max;
        ch->object->object_sum_max  += object_sum_max;
        ch->state = function_header_state;
    }
}

static void
object_header_state(tce_channel_data *ch)
{
    char *space;

    if (strcmp(ch->buffer, "end") == 0)
    {
        ch->state = NULL;
        return;
    }

    lock_data();
    space = strchr(ch->buffer,  ' ');
    if (space == NULL)
    {
        tce_report_error("peer %d, error near '%s'", 
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
        tce_object_info *oi;

        *space++ = '\0';
        oi = tce_get_object_info(ch->peer_id, ch->buffer);
        
        if (strncmp(space, "new ", 4) == 0)
        {
            unsigned gcov_version;
            unsigned checksum, program_checksum, ctr_mask;

            tce_print_debug("new format peer detected");
            space += 4;
            if(sscanf(space, "%u %u %u %u %ld %u",
                      &gcov_version, &oi->stamp, 
                      &checksum, 
                      &program_checksum,
                      &object_functions, 
                      &ctr_mask) != 6)
            {
                tce_report_error("error parsing '%s' for peer %d", 
                                 space, ch->peer_id);
                ch->state = NULL;
                return;
            }
            if (oi->gcov_version != 0 && oi->gcov_version != gcov_version)
            {
                tce_report_error("GCOV version mismatch for peer %d",
                                 ch->peer_id);
                ch->state = NULL;
                return;
            }
            oi->gcov_version = gcov_version;
            if (oi->checksum != 0 && 
                (oi->checksum != checksum || 
                 oi->program_checksum != program_checksum))
            {
                tce_report_error("checksum mismatch for peer %d",
                                 ch->peer_id);
                ch->state = NULL;
                return;
            }
            oi->checksum = checksum;
            oi->program_checksum = program_checksum;
            if (oi->object_functions != 0 && 
                object_functions != oi->object_functions)
            {
                tce_report_error("function number mismatch for peer %d",
                                 ch->peer_id);
                ch->state = NULL;
                return;
            }
            oi->object_functions = object_functions;
            oi->ctr_mask |= ctr_mask;
            ch->state = summary_state;
            ch->object = oi;
        }
        else
        {
            if(sscanf(space, "%ld %ld %Ld %Ld %ld %Ld %Ld",
                      &object_functions, 
                      &program_arcs, 
                      &program_sum, 
                      &program_max, 
                      &ncounts, 
                      &object_sum, 
                      &object_max) != 7)
            {
                tce_report_error("error parsing '%s' for peer %d", 
                                 space, ch->peer_id);
                ch->state = NULL;
                return;
            }
            
            if (oi->ncounts && oi->ncounts != ncounts)
            {
                tce_report_error("peer %d, error near '%s'", 
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
                oi->ctr_mask = 1;
                if (ncounts != 0)
                {
                    ch->state = function_header_state;
                    ch->object = oi;
                }
            }
        }
    }
}

static void
auth_state(tce_channel_data *ch)
{
    ch->peer_id = strtoul(ch->buffer, NULL, 10);
    ch->state = (ch->peer_id != 0 ? object_header_state : NULL);
}

static te_bool 
are_there_working_channels(void)
{
    tce_channel_data *iter;
    for (iter = channels; iter != NULL; iter = iter->next)
    {
        if (iter->state != NULL)
        {
            tce_report_notice("there are working channels");
            return TRUE;
        }
    }
    return FALSE;
}


static void
read_data(int channel)
{
    tce_channel_data *found;
    
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
        tce_report_notice("new peer connected");
    }

    if (found->state == NULL)
        found->state = auth_state;
     
    collect_line(found);
    if (found->state == NULL)
    {
        FD_CLR(found->fd, &active_channels);
        if (peers_counter >= 0)
            peers_counter--;
        else
            tce_report_notice("unregistered peers detected");
        close(found->fd);
        if (dump_request && !are_there_working_channels())
            raise(SIGHUP);
    }
}

static void
collect_line(tce_channel_data *ch)
{
    int len;
    tce_print_debug("requesting %d bytes on %d", ch->remaining, ch->fd);
    len = read(ch->fd, ch->bufptr, ch->remaining);
    tce_print_debug("read %d bytes from %d", len, ch->fd);
    if (len <= 0)
    {
        tce_report_error("read error on %d: %s", ch->fd,
                     strerror(errno));
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
                tce_print_debug("got %s", ch->buffer);
                ch->state(ch);
#if 0
                tce_report_notice("processed");
#endif
                len = ch->bufptr - found_newline - 1;
                memmove(ch->buffer, found_newline + 1, len);
                ch->bufptr = ch->buffer + len;
                ch->remaining = sizeof(ch->buffer) - 1 - len;
            }
            else if (ch->remaining == 0)
            {
                tce_report_error("too long line on %d", ch->fd);
                ch->state = NULL;
            }
        } while (found_newline != NULL && ch->state != NULL);
    }
}

#define TCE_HASH_SIZE 11113
static tce_object_info *tce_hash_table[TCE_HASH_SIZE];

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
    return ret % TCE_HASH_SIZE;
}


tce_object_info *
tce_get_object_info(int peer_id, const char *filename)
{
    unsigned key = hash(peer_id, filename);
    tce_object_info *found;
    
    for (found = tce_hash_table[key]; found != NULL; found = found->next)
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
    found->gcov_version = 0;
    found->checksum = 0;
    found->program_checksum = 0;
    found->program_ncounts  = 0;
    found->program_sum_max  = 0;
    found->object_sum_max   = 0;
    found->object_sum       = 0;
    found->program_sum      = 0;
    found->object_max       = 0;
    found->program_max      = 0;
    found->object_runs      = 0;
    found->program_runs     = 0;
    found->ctr_mask = 0;
    found->stamp    = 0;
    found->next = tce_hash_table[key];
    tce_hash_table[key] = found;
    return found;
}

tce_function_info *
tce_get_function_info(tce_object_info *oi, const char *name, 
                      long arc_count, long checksum)
{
    tce_function_info *fi;
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
        memset(fi->groups, 0, sizeof(fi->groups));
        oi->function_infos = fi;
    }
    else
    {
        if (fi->arc_count != arc_count)
        {
            tce_report_error("arc count mismatch for function %s", 
                         fi->name);
            return NULL;
        }
        if (fi->checksum != checksum)
        {
            tce_report_error("arc count mismatch for function %s", 
                         fi->name);
            return NULL;
        }
    }
    return fi;
}


static void
dump_data(void)
{
    tce_object_info *iter;
    unsigned idx;
    struct flock lock;

    if (already_dumped)
        return;

    if (are_there_working_channels())
    {
        dump_request = TRUE;
        return;
    }

    dump_request = FALSE;
    clear_data();
    tce_report_notice("Dumping TCE data");
    tce_obtain_kernel_coverage();
    for (idx = 0; idx < TCE_HASH_SIZE; idx++)
    {
        for (iter = tce_hash_table[idx]; iter != NULL; iter = iter->next)
            dump_object(iter);
    }

    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(data_lock, F_SETLK, &lock);
    tce_report_notice("TCE data dumped");
    already_dumped = TRUE;
}


static void
dump_object(tce_object_info *oi)
{
    static char tar_name[RCF_MAX_PATH + 16];
    static char tar_header[512];
    FILE *tar_file;
    long pos, len;
    unsigned long checksum;
    unsigned idx;

    sprintf(tar_name, "%s%d.tar", tar_file_prefix, oi->peer_id);
    tce_print_debug("dumping to %s", tar_name);
    tar_file = fopen(tar_name, "r+");
    if (tar_file == NULL)
    {
        tar_file = fopen(tar_name, "w+");
        if (tar_file == NULL)
        {
            tce_report_error("cannot open %s: %s", 
                             tar_name, strerror(errno));
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
    sprintf(tar_header + TAR_MODE, "%#7.7o", TUREAD | TUWRITE | 
            TGREAD | TOREAD);
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
        tce_report_error("error writing to %s: %s", 
                         tar_name, strerror(errno));
        fclose(tar_file);
        remove(tar_name);
    }
}

static te_bool
dump_new_object_data(tce_object_info *oi, FILE *tar_file)
{
    const unsigned magic = GCOV_DATA_MAGIC;
    const unsigned func_magic[2] = {GCOV_TAG_FUNCTION, 
                                    GCOV_TAG_FUNCTION_LENGTH};
    unsigned ident;
    unsigned group_magic[2];
    const unsigned obj_summary_magic[2] = {GCOV_TAG_OBJECT_SUMMARY,
                                           GCOV_TAG_SUMMARY_LENGTH};
    const unsigned prog_summary_magic[2] = {GCOV_TAG_PROGRAM_SUMMARY,
                                           GCOV_TAG_SUMMARY_LENGTH};
            

    int group;
    int c_offset = 0;
    unsigned count;
    tce_function_info *fi;

#define SAFE_FWRITE(data, size, count, stream)     \
    if (fwrite(data, size, count, stream) < count) \
    {                                              \
       tce_report_error("error dumping data: %s",  \
                        strerror(errno));          \
       return FALSE;                               \
    }

    SAFE_FWRITE(&magic, sizeof(magic), 1, tar_file);
    SAFE_FWRITE(&oi->gcov_version, sizeof(oi->gcov_version), 1, tar_file);
    SAFE_FWRITE(&oi->stamp, sizeof(oi->stamp), 1, tar_file);

    for (fi = oi->function_infos; fi != NULL; fi = fi->next)
    {
        tce_print_debug("dumping function %s %ld", fi->name, fi->arc_count);
        ident = strtoul(fi->name, NULL, 10);
        SAFE_FWRITE(func_magic, sizeof(func_magic), 1, tar_file);
        SAFE_FWRITE(&ident, sizeof(ident), 1, tar_file);
        SAFE_FWRITE(&fi->checksum, sizeof(fi->checksum), 1, tar_file);
        for (c_offset = 0, group = 0; group < GCOV_COUNTER_GROUPS; group++)
        {
            if (!((1 << group) & oi->ctr_mask))
                continue;
            tce_print_debug("dumping counter group %d (#%d)", group,
                            fi->groups[group].number);
            group_magic[0] = GCOV_TAG_FOR_COUNTER (group);
            count = fi->groups[group].number;
            group_magic[1] = GCOV_TAG_COUNTER_LENGTH (count);
            SAFE_FWRITE(group_magic, sizeof(group_magic), 1, tar_file);
            for (; count != 0; count--, c_offset++)
            {
                SAFE_FWRITE(fi->counts + c_offset, sizeof(*fi->counts), 
                            1, tar_file);
            }
        }
    }
    SAFE_FWRITE(obj_summary_magic, sizeof(obj_summary_magic), 1, tar_file);
    SAFE_FWRITE(&oi->checksum, sizeof(oi->checksum), 1, tar_file);
    tce_print_debug("object counters: %d", oi->ncounts);
    SAFE_FWRITE(&oi->ncounts, sizeof(oi->ncounts), 1, tar_file);
    SAFE_FWRITE(&oi->object_runs, sizeof(oi->object_runs), 1, tar_file);
    tce_print_debug("object sum: %ld", oi->object_sum);
    SAFE_FWRITE(&oi->object_sum, sizeof(oi->object_sum), 1, tar_file);
    tce_print_debug("object max: %ld", oi->object_max);
    SAFE_FWRITE(&oi->object_max, sizeof(oi->object_max), 1, tar_file);
    SAFE_FWRITE(&oi->object_sum_max, sizeof(oi->object_sum_max), 
                1, tar_file);
    SAFE_FWRITE(prog_summary_magic, sizeof(prog_summary_magic), 
                1, tar_file);
    SAFE_FWRITE(&oi->program_checksum, sizeof(oi->program_checksum), 
                1, tar_file);
    tce_print_debug("program counters: %d", oi->program_ncounts);
    SAFE_FWRITE(&oi->program_ncounts, sizeof(oi->program_ncounts), 
                1, tar_file);
    SAFE_FWRITE(&oi->program_runs, sizeof(oi->program_runs), 1, tar_file);
    SAFE_FWRITE(&oi->program_sum, sizeof(oi->program_sum), 1, tar_file);
    SAFE_FWRITE(&oi->program_max, sizeof(oi->program_max), 1, tar_file);
    SAFE_FWRITE(&oi->program_sum_max, sizeof(oi->program_sum_max), 
                1, tar_file);
    return TRUE;
}

static te_bool
dump_object_data(tce_object_info *oi, FILE *tar_file)
{
    tce_function_info *fi;
    long long *ci;
    long arc_count;
 
    if (oi->gcov_version != 0)
        return dump_new_object_data(oi, tar_file);
   
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
        tce_report_error("error writing output file");
        return FALSE;
    }

    for (fi = oi->function_infos; fi != NULL; fi = fi->next)
    {
        if (__write_gcov_string(fi->name,
                                strlen(fi->name), tar_file, -1)
            || __write_long(fi->checksum, tar_file, 4)
            || __write_long(fi->arc_count, tar_file, 4))
        {
            tce_report_error("error writing output file");
            return FALSE;
        }
        for (arc_count = fi->arc_count, ci = fi->counts; 
             arc_count != 0; 
             arc_count--, ci++)
        {
            if (__write_gcov_type (*ci, tar_file, 8))
            {
                tce_report_error("error writing output file");
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
    tce_object_info *iter;
    
    char buffer[RCF_MAX_PATH + 16];
    for (idx = 0; idx < TCE_HASH_SIZE; idx++)
    {
        for (iter = tce_hash_table[idx]; iter != NULL; iter = iter->next)
        {
            sprintf(buffer, "%s%d.tar", tar_file_prefix, iter->peer_id);
            remove(buffer);
        }
    }

}

