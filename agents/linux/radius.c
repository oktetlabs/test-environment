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
    te_bool modified;
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
    parm->modified = FALSE;
    parm->kind = kind;
    parm->name = name ? strdup(name) : NULL;
    if (value != NULL)
    {
        parm->value = expand_rp(value, parent);
    }
    else
    {
        parm->value = NULL;
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
    int line_count = 0;
    
    while (fgets(buffer, sizeof(buffer) - 1, conf) != NULL)
    {
        line_count++;
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
                if (top->kind != RP_SECTION)
                    ERROR("extra closing brace found at line %d", line_count);
                else
                {
                    VERB("end RADIUS section %s", top->name);
                    top = top->parent;
                }
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

static int write_radius(radius_parameter *top);

static void
write_radius_parameter (FILE *outfile, radius_parameter *parm, int indent)
{
    if (!parm->deleted)
    {
        int i;
        
        for(i = indent; i > 0; i--)
            putc(' ', outfile);
        switch(parm->kind)
        {
            case RP_FLAG:
                fputs(parm->name, outfile);
                fputc('\n', outfile);
                break;
            case RP_ATTRIBUTE:
                if (parm->value != NULL)
                    fprintf(outfile, "%s = %s\n", parm->name, parm->value);
                break;
            case RP_SECTION:
            {
                radius_parameter *child;
                fprintf(outfile, "%s %s {\n", parm->name, 
                        parm->value == NULL || *parm->value == '#' ? "" : parm->value);
                for (child = parm->children; child != NULL; child = child->next)
                    write_radius_parameter(outfile, child, indent + 4);
                for(i = indent; i > 0; i--)
                    putc(' ', outfile);
                fputs("}\n", outfile);
                break;
            }
            case RP_FILE:
                fprintf(outfile, "$INCLUDE %s\n", parm->name);
                write_radius(parm);
        }
    }
}

static int
write_radius(radius_parameter *top)
{
    FILE *outfile;
    int indent = 0;
    int i;

    if (top->kind != RP_FILE)
    {
        ERROR("attempt to write a RADIUS branch that is not a file");
        return EINVAL;
    }
    if (!top->modified)
        VERB("nothing to write");
    top->modified = FALSE;
    ds_config_touch(top->backup_index);
    outfile = fopen(top->name, "w");
    if (outfile == NULL)
    {
        int rc = errno;
        ERROR("cannot open %s: %s", top->name, strerror(rc));
        return rc;
    }

    INFO("updating RADIUS config %s", top->name);
    for (top = top->children; top != NULL; top = top->next)
    { 
        write_radius_parameter(outfile, top, 0);
    }
    fclose(outfile);
    return 0;
}


static radius_parameter *
resolve_rp_name (radius_parameter *origin, const char **name)
{
    if (**name == '.')
        (*name)++;
    else
    {
        for (; origin->parent != NULL; origin = origin->parent)
            ;
    }
    for (; **name == '.'; (*name)++)
    {
        for (origin = origin->parent; origin->kind == RP_FILE; origin = origin->parent)
            ;
    }
    return origin;
}


static radius_parameter *
find_rp (radius_parameter *base, const char *name, te_bool create, te_bool create_now,
         te_bool (*enumerator)(radius_parameter *rp, void *extra), void *extra)
{
    radius_parameter *iter;
    const char *next  = NULL;
    const char *value = NULL;
    int         name_length;
    int         value_length  = 0;

    VERB("looking for RADIUS parameter %s", name);
    for (next = name; *next != '\0' && *next != '.'; next++)
    {
        if (*next == '(')
        {
            int nesting = 0;
            
            value = next + 1;
            do
            {
                switch(*next++)
                {
                    case '\0':
                        ERROR("missing close parenthesis in %s", name);
                        return NULL;
                    case '(':
                        nesting++;
                        break;
                    case ')':
                        nesting--;
                        break;
                    default:
                        /* do nothing */
                        ;
                }
            }
            while (nesting > 0);
            value_length = next - value - 1;
        }
        if (value != NULL)
        {
            if (*next != '.' && *next != '\0')
            {
                ERROR("syntax error in RADIUS parameter name %s", name);
                return NULL;
            }
            break;
        }
    }
    
    name_length = (value == NULL ? next : value - 1) - name;
    for (iter = base->children; iter != NULL; iter = iter->next)
    {
        if (iter->kind == RP_FILE)
        {
            radius_parameter *tmp = find_rp(iter, name, create, FALSE, enumerator, extra);
            if (tmp != NULL)
            {
                iter = tmp;
                break;
            }
        }
        else
        {
            if (create || !iter->deleted)
            {
                if (strncmp(iter->name, name, name_length) == 0 &&
                    iter->name[name_length] == '\0')
                {
                    if (value == NULL || 
                        (iter->value != NULL && 
                         strncmp(iter->value, value, value_length) == 0 &&
                         iter->value[value_length] == '\0'))
                    {
                        if (enumerator == NULL || enumerator(iter, extra))
                        {
                            if (iter->deleted)
                                iter->deleted = FALSE;
                            break;
                        }
                    }
                }
            }
        }
    }
    if (iter == NULL && create_now)
    {
        iter = make_rp(*next == '\0' ? RP_ATTRIBUTE : RP_SECTION, 
                       NULL, NULL, base);
        iter->name = malloc(name_length + 1);
        memcpy(iter->name, name, name_length);
        iter->name[name_length] = '\0';
        if (value != NULL)
        {
            iter->value = malloc(value_length + 1);
            memcpy(iter->value, value, value_length);
            iter->value[value_length] = '\0';
        }
        VERB("created RADIUS parameter %s %s", iter->name, iter->value ? iter->value : "");
    }
    
    
    return iter == NULL || *next == '\0' ? iter : 
        find_rp(iter, next + 1, create, create, enumerator, extra);
}

static te_bool
retrieve_rp (radius_parameter *top, const char *name, const char **value)
{
    radius_parameter *rp;

    top = resolve_rp_name(top, &name);
    rp = find_rp(top, name, FALSE, FALSE, NULL, NULL);
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
mark_rp_changes(radius_parameter *rp)
{
    radius_parameter *file;
    for (file = rp->parent; file->kind != RP_FILE; file = file->parent)
        ;
    file->modified = TRUE;
}

static void
wipe_rp_section (radius_parameter *rp)
{
    mark_rp_changes(rp);
    for (rp = rp->children; rp != NULL; rp = rp->next)
    {
        if (rp->kind != RP_FILE)
        {
            rp->deleted = TRUE;
            if (rp->kind != RP_SECTION && rp->value != NULL)
            {
                free(rp->value);
                rp->value = NULL;
            }
        }
        if (rp->kind == RP_FILE || rp->kind == RP_SECTION)
        {
            wipe_rp_section(rp);
        }
    }
}

static int
update_rp (radius_parameter *top, enum radius_parameters kind, 
           const char *name, const char *value)
{
    radius_parameter *rp = find_rp(top, name, TRUE, TRUE, NULL, NULL);

    if (rp == NULL)
    {
        ERROR("RADIUS parameter %s not found", name);
        return ENOENT;
    }

    if (rp->kind != RP_SECTION && rp->value != NULL)
        free(rp->value);
    if (value == RP_DELETE_VALUE)
    {
        rp->deleted = TRUE;
        if (rp->kind != RP_SECTION)
            rp->value = NULL;
        else
        {
            wipe_rp_section(rp);
        }
        VERB("deleted RADIUS parameter %s", name);
    }
    else
    {
        rp->deleted = FALSE;
        rp->kind = kind;
        if (rp->kind != RP_SECTION || rp->value == NULL)
            rp->value = value ? strdup(value) : NULL;

        VERB("updated RADIUS parameter %s to %s", name, rp->value ? rp->value : "empty");
    }
    mark_rp_changes(rp);

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
                     const char *value, const char *unused, 
                     const char *client_name)
{
    int rc;
    UNUSED(unused);
    UNUSED(value);
    UNUSED(oid);
    UNUSED(gid);
    snprintf(client_buffer, sizeof(client_buffer), "client(%s)", client_name);
    RING("adding RADIUS client %s for %s", client_buffer, oid);
    rc = update_rp(radius_conf, RP_SECTION, client_buffer, NULL);
    if (rc == 0)
    {
        snprintf(client_buffer, sizeof(client_buffer), "client(%s).secret", client_name);
        rc = update_rp(radius_conf, RP_ATTRIBUTE, client_buffer, NULL);
        if (rc == 0)
        {
            write_radius(radius_conf);
            RING("added client %s", client_buffer);
        }
    }
    return rc;
}

static int
ds_radius_client_del(unsigned int gid, const char *oid,
                     const char *instance, 
                     const char *client_name, ...)
{
    snprintf(client_buffer, sizeof(client_buffer), "client(%s)", client_name);
    update_rp(radius_conf, RP_SECTION, client_buffer, RP_DELETE_VALUE);
    write_radius(radius_conf);
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

    find_rp(radius_conf, "client", FALSE, FALSE, client_count, &size);
    c_iter = *list = malloc(size + 1);
    find_rp(radius_conf, "client", FALSE, FALSE, client_list, &c_iter);
    *c_iter = '\0';
    return 0;
}

static int
ds_radius_secret_get(unsigned int gid, const char *oid,
                     char *value, const char *instance,
                     const char *client_name, ...)
{
    const char *val;
    RING("getting client secret");
    snprintf(client_buffer, sizeof(client_buffer), "client(%s).secret", client_name);
    if(!retrieve_rp(radius_conf, client_buffer, &val))
    {
        ERROR("Client %s not found", client_name);
        return TE_RC(TE_TA_LINUX, ENOENT);
    }
    if (val == NULL)
        *value = '\0';
    else
        strcpy(value, val);
    return 0;
}

static int
ds_radius_secret_set(unsigned int gid, const char *oid,
                     const char *value, const char *instance,
                     const char *client_name)
{
    RING("setting client secret");
    snprintf(client_buffer, sizeof(client_buffer), "client(%s).secret", client_name);
    update_rp(radius_conf, RP_SECTION, client_buffer, value);    
    write_radius(radius_conf);
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
    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#auth).ipaddr", value);
    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#acct).ipaddr", value);
    write_radius(radius_conf);
}

static int
ds_radiusserver_acctport_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    const char *v;
    retrieve_rp(radius_conf, "listen(#acct).port", &v);
    strcpy(value, v);
}

static int
ds_radiusserver_acctport_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#acct).port", value);
    write_radius(radius_conf);
}

static int
ds_radiusserver_authport_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    const char *v;
    retrieve_rp(radius_conf, "listen(#auth).port", &v);
    strcpy(value, v);
}

static int
ds_radiusserver_authport_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#auth).port", value);
    write_radius(radius_conf);
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
                                       "modules",
                                       "instantiate",
                                       "authorize",
                                       "authenticate",
                                       "preacct",
                                       "accounting",
                                       "session",
                                       "post-auth",
                                       "pre-proxy",
                                       "post-proxy",
                                       NULL
};

#define RP_DEFAULT_EMPTY_SECTION ((const char *)-1)

const char *radius_predefined_params[] = {
    "listen(#auth).type", "auth",
    "listen(#acct).type", "acct",
    "modules.pap.encryption_scheme", "crypt",
    "modules.chap.authtype", "chap",
    "modules.files.usersfile", "/tmp/te_radius_users",
    "modules.eap.default_eap_type", "md5",
    "modules.eap.md5", RP_DEFAULT_EMPTY_SECTION,
    "modules.eap.gtc.auth_type", "PAP",
    "modules.eap.mschapv2", RP_DEFAULT_EMPTY_SECTION,
    "modules.mschap.authtype", "MS-CHAP",
    "authorize.chap", NULL,
    "authorize.mschap", NULL,
    "authorize.eap", NULL,
    "authorize.files", NULL,
    "authenticate.Auth-Type(PAP).pap", NULL,
    "authenticate.Auth-Type(CHAP).chap", NULL,
    "authenticate.Auth-Type(MS-CHAP).mschap", NULL,
    "authenticate.eap", NULL,
    /* "post-auth.files", NULL, */ 
    NULL, NULL
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
    if (rp->kind == RP_SECTION)
        wipe_rp_section(rp);
    mark_rp_changes(rp);
    return FALSE;
}

void
ds_init_radius_server (rcf_pch_cfg_object **last)
{
    const char **ignored;
    const char **defaulted;

    RING("Initializing RADIUS");
    if (file_exists("/etc/raddb/radiusd.conf"))
        radius_conf = read_radius_file("/etc/raddb/radiusd.conf", NULL);
    else if (file_exists("/etc/freeradius/radiusd.conf"))
        radius_conf = read_radius_file("/etc/freeradius/radiusd.conf", NULL);
    else
    {
        ERROR("No RADIUS config found");
        return;
    }
    DS_REGISTER(radiusserver);
    for (ignored = radius_ignored_params; *ignored != NULL; ignored++)
    {
        find_rp(radius_conf, *ignored, FALSE, FALSE, rp_delete_all, NULL);
    }
    for (defaulted = radius_predefined_params; *defaulted != NULL; defaulted += 2)
    {
        update_rp(radius_conf, defaulted[1] == NULL ? RP_FLAG :
                  (defaulted[1] == RP_DEFAULT_EMPTY_SECTION ? RP_SECTION : 
                   RP_ATTRIBUTE), *defaulted, 
                  defaulted[1] == RP_DEFAULT_EMPTY_SECTION ? NULL : defaulted[1]);
    }
    write_radius(radius_conf);
}

#endif /* ! WITH_RADIUS_SERVER */
