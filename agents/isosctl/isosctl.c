/** @file
 * @brief ISOS Proxy Test Agent
 *
 * Test Agent running on the Linux and used to control the ISOS NUT
 * via serial port.
 *
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * 
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <termios.h>

#include "te_defs.h"
#include "te_errno.h"
#include "logger_ta.h"
#include "comm_agent.h"
#include "rcf_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#define TE_LGR_USER      "Main"


char *ta_name;

static pthread_mutex_t ta_lock = PTHREAD_MUTEX_INITIALIZER;
static char *devname;

static int agent_list(char *oid, char **list);

static rcf_pch_conf_object node_agent = 
    { "agent", NULL, NULL, NULL, NULL, NULL, NULL, 
      (rcf_ch_conf_list)agent_list};


/* Send answer to the TEN */
#define SEND_ANSWER(_fmt...) \
    do {                                                                  \
        int rc;                                                           \
        if (snprintf(cbuf + answer_plen,                                  \
                     buflen - answer_plen, _fmt) >= buflen - answer_plen) \
        {                                                                 \
            VERB("answer is truncated\n");                                \
        }                                                                 \
        rcf_ch_lock();                                                    \
        rc = rcf_comm_agent_reply(handle, cbuf, strlen(cbuf) + 1);        \
        rcf_ch_unlock();                                                  \
        return rc;                                                        \
    } while (0)

/**
 * Initialize structures.
 *
 * @return error code
 */
int 
rcf_ch_init()
{
    return 0;
}

/**
 * Mutual exclusion lock access to data connection.
 */
void 
rcf_ch_lock()
{
    pthread_mutex_lock(&ta_lock);
}


/**
 * Unlock access to data connection.
 */
void 
rcf_ch_unlock()
{
    pthread_mutex_unlock(&ta_lock);
}


/* See description in rcf_ch_api.h */
int 
rcf_ch_shutdown(char *cbuf, int buflen,
                struct rcf_comm_connection *handle, int answer_plen)
{
    /* Standard handler is OK */
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(handle);
    UNUSED(answer_plen);
    return -1;
}

/*------------------------ NUT reboot support -------------------------*/

#define DEVICE_DELAY    1    /* Delay of device output in seconds */
#define REBOOT_TIMEOUT  60   /* Maximum duration of reloading */

char buf[1024];

static struct prompt {
    char *prompt;
    char *command;
} prompts[] = { { "-->", "system restart\r\n"},
                { "Login", "\r\nadmin\r\nadmin\r\nsystem restart\r\n" },
                { "Quantum>", "system restart\r\n" },
                { "Debug>", "system restart\r\n" } };

/**
 * Read data from the device. 
 *
 * @param s     file descriptor corresponding to box's tty
 *
 * @return 0 (success) or -1 (timeout)
 *
 * @alg 
 * Data are read from the device until it sends them or space in the
 * buffer exist. Thus we obtain the whole bulk of data, not 
 * unpredictable pieces only.
 */
static int
read_device(int s)
{
    int n = 0;
    
    memset(buf, 0, sizeof(buf));
    
    while (1)
    {
        struct timeval tv = { DEVICE_DELAY, 0 };
        fd_set         set;
        int            l;
        
        FD_ZERO(&set);
        FD_SET(s, &set);
    
        select(s + 1, &set, NULL, NULL, &tv);
    
        if (!FD_ISSET(s, &set))
            break;
        
        l = read(s, buf + n, sizeof(buf) - n - 1);
        if (l <= 0)
            return n > 0 ? n : -1;
        n += l;
        if (n == sizeof(buf))
            break;
    }
        
    if (n == 0)
        return -1;
        
    return n;
}

/**
 * Reboot the box.
 *
 * @return 0 (success) or EIO (failure)
 */
static int
reboot_box()
{
    int s = -1;
    
    unsigned int i;
    unsigned int start;
    
    struct termios tios;
    
#define CHECKERR(x) \
    do {                                \
       if ((x) >= 0)                    \
           break;                       \
       if (s >= 0)                      \
           close(s);                    \
       return EIO;                      \
    } while (0)
    
    CHECKERR(s = open(devname, O_RDWR | O_NDELAY, 0));
    
    tcgetattr(s, &tios);
    cfmakeraw(&tios);
    tios.c_iflag |= BRKINT;
    tcsetattr(s, TCSANOW, &tios);
    
    /* Read the prompt */
    CHECKERR(write(s, "\r\n", 1));
    CHECKERR(read_device(s));
    
    /* Execute appropriate command */
    for (i = 0; i < sizeof(prompts) / sizeof(struct prompt); i++)
    {
        if (strstr(buf, prompts[i].prompt) != NULL)
        {
            CHECKERR(write(s, prompts[i].command,
                           strlen(prompts[i].command)));
            break;
        }
    }
    
    read_device(s); /* Skipping "Login: admin, etc." */
    
    /* Wait while the box is rebooted */
    start = time(NULL);
    while (time(NULL) - start < REBOOT_TIMEOUT)
    {
        read_device(s);
        if (strstr(buf, "Login:") != NULL)
        {
            close(s);
            return 0;
        }
    }
    
    close(s);
    return EIO;
    
#undef CHECKERR    
}


/* See description in rcf_ch_api.h */
int 
rcf_ch_reboot(char *cbuf, int buflen, char *parms,
              char *ba, int cmdlen, 
              struct rcf_comm_connection *handle, int answer_plen)
{
    int rc = reboot_box();
    
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(parms);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(handle);
    UNUSED(answer_plen);
    
    // SEND_ANSWER("%d", rc); 
    SEND_ANSWER("0");
    
    return 0;
}


/* See description in rcf_ch_api.h */
int 
rcf_ch_configure(char *cbuf, int buflen, char *ba, int cmdlen, 
                 struct rcf_comm_connection *handle, 
                 int answer_plen, int op, char *oid, char *val)
{
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(buflen);
    UNUSED(handle);
    UNUSED(answer_plen);
    UNUSED(op);
    UNUSED(oid);
    UNUSED(val);
    
    /* Standard handler is OK */
    return -1;
}


/* See description in rcf_ch_api.h */
int
rcf_ch_vread(char *cbuf, int buflen,
             struct rcf_comm_connection *handle, 
             int answer_plen, int type, char *var)
{
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(handle);
    UNUSED(answer_plen);
    UNUSED(type);
    UNUSED(var);
    return -1;
}


/* See description in rcf_ch_api.h */
int 
rcf_ch_vwrite(char *cbuf, int buflen,
              struct rcf_comm_connection *handle, 
              int answer_plen, int type, char *var, 
              uint64_t val_int, char *val_string)
{
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(buflen);
    UNUSED(handle);
    UNUSED(answer_plen);
    UNUSED(type);
    UNUSED(var);
    UNUSED(val_int);
    UNUSED(val_string);
    return -1;
}


/* See description in rcf_ch_api.h */
void *
rcf_ch_symbol_addr(const char *name, int is_func)
{
    UNUSED(name);
    UNUSED(is_func);
    return NULL;
}


/* See description in rcf_ch_api.h */
int 
rcf_ch_file(char *cbuf, int buflen, char *ba, int cmdlen, 
            struct rcf_comm_connection *handle, 
            int answer_plen, rcf_op_t op, char *filename)
{
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(ba);
    UNUSED(cmdlen);
    UNUSED(handle);
    UNUSED(answer_plen);
    UNUSED(op);
    UNUSED(filename);
    SEND_ANSWER("%d", EOPNOTSUPP);
    return 0;
}


/* See description in rcf_ch_api.h */
int 
rcf_ch_call(char *cbuf, int buflen, 
            struct rcf_comm_connection *handle, 
            int answer_plen, char *rtn, int argc, int argv,
            uint32_t *params)
{
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(handle);
    UNUSED(answer_plen);
    UNUSED(rtn);
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(params);
    SEND_ANSWER("%d", EOPNOTSUPP);
    return 0;
}


/* See description in rcf_ch_api.h */
int 
rcf_ch_start_task(char *cbuf, int buflen, 
                  struct rcf_comm_connection *handle, int answer_plen, 
                  int priority, char *rtn, int argc, int argv,
                  uint32_t *params)
{
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(handle);
    UNUSED(answer_plen);
    UNUSED(priority);
    UNUSED(rtn);
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(params);
    SEND_ANSWER("%d", EOPNOTSUPP);
    return 0;
}


/* See description in rcf_ch_api.h */
int 
rcf_ch_kill_task(char *cbuf, int buflen, 
                 struct rcf_comm_connection *handle, 
                 int answer_plen, int pid)
{
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(pid);
    SEND_ANSWER("%d", EOPNOTSUPP);
    return 0;
}

/**
 * Get instance list for object "agent". Returns TA name.
 *
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 *
 * @return error code
 * @retval 0            success
 * @retval ENOMEM       cannot allocate memory
 */
static int 
agent_list(char *oid, char **list)
{
    UNUSED(oid);
    return (*list = strdup(rcf_ch_conf_agent())) == NULL ? ENOMEM : 0;
}

/**
 * Get root of the tree of supported objects.
 *
 * @return root pointer
 */
rcf_pch_conf_object *
rcf_ch_conf_root()
{
    return &node_agent;
}

/**
 * Get Test Agent name.
 *
 * @return name pointer
 */
char *
rcf_ch_conf_agent()
{
    return ta_name;
}

/**
 * Release resources allocated for configuration support.
 */
void 
rcf_ch_conf_release()
{
}


/**
 * Entry point of the Linux Test Agent.
 *
 * Usage:
 *     talinux <ta_name> <communication library configuration string>
 */
int
main(int argc, char **argv)
{
    int rc; 
    
    if (argc != 4)
        return -1;

    if ((rc = log_init()) != 0)
        return rc;
        
    ta_name = argv[1];
    devname = argv[3];
    VERB("started\n");
    return rcf_pch_start_pch(argv[2]);
}
