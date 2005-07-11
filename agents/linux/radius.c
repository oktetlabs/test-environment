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
#include <stddef.h>
#include "linuxconf_daemons.h"
#include <sys/wait.h>

/** RADIUS configuration parameter types */
enum radius_parameters { 
    RP_FLAG,          /**< a parameter which has no value */
    RP_ATTRIBUTE,     /**< a parameter with the value */
    RP_SECTION,       /**< a (sub)section */ 
    RP_FILE           /**< an included config file */
};    

/** Node of the RADIUS configuration parameter */
typedef struct radius_parameter
{
    te_bool deleted;
    enum radius_parameters kind;
    char *name;
    char *value;
    /* The following two are only meaningful for RP_FILE */
    int backup_index;
    te_bool modified;
    struct radius_parameter *parent;
    struct radius_parameter *next;
    struct radius_parameter *children, *last_child;
} radius_parameter;


#define RP_DELETE_VALUE ((char *)-1)

static struct radius_parameter *radius_conf;

/** An attribute==value check in RADIUS users file */
typedef struct radius_user_check
{
    char *attribute;
    char *value;
} radius_user_check;

/** A reply record for a RADIUS user */
typedef struct radius_user_reply
{
    char *attribute;    /**< Attribute to set */
    char *in_accept;    /**< A value to set in Access-Accept packets */
    char *in_challenge; /**< A value to set in Access-Challenge packets */
} radius_user_reply;

/** A record for a RADIUS user */
typedef struct radius_user
{
    te_bool reject;
    char *name;
    radius_user_check *checks;
    int n_replies;
    int n_checks;
    radius_user_reply *replies;
    struct radius_user *next;
} radius_user;

/** A supplicant <-> interface correspondence */
typedef struct supplicant
{
    char *interface;          /** interface name */
    pid_t process;            /** Xsupplicant process PID
                                  (pid_t)-1 when not running
                              */
    radius_parameter *config; /** Supplicant config tree */
    struct supplicant *next;  /** chain link */
} supplicant;

static FILE *radius_users_file;
static struct radius_user *radius_users, *radius_last_user;

static char *expand_rp (const char *value, radius_parameter *top);

/** 
 * Creates a new RADIUS parameter inside 'parent' 
 */
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

/**
 * Destroys the parameter and all its children if any 
 * Note: this function does not exclude the parameter
 * from its parent's children list, so it's normally
 * should be called on a topmost parameter only.
 */
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

/**
 * Reads a RADIUS config file named 'filename' and creates 
 * a RP_FILE record inside 'top'. All the parameters read
 * from the file will be inside that record.
 */
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

/**
 * Reads lines from 'conf' until EOF, skips comments and
 * creates RADIUS parameters inside 'top'
 */
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

/**
 * Writes a single RADIUS parameter 'parm' to 'outfile'
 * precedent by 'indent' spaces
 */
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

/**
 * Updates a RADIUS config file corresponding to 'top' which must
 * be a RP_FILE record.
 * If the record has not been modified, all the RP_FILE subrecords
 * are still attempted to update
 */
static int
write_radius(radius_parameter *top)
{
    FILE *outfile;

    if (top->kind != RP_FILE)
    {
        ERROR("attempt to write a RADIUS branch that is not a file");
        return EINVAL;
    }
    if (!top->modified)
    {
        for (top = top->children; top != NULL; top = top->next)
        { 
            if (top->kind == RP_FILE)
                write_radius(top);
        }
    }
    else
    {
        top->modified = FALSE;
        ds_config_touch(top->backup_index);
        RING("Writing RADIUS config %s", top->name);
        outfile = fopen(top->name, "w");
        if (outfile == NULL)
        {
            int rc = errno;
            ERROR("cannot open %s: %s", top->name, strerror(rc));
            return rc;
        }

        for (top = top->children; top != NULL; top = top->next)
        { 
            write_radius_parameter(outfile, top, 0);
        }
        fclose(outfile);
    }
    return 0;
}

/**
 * Converts a relative RADIUS parameter name to an absolute one
 */
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


/**
 * Finds a RADIUS parameter inside 'name' and creates it if there isn't one and 
 * 'create' is TRUE.
 * 
 * @param create_now  This parameter is for recursive calls on RP_FILE records.
                      A user should normally set it equal to 'create'.
 * @param enumerator  If not NULL, the function that is called on every parameter
                      with matching name. If it returns FALSE, the parameter is
                      not considered matching.
 * @param extra       Passed to 'enumerator' as its second argument
 */
static radius_parameter *
find_rp (radius_parameter *base, const char *name, te_bool create, te_bool create_now,
         te_bool (*enumerator)(radius_parameter *rp, void *extra), void *extra)
{
    radius_parameter *iter;
    const char *next  = NULL;
    const char *value = NULL;
    int         name_length;
    int         value_length  = 0;
    te_bool     wildcard      = FALSE;

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
    if (name[name_length - 1] == '*')
    {
        wildcard = TRUE;
        name_length--;
    }
    for (iter = base->children; iter != NULL; iter = iter->next)
    {
        if (iter->kind == RP_FILE)
        {
            radius_parameter *tmp = find_rp(iter, name, create, FALSE, enumerator, extra);
            if (tmp != NULL)
                return tmp;
        }
        else
        {
            if (create || !iter->deleted)
            {
                if (strncmp(iter->name, name, name_length) == 0 &&
                    (wildcard || iter->name[name_length] == '\0'))
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
        VERB("created RADIUS parameter %s %s", iter->name, iter->value ? iter->value : "EMPTY");
    }
    
    if (*next != '\0' && iter != NULL && iter->kind != RP_SECTION)
    {
        ERROR("attempting to find %s under %s which is not a section",
              next, iter->name);
        return NULL;
    }

    return iter == NULL || *next == '\0' ? iter :
        find_rp(iter, next + 1, create, create, enumerator, extra);
}

/**
 * Finds a RADIUS parameter 'name' inside 'top'. The name is absolutized.
 *
 * @returns TRUE if the parameter is found, FALSE otherwise
 *
 * @param top   The root of the tree to search
 * @param name  The parameter name
 * @param value The value of the found parameter (OUT)
 */
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

/**
 * Expands a string which may contain references to RADIUS parameters*
 *
 * @returns A malloced string with references expanded.
 */
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

/**
 *  Marks the RP_FILE record containing (may be indirectly) 'top' as modified.
 */
static void
mark_rp_changes(radius_parameter *rp)
{
    radius_parameter *file;
    for (file = rp->parent; file->kind != RP_FILE; file = file->parent)
        ;
    file->modified = TRUE;
}

/**
 * Recursively marks as deleted all descendands of a given node
 */
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

/**
 * Updates a parameter 'name' within 'top' to hold a 'value'.
 * If the parameter does not exist, it is created.
 * If 'value' == RP_DELETED_VALUE, the parameter is marked as deleted.
 * If 'value' == NULL, the parameter is just created with a default
 * value (which may be encoded in 'name').
 */
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

    if (value != NULL)
    {
        free(rp->value);
    }
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
        if (value != NULL)
            rp->value = strdup(value);

        VERB("updated RADIUS parameter %s to %s", name, rp->value ? rp->value : "empty");
    }
    mark_rp_changes(rp);

    return 0;
}

/**
 * Creates a RADIUS user record named 'name' and adds it 
 * to 'radius_users' list
 */
static radius_user *
make_radius_user (const char *name)
{
    radius_user *user = malloc(sizeof(*user));
    user->reject = FALSE;
    user->name = strdup(name);
    user->n_checks = 0;
    user->checks = NULL;
    user->n_replies = 0;
    user->replies = NULL;
    user->next = NULL;
    if (radius_last_user != NULL)
        radius_last_user->next = user;
    else
        radius_users = user;
    radius_last_user = user;
    return user;
}

/**
 * Finds a record for a user named 'name'
 */
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

/**
 * Deletes a user named 'name' from 'radius_users' list
 */
static void
delete_radius_user (const char *name)
{
    radius_user *user, *prev = NULL;

    for (user = radius_users; user != NULL; prev = user, user = user->next)
    {
        if (strcmp(user->name, name) == 0)
        {
            if (prev == NULL)
                radius_users = user->next;
            else 
                prev->next = user->next;
            if (user == radius_last_user)
                radius_last_user = prev;
            free(user->name);
            if (user->checks != NULL)
                free(user->checks);
            if (user->replies != NULL)
                free(user->replies);
            free(user);
            return;
        }
    }
}

/**
 * Given the 'string' has the form "Attribute=Value[,Attribute=Value...]",
 * puts the attribute into 'attr' and the value into value
 *
 * @returns the pointer to the next pair or the terminating zero.
 */
static const char *
parse_attr_value_pair (const char *string, char **attr, char **value)
{
    static char attr_buffer[64];
    static char value_buffer[256];
    char *bufptr;

    for (bufptr = attr_buffer; *string != '='; string++, bufptr++)
    {
        if (*string == '\0')
        {
            *attr = *value = NULL;
            ERROR("syntax error in attribute string");
            return NULL;
        }
        if (bufptr == attr_buffer + sizeof(attr_buffer))
        {
            *attr = *value = NULL;
            ERROR("too long attribute name");
            return NULL;
        }
        *bufptr = *string;
    }
    *bufptr = '\0';
    *attr = attr_buffer;
    for (bufptr = value_buffer, string++; 
         *string != ',' && *string != '\0'; 
         string++, bufptr++)
    {
        if (bufptr == value_buffer + sizeof(value_buffer))
        {
            *attr = *value = NULL;
            ERROR("too long value name");
            return NULL;
        }
        *bufptr = *string;
    }
    *bufptr = '\0';
    *value = value_buffer;
    if (*string == ',')
        string++;
    return *string == '\0' ? NULL : string;
}

/**
 * Parses 'check' as per 'parse_attr_value_pair' and
 * adds the result to a list of checks for a user named 'name'
 */
static int
set_user_checks(const char *name, const char *checks)
{
    radius_user *user = find_radius_user(name);
    radius_user_check *ch;
    char *attr;
    char *value;

    if (user == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);

    do
    {
        checks = parse_attr_value_pair(checks, &attr, &value);
        if (attr == NULL)
            return TE_RC(TE_TA_LINUX, EINVAL);
        for (ch = user->checks; ch != NULL && ch->attribute != NULL; ch++)
        {
            if (strcmp(ch->attribute, attr) == 0)
            {
                if (ch->value != NULL)
                    free(ch->value);
                ch->value = *value == '\0' ? NULL : strdup(value);
                break;
            }
        }
        if ((ch == NULL || ch->attribute == NULL) && *value != '\0')
        {
            user->n_checks++;
            user->checks = realloc(user->checks, (user->n_checks + 1) * sizeof(*user->checks));
            user->checks[user->n_checks - 1].attribute = strdup(attr);
            user->checks[user->n_checks - 1].value     = strdup(value);
            user->checks[user->n_checks].attribute = NULL;
        }
    } while(checks != NULL);
    return 0;
}

/**
 * Parses 'replies' as per 'parse_attr_value_pair' and 
 * adds the results to a list of replies for a user named 'name'.
 *
 * The replies are for Access-Accept if 'in_accept' is TRUE,
 * for Access-Challenge otherwise
 */
static int
set_user_replies(const char *name, const char *replies, te_bool in_accept)
{
    radius_user *user = find_radius_user(name);
    radius_user_reply *rep;
    char *attr;
    char *value;

    if (user == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);

    do
    {
        replies = parse_attr_value_pair(replies, &attr, &value);
        if (attr == NULL)
            return TE_RC(TE_TA_LINUX, EINVAL);
        for (rep = user->replies; rep != NULL && rep->attribute != NULL; rep++)
        {
            if (strcmp(rep->attribute, attr) == 0)
            {
                if ((in_accept ? rep->in_accept : rep->in_challenge) != NULL)
                    free(in_accept ? rep->in_accept : rep->in_challenge);
                if (*value == '\0')
                {
                    if (in_accept) 
                    {
                        rep->in_accept = NULL;
                    }
                    else
                    {
                        rep->in_challenge = NULL;
                    }
                }
                else
                {
                    if (in_accept) 
                    {
                        rep->in_accept = strdup(value);
                    }
                    else
                    {
                        rep->in_challenge = strdup(value);
                    }
                }
                break;
            }
        }
        if ((rep == NULL || rep->attribute == NULL) && *value != '\0')
        {
            user->n_replies++;
            user->replies = realloc(user->replies, (user->n_replies + 1) * sizeof(*user->replies));
            user->replies[user->n_replies - 1].attribute = strdup(attr);
            if (in_accept)
            {
                user->replies[user->n_replies - 1].in_accept     = strdup(value);
                user->replies[user->n_replies - 1].in_challenge  = NULL;
            }
            else
            {
                user->replies[user->n_replies - 1].in_challenge  = strdup(value);
                user->replies[user->n_replies - 1].in_accept  = NULL;
            }
            user->replies[user->n_replies].attribute = NULL;
        }
    } while(replies != NULL);
    return 0;
}

/**
 * A generic routine to convert a user check or reply list 
 * to a textual form 
 *
 * @param dest         The address of the buffer to put data
 * @param data         The list
 * @param itemsize     The size of a list item
 * @param attr_at      The offset inside an item of an attribute field
 * @param value_at     The offset inside an item of a value field
 */
static void
stringify_attribute_values(char *dest, 
                           void *data, size_t itemsize, size_t attr_at, size_t value_at)
{
    void *ptr;
    int tmp;
    te_bool need_comma = FALSE;

    if (data == NULL)
    {
        *dest = '\0';
        return;
    }

#define ATTR(ptr)  *(char **)((char *)ptr + attr_at)
#define VALUE(ptr) *(char **)((char *)ptr + value_at)
    for (ptr = data; ATTR(ptr) != NULL; ptr = (char *)ptr + itemsize)
    {
        if (VALUE(ptr) != NULL)
        {
            if (need_comma)
            {
                *dest++ = ',';
            }
            tmp = strlen(ATTR(ptr));
            memcpy(dest, ATTR(ptr), tmp);
            dest += tmp;
            *dest++ = '=';
            tmp = strlen(VALUE(ptr));
            memcpy(dest, VALUE(ptr), tmp);
            dest += tmp;
            need_comma = TRUE;
        }
    }
    *dest = '\0';
#undef ATTR
#undef VALUE
}

/**
 * Writes user records from 'users' to 'conf'
 */
static void
write_radius_users (FILE *conf, radius_user *users)
{
    radius_user_check *check;
    radius_user_reply *action;

    rewind(conf);
    ftruncate(fileno(conf), 0);
    for (; users != NULL; users = users->next)
    {
        te_bool need_comma = FALSE;
        if (!users->reject)
        {
            fprintf(conf, "\"%s\" Response-Packet-Type == Access-Accept",
                    users->name);
            for (check = users->checks; check != NULL && check->attribute != NULL; check++)
            {
                if (check->value != NULL)
                {
                    fprintf(conf, ", %s == %s", 
                            check->attribute, check->value);
                }
            }
            fputc('\n', conf);
            need_comma = FALSE;
            for (action = users->replies; action != NULL && action->attribute != NULL; action++)
            {
                if (action->in_accept != NULL)
                {
                    if (action->in_challenge == NULL || 
                        strcmp(action->in_accept, action->in_challenge) != 0)
                    {
                        fprintf(conf, "%s    %s := %s", 
                                need_comma ? ",\n" : "",
                                action->attribute,
                                action->in_accept);
                        need_comma = TRUE;
                    }
                }
                else if (action->in_challenge != NULL)
                {
                    fprintf(conf, "%s    %s -= %s", 
                            need_comma ? ",\n" : "",
                            action->attribute,
                            action->in_challenge);
                    need_comma = TRUE;
                }
            }
            fputs("\n\n", conf);
        }
        fprintf(conf, "\"%s\" Auth-Type := %s",
                users->name, users->reject ? "Reject" : "EAP");
        for (check = users->checks; check != NULL && check->attribute != NULL; check++)
        {
            if (check->value != NULL)
            {
                fprintf(conf, ", %s == %s", 
                        check->attribute, check->value);
            }
        }
        fputc('\n', conf);
        need_comma = FALSE;
        for (action = users->replies; action != NULL && action->attribute != NULL; action++)
        {
            if (action->in_challenge != NULL)
            {
                fprintf(conf, "%s    %s += %s", 
                        need_comma ? ",\n" : "",
                        action->attribute,
                        action->in_challenge);
                need_comma = TRUE;
            }
        }
        fputc('\n', conf);
    }
    fflush(conf);
}

static void
log_radius_tree (radius_parameter *parm)
{
    RING("%p %d %s = %s %s %p %p\n", parm, parm->kind, parm->name,
         parm->value ? parm->value : "EMPTY", 
         parm->deleted ? "DELETED" : "", parm->children, parm->next);
    for (parm = parm->children; parm != NULL; parm = parm->next)
        log_radius_tree(parm);
}

static char *radius_daemon = NULL;

/**
 * The functions to query/change the status of RADIUS server
 * 
 * Since the server may be named either 'freeradius' or 'radiusd',
 * both names are first tried and, if either is detected, 
 * 'radius_daemon' is set appropriately.
 */
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
                     char *value, const char *instance, 
                     const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (user == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    stringify_attribute_values(value, user->replies, sizeof(*user->replies), 
                                      offsetof(radius_user_reply, attribute),
                                      offsetof(radius_user_reply, in_accept));
    return 0;
}

static int
ds_radius_accept_set(unsigned int gid, const char *oid,
                     const char *value, const char *instance, 
                     const char *username, ...)
{
    int rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    

    if (radius_users_file == NULL)
        return TE_RC(TE_TA_LINUX, EBADF);
    rc = set_user_replies(username, value, TRUE);
    if (rc == 0)
        write_radius_users(radius_users_file, radius_users);
    return rc;
    return 0;
}

static int
ds_radius_challenge_get(unsigned int gid, const char *oid,
                        char *value, const char *instance, 
                        const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (user == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    stringify_attribute_values(value, user->replies, sizeof(*user->replies), 
                                      offsetof(radius_user_reply, attribute),
                                      offsetof(radius_user_reply, in_challenge));
    return 0;
}

static int
ds_radius_challenge_set(unsigned int gid, const char *oid,
                        const char *value, const char *instance, 
                        const char *username, ...)
{
    int rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (radius_users_file == NULL)
        return TE_RC(TE_TA_LINUX, EBADF);
    rc = set_user_replies(username, value, FALSE);
    if (rc == 0)
        write_radius_users(radius_users_file, radius_users);
    return rc;
}

static int
ds_radius_check_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, 
                    const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (user == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    stringify_attribute_values(value, user->checks, sizeof(*user->checks), 
                                      offsetof(radius_user_check, attribute),
                                      offsetof(radius_user_check, value));
    return 0;
}

static int
ds_radius_check_set(unsigned int gid, const char *oid,
                    const char *value, const char *instance, 
                    const char *username, ...)
{
    int rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (radius_users_file == NULL)
        return TE_RC(TE_TA_LINUX, EBADF);
    rc = set_user_checks(username, value);
    if (rc == 0)
        write_radius_users(radius_users_file, radius_users);
    return rc;
}

static int
ds_radius_user_add(unsigned int gid, const char *oid,
                   const char *value, const char *instance, 
                   const char *username, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (radius_users_file == NULL)
        return TE_RC(TE_TA_LINUX, EBADF);
    else
    {
        radius_user *user = find_radius_user(username);
        
        if (user != NULL)
            return TE_RC(TE_TA_LINUX, EEXIST);
        user = make_radius_user(username);
        if (user == NULL)
            return TE_RC(TE_TA_LINUX, EFAULT);
        user->reject = (*value == '0');
        write_radius_users(radius_users_file, radius_users);
        return 0;
    }
}

static int
ds_radius_user_set(unsigned int gid, const char *oid,
                   const char *value, const char *instance, 
                   const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    
    if (user == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    user->reject = (*value == '0');
    return 0;
}

static int
ds_radius_user_get(unsigned int gid, const char *oid,
                   char *value, const char *instance,
                   const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    
    if (user == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    *value = (user->reject ? '0' : '1');
    value[1] = '\0';
    return 0;
}

static int
ds_radius_user_del(unsigned int gid, const char *oid,
                   const char *instance, const char *username, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    delete_radius_user(username);
    write_radius_users(radius_users_file, radius_users);
    return 0;
}

static int
ds_radius_user_list(unsigned int gid, const char *oid,
                    char **list,
                    const char *instance, ...)
{
    radius_user *user;
    int size;
    char *iter;
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

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
    VERB("adding RADIUS client %s for %s", client_buffer, oid);
    rc = update_rp(radius_conf, RP_SECTION, client_buffer, NULL);
    log_radius_tree(radius_conf);
    if (rc == 0)
    {
        snprintf(client_buffer, sizeof(client_buffer), "client(%s).secret", client_name);
        rc = update_rp(radius_conf, RP_ATTRIBUTE, client_buffer, NULL);
        if (rc == 0)
        {
            snprintf(client_buffer, sizeof(client_buffer), "client(%s).shortname", client_name);
            rc = update_rp(radius_conf, RP_ATTRIBUTE, client_buffer, client_name);
        }
        if (rc == 0)
        {
            write_radius(radius_conf);
            VERB("added client %s", client_buffer);
        }
    }
    return rc;
}

static int
ds_radius_client_del(unsigned int gid, const char *oid,
                     const char *instance, 
                     const char *client_name, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    snprintf(client_buffer, sizeof(client_buffer), "client(%s)", client_name);
    update_rp(radius_conf, RP_SECTION, client_buffer, RP_DELETE_VALUE);
    write_radius(radius_conf);
    return 0;
}

static te_bool client_count(radius_parameter *rp, void *extra)
{
    int *count = extra;
    UNUSED(rp);
    if (rp->value != NULL)
        (*count) += strlen(rp->value) + 1;
    return FALSE;
}

static te_bool client_list(radius_parameter *rp, void *extra)
{
    char **iter = extra;
    if (rp->value != NULL)
    {
        int len = strlen(rp->value);
        memcpy(*iter, rp->value, len);
        (*iter)[len] = ' ';
        (*iter) += len + 1;
    }
    return FALSE;
}

static int
ds_radius_client_list(unsigned int gid, const char *oid,
                    char **list,
                    const char *instance, ...)
{
    int size = 0;
    char *c_iter;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    VERB("obtaining client list");
    find_rp(radius_conf, "client", FALSE, FALSE, client_count, &size);
    VERB("allocation %d bytes for list of clients", size);
    c_iter = *list = malloc(size + 1);
    find_rp(radius_conf, "client", FALSE, FALSE, client_list, &c_iter);
    *c_iter = '\0';
    VERB("client list is '%s'", *list);
    return 0;
}

static int
ds_radius_secret_get(unsigned int gid, const char *oid,
                     char *value, const char *instance,
                     const char *client_name, ...)
{
    const char *val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    VERB("getting client secret");
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
    int rc;


    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    VERB("setting client secret to %s", value);
    snprintf(client_buffer, sizeof(client_buffer), "client(%s).secret", client_name);
    rc = update_rp(radius_conf, RP_ATTRIBUTE, client_buffer, value);    
    if (rc != 0)
    {
        return rc;
    }
    write_radius(radius_conf);
    return 0;
}

RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_user_accept_attrs, "accept-attrs",
                    NULL, NULL,
                    ds_radius_accept_get, ds_radius_accept_set);
RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_user_challenge_attrs, "challenge-attrs",
                    NULL, &node_ds_radiusserver_user_accept_attrs,
                    ds_radius_challenge_get, ds_radius_challenge_set);
RCF_PCH_CFG_NODE_RW(node_ds_radiusserver_user_check, "check",
                    NULL, &node_ds_radiusserver_user_challenge_attrs,
                    ds_radius_check_get, ds_radius_check_set);

static rcf_pch_cfg_object node_ds_radiusserver_user = {
    "user", 0,
    &node_ds_radiusserver_user_check, NULL,
    (rcf_ch_cfg_get)ds_radius_user_get,
    (rcf_ch_cfg_set)ds_radius_user_set,
    (rcf_ch_cfg_add)ds_radius_user_add,
    (rcf_ch_cfg_del)ds_radius_user_del,
    ds_radius_user_list,
    NULL, NULL
};

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
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    retrieve_rp(radius_conf, "listen.ipaddr", &v);
    strcpy(value, *v == '*' ? "0.0.0.0" : v);
    return 0;
}

static int
ds_radiusserver_netaddr_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    if (strcmp(value, "0.0.0.0") == 0)
        value = "*";
    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#auth).ipaddr", value);
    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#acct).ipaddr", value);
    write_radius(radius_conf);
    return 0;
}

static int
ds_radiusserver_acctport_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    const char *v;
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    retrieve_rp(radius_conf, "listen(#acct).port", &v);
    strcpy(value, v);
    return 0;
}

static int
ds_radiusserver_acctport_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#acct).port", value);
    write_radius(radius_conf);
    return 0;
}

static int
ds_radiusserver_authport_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, ...)
{
    const char *v;
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    retrieve_rp(radius_conf, "listen(#auth).port", &v);
    strcpy(value, v);
    return 0;
}

static int
ds_radiusserver_authport_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#auth).port", value);
    write_radius(radius_conf);
    return 0;
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

static supplicant *supplicant_list;

static supplicant *
make_supplicant (const char *ifname)
{
    static char conf_name[64];
    supplicant *ns = malloc(sizeof(*ns));

    ns->interface = strdup(ifname);
    ns->process   = (pid_t)-1;
    ns->next      = supplicant_list;
    snprintf(conf_name, sizeof(conf_name), 
             "/tmp/te_supp_%s.conf", ifname);
    ns->config    = make_rp(RP_FILE, conf_name, NULL, NULL);
    update_rp(ns->config, RP_ATTRIBUTE,
              "network_list", "tester");
    update_rp(ns->config, RP_ATTRIBUTE, 
              "allow_interfaces", ifname);
    write_radius(ns->config);
    supplicant_list = ns;
    return ns;
}

static supplicant *
find_supplicant (const char *ifname)
{
    supplicant *found;
    
    for (found = supplicant_list; found != NULL; found = found->next)
    {
        if (strcmp(found->interface, ifname) == 0)
            return found;
    }
    return NULL;
}

static int
ds_supplicant_get(unsigned int gid, const char *oid,
                  char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    if (supp->process == (pid_t)-1)
    {
        *value    = '0';
        value[1] = '\0';
        return 0;
    }
    if (kill(supp->process, 0))
    {
        int status = 0;
        waitpid(supp->process, &status, 0);
        WARN("Supplicant (pid = %u), on interface %s apparently died "
             "with status = %8.8x",
             (unsigned)supp->process, supp->interface, status);
        supp->process = (pid_t)-1;
        *value    = '0';
        value[1] = '\0';
    }
    else
    {
        *value    = '1';
        value[1] = '\0';
    }
    return 0;
}

static int
ds_supplicant_set(unsigned int gid, const char *oid,
                  const char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    if (*value == '0')
    {
        if (supp->process == (pid_t)-1)
        {
            WARN("Supplicant for interface %s was not running", 
                 supp->interface);
        }
        else
        {
            if (kill(supp->process, SIGTERM))
            {
                WARN("Unable to stop supplicant on interface %s, "
                     "pid = %u: %s", supp->interface,
                     (unsigned)supp->process, strerror(errno));
            }
            else
            {
                int status;
                waitpid(supp->process, &status, 0);
                INFO("Xsupplicant terminated with status %8.8x", status);
            }
            supp->process = (pid_t)-1;
        }
    }
    else
    {
        if (supp->process != (pid_t)-1)
        {
            WARN("Supplicant for interface %s already running, pid = %u", 
                 supp->interface, supp->process);
        }
        else
        {
            supp->process = fork();
            if (supp->process == (pid_t)-1)
            {
                int rc = errno;
                ERROR("Cannot fork a supplicant on interface %s: %s", 
                      supp->interface, strerror(rc));
                return TE_RC(TE_TA_LINUX, rc);
            }
            else if (supp->process == 0)
            {
                execl("xsupplicant", "xsupplicant", "-i", supp->interface, 
                      "-c", supp->config->name, NULL);
                _exit(EXIT_FAILURE);
            }
            else
            {
                RING("Xsupplicant start on %s with pid = %u",
                     supp->interface, (unsigned)supp->process);
            }
        }
    }
    return 0;
}

static int
ds_supplicant_add(unsigned int gid, const char *oid,
                  const char *value, const char *instance, ...)
{   
    if (find_supplicant(instance) != NULL)
        return TE_RC(TE_TA_LINUX, EEXIST);
    make_supplicant(instance);
    if (*value == '1')
    {
        RING("adding in started state");
        return ds_supplicant_set(gid, oid, value, instance);
    }
    else
        return 0;
}

static void
destroy_supplicant (supplicant *supp)
{
    supplicant *prev = NULL, *iter;

    for (iter = supplicant_list; iter != NULL; prev = iter, iter = iter->next)
    {
        if (supp == iter)
            break;
    }
    if (iter != NULL)
    {
        if (iter->process != (pid_t)-1)
            ds_supplicant_set(0, NULL, "0", supp->interface);
        destroy_rp(iter->config);
        free(iter->interface);
        if (prev != NULL)
            prev->next      = iter->next;
        else
            supplicant_list = iter->next;
        free(iter);
    }
}

static int
ds_supplicant_del(unsigned int gid, const char *oid,
                  const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    destroy_supplicant(find_supplicant(instance));
    return 0;
}


static int
ds_supplicant_list(unsigned int gid, const char *oid,
                   char **value, const char *instance, ...)
{
    supplicant *iter;
    int length = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);
    
    for (iter = supplicant_list; iter != NULL; iter = iter->next)
    {
        length += strlen(iter->interface) + 1;
    }
    *value = malloc(length + 1);
    **value = '\0';
    for (iter = supplicant_list; iter != NULL; iter = iter->next)
    {
        strcat(*value, iter->interface);
        strcat(*value, " ");
    }
    return 0;
}

static char supplicant_buffer[256];
static char supplicant_buffer2[256];

static int
ds_supp_method_username_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, 
                            const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);
    int rc;

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);

    snprintf(supplicant_buffer2, sizeof(supplicant_buffer2),
             "tester.eap_%s.username", method);
    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "<BEGIN_UNAME>%s<END_UNAME>", value);
    rc = update_rp(supp->config, RP_ATTRIBUTE, supplicant_buffer2,
                   supplicant_buffer);
    if (rc != 0)
        return rc;
    write_radius(supp->config);
    return 0;
}

static int
ds_supp_method_username_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, 
                            const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);
    const char *username;

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);

    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "tester.eap_%s.username", method);
    retrieve_rp(supp->config, supplicant_buffer, &username);
    
    if (username == NULL)
        *value = '\0';
    else
    {
        const char *subptr = strstr(username, "<BEGIN_UNAME>");
        if (subptr != NULL)
            username = subptr + 13;
        strcpy(value, username);
        value = strstr(value, "<END_UNAME>");
        if (value != NULL)
            *value = '\0';
    }
    return 0;
}

static int
ds_supp_method_passwd_set(unsigned int gid, const char *oid,
                          const char *value, const char *instance, 
                          const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);
    int rc;

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);

    snprintf(supplicant_buffer2, sizeof(supplicant_buffer2),
             "tester.eap_%s.password", method);
    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "<BEGIN_PASS>%s<END_PASS>", value);
    rc = update_rp(supp->config, RP_ATTRIBUTE, supplicant_buffer2,
                   supplicant_buffer);
    if (rc != 0)
        return rc;
    write_radius(supp->config);
    return 0;
}

static int
ds_supp_method_passwd_get(unsigned int gid, const char *oid,
                          char *value, const char *instance, 
                          const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);
    const char *password;

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);

    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "tester.eap_%s.password", method);
    retrieve_rp(supp->config, supplicant_buffer, &password);
    
    if (password == NULL)
        *value = '\0';
    else
    {
        const char *subptr = strstr(password, "<BEGIN_PASS>");
        if (subptr != NULL)
            password = subptr + 12;
        strcpy(value, password);
        value = strstr(value, "<END_PASS>");
        if (value != NULL)
            *value = '\0';
    }
    return 0;
}

static int
ds_supplicant_method_del(unsigned int gid, const char *oid,
                         const char *instance, 
                         const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "eap_%s", method);
    update_rp(supp->config, RP_SECTION, method, RP_DELETE_VALUE);
    write_radius(supp->config);
    return 0;
}


static int
ds_supplicant_method_add(unsigned int gid, const char *oid,
                         const char *value, const char *instance, 
                         const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);
 
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
   
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "eap_%s", method);
    update_rp(supp->config, RP_SECTION, method, NULL);
    write_radius(supp->config);
    return 0;
}

static te_bool 
method_count(radius_parameter *rp, void *extra)
{
    int *count = extra;
    UNUSED(rp);
    if (rp->kind == RP_SECTION && rp->value != NULL)
        (*count) += strlen(rp->value) - 4 + 1;
    return FALSE;
}

static te_bool 
method_list(radius_parameter *rp, void *extra)
{
    char **iter = extra;
    if (rp->kind == RP_SECTION && rp->value != NULL)
    {
        int len = strlen(rp->value + 4);
        memcpy(*iter, rp->value + 4, len);
        (*iter)[len] = ' ';
        (*iter) += len + 1;
    }
    return FALSE;
}

static int
ds_supplicant_method_list(unsigned int gid, const char *oid,
                         char **list,
                         const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);
    int size;
    char *c_iter;
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);

    UNUSED(gid);
    UNUSED(oid);
    
    find_rp(supp->config, "tester.eap_*", FALSE, FALSE, method_count, &size);
    c_iter = *list = malloc(size + 1);
    find_rp(supp->config, "tester.eap_*", FALSE, FALSE, method_list, &c_iter);
    *c_iter = '\0';
    return 0;

}

static int
ds_supplicant_cur_method_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);
    const char *type;

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    retrieve_rp(supp->config, "tester.allow_types", &type);
    
    if (type == NULL)
        *value = '\0';
    else
    {
        if (strncmp(type, "eap_", 4) == 0)
            type += 4;
        strcpy(value, type);
    }
    return 0;
}

static int
ds_supplicant_cur_method_set(unsigned int gid, const char *oid,
                             const char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    snprintf(supplicant_buffer, sizeof(supplicant_buffer), "eap_%s", value);
    update_rp(supp->config, RP_ATTRIBUTE, "tester.allow_types", supplicant_buffer);
    write_radius(supp->config);
    return 0;
}

static int
ds_supplicant_identity_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);
    const char *identity;

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    retrieve_rp(supp->config, "tester.identity", &identity);
    
    if (identity == NULL)
        *value = '\0';
    else
    {
        const char *subptr = strstr(identity, "<BEGIN_ID>");
        if (subptr != NULL)
            identity = subptr + 10;
        strcpy(value, identity);
        value = strstr(value, "<END_ID>");
        if (value != NULL)
            *value = '\0';
    }
    return 0;
}

static int
ds_supplicant_identity_set(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);
    int rc;

    UNUSED(gid);
    UNUSED(oid);
    
    if (supp == NULL)
        return TE_RC(TE_TA_LINUX, ENOENT);
    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "<BEGIN_ID>%s<END_ID>", value);
    rc = update_rp(supp->config, RP_ATTRIBUTE, "tester.identity", 
                   supplicant_buffer);
    if (rc != 0)
        return rc;
    write_radius(supp->config);
    return 0;
}

RCF_PCH_CFG_NODE_RW(node_ds_supp_method_passwd, "passwd",
                    NULL, NULL,
                    ds_supp_method_passwd_get, 
                    ds_supp_method_passwd_set);

RCF_PCH_CFG_NODE_RW(node_ds_supp_method_username, "username",
                    NULL, &node_ds_supp_method_passwd,
                    ds_supp_method_username_get, 
                    ds_supp_method_username_set);

RCF_PCH_CFG_NODE_COLLECTION(node_ds_supplicant_method, "method",
                            &node_ds_supp_method_username, NULL,
                            ds_supplicant_method_add,
                            ds_supplicant_method_del,
                            ds_supplicant_method_list, 
                            NULL);
                            
RCF_PCH_CFG_NODE_RW(node_ds_supplicant_cur_method, "cur_method",
                    NULL, &node_ds_supplicant_method,
                    ds_supplicant_cur_method_get, 
                    ds_supplicant_cur_method_set);

RCF_PCH_CFG_NODE_RW(node_ds_supplicant_identity, "identity",
                    NULL, &node_ds_supplicant_cur_method,
                    ds_supplicant_identity_get, 
                    ds_supplicant_identity_set);

static rcf_pch_cfg_object node_ds_supplicant = {
    "supplicant", 0,
    &node_ds_supplicant_identity, NULL,
    (rcf_ch_cfg_get)ds_supplicant_get,
    (rcf_ch_cfg_set)ds_supplicant_set,
    (rcf_ch_cfg_add)ds_supplicant_add,
    (rcf_ch_cfg_del)ds_supplicant_del,
    ds_supplicant_list,
    NULL, NULL
};


/** The list of parameters that must be deleted on startup */
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

#define RADIUS_USERS_FILE "/tmp/te_radius_users"

/** The list of pairs attribute, value that must be set on startup.
 *  Use RP_DEFAULT_EMPTY_SECTION to create an empty section
 */ 
const char *radius_predefined_params[] = {
    "listen(#auth).type", "auth",
    "listen(#auth).ipaddr", "*",
    "listen(#acct).type", "acct",
    "listen(#acct).ipaddr", "*",
    "modules.pap.encryption_scheme", "crypt",
    "modules.chap.authtype", "chap",
    "modules.files.usersfile", RADIUS_USERS_FILE,
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
    INFO("Wiping out RADIUS parameter %s %s", rp->name, 
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

/**
 * Initializes support for RADIUS server.
 * - The config files are read and parsed
 * - Ignored and defaulted parameters are processed
 * - RP_RADIUS_USERS_FILE is created and opened
 */
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
    DS_REGISTER(supplicant);
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
    {
        int fd = open(RADIUS_USERS_FILE, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IROTH);
        if (fd < 0)
        {
            ERROR("Unable to create " RADIUS_USERS_FILE ", %s", strerror(errno));
        }
        else
        {
            radius_users_file = fdopen(fd, "w");
            if (radius_users_file == NULL)
            {
                close(fd);
                ERROR("cannot reopen " RADIUS_USERS_FILE ", %s", strerror(errno));
            }
        }
    }
}

void
ds_shutdown_radius_server(void)
{
    if (radius_users_file != NULL)
    {
        fclose(radius_users_file);
        remove(RADIUS_USERS_FILE);
    }
}

#endif /* ! WITH_RADIUS_SERVER */
