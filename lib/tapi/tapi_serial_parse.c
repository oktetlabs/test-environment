/** @file
 * @brief Test API of serial console parsers
 *
 * Implementation of Test API for serial console parse configuring support.
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
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
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TAPI serial parse"

#include "te_config.h"
#include "conf_api.h"
#include "logger_api.h"
#include "tapi_serial_parse.h"
#include "te_sigmap.h"
#include "tapi_test_log.h"
#include "tapi_test.h"

/* Configurator object subtrees */
#define TE_SERIAL_PARSER "/agent:%s/parser:%s"
#define TE_SERIAL_LOG   TE_SERIAL_PARSER "/logging:" 
#define TE_SERIAL_EVENT TE_SERIAL_PARSER "/event:%s" 
#define TE_SERIAL_PATT  TE_SERIAL_EVENT  "/pattern:%s" 
#define TE_TESTER_EVENT "/local:/tester:/event:%s"
#define TE_TESTER_HANDL TE_TESTER_EVENT "/handler:%s"

/* See description in the tapi_serial_parser.h */
tapi_parser_id *
tapi_serial_id_init(const char *agent, const char *c_name, const char *name)
{
    tapi_parser_id *id;

    id = malloc(sizeof(tapi_parser_id));
    if (id == NULL)
        return NULL;

    id->ta = strdup(agent);
    if (id->ta == NULL)
    {
        free(id);
        return NULL;
    }
    id->c_name = strdup(c_name);
    if (id->c_name == NULL)
    {
        free(id->ta);
        free(id);
        return NULL;
    }
    id->name = strdup(name);
    if (id->name == NULL)
    {
        free(id->ta);
        free(id->c_name);
        free(id);
        return NULL;
    }

    id->user     = NULL;
    id->port     = TE_SERIAL_PARSER_PORT;
    id->interval = -1;

    return id;
}

/* See description in the tapi_serial_parser.h */
void
tapi_serial_id_cleanup(tapi_parser_id *id)
{
    if (id == NULL)
        return;
    if (id->name != NULL)
        free(id->name);
    if (id->c_name != NULL)
        free(id->c_name);
    if (id->ta != NULL)
        free(id->ta);
    if (id->user != NULL)
        free(id->user);
    free(id);
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_parser_add(tapi_parser_id *id)
{
    int rc;

    rc = cfg_add_instance_fmt(NULL, CVT_STRING, id->c_name,
                              TE_SERIAL_PARSER, id->ta, id->name);
    if (rc != 0)
    {
        ERROR("Failed to add parser %s to agent %s", id->name, id->ta);
        return rc;
    }

    if (id->user == NULL)
        id->user = strdup(TE_SERIAL_PARSER_USER);
    rc = cfg_set_instance_fmt(CVT_STRING, id->user, 
                              TE_SERIAL_PARSER "/user:", id->ta, id->name);
    if (rc != 0)
    {
        ERROR("Couldn't set the user name %s", id->user);
        return rc;
    }

    rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)id->interval,
                              TE_SERIAL_PARSER "/interval:", id->ta,
                              id->name);
    if (rc != 0)
    {
        ERROR("Couldn't set the polling interval %d", id->interval);
        return rc;
    }

    rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)id->port,
                              TE_SERIAL_PARSER "/port:", id->ta,
                              id->name);
    if (rc != 0)
    {
        ERROR("Couldn't set the console port %d", id->port);
        return rc;
    }

    rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)TRUE,
                              TE_SERIAL_PARSER "/enable:", id->ta,
                              id->name);
    if (rc != 0)
    {
        ERROR("Couldn't launch the parser %s", id->name);
        return rc;
    }
    return 0;
}

/* See description in the tapi_serial_parser.h */
void
tapi_serial_parser_del(tapi_parser_id *id)
{
    if (cfg_del_instance_fmt(FALSE, TE_SERIAL_PARSER, id->ta,
                             id->name) != 0)
        WARN("Couldn't delete the serial parser %s", id->name);
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_parser_enable(tapi_parser_id *id)
{
    te_errno rc;

    if ((rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)TRUE,
                                   TE_SERIAL_PARSER "/enable:", id->ta,
                                   id->name)) != 0)
    {
        ERROR("Couldn't enable the serial parser %s: %s", id->name,
              te_rc_err2str(rc));
        return rc;
    }

    return 0;
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_parser_disable(tapi_parser_id *id)
{
    te_errno rc;

    if ((rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)FALSE,
                                   TE_SERIAL_PARSER "/enable:", id->ta,
                                   id->name)) != 0)
    {
        ERROR("Couldn't disable the serial parser %s: %s", id->name,
              te_rc_err2str(rc));
        return rc;
    }

    return 0;
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_logging_enable(tapi_parser_id *id, const char *level)
{
    int rc;

    if (level != NULL)
    {
        rc = cfg_set_instance_fmt(CVT_STRING, level,
                                  TE_SERIAL_LOG "/level:",
                                  id->ta, id->name);
        if (rc != 0)
        {
            ERROR("Couldn't set the logging level %d", id->port);
            return rc;
        }
    }

    rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)TRUE,
                              TE_SERIAL_LOG, id->ta, id->name);
    if (rc != 0)
    {
        ERROR("Couldn't enable logging of the serial parser %s", id->name);
        return rc;
    }

    return 0;
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_logging_disable(tapi_parser_id *id)
{
    te_errno rc;

    if ((rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)FALSE,
                                   TE_SERIAL_LOG, id->ta, id->name)) != 0)
    {
        ERROR("Couldn't disable logging of the serial parser %s", id->name);
        return rc;
    }
    return 0;
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_parser_event_add(tapi_parser_id *id, const char *name,
                             const char *t_name)
{
    te_errno rc;

    if ((rc = cfg_add_instance_fmt(NULL, CVT_STRING, t_name,
                                   TE_SERIAL_EVENT, id->ta, id->name,
                                   name)) != 0)
    {
        ERROR("Couldn't add event to the serial parser %s", id->name);
        return rc;
    }
    return 0;
}

/* See description in the tapi_serial_parser.h */
void
tapi_serial_parser_event_del(tapi_parser_id *id, const char *name)
{
    if (cfg_del_instance_fmt(FALSE, TE_SERIAL_EVENT, id->ta, id->name,
                             name) != 0)
        WARN("Couldn't delete event %s from the serial parser %s",
             name, id->name);
}

/* See description in the tapi_serial_parser.h */
int
tapi_serial_parser_pattern_add(tapi_parser_id *id, const char *e_name,
                               const char *pattern)
{
    unsigned        n_handles;
    cfg_handle     *handles     = NULL;
    int             rc;
    int             i;
    int             max_i = 0;
    int             cur_i;
    char            name[TE_SERIAL_MAX_NAME];
    char           *i_name;

    rc = cfg_find_pattern_fmt(&n_handles, &handles, TE_SERIAL_PATT,
                              id->ta, id->name, e_name, "*");
    if (rc != 0)
    {
        ERROR("cfg_find_pattern_fmt error %d", rc);
        return -1;
    }

    for (i = 0; (unsigned)i < n_handles; i++)
    {
        rc = cfg_get_inst_name(handles[i], &i_name);
        if (rc != 0 || i_name == NULL)
        {
            free(handles);
            ERROR("Couldn't get index of pattern");
            return -1;
        }
        cur_i = atoi(i_name);
        if (cur_i > max_i)
            max_i = cur_i;
        free(i_name);
    }
    if (handles != NULL)
        free(handles);

    max_i++;
    snprintf(name, TE_SERIAL_MAX_NAME, "%d", max_i);

    rc = cfg_add_instance_fmt(NULL, CVT_STRING, pattern, TE_SERIAL_PATT,
                              id->ta, id->name, e_name, name);
    if (rc != 0)
    {
        ERROR("Couldn't add the instance: " TE_SERIAL_PATT, id->ta,
              id->name, e_name, name);
        return -1;
    }

    return max_i;
}

/* See description in the tapi_serial_parser.h */
void
tapi_serial_parser_pattern_del(tapi_parser_id *id, const char *e_name,
                               int pat_i)
{
    if (cfg_del_instance_fmt(FALSE, TE_SERIAL_EVENT  "/pattern:%d", id->ta,
                             id->name, e_name, pat_i) != 0)
        WARN("Couldn't delete pattern from event %s", e_name);
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_parser_reset(tapi_parser_id *id)
{
    te_errno rc;

    if ((rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)TRUE,
                                   TE_SERIAL_PARSER "/reset:", id->ta,
                                   id->name)) != 0)
    {
        ERROR("Couldn't reset status of events");
        return rc;
    }
    return 0;
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_tester_event_add(const char *name)
{
    te_errno rc;

    if ((rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL, TE_TESTER_EVENT,
                                   name)) != 0)
    {
        ERROR("Couldn't add event %s to the tester", name);
        return rc;
    }
    return 0;
}

/* See description in the tapi_serial_parser.h */
void
tapi_serial_tester_event_del(const char *name)
{
    if (cfg_del_instance_fmt(FALSE, TE_TESTER_EVENT, name) != 0)
        WARN("Couldn't delete event %s from the tester", name);
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_handler_ext_add(const char *ev_name, const char *h_name,
                            int priority, const char *path)
{
    int rc;

    rc = cfg_add_instance_fmt(NULL, CVT_STRING, path, TE_TESTER_HANDL,
                              ev_name, h_name);
    if (rc != 0)
    {
        ERROR("Couldn't add handler %s to the event %s", h_name, ev_name);
        return rc;
    }

    rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)priority,
                             TE_TESTER_HANDL "/priority:", ev_name, h_name);
    if (rc != 0)
    {
        ERROR("Couldn't set the handler priority to %d", priority);
        return rc;
    }

    rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)FALSE,
                             TE_TESTER_HANDL "/internal:", ev_name, h_name);
    if (rc != 0)
    {
        ERROR("Couldn't set the handler internal flag to false");
        return rc;
    }

    return 0;
}

/* See description in the tapi_serial_parser.h */
te_errno
tapi_serial_handler_int_add(const char *ev_name, const char *h_name,
                            int priority, int signo)
{
    int   rc;
    char *signame;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL, TE_TESTER_HANDL,
                              ev_name, h_name);
    if (rc != 0)
    {
        ERROR("Couldn't add handler %s to the event %s", h_name, ev_name);
        return rc;
    }

    rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)priority,
                             TE_TESTER_HANDL "/priority:", ev_name, h_name);
    if (rc != 0)
    {
        ERROR("Couldn't set the handler priority to %d", priority);
        return rc;
    }

    rc = cfg_set_instance_fmt(CVT_INTEGER, (void *)TRUE,
                             TE_TESTER_HANDL "/internal:", ev_name, h_name);
    if (rc != 0)
    {
        ERROR("Couldn't set the handler internal flag to true");
        return rc;
    }

    signame = map_signo_to_name(signo);
    if (signame == NULL)
    {
        ERROR("Couldn't get the signal name of %d", signo);
        return TE_EFAIL;
    }

    rc = cfg_set_instance_fmt(CVT_STRING, (void *)signame,
                              TE_TESTER_HANDL "/signal:", ev_name, h_name);
    free(signame);

    if (rc != 0)
    {
        ERROR("Couldn't set the handler signal to %d: %s", signo,
              te_rc_err2str(rc));
        return rc;
    }

    return 0;
}

/* See description in the tapi_serial_parser.h */
void
tapi_serial_handler_del(const char *ev_name, const char *h_name)
{
    if (cfg_del_instance_fmt(FALSE, TE_TESTER_HANDL, ev_name, h_name) != 0)
        WARN("Couldn't delete serial handler %s from event %s",
             h_name, ev_name);
}
