/** @file
 * @brief Linux Test Agent
 *
 * Support for RADIUS testing
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef WITH_RADIUS_SERVER
#include "linuxconf_daemons.h"

enum radius_parameters { RP_FLAG, RP_ATTRIBUTE, RP_SECTION, RP_FILE};

typedef struct radius_parameter
{
    te_bool deleted;
    enum radius_parameters kind;
    char *name;
    char *value;
    int backup_index;
    struct radius_parameter *parent;
    struct radius_parameter *next;
    struct radius_parameter *children, *last_child;
} radius_parameter;


#define RP_DELETE_VALUE ((char *)-1)

static struct radius_parameter *radius_conf;

typedef struct radius_action
{
    char *attribute;
    char *operator;
    char *value;
    struct radius_action *next;
} radius_action;

typedef struct radius_user_record
{
    radius_action *checks, *last_check;
    radius_action *sets, *last_set;
    struct radius_user_record *next;
} radius_user_record;

typedef struct radius_user
{
    char *name;
    radius_user_record *records;
    radius_user_record *last_record;
    struct radius_user *next;
} radius_user;

static struct radius_user *radius_users, *radius_last_user;

static char *expand_rp (const char *value, radius_parameter *top);

static radius_parameter *
make_rp(enum radius_parameters kind,
        const char *name, 
        const char *value, radius_parameter *parent)
{
    radius_parameter *parm = malloc(sizeof(*parm));

    if (parm == NULL)
    {
        ERROR("make_rp: not enough memory");
        return NULL;
    }

    parm->deleted = FALSE;
    parm->kind = kind;
    parm->name = name ? strdup(name) : NULL;
    if (value != NULL)
    {
        parm->value = expand_rp(value, parent);
    }
    parm->next = parm->children = parm->last_child = NULL;
    parm->parent = parent;
    if (parent != NULL)
    {
        if (parent->children == NULL)
            parent->children = parent->last_child = parm;
        else
        {
            parent->last_child->next = parm;
            parent->last_child = parm;
        }
    }
    return parm;
}

static void
destroy_rp (radius_parameter *parm)
{
    radius_parameter *child, *next;
    
    if (parm->name != NULL)
        free(parm->name);
    if (parm->value != NULL)
        free(parm->value);
    for (child = parm->children; child != NULL; child = next)
    {
        next = child->next;
        destroy_rp(child);
    }
    free(parm);
}

static void read_radius(FILE *conf, radius_parameter *top);

static radius_parameter *
read_radius_file (const char *filename, radius_parameter *top)
{
    FILE *newfile; 
    radius_parameter *fp;
    int index;
    static char directory[PATH_MAX + 1];
    const char *dirptr;

    dirptr = strrchr(filename, '/');
    if (dirptr == NULL)
        *directory = '\0';
    else
    {
        memcpy(directory, filename, dirptr - filename + 1);
        directory[dirptr - filename + 2] = '\0';
    }
    if (ds_create_backup(directory, dirptr == NULL ? filename : dirptr + 1, &index) != 0)
        return NULL;

    RING("Reading RADIUS config %s", filename);
    newfile = fopen(filename, "r");
    if (newfile == NULL)
    {
        ERROR ("cannot open %s: %s", filename, strerror(errno));
        return NULL;
    }
    
    fp = make_rp(RP_FILE, filename, NULL, top);
    if (fp != NULL)
    {
        fp->backup_index = index;
        read_radius(newfile, fp);
    }
    fclose(newfile);
    return fp;
}

static void 
read_radius(FILE *conf, radius_parameter *top)
{
    static char buffer[256];
    radius_parameter *initial_top = top;
    int skip_spaces;
    
    while (fgets(buffer, sizeof(buffer) - 1, conf) != NULL)
    {
        skip_spaces = strspn(buffer, " \t\n");
        if (strncmp(buffer + skip_spaces, "$INCLUDE", 8) == 0)
        {
            char *fname;
            strtok(buffer, " \t\n");
            fname = expand_rp(strtok(NULL, " \t\n"), top);
            read_radius_file(fname, top);
            free(fname);
        }
        else if(buffer[skip_spaces] == '#')
        {
            continue;
        }
        else
        {
            char *name, *next;
            name = strtok(buffer, " \t\n");

            if (name == NULL)
                continue;

            if (*name == '}' && name[1] == '\0')
            {
                VERB("end RADIUS section %s", top->name);
                top = top->parent;
                continue;
            }

            next = strtok(NULL, " \t\n");
            if (next == NULL)
            {
                VERB("processing RADIUS parameter %s", name);
                make_rp(RP_FLAG, name, NULL, top);
            }
            else if (*next == '=' && next[1] == '\0')
            {
                char *val = strtok(NULL, "\n");
                VERB("processing RADIUS attribute %s = %s", name, val ? val : "");
                make_rp(RP_ATTRIBUTE, name, val, top);
            }
            else
            {
                if (*next == '{' && next[1] == '\0')
                    next = NULL;
                VERB("start RADIUS section %s %s", name, next ? next : "");
                top = make_rp(RP_SECTION, name, next, top);
            }

        }
    }
    if (top != initial_top)
        ERROR("section %s is not closed!!!", top->name);
}

static int
write_radius(radius_parameter *top, te_bool recursive)
{
    FILE *outfile;
    int indent = 0;
    int i;

    if (top->kind != RP_FILE)
    {
        ERROR("attempt to write a RADIUS branch that is not a file");
        return EINVAL;
    }
    ds_config_touch(top->backup_index);
    outfile = fopen(top->name, "w");
    if (outfile == NULL)
    {
        int rc = errno;
        ERROR("cannot open %s: %s", top->name, strerror(rc));
        return rc;
    }

    RING("updating RADIUS config %s", top->name);
    for (top = top->children; top != NULL;)
    { 
        if (!top->deleted)
        {
            for(i = indent; i > 0; i--)
                putc(' ', outfile);
            switch(top->kind)
            {
                case RP_FLAG:
                    fputs(top->name, outfile);
                    fputc('\n', outfile);
                    break;
                case RP_ATTRIBUTE:
                    fprintf(outfile, "%s = %s\n", top->name, top->value);
                    break;
                case RP_SECTION:
                    fprintf(outfile, "%s %s {\n", top->name, top->value == NULL ? "" : top->value);
                    if (top->children == NULL)
                    {
                        for(i = indent; i > 0; i--)
                            putc(' ', outfile);
                        fputs("}\n", outfile);
                    }
                    else
                    {
                        top = top->children;
                        indent += 4;
                        continue;
                    }
                    break;
                case RP_FILE:
                    if (recursive)
                        write_radius(top, TRUE);
            }
        }
        while (top->next == NULL && top->parent != NULL)
        {
            top = top->parent;
            indent -= 4;
            for(i = indent; i > 0; i--)
                putc(' ', outfile);
            fputs("}\n", outfile);
        }
        top = top->next;
    }
    fclose(outfile);
    return 0;
}


static radius_parameter *
find_rp (radius_parameter *top, const char *name, te_bool create, 
         te_bool (*enumerator)(radius_parameter *, void *), void *extra)
{
    const char *next  = NULL;
    const char *value = NULL;
    int         name_length;
    int         value_counter;
    
    if (*name == '.')
        name++;
    else
    {
        for (; top->parent != NULL; top = top->parent)
            ;
    }
    for (; *name == '.'; name++)
    {
        for (top = top->parent; top->kind == RP_FILE; top = top->parent)
            ;
    }
    for (next = name; *next != '\0' && *next != '.'; next++)
    {
        if (value == NULL && (*next == ':' || *next == '#'))
            value = next + 1;
    }
    name_length = (value == NULL ? next : value - 1) - name;
    for (top = top->children; top != NULL; )
    {
        if (top->kind == RP_FILE)
            top = top->children;
        else
        {
            if (create || !top->deleted)
            {
                if (strncmp(top->name, name, name_length) == 0 &&
                    top->name[name_length] == '\0')
                {
                    if (value == NULL || 
                        (top->value != NULL && 
                         strncmp(top->value, value, value - name - 1) == 0 &&
                         top->value[value - name - 1] == '\0'))
                    {
                        if (enumerator == NULL || enumerator(top, extra))
                            break;
                    }
                }
            }
            if (top->next == NULL)
            {
                if (top->parent != NULL && 
                    top->parent->kind == RP_FILE)
                {
                    top = top->parent;
                }
                else if (create)
                {
                    top = make_rp(*next == '\0' ? RP_ATTRIBUTE : RP_SECTION, 
                                  name, value, top->parent);
                    break;
                }
            }
            top = top->next;
        }
    }
    
    return top == NULL || *next == '\0' ? top : 
        find_rp(top, next + 1, create, enumerator, extra);
}

static te_bool
retrieve_rp (radius_parameter *top, const char *name, const char **value)
{
    radius_parameter *rp = find_rp(top, name, FALSE, NULL, NULL);
    if (rp != NULL)
    {
        if (value != NULL)
            *value = rp->value;
        return TRUE;       
    }
    if (value != NULL)
        *value = NULL;
    return FALSE;
}

static char *
expand_rp (const char *value, radius_parameter *top)
{
    int value_len = strlen(value) + 1;
    char *new_val = malloc(value_len);
    char *new_ptr;
    char *next;

    memcpy(new_val, value, value_len);
    for (new_ptr = new_val; *new_ptr != '\0'; new_ptr++)
    {
        if (*new_ptr == '$' && new_ptr[1] == '{' && 
            (next = strchr(new_ptr, '}')) != NULL)
        {
            const char *rp_val;
            int rpv_len;
            int diff_len;

            *next = '\0';
            if (!retrieve_rp (top, new_ptr + 2, &rp_val))
            {
                ERROR("Undefined RADIUS parameter: %s", new_ptr + 2);
                continue;
            }
            rpv_len = strlen(rp_val);
            diff_len = rpv_len - (next - new_ptr + 1);
            if (diff_len > 0)
            {
                value_len += diff_len;
                diff_len = new_ptr - new_val;
                new_val = realloc(new_val, value_len);
                new_ptr = new_val + diff_len;
            }
            memmove(new_ptr + rpv_len, next + 1, strlen(next + 1) + 1);
            memcpy(new_ptr, rp_val, rpv_len);
            new_ptr += rpv_len - 1;
        }
    }
    return new_val;
}

static int
update_rp (radius_parameter *top, enum radius_parameters kind, 
           const char *name, const char *value)
{
    radius_parameter *rp = find_rp(top, name, TRUE, NULL, NULL);
    radius_parameter *file;
    
    if (rp == NULL)
    {
        return ENOENT;
    }

    if (rp->kind != RP_SECTION && rp->value != NULL)
        free(rp->value);
    if (value == RP_DELETE_VALUE)
    {
        rp->deleted = TRUE;
        if (rp->kind != RP_SECTION)
            rp->value = NULL;
    }
    else
    {
        rp->kind = kind;
        rp->deleted = FALSE;
        if (rp->kind != RP_SECTION)
            rp->value = value ? strdup(value) : NULL;
    }

    for (file = rp->parent; file->kind != RP_FILE; file = file->parent)
        ;
    write_radius(file, FALSE);
    return 0;
}

static radius_user *
make_radius_user (const char *name)
{
    radius_user *user = malloc(sizeof(*user));
    user->name = strdup(name);
    user->records = malloc(sizeof(*user->records));
    user->last_record = user->records;
    user->records->checks = user->records->last_check = NULL;
    user->records->sets   = user->records->last_set   = NULL;
    user->records->next   = NULL;
    user->next   = NULL;
    if (radius_last_user != NULL)
        radius_last_user->next = user;
    else
        radius_users = user;
    radius_last_user = user;
    return user;
}

static radius_action *
make_radius_action (const char *attribute, const char *op, const char *value)
{
    radius_action *action = malloc(sizeof(*action));

    action->attribute = strdup(attribute);
    action->operator  = strdup(op);
    action->value     = strdup(value);
    action->next      = NULL;
    return action;
}

static radius_user *
find_radius_user (const char *name)
{
    radius_user *user;

    for (user = radius_users; user != NULL; user = user->next)
    {
        if (strcmp(user->name, name) == 0)
        {
            break;
        }
    }
    return user;
}

static void
write_radius_users (FILE *conf, radius_user *users)
{
    radius_user_record *rec;
    radius_action *action;

    for (; users != NULL; users = users->next)
    {
        for (rec = users->records; rec != NULL; rec = rec->next)
        {
            fputs(users->name, conf);
            fputc(' ', conf);
            for (action = rec->checks; action != NULL; action = action->next)
            {
                fprintf(conf, "%s %s %s%s", 
                        action->attribute, action->operator, action->value,
                        action->next == NULL ? "\n" : ", ");
            }
            for (action = rec->sets; action != NULL; action = action->next)
            {
                fprintf(conf, "    %s %s %s%s\n", 
                        action->attribute, action->operator, action->value,
                        action->next == NULL ? "" : ",");
            }
            fputc('\n', conf);
        }
    }
}


static char *radius_daemon = NULL;

static int
ds_radiusserver_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    int rc;

    UNUSED(oid);
    UNUSED(instance);

    VERB("Querying RADIUS status");
    if (radius_daemon != NULL)
        return daemon_get(gid, radius_daemon, value);

    rc = daemon_get(gid, "radiusd", value);
    if (rc != 0)
        return rc;
    if (*value != '0')
        radius_daemon = "radiusd";
    else
    {
        rc = daemon_get(gid, "freeradius", value);
        if (rc != 0)
        {
            *value = '0';
            return 0;
        }
        if (*value != '0')
            radius_daemon = "freeradius";
    }
    if (radius_daemon != NULL)
        INFO("RADIUS server is named %s", radius_daemon);
    return 0;
}


static int
ds_radiusserver_set(unsigned int gid, const char *oid,
                    const char *value, const char *instance, ...)
{
    int rc1, rc2;

    UNUSED(oid);
    UNUSED(instance);
    if (radius_daemon != NULL)
        return daemon_set(gid, radius_daemon, value);

    rc1 = daemon_set(gid, "radiusd", value);
    rc2 = daemon_set(gid, "freeradius", value);

    if (rc1 == 0 && rc2 != 0)
        radius_daemon = "radiusd";
    else if (rc1 != 0 && rc2 == 0)
        radius_daemon = "freeradius";
    if (radius_daemon != NULL)
        INFO("RADIUS server is named %s", radius_daemon);
    return rc1 != 0 && rc2 != 0 ? rc2 : 0;
}

static int
ds_radius_accept_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
}

static int
ds_radius_accept_set(unsigned int gid, const char *oid,
                     const char *value, const char *instance, ...)
{
}

static int
ds_radius_user_add(unsigned int gid, const char *oid,
                   const char *value, const char *instance, ...)
{
    make_radius_user(instance);
}

static int
ds_radius_user_del(unsigned int gid, const char *oid,
                   const char *instance, ...)
{
    return TE_RC(TE_TA_LINUX, ETENOSUPP);
}

static int
ds_radius_user_list(unsigned int gid, const char *oid,
                    char **list,
                    const char *instance, ...)
{
    radius_user *user;
    int size;
    char *iter;
    
    for (user = radius_users, size = 0; user != NULL; size += strlen(user->name) + 1, user = user->next)
        ;
    *list = malloc(size + 1);
    for (user = radius_users, iter = *list; user != NULL; user = user->next, iter += size + 1)
    {
        size = strlen(user->name);
        memcpy(iter, user->name, size);
        iter[size] = ' ';
    }
    *iter = '\0';
    return 0;
}

static char client_buffer[256];

static int
ds_radius_client_add(unsigned int gid, const char *oid,
                   const char *value, const char *instance, ...)
{
    snprintf(client_buffer, sizeof(client_buffer), "client:%s", instance);
    RING("adding RADIUS client %s for %s", client_buffer, oid);
    update_rp(radius_conf, RP_SECTION, client_buffer, NULL);
    RING("added client %s", client_buffer);
    return 0;
}

static int
ds_radius_client_del(unsigned int gid, const char *oid,
                   const char *instance, ...)
{
    snprintf(client_buffer, sizeof(client_buffer), "client:%s", instance);
    update_rp(radius_conf, RP_SECTION, client_buffer, RP_DELETE_VALUE);
    return 0;
}

static te_bool client_count(radius_parameter *rp, void *extra)
{
    int *count = extra;
    UNUSED(rp);
    (*count) += strlen(rp->name) + 1;
    return FALSE;
}

static te_bool client_list(radius_parameter *rp, void *extra)
{
    char **iter = extra;
    int len = strlen(rp->value);
    memcpy(*iter, rp->name, len);
    (*iter)[len] = ' ';
    (*iter) += len + 1;
    return FALSE;
}

static int
ds_radius_client_list(unsigned int gid, const char *oid,
                    char **list,
                    const char *instance, ...)
{
    int size = 0;
    char *c_iter;

    find_rp(radius_conf, "client", FALSE, client_count, &size);
    c_iter = *list = malloc(size + 1);
    find_rp(radius_conf, "client", FALSE, client_list, &c_iter);
    *c_iter = '\0';
    return 0;
}

static int
ds_radius_secret_get(unsigned int gid, const char *oid,
                             char *value, const char *instance, ...)
{
}

static int
ds_radius_secret_set(unsigned int gid, const char *oid,
                     const char *value, const char *instance, ...)
{
}

RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_user_accept_attrs, "accept-attrs",
                    NULL, NULL,
                    ds_radius_accept_get, ds_radius_accept_set);
RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_user_challenge_attrs, "challenge-attrs",
                    NULL, &node_ds_radiusserver_user_accept_attrs,
                    ds_radiusserver_get, ds_radiusserver_set);
RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_user_check, "check",
                    NULL, &node_ds_radiusserver_user_challenge_attrs,
                    ds_radiusserver_get, ds_radiusserver_set);
RCF_PCH_CFG_NODE_COLLECTION(node_ds_radiusserver_user, "user",
                            &node_ds_radiusserver_user_check,
                            NULL,
                            ds_radius_user_add,
                            ds_radius_user_del,
                            ds_radius_user_list,
                            NULL);
RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_client_secret, "secret",
                    NULL, NULL,
                    ds_radius_secret_get, ds_radius_secret_set);
RCF_PCH_CFG_NODE_COLLECTION(node_ds_radiusserver_client, "client",
                            &node_ds_radiusserver_client_secret,
                            &node_ds_radiusserver_user,
                            ds_radius_client_add,
                            ds_radius_client_del,
                            ds_radius_client_list,
                            NULL);

static int
ds_radiusserver_netaddr_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    const char *v;
    retrieve_rp(radius_conf, "listen.ipaddr", &v);
    strcpy(value, v);
}

static int
ds_radiusserver_netaddr_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    update_rp(radius_conf, RP_ATTRIBUTE, "listen#0.ipaddr", value);
    update_rp(radius_conf, RP_ATTRIBUTE, "listen#1.ipaddr", value);
}

static int
ds_radiusserver_acctport_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    const char *v;
    retrieve_rp(radius_conf, "listen#1.port", &v);
    strcpy(value, v);
}

static int
ds_radiusserver_acctport_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    update_rp(radius_conf, RP_ATTRIBUTE, "listen#1.port", value);
}

static int
ds_radiusserver_authport_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    const char *v;
    retrieve_rp(radius_conf, "listen#0.port", &v);
    strcpy(value, v);
}

static int
ds_radiusserver_authport_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    update_rp(radius_conf, RP_ATTRIBUTE, "listen#0.port", value);
}

RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_net_addr, "net_addr",
                    NULL, &node_ds_radiusserver_client,
                    ds_radiusserver_netaddr_get, 
                    ds_radiusserver_netaddr_set);
RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_acct_port, "acct_port",
                    NULL, &node_ds_radiusserver_net_addr,
                    ds_radiusserver_acctport_get, ds_radiusserver_acctport_set);
RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_auth_port, "auth_port",
                    NULL, &node_ds_radiusserver_acct_port,
                    ds_radiusserver_authport_get, ds_radiusserver_authport_set);
RCF_PCH_CFG_NODE_RW(node_ds_radiusserver, "radiusserver",
                    &node_ds_radiusserver_auth_port, NULL,
                    ds_radiusserver_get, ds_radiusserver_set);

const char *radius_ignored_params[] = {"bind_address", 
                                       "port", 
                                       "listen",
                                       "client",
                                       "authorize",
                                       "authenticate",
                                       "post-auth",
                                       NULL
};

static te_bool
rp_delete_all(radius_parameter *rp, void *extra)
{
    UNUSED(extra);
    RING("Wiping out RADIUS parameter %s %s", rp->name, 
         rp->value == NULL ? "" : rp->value);
    if (rp->kind != RP_SECTION && rp->value != NULL)
    {
        free(rp->value);
        rp->value = NULL;
    }
    rp->deleted = TRUE;
    for (rp = rp->parent; rp->kind != RP_FILE; rp = rp->parent)
        ;
    write_radius(rp, FALSE);
    return FALSE;
}

void
ds_init_radius_server (rcf_pch_cfg_object **last)
{
    const char **ignored;
    RING("Initializing RADIUS");
    if (file_exists("/etc/raddb/radiusd.conf"))
        radius_conf = read_radius_file("/etc/raddb/radiusd.conf", NULL);
    else if (file_exists("/etc/freeradius/radiusd.conf"))
        radius_conf = read_radius_file("/etc/freeradius/radiusd.conf", NULL);
    else
    {
        WARN("No RADIUS config found");
        return;
    }
    DS_REGISTER(radiusserver);
    for (ignored = radius_ignored_params; *ignored != NULL; ignored++)
    {
        find_rp(radius_conf, *ignored, FALSE, rp_delete_all, NULL);
    }
}

#endif /* ! WITH_RADIUS_SERVER */
