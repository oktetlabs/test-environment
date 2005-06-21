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

#include "te_config.h"
#include "config.h"


enum radius_parameters { RP_FLAG, RP_ATTRIBUTE, RP_SECTION, RP_FILE};

typedef struct radius_parameter
{
    te_bool deleted;
    enum radius_parameters kind;
    char *name;
    char *value;
    struct radius_parameter *parent;
    struct radius_parameter *next;
    struct radius_parameter *children, *last_child;
} radius_parameter;


static struct radius_parameters *radius_conf;

typedef struct radius_action
{
    char *attribute;
    char *operator;
    char *value;
    struct radius_action *next;
} radius_action;

typedef struct radius_user
{
    char *name;
    radius_action *checks;
    radius_action *sets;
    struct radius_user *next;
} radius_user;

static struct radius_user *radius_users, *radius_last_user;

static radius_parameter *
make_rp(enum radius_parameters kind,  const char *name, 
        const char *value, radius_parameter *parent)
{
    radius_parameter *parm = malloc(*parm);
    parm->deleted = FALSE;
    parm->ref = TRUE;
    parm->kind = kind;
    parm->name = name ? strdup(name) : NULL;
    parm->value = value ? strdup(value) : NULL;
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
}

static radius_parameter *
read_radius_file (const char *filename, radius_parameter *top)
{
    FILE *newfile = fopen();
    radius_parameter *fp = make_rp(RP_FILE, FALSE, filename, NULL, top);
    read_radius(newfile, fp);
    fclose(newfile);
    return fp;
}

static void 
read_radius(FILE *conf, radius_parameter *top)
{
    static char buffer[256];
    
    while (fgets(buffer, sizeof(buffer) - 1, conf) != NULL)
    {
        if (strncmp(buffer, "$INCLUDE", 8) == 0)
        {
            strtok(buffer, " \t\n");
            read_radius_file(strtok(NULL, " \t\n"), top);
        }
        else if(*buffer == '#')
        {
            continue;
        }
        else
        {
            char *name, *next;
            name = strtok(buffer, " \t\n");

            if (*name == '}' && name[1] == '\0')
            {
                top = top->parent;
                continue;
            }

            next = strtok(NULL, " \t\n");
            if (next == NULL)
            {
                make_rp(RP_FLAG, FALSE, name, NULL, top);
            }
            else if (*next == '=' && next[1] == '\0')
            {
                char *val = strtok(NULL, "\n");
                te_bool ref = (*val == '$' && val[1] == '{');
                if (ref)
                {
                    val += 2;
                    val[strlen(val) - 1] = '\0';
                }
                make_rp(RP_ATTRIBUTE, ref, name, val, top);
            }
            else
            {
                if (*next == '{' && next[1] == '\0')
                    next = NULL;
                top = make_rp(RP_SECTION, FALSE, name, next, top);
            }
        }
    }
}

static void
write_radius(radius_parameter *top, te_bool recursive)
{
    FILE *outfile;
    int indent = 0;
    int i;

    if (top->type != RP_FILE)
        return;
    outfile = fopen(top->name, "w");
    for (top = top->children; top != NULL; top = top->next)
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
                break;
            case RP_FILE:
                if (recursive)
                    write_radius(top, TRUE);
        }
        if (top->next == NULL)
        {
            top = top->parent;
            indent -= 4;
            for(i = indent; i > 0; i--)
                putc(' ', outfile);
            puts("}", outfile);
        }
    }
    fclose(outfile);
}

static radius_parameter *
lookup_rp (radius_parameter *list, const char *name, int namelen, 
           const char *sectname, int snamelen, int seqno)
{
}

static radius_parameter *
find_rp (radius_parameter *top, const char *name)
{
    if (*name == '/')
    {
        const char *sep = strsep(name, "/:");
        const char *next = (sep == NULL || *sep == '/' ? sep : strchr(sep, '/'));
    }
    else 
    {
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
        return lookup_parameter();
    }
}

static te_bool
retrieve_rp (radius_parameter *top, const char *name, const char **value)
{
    radius_parameter *rp = find_rp(top, name, FALSE);
    if (rp != NULL)
    {
        if (rp->ref)
            return retrieve_fp (top, rp->value, value);       
        *value = rp->value;
        return TRUE;       
    }
    *value = NULL;
    return FALSE;
}

static int
update_rp (radius_parameter *top, const char *name, const char *value)
{
    radius_parameter *rp = find_rp(top, name, TRUE);
    radius_parameter *file;
    FILE *conf;
    
    if (rp == NULL)
    {
        return -1;
    }

    if (rp->value != NULL)
        free(rp->value);
    rp->ref = FALSE;
    if (value == RP_DELETE_VALUE)
    {
        rp->deleted = TRUE;
        rp->value = NULL;      
    }
    else
    {
        rp->value = value ? strdup(value) : NULL;
    }

    for (file = rp->parent; file->type != RP_FILE; file = file->parent)
        ;
    write_radius(file, FALSE);
}
