/* Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001, 2002  Free Software Foundation, Inc. */
/* Copyright (C) 2005  The authors of the Test Environment */

/* This code is based on libgcc2.c from GCC distribution.
   It uses gcov-related routines from there modifying them to
   use TE logger instead of normal file I/O
*/

/* As a special exception, if you link this library with other files,
   some of which are compiled with GCC, to produce an executable,
   this library does not by itself cause the resulting executable
   to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  
*/

#include <te_config.h>
#include <stdio.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#ifdef HAVE_NETINET_TCP_H
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#endif
#include <unistd.h>



struct bb_function_info 
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
    struct bb_function_info *function_infos;
};


/* Chain of per-object file bb structures.  */
struct bb *__bb_head;

enum CONNECT_MODES { CONNECT_FIFO, CONNECT_UNIX, CONNECT_TCP };

static enum CONNECT_MODES connect_mode;
static int peer_id;
static char collector_path[PATH_MAX + 1];
static struct in_addr tcp_address;
static int tcp_port;

void
__bb_init_connection(const char *mode, int peer)
{
    if (strncmp(mode, "fifo:", 5) == 0)
    {
        connect_mode = CONNECT_FIFO;
        strcpy(collector_path, mode + 5);
    }
#ifdef HAVE_SYS_SOCKET_H
#ifdef HAVE_SYS_UN_H
    else if (strncmp(mode, "unix:", 5) == 0)
    {
        connect_mode = CONNECT_UNIX;
        strcpy(collector_path, mode + 5);
    }
#endif
#ifdef HAVE_NETINET_TCP_H
    else if (strncmp(mode, "tcp:", 4) == 0)
    {
        char *tmp;
        tcp_port = htons(strtoul(mode + 4, &tmp, 10));
                
        if (*tmp == '\0')
        {
            tcp_address.s_addr = 0;
        }
        else
        {
            inet_pton(AF_INET, tmp + 1, &tcp_address);
        }
    }
#endif
#endif /* HAVE_SYS_SOCKET_H */
    peer_id = peer;
}


/* Dump the coverage counts. We merge with existing counts when
   possible, to avoid growing the .da files ad infinitum.  */

void
__bb_exit_func (void)
{
    struct bb *ptr;
    int i;
    long long program_sum = 0;
    long long program_max = 0;
    long program_arcs = 0;
    int fd;
    static char buffer[128];

    if (peer_id == 0) /* __bb_init_function has not been called */
    {
        const char *env = getenv("TCE_CONNECTION");
        char name[PATH_MAX + 1];
        char *space_at;
        int peer_id;
        if (env == NULL)
            return;
        strncpy(name, env, sizeof(name) - 1);
        space_at = strchr(name, ' ');
        if (space_at == NULL ||
            (peer_id = strtol(space_at, NULL, 0)) == 0)
        {
            fprintf(stderr, "invalid TCE_CONNECTION var '%s'\n", env);
            return;
        }
        *space_at = '\0';
        __bb_init_connection(name, peer_id);
    }

    errno = 0;

    switch (connect_mode)
    {
#ifdef HAVE_NETINET_TCP_H
        case CONNECT_TCP:
        {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr = tcp_address;
            addr.sin_port = tcp_port;
            fd = socket(PF_INET, SOCK_STREAM, 0);
            connect(fd, (struct sockaddr *)&addr, sizeof(addr));
            break;
        }
#endif
#ifdef HAVE_SYS_UN_H
        case CONNECT_UNIX:
        {
            struct sockaddr_un addr;
            addr.sun_family = AF_UNIX;
            strcpy(addr.sun_path, collector_path);
            fd = socket(PF_UNIX, SOCK_STREAM, 0);
            connect(fd, (struct sockaddr *)&addr, sizeof(addr));
            break;
        }
#endif
        case CONNECT_FIFO:
        {
            fd = open(collector_path, O_WRONLY);
            break;
        }
        default:
        {
            fputs("internal error: invalid connection mode\n", stderr);
            return;
        }
    }
    
    if (errno)
    {
        fprintf(stderr, "cannot connect to TCE collector: %s\n", 
                strerror(errno));
        return;
    }

    sprintf(buffer, "%d\n", peer_id);
    write(fd, buffer, strlen(buffer)); 
    
    /* Non-merged stats for this program.  */
    for (ptr = __bb_head; ptr; ptr = ptr->next)
    {
        for (i = 0; i < ptr->ncounts; i++)
        {
            program_sum += ptr->counts[i];
            
            if (ptr->counts[i] > program_max)
                program_max = ptr->counts[i];
        }
        program_arcs += ptr->ncounts;
    }
    
    for (ptr = __bb_head; ptr; ptr = ptr->next)
    {
        long long object_max = 0;
        long long object_sum = 0;
        long object_functions = 0;
        int error = 0;
        struct bb_function_info *fn_info;
        long long *count_ptr;
        
        if (!ptr->filename)
            continue;
        
        for (fn_info = ptr->function_infos; 
             fn_info->arc_count != -1; 
             fn_info++)
        {
            object_functions++;
        }
        
        /* Calculate the per-object statistics.  */
        for (i = 0; i < ptr->ncounts; i++)
        {
            object_sum += ptr->counts[i];
            
            if (ptr->counts[i] > object_max)
                object_max = ptr->counts[i];
        }

        sprintf(buffer, "%s %d %d %Ld %Ld %d %Ld %Ld\n", 
                ptr->filename,
                object_functions, 
                program_arcs,
                program_sum, 
                program_max, 
                ptr->ncounts, 
                object_sum,
                object_max);
        write(fd, buffer, strlen(buffer));
      
        /* Write execution counts for each function.  */
        count_ptr = ptr->counts;
        
        for (fn_info = ptr->function_infos; fn_info->arc_count >= 0;
             fn_info++)
        {          
            sprintf(buffer, "*%s %d %d\n", fn_info->name,
                    fn_info->checksum, fn_info->arc_count);
            write(fd, buffer, strlen(buffer));
            for (i = fn_info->arc_count; i > 0; i--, count_ptr++)
            {
                sprintf(buffer, "+%Ld\n", *count_ptr);
                write(fd, buffer, strlen(buffer));
            }
        }
    }
    write(fd, "end\n", 4);
    close(fd);
}

/* Add a new object file onto the bb chain.  Invoked automatically
   when running an object file's global ctors.  */

void
__bb_init_func (struct bb *blocks)
{
    if (blocks->zero_word)
        return;

    /* Initialize destructor and per-thread data.  */
    if (!__bb_head)
    {
        atexit (__bb_exit_func);
    }

    /* Set up linked list.  */
    blocks->zero_word = 1;
    blocks->next = __bb_head;
    __bb_head = blocks;
}

/* Called before fork or exec - reset gathered coverage info to zero.  
   This avoids duplication or loss of the
   profile information gathered so far.  */


void
__bb_fork_func (void)
{
    struct bb *ptr;

    for (ptr = __bb_head; ptr != NULL; ptr = ptr->next)
    {
        long i;
        for (i = ptr->ncounts - 1; i >= 0; i--)
            ptr->counts[i] = 0;
    }
}


void _target_init(void) __attribute__ ((weak));
void _target_fini(void) __attribute__ ((weak));


void
_target_init(void) 
{
} 

void
_target_fini(void) 
{
} 

static void target_init_caller(void) __attribute__ ((constructor));
static void target_fini_caller(void) __attribute__ ((destructor));

static void
target_init_caller (void) 
{
    _target_init();
} 

static void
target_fini_caller (void) 
{
    _target_fini();
}
