/** @file
 * @brief Unix Test Agent
 *
 * Authentication daemons configuring (FreeRADIUS, XSupplicant)
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
 * @author Boris Misenov <Boris.Misenov@oktetlabs.ru>
 *
 * $Id$
 */

#include <stddef.h>
#include "conf_daemons.h"

/* Part 1: Common parsing and creating configuration files */

/**
 * Both FreeRADIUS server and XSupplicant use similar scheme of configuration
 * files that consist of lines of the following types:
 *
 *      section name1 {
 *          attribute1 = value1         # comment
 *          flag1
 *          section name2 {
 *          }
 *      }
 *      attribute2 = value2
 *      flag2
 *
 * Functions below perform parsing and generation of such a config and
 * its representation as a tree of nodes.
 */

/** Type of node of configuration file */
enum radius_parameters { 
    RP_FLAG,          /**< a parameter which has no value */
    RP_ATTRIBUTE,     /**< a parameter with the value */
    RP_SECTION,       /**< a (sub)section */ 
    RP_FILE           /**< an included config file */
};

/** Node of the configuration file */
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

#define RP_DEFAULT_EMPTY_SECTION ((const char *)-1)

static char *expand_rp (const char *value, radius_parameter *top);

/** 
 * Creates a new RADIUS parameter inside 'parent' 
 */
static radius_parameter *
make_rp(enum radius_parameters kind,
        const char *name, 
        const char *value, radius_parameter *parent)
{
    radius_parameter *parm = (radius_parameter *)malloc(sizeof(*parm));

    if (parm == NULL)
    {
        ERROR("%s(): not enough memory", __FUNCTION__);
        return NULL;
    }

    parm->deleted = FALSE;
    parm->modified = FALSE;
    parm->kind = kind;
    parm->name = (name != NULL) ? strdup(name) : NULL;
    parm->value = expand_rp(value, parent);
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
 *
 * Note: this function does not exclude the parameter
 * from its parent's children list, so it's normally
 * should be called on a topmost parameter only.
 */
static void
destroy_rp(radius_parameter *parm)
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

#ifdef WITH_RADIUS_SERVER
/**
 * Note: Supplicant configuration files are created from scratch,
 * without reading - so we mark reading as radiusserver-specific
 */

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
        ERROR("cannot open %s: %s", filename, strerror(errno));
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
        else if (buffer[skip_spaces] == '#')
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
#endif

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

        for (i = indent; i > 0; i--)
            putc(' ', outfile);
        switch (parm->kind)
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
                        (parm->value == NULL || *parm->value == '#') ?
                        "" : parm->value);
                for (child = parm->children; child != NULL; child = child->next)
                    write_radius_parameter(outfile, child, indent + 4);
                for (i = indent; i > 0; i--)
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
        return TE_EINVAL;
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
#ifdef WITH_RADIUS_SERVER
        /* Supplicant files were not backed up at all */
        ds_config_touch(top->backup_index);
#endif
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
    size_t      name_length;
    size_t      value_length  = 0;
    te_bool     wildcard      = FALSE;

    VERB("looking for RADIUS parameter %s", name);

    if (base == NULL)
        return NULL;

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
                        ERROR("missing closing parenthesis in %s", name);
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
 * Expands a string with references to RADIUS parameters
 * (in the form of ${param_name}) by replacing such references
 * with corresponding parameter value.
 *
 * @param value   String to expand
 * @param top     Top node of the RADIUS parameters tree
 *
 * @returns A malloced string with references expanded
 *          or NULL in the case of error.
 */
static char *
expand_rp(const char *value, radius_parameter *top)
{
    size_t  value_len;
    char   *new_val;
    char   *new_ptr;
    char   *next;

    if (value == NULL)
        return NULL;

    if ((new_val = strdup(value)) == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    value_len = strlen(new_val) + 1;

    for (new_ptr = new_val; *new_ptr != '\0'; new_ptr++)
    {
        if (new_ptr[0] == '$' && new_ptr[1] == '{' && 
            (next = strchr(new_ptr, '}')) != NULL)
        {
            const char *rp_val;
            size_t      rpv_len;
            size_t      diff_len;

            *next = '\0';
            if (!retrieve_rp(top, new_ptr + 2, &rp_val))
            {
                ERROR("Undefined RADIUS parameter '%s' in '%s'",
                      new_ptr + 2, value);
                *next = '}';
                continue;
            }
            rpv_len = strlen(rp_val);
            diff_len = rpv_len - (next - new_ptr + 1);
            if (diff_len > 0)
            {
                size_t  next_ofs = next - new_val;
                char   *p;

                value_len += diff_len;
                diff_len = new_ptr - new_val;
                if ((p = (char *)realloc(new_val, value_len)) == NULL)
                {
                    ERROR("%s(): failed to allocate memory", __FUNCTION__);
                    free(new_val);
                    return NULL;
                }
                new_val = p;
                new_ptr = new_val + diff_len;
                next = new_val + next_ofs;
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
wipe_rp_section(radius_parameter *rp)
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
update_rp(radius_parameter *top, enum radius_parameters kind, 
          const char *name, const char *value)
{
    radius_parameter *rp = find_rp(top, name, TRUE, TRUE, NULL, NULL);

    if (rp == NULL)
    {
        ERROR("RADIUS parameter %s not found", name);
        return TE_ENOENT;
    }

    if (value != NULL)
    {
        free(rp->value);
        rp->value = NULL;
    }
    if (value == RP_DELETE_VALUE)
    {
        rp->deleted = TRUE;
        if (rp->kind == RP_SECTION)
            wipe_rp_section(rp);

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

#ifdef WITH_RADIUS_SERVER
/* Part 2: FreeRADIUS-specific functions */

/**
 * FreeRADIUS user authentication rules are configured via the separate
 * file, e.g. /etc/radiusd/users, with the following syntax:
 *
 * Usename  Attr1 == Value1, Attr2 == Value2, Auth-Type := Reject
 *      Attr3 := Value3
 *      Attr4 := Value4
 *
 * Here Attr1 and Attr2 are the attributes (and their values) required
 * to be present in incoming RADIUS requests, Attr3 and Attr4 will be
 * placed into the server reply. Auth-Type may be specified here
 * (as Reject or Accept) or omitted (for EAP requests).
 *
 * There are many operators that can be used in check and reply list,
 * not '==' and ':=' only, but we use only a subset of syntax.
 *
 * For each user we create two rules - separately for outgoing 
 * Access-Challenge and Access-Accept packets. (This feature requires
 * patched version of FreeRADIUS server, because it needs to check
 * Response-Packet-Type pseudoattribute, that is not provided to user
 * rules checking function in original code.)
 */

/** Root entry of the tree created from RADIUS configuration file */
static struct radius_parameter *radius_conf;

/** Temporary FreeRADIUS users file created for TE */
static FILE *radius_users_file = NULL;

/** Name of temporary FreeRADIUS users file created for TE */
#define RADIUS_USERS_FILE "/tmp/te_radius_users"

/** An attribute==value pair for RADIUS users file */
typedef struct radius_attr
{
    char               *name;   /**< Attribute name */
    char               *value;  /**< Attribute value in textual form */
} radius_attr;

/** A record for a RADIUS user */
typedef struct radius_user
{
    te_bool             reject;
    char               *name;
    radius_attr        *checks;
    unsigned int        n_checks;
    radius_attr        *accept_replies;
    unsigned int        n_accept_replies;
    radius_attr        *challenge_replies;
    unsigned int        n_challenge_replies;
    struct radius_user *next;
} radius_user;

/** Head of the list of FreeRADIUS users */
static struct radius_user *radius_users;

/** Tail of the list of FreeRADIUS users */
static struct radius_user *radius_last_user;

/**
 * Creates a RADIUS user record named 'name' and adds it 
 * to the end of 'radius_users' list
 */
static radius_user *
make_radius_user(const char *name)
{
    radius_user *user;

    if (name == NULL)
    {
        ERROR("%s(): NULL argument", __FUNCTION__);
        return NULL;
    }
    if ((user = (radius_user *)calloc(1, sizeof(*user))) == NULL)
    {
        ERROR("%s(): not enough memory", __FUNCTION__);
        return NULL;
    }
    user->reject = FALSE;
    if ((user->name = strdup(name)) == NULL)
    {
        ERROR("%s(): not enough memory", __FUNCTION__);
        free(user);
        return NULL;
    }
    user->n_checks = 0;
    user->checks = NULL;
    user->n_accept_replies = 0;
    user->accept_replies = NULL;
    user->n_challenge_replies = 0;
    user->challenge_replies = NULL;
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
find_radius_user(const char *name)
{
    radius_user *user;

    for (user = radius_users; user != NULL; user = user->next)
    {
        if (strcmp(user->name, name) == 0)
            break;
    }
    return user;
}

/**
 * Free an array of 'attribute=value' pairs.
 */
static void
radius_free_attr_array(radius_attr **attrs, unsigned int *n_attrs)
{
    unsigned int i;

    for (i = 0; i < *n_attrs; i++)
    {
        free((*attrs)[i].name);
        free((*attrs)[i].value);
    }
    free(*attrs);
    *attrs = NULL;
    *n_attrs = 0;
}

/**
 * Deletes a user named 'name' from 'radius_users' list
 */
static void
delete_radius_user(const char *name)
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
            radius_free_attr_array(&user->checks, &user->n_checks);
            radius_free_attr_array(&user->accept_replies,
                                   &user->n_accept_replies);
            radius_free_attr_array(&user->challenge_replies,
                                   &user->n_challenge_replies);
            free(user);
            return;
        }
    }
}

/**
 * Given the 'string' has the form "Attribute=Value[,Attribute=Value...]",
 * separates 'Attribute' and 'Value' and allocates new arrays for them.
 *
 * @param string   Pointer to start of 'attribute=value' string,
 *                 after function execution this pointer moves to the next
 *                 'attribute=value' pair, or NULL, if no attributes
 *                 can be read
 * @param attr     Pointer to allocated attribute name string
 * @param value    Pointer to allocated attribute value string
 *
 * @returns Status code.
 */
static te_errno
radius_parse_attr_value_pair(const char **string, char **attr, char **value)
{
    const char *p, *start;
    size_t      len;

    if (string == NULL)
        return TE_EINVAL;

    start = *string;

    /* End of attribute list */
    if (start == NULL || *start == '\0')
    {
        *string = NULL;
        return 0;
    }

    /* Attribute name */
    if ((p = strchr(start, '=')) == NULL)
    {
        ERROR("%s(): attribute has no value in '%s'", __FUNCTION__, start);
        return TE_EINVAL;
    }
    if ((*attr = (char *)calloc(p - start + 1, 1)) == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return TE_ENOMEM;
    }
    memcpy(*attr, start, p - start);

    /* Attribute value */
    start = p + 1;
    if ((p = strchr(start, ',')) != NULL)
        len = p - start;
    else
        len = strlen(start);

    if (len == 0)
    {
        ERROR("%s(): attribute '%s' has empty value", __FUNCTION__, *attr);
        free(*attr);
        *attr = NULL;
        return TE_EINVAL;
    }

    if ((*value = (char *)calloc(len + 1, 1)) == NULL)
    {
        ERROR("%s(): failed to alocate memory", __FUNCTION__);
        free(*attr);
        *attr = NULL;
        return TE_ENOMEM;
    }
    memcpy(*value, start, len);

    *string = start + len;
    return 0;
}

/**
 * Parses string of RADIUS attribute 'name=value' pairs and
 * creates corresponding array of radius_attr structures
 *
 * @param attr_array     Pointer to array of radius_attr structures
 *                       (if *attr_array is not NULL, the old array will
 *                       be deallocated)
 * @param attr_array_len Number of items in 'attr_array'
 * @param attr_string    String of comma-separated 'Attribute=Value' pairs
 */
static te_errno
radius_set_attr_array(radius_attr **attr_array, unsigned int *attr_array_len,
                      const char *attr_string)
{
    unsigned int  n_attrs = 0;
    radius_attr  *attrs = NULL;

    RING("%s('%s')", __FUNCTION__, attr_string);
    while (TRUE)
    {
        char        *name;
        char        *value;
        radius_attr *p;
        te_errno     rc;

        if ((rc = radius_parse_attr_value_pair(&attr_string, &name,
                                               &value)) != 0)
            return TE_RC(TE_TA_UNIX, rc);

        if (attr_string == NULL)
            break;

        p = (radius_attr *)realloc(attrs,
                                   (n_attrs + 1) * sizeof(radius_attr));
        if (p == NULL)
        {
            ERROR("%s(): failed to allocate memory", __FUNCTION__);
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }
        attrs = p;

        attrs[n_attrs].name = name;
        attrs[n_attrs].value = value;
        n_attrs++;
    }

    radius_free_attr_array(attr_array, attr_array_len);
    *attr_array = attrs;
    *attr_array_len = n_attrs;
    return 0;
}

/**
 * A generic routine to convert an array of RADIUS attribute 
 * name-value pairs to a textual form
 *
 * @param dest           The address of the buffer to put data
 * @param attr_array     Attribute array to convert
 * @param attr_array_len Number of items in 'attr_array'
 */
static void
stringify_attr_array(char *dest, radius_attr *attr_array,
                     unsigned int attr_array_len)
{
    unsigned int i;
    size_t       len;

    for (i = 0; i < attr_array_len; i++)
    {
        radius_attr *attr = &attr_array[i];

        if (i != 0)
            *dest++ = ',';

        len = strlen(attr->name);
        memcpy(dest, attr->name, len);
        dest += len;
        *dest++ = '=';

        len = strlen(attr->value);
        memcpy(dest, attr->value, len);
        dest += len;
    }
    *dest = '\0';
}

#ifdef HAVE_FREERADIUS_UPDATE
/**
 * Compare two arrays of RADIUS 'attribute=value' pairs for equality
 * (arrays are equal if they contains the same items in the same order)
 *
 * @param attrs1    First array
 * @param n_attrs1  Number of items in 'attrs1'
 * @param attrs2    Second array
 * @param n_attrs2  Number of items in 'attrs2'
 *
 * @return TRUE if arrays are equal, FALSE otherwise.
 */
static te_bool
radius_equal_attr_array(const radius_attr *attrs1, unsigned int n_attrs1,
                        const radius_attr *attrs2, unsigned int n_attrs2)
{
    unsigned int i;

    if (n_attrs1 != n_attrs2)
        return FALSE;

    for (i = 0; i < n_attrs1; i++)
    {
        if (strcmp(attrs1[i].name, attrs2[i].name) != 0 ||
            strcmp(attrs1[i].value, attrs2[i].value) != 0)
            return FALSE;
    }
    return TRUE;
}
#endif

/**
 * Write array of 'attribute=value' pairs to the file
 * in the form of comma-separated list
 *
 * @param f          Opened file to write to
 * @param attrs      Array to write
 * @param n_attrs    Number of items in 'attrs'
 * @param operator   'Operator' string to place between attribute name and
 *                   its value (e.g., operator ':=' gives 'Attribute := Value')
 * @param separator  Additional symbols to place after separating commas
 *                   (usually space or "\n\t")
 */
static void
radius_write_attr_array(FILE *f, const radius_attr *attrs,
                        unsigned int n_attrs,
                        const char *operator, const char *separator)
{
    unsigned int i;

    for (i = 0; i < n_attrs; i++)
    {
        if (i != 0)
            fprintf(f, ",%s", separator);

        fprintf(f, "%s %s %s", attrs[i].name, operator, attrs[i].value);
    }
}

/**
 * Writes list of users to the FreeRADIUS users configuration file
 *
 * @param conf   Opened file to write to
 */
static void
write_radius_users(FILE *conf)
{
    const radius_user *user;

    rewind(conf);
    ftruncate(fileno(conf), 0);
    for (user = radius_users; user != NULL; user = user->next)
    {
        if (user->reject)
        {
            fprintf(conf, "\"%s\" Auth-Type := Reject\n\n", user->name);
        }
        else
#ifdef HAVE_FREERADIUS_UPDATE
        if (radius_equal_attr_array(user->accept_replies,
                                    user->n_accept_replies,
                                    user->challenge_replies,
                                    user->n_challenge_replies))
#endif
        {
            /* Common configuration for all replies */
            fprintf(conf, "\"%s\" ", user->name);
            radius_write_attr_array(conf, user->checks,
                                    user->n_checks, "==", " ");
            fputs("\n\t", conf);
            radius_write_attr_array(conf, user->accept_replies,
                                    user->n_accept_replies, ":=", "\n\t");
            fputs("\n\n", conf);
        }
#ifdef HAVE_FREERADIUS_UPDATE
        else
        {
            /* Common part */
            fprintf(conf, "\"%s\" ", user->name);
            radius_write_attr_array(conf, user->checks,
                                    user->n_checks, "==", " ");
            fputs("\n\tFall-Through = Yes\n\n", conf);

            /* Access-Accept configuration */
            fprintf(conf, "\"%s\" ", user->name);
            radius_write_attr_array(conf, user->checks,
                                    user->n_checks, "==", " ");
            fputs(", Response-Packet-Type == Access-Accept\n\t", conf);
            radius_write_attr_array(conf, user->accept_replies,
                                    user->n_accept_replies, ":=", "\n\t");
            fputs("\n\n", conf);

            /* Access-Challenge configuration */
            fprintf(conf, "\"%s\" ", user->name);
            radius_write_attr_array(conf, user->checks,
                                    user->n_checks, "==", " ");
            fputs(", Response-Packet-Type == Access-Challenge\n\t", conf);
            radius_write_attr_array(conf, user->challenge_replies,
                                    user->n_challenge_replies, ":=", "\n\t");
            fputs("\n\n", conf);
        }
#endif
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

/** Name of RADIUS server in the distribution */
static const char *radius_daemon = NULL;

/**
 * Find the service name of FreeRADIUS in particular distribution.
 * It is supposed that /etc/init.d scripts system is used. The following
 * names are queried:
 *    radiusd (Fedora, Gentoo)
 *    freeradius (Debian)
 *
 * @return Status code.
 */
static te_errno
radiusserver_find_name()
{
    const char   *candidate[] = { "radiusd", "freeradius" };
    char          buf[128];
    unsigned int  i;

    for (i = 0; i < sizeof(candidate) / sizeof(candidate[0]); i++)
    {
        snprintf(buf, sizeof(buf), "test -x /etc/init.d/%s", candidate[i]);
        if (ta_system(buf) == 0)
        {
            RING("RADIUS server named '%s' is detected", candidate[i]);
            radius_daemon = candidate[i];
            return 0;
        }
        else
            ERROR("'test -x /etc/init.d/%s' fails", candidate[i]);
    }
    return TE_ENOENT;
}

/**
 * The functions to query/change the status of RADIUS server
 * 
 * Since the server may be named either 'freeradius' or 'radiusd',
 * both names are first tried and, if either is detected, 
 * 'radius_daemon' is set appropriately.
 */
static te_errno
ds_radiusserver_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, ...)
{
    UNUSED(oid);
    UNUSED(instance);

    if (radius_daemon == NULL && radiusserver_find_name() != 0)
    {
        RING("radius_daemon = %x, radiusserver_find_name() = %x",
             radius_daemon, radiusserver_find_name());

        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return daemon_get(gid, radius_daemon, value);
}


static te_errno
ds_radiusserver_set(unsigned int gid, const char *oid,
                    const char *value, const char *instance, ...)
{
    UNUSED(oid);
    UNUSED(instance);

    if (radius_daemon == NULL && radiusserver_find_name() != 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return daemon_set(gid, radius_daemon, value);
}

static int
radiusserver_reload()
{
    char buf[128];

    if (radius_daemon == NULL && radiusserver_find_name() != 0)
        return TE_ENOENT;
    /*
     * TODO
     * Temporarily make 'restart' instead of 'reload' because configuration
     * files are invalid at some points and server can be not running
     * unexpectedly. Should be implemented using 'commit' action. Also server
     * should not be restarted if it is supposed not to be running before.
     */
    snprintf(buf, sizeof(buf), "/etc/init.d/%s restart >/dev/null",
             radius_daemon);
    return ta_system(buf);
}

static te_errno
ds_radius_accept_get(unsigned int gid, const char *oid,
                     char *value, const char *instance, 
                     const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    stringify_attr_array(value, user->accept_replies, user->n_accept_replies);
    return 0;
}

static te_errno
ds_radius_accept_set(unsigned int gid, const char *oid,
                     const char *value, const char *instance, 
                     const char *username, ...)
{
    te_errno     rc;
    radius_user *user;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (radius_users_file == NULL)
        return TE_RC(TE_TA_UNIX, TE_EBADF);

    if ((user = find_radius_user(username)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((rc = radius_set_attr_array(&user->accept_replies,
                                    &user->n_accept_replies, value)) == 0)
    {
        write_radius_users(radius_users_file);
        radiusserver_reload();
    }
    return rc;
}

static te_errno
ds_radius_challenge_get(unsigned int gid, const char *oid,
                        char *value, const char *instance, 
                        const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    stringify_attr_array(value, user->challenge_replies,
                         user->n_challenge_replies);
    return 0;
}

static te_errno
ds_radius_challenge_set(unsigned int gid, const char *oid,
                        const char *value, const char *instance, 
                        const char *username, ...)
{
    te_errno     rc;
    radius_user *user;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (radius_users_file == NULL)
        return TE_RC(TE_TA_UNIX, TE_EBADF);

    if ((user = find_radius_user(username)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((rc = radius_set_attr_array(&user->challenge_replies,
                                    &user->n_challenge_replies, value)) == 0)
    {
        write_radius_users(radius_users_file);
        radiusserver_reload();
    }
    return rc;
}

static te_errno
ds_radius_check_get(unsigned int gid, const char *oid,
                    char *value, const char *instance, 
                    const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    stringify_attr_array(value, user->checks, user->n_checks);
    return 0;
}

static te_errno
ds_radius_check_set(unsigned int gid, const char *oid,
                    const char *value, const char *instance, 
                    const char *username, ...)
{
    te_errno     rc;
    radius_user *user;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (radius_users_file == NULL)
        return TE_RC(TE_TA_UNIX, TE_EBADF);

    if ((user = find_radius_user(username)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((rc = radius_set_attr_array(&user->checks,
                                    &user->n_checks, value)) == 0)
    {
        write_radius_users(radius_users_file);
        radiusserver_reload();
    }
    return rc;
}

static te_errno
ds_radius_user_add(unsigned int gid, const char *oid,
                   const char *value, const char *instance, 
                   const char *username, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (radius_users_file == NULL)
        return TE_RC(TE_TA_UNIX, TE_EBADF);
    else
    {
        radius_user *user = find_radius_user(username);

        if (user != NULL)
            return TE_RC(TE_TA_UNIX, TE_EEXIST);

        user = make_radius_user(username);
        if (user == NULL)
            return TE_RC(TE_TA_UNIX, TE_EFAULT);

        user->reject = (*value == '0');
        write_radius_users(radius_users_file);
        radiusserver_reload();
        return 0;
    }
}

static te_errno
ds_radius_user_set(unsigned int gid, const char *oid,
                   const char *value, const char *instance, 
                   const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    user->reject = (*value == '0');
    return 0;
}

static te_errno
ds_radius_user_get(unsigned int gid, const char *oid,
                   char *value, const char *instance,
                   const char *username, ...)
{
    radius_user *user = find_radius_user(username);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    value[0] = (user->reject ? '0' : '1');
    value[1] = '\0';
    return 0;
}

static te_errno
ds_radius_user_del(unsigned int gid, const char *oid,
                   const char *instance, const char *username, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    delete_radius_user(username);
    write_radius_users(radius_users_file);
    radiusserver_reload();
    return 0;
}

static te_errno
ds_radius_user_list(unsigned int gid, const char *oid,
                    char **list,
                    const char *instance, ...)
{
    radius_user *user;
    size_t       size = 0;
    char        *iter;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    for (user = radius_users; user != NULL; user = user->next)
        size += strlen(user->name) + 1;

    if ((*list = (char *)malloc(size + 1)) == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    for (user = radius_users, iter = *list; user != NULL; user = user->next)
    {
        size = strlen(user->name);
        memcpy(iter, user->name, size);
        iter[size] = ' ';
        iter += size + 1;
    }
    *iter = '\0';
    return 0;
}

static char client_buffer[256];

static te_errno
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
    if (rc == 0)
    {
        snprintf(client_buffer, sizeof(client_buffer),
                 "client(%s).secret", client_name);
        rc = update_rp(radius_conf, RP_ATTRIBUTE, client_buffer, NULL);
        if (rc == 0)
        {
            snprintf(client_buffer, sizeof(client_buffer),
                     "client(%s).shortname", client_name);
            rc = update_rp(radius_conf, RP_ATTRIBUTE, client_buffer,
                           client_name);
        }
        if (rc == 0)
        {
            write_radius(radius_conf);
            radiusserver_reload();
            VERB("added client %s", client_buffer);
        }
    }
    return rc;
}

static te_errno
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
    radiusserver_reload();
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

static te_errno
ds_radius_client_list(unsigned int gid, const char *oid,
                    char **list,
                    const char *instance, ...)
{
    int   size = 0;
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

static te_errno
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
    if (!retrieve_rp(radius_conf, client_buffer, &val))
    {
        ERROR("Client %s not found", client_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    if (val == NULL)
        *value = '\0';
    else
        strcpy(value, val);
    return 0;
}

static te_errno
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
    radiusserver_reload();
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

static te_errno
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

static te_errno
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
    radiusserver_reload();
    return 0;
}

static te_errno
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

static te_errno
ds_radiusserver_acctport_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#acct).port", value);
    write_radius(radius_conf);
    radiusserver_reload();
    return 0;
}

static te_errno
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

static te_errno
ds_radiusserver_authport_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    update_rp(radius_conf, RP_ATTRIBUTE, "listen(#auth).port", value);
    write_radius(radius_conf);
    radiusserver_reload();
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
    "modules.realm(suffix).format", "suffix",
    "modules.realm(suffix).delimiter", "\"@\"",
    "modules.realm(suffix).ignore_default", "no",
    "modules.realm(suffix).ignore_null", "no",
    "modules.detail.detailfile", "${radacctdir}/%{Client-IP-Address}/detail-%Y%m%d",
    "modules.detail.detailperm", "0600",
    "modules.acct_unique.key", "\"User-Name, Acct-Session-Id, NAS-IP-Address, Client-IP-Address, NAS-Port\"",

    "preacct.acct_unique", NULL,
    "accounting.detail", NULL,
        
    "authorize.chap", NULL,
    "authorize.mschap", NULL,
    "authorize.eap", NULL,
    "authorize.files", NULL,
    "authenticate.Auth-Type(PAP).pap", NULL,
    "authenticate.Auth-Type(CHAP).chap", NULL,
    "authenticate.Auth-Type(MS-CHAP).mschap", NULL,
    "authenticate.eap", NULL,
#ifdef HAVE_FREERADIUS_UPDATE
    "post-auth.files", NULL,    /* Patched FreeRADIUS is required */
#endif
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
te_errno 
radiusserver_grab(const char *name)
{
    const char **param;
    te_errno     rc;

    UNUSED(name);

    if ((rc = rcf_pch_add_node("/agent", &node_ds_radiusserver)) != 0)
        return rc;

    if (file_exists("/etc/raddb/radiusd.conf"))
        radius_conf = read_radius_file("/etc/raddb/radiusd.conf", NULL);
    else if (file_exists("/etc/freeradius/radiusd.conf"))
        radius_conf = read_radius_file("/etc/freeradius/radiusd.conf", NULL);
    else
    {
        ERROR("No RADIUS config found");
        rcf_pch_del_node(&node_ds_radiusserver);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    for (param = radius_ignored_params; *param != NULL; param++)
    {
        find_rp(radius_conf, *param, FALSE, FALSE, rp_delete_all, NULL);
    }
    for (param = radius_predefined_params; *param != NULL; param += 2)
    {
        update_rp(radius_conf, param[1] == NULL ? RP_FLAG :
                  (param[1] == RP_DEFAULT_EMPTY_SECTION ? RP_SECTION : 
                   RP_ATTRIBUTE), *param,
                  param[1] == RP_DEFAULT_EMPTY_SECTION ? NULL : param[1]);
    }
    write_radius(radius_conf);
    {
        int fd = creat(RADIUS_USERS_FILE, S_IRUSR | S_IROTH | S_IWUSR);
        
        RING("Open %s, fd=%d", RADIUS_USERS_FILE, fd);
        if (fd < 0)
        {
            ERROR("Unable to create " RADIUS_USERS_FILE ", %s", strerror(errno));
        }
        else
        {
            RING("Reopen fd %d", fd);
            radius_users_file = fdopen(fd, "w");
            if (radius_users_file == NULL)
            {
                close(fd);
                ERROR("cannot reopen " RADIUS_USERS_FILE ", %s", strerror(errno));
            }
        }
    }
    return 0;
}

/**
 * Release '/agent/radiusserver' resource
 *
 * @param name    Name of resource (unused)
 */
te_errno
radiusserver_release(const char *name)
{
    UNUSED(name);

    rcf_pch_del_node(&node_ds_radiusserver);
    if (radius_users_file != NULL)
    {
        fclose(radius_users_file);
        remove(RADIUS_USERS_FILE);
        radius_users_file = NULL;
    }

    return 0;
}
#endif /* WITH_RADIUS_SERVER */

#ifdef ENABLE_8021X
/* Part 3: Supplicant-specific functions */

/** A supplicant <-> interface correspondence */
typedef struct supplicant
{
    char              *ifname;     /**< interface name */
    te_bool            started;    /**< Supplicant was started and
                                        is supposed to be running */
    radius_parameter  *config;     /**< Supplicant config tree */
    struct supplicant *next;       /**< chain link */
} supplicant;

/** List of all available supplicants */
static supplicant *supplicant_list = NULL;

/**
 * Create new supplicant structure for the interface
 *
 * @param ifname   Name of interface to create supplicant at
 *
 * @return Pointer to new supplicant structure, or NULL if creation failed.
 */
static supplicant *
make_supplicant(const char *ifname)
{
    static char  conf_name[64];
    supplicant  *ns;

    if ((ns = calloc(1, sizeof(supplicant))) == NULL)
        return NULL;

    ns->ifname = strdup(ifname);
    if (ns->ifname == NULL)
    {
        free(ns);
        return NULL;
    }
    ns->started = FALSE;

    snprintf(conf_name, sizeof(conf_name), 
             "/tmp/te_supp_%s.conf", ifname);
    ns->config    = make_rp(RP_FILE, conf_name, NULL, NULL);
    update_rp(ns->config, RP_ATTRIBUTE, "network_list", "tester");
    update_rp(ns->config, RP_ATTRIBUTE, "default_netname", "tester");
    update_rp(ns->config, RP_ATTRIBUTE, "logfile", "/tmp/te_supp.log");
    update_rp(ns->config, RP_SECTION, "tester", NULL);
    update_rp(ns->config, RP_ATTRIBUTE, "tester.allow_types", "all");

    /* Set default value for all available methods */
    update_rp(ns->config, RP_SECTION, "tester.eap-md5", NULL);
    update_rp(ns->config, RP_ATTRIBUTE, "tester.eap-md5.username", "\"\"");
    update_rp(ns->config, RP_ATTRIBUTE, "tester.eap-md5.password", "\"\"");

    write_radius(ns->config);

    ns->next = supplicant_list;
    supplicant_list = ns;

    return ns;
}

/**
 * Find the supplicant for specified interface in the list of available
 * supplicants
 *
 * @param ifname  Name of interface to find
 *
 * @return Pointer to supplicant structure or NULL if there is no supplicant
 *         configured at the specified interface.
 */
static supplicant *
find_supplicant(const char *ifname)
{
    supplicant *found;

    for (found = supplicant_list; found != NULL; found = found->next)
    {
        if (strcmp(found->ifname, ifname) == 0)
            return found;
    }
    return NULL;
}

/**
 * XSupplicant service control functions
 *
 * We do not use /etc/init.d/ scripts because XSupplicant is not common
 * package in all of distributions. Instead functions provided below are
 * used.
 *
 * We cannot determine pid of XSupplicant because it forks from the initial
 * process while daemonize and does not create pid files in /var/run.
 * The presence of XSupplicant is detected by Unix socket named
 *    /tmp/xsupplicant.sock.<ifname>
 * that is used by XSupplicant itself for its own IPC.
 *
 * If another instance of XSupplicant is started IPC socket may be lost
 * so additional check via 'ps' is performed.
 */

/** Prefix of XSupplicant socket name */
#define XSUPPLICANT_SOCK_NAME "/tmp/xsupplicant.sock."

/**
 * XSupplicant daemon presence check - any instance fits
 *
 * @param ifname   Name of interface where XSupplicant should listen
 *
 * @return TRUE if XSupplicant is found, FALSE otherwise.
 */
static te_bool
xsupplicant_get(const char *ifname)
{
    char buf[128];

    snprintf(buf, sizeof(buf),
             "ps ax | grep xsupplicant | grep -v grep | grep -q %s",
             ifname);
    if (ta_system(buf) == 0)
        return TRUE;

    return FALSE;
}

/**
 * XSupplicant daemon presence check - only active instance (that
 * owns IPC socket) fits
 *
 * @param ifname   Name of interface where XSupplicant should listen
 *
 * @return TRUE if XSupplicant is found, FALSE otherwise.
 */
static te_bool
xsupplicant_get_valid(const char *ifname)
{
    char buf[128];

    snprintf(buf, sizeof(buf), "fuser -s %s%s >/dev/null 2>&1",
             XSUPPLICANT_SOCK_NAME, ifname);
    if (ta_system(buf) == 0)
        return TRUE;

    return FALSE;
}

/**
 * XSupplicant daemon stop
 *
 * @param ifname    Name of interface listened
 *
 * @return Status code.
 */
static te_errno
xsupplicant_stop(const char *ifname)
{
    char buf[128];

    if (!xsupplicant_get(ifname))
    {
        WARN("%s: XSupplicant on %s is not running", __FUNCTION__, ifname);
        return 0;
    }
    RING("Stopping xsupplicant on %s", ifname);

    /* Kill acting instance */
    snprintf(buf, sizeof(buf), "fuser -k -TERM %s%s && rm -f %s%s",
             XSUPPLICANT_SOCK_NAME, ifname,
             XSUPPLICANT_SOCK_NAME, ifname);
    RING("Running '%s'", buf);
    if (ta_system(buf) != 0)
        WARN("Command '%s' failed", buf);

    /* Kill stale instances not owning IPC socket */
    if (xsupplicant_get(ifname))
    {
        snprintf(buf, sizeof(buf),
                 "kill `ps ax | grep xsupplicant | grep %s | grep -v grep"
                 "| awk ' { print $1 }'`", ifname);
        if (ta_system(buf) != 0)
            WARN("Command '%s' failed", buf);
    }
    return 0;
}

/**
 * XSupplicant daemon start
 *
 * @param ifname      Name of interface to listen
 * @param conf_fname  Name of configuration file
 *
 * @return Status code.
 */
static te_errno
xsupplicant_start(const char *ifname, const char *conf_fname)
{
    char  buf[128];

    if (xsupplicant_get(ifname))
    {
        if (xsupplicant_get_valid(ifname))
        {
            WARN("%s: XSupplicant on %s is already running, doing nothing",
                 __FUNCTION__, ifname);
            return 0;
        }
        else
        {
            WARN("%s: XSupplicant on %s is already running, but seems "
                 "not valid, restarting",
                 __FUNCTION__, ifname);
            xsupplicant_stop(ifname);
        }
    }
    RING("Starting xsupplicant on %s", ifname);
    snprintf(buf, sizeof(buf), "xsupplicant -i %s -c %s -dA >/dev/null 2>&1", 
             ifname, conf_fname);
    if (ta_system(buf) != 0)
    {
        ERROR("Command <%s> failed", buf);
        return TE_ESHCMD;
    }
    if (!xsupplicant_get(ifname))
    {
        ERROR("Failed to start XSupplicant on %s", ifname);
        return TE_EFAIL;
    }
    return 0;
}

/**
 * XSupplicant daemon configuration reload
 */
static void
reload_supplicant(supplicant *supp)
{
    if (supp != NULL && supp->started)
    {
        xsupplicant_stop(supp->ifname);
        xsupplicant_start(supp->ifname, supp->config->name);
    }
}

static te_errno
ds_supplicant_get(unsigned int gid, const char *oid,
                  char *value, const char *instance, ...)
{
    supplicant *supp;

    UNUSED(gid);
    UNUSED(oid);

    if ((supp = find_supplicant(instance)) == NULL)
    {
        if ((supp = make_supplicant(instance)) == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    if (xsupplicant_get(supp->ifname))
        strcpy(value, "1");
    else
        strcpy(value, "0");

    return 0;
}

static te_errno
ds_supplicant_set(unsigned int gid, const char *oid,
                  const char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);
    te_errno    rc;

    UNUSED(gid);
    UNUSED(oid);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (*value == '0')
    {
        if ((rc = xsupplicant_stop(supp->ifname)) != 0)
            return TE_RC(TE_TA_UNIX, rc);
        supp->started = FALSE;
    }
    else
    {
        if ((rc = xsupplicant_start(supp->ifname, supp->config->name)) != 0)
            return TE_RC(TE_TA_UNIX, rc);
        supp->started = TRUE;
    }

    return 0;
}

/* Temporary buffers for string operations */
static char supplicant_buffer[256];
static char supplicant_buffer2[256];

static te_errno
ds_supp_method_username_set(unsigned int gid, const char *oid,
                            const char *value, const char *instance, 
                            const char *supp_inst, const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);
    int rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(supp_inst);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(supplicant_buffer2, sizeof(supplicant_buffer2),
             "tester.%s.username", method);
    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "\"%s\"", value);
    rc = update_rp(supp->config, RP_ATTRIBUTE, supplicant_buffer2,
                   supplicant_buffer);
    if (rc != 0)
        return rc;
    write_radius(supp->config);
    reload_supplicant(supp);

    return 0;
}

/**
 * Copy null-terminated string with removing enclosing quotes if they exist
 * (e.g. from strings "text" or "\"text\"" we get "text" in both cases)
 *
 * @param src   Source string
 * @param dst   Destination string
 */
static void
strcpy_dequote(char *dst, const char *src)
{
    size_t len = strlen(src);

    if (len > 1 && src[0] == '"' && src[len - 1] == '"')
    {
        /* Quoted */
        strcpy(dst, src + 1);
        dst[len - 2] = '\0';
    }
    else
    {
        /* Not quoted */
        strcpy(dst, src);
    }
}

static te_errno
ds_supp_method_username_get(unsigned int gid, const char *oid,
                            char *value, const char *instance, 
                            const char *supp_inst, const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);
    const char *username;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(supp_inst);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "tester.%s.username", method);
    retrieve_rp(supp->config, supplicant_buffer, &username);

    if (username == NULL)
        *value = '\0';
    else
        strcpy_dequote(value, username);

    return 0;
}

static te_errno
ds_supp_method_passwd_set(unsigned int gid, const char *oid,
                          const char *value, const char *instance, 
                          const char *supp_inst, const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);
    int rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(supp_inst);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(supplicant_buffer2, sizeof(supplicant_buffer2),
             "tester.%s.password", method);
    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "\"%s\"", value);
    rc = update_rp(supp->config, RP_ATTRIBUTE, supplicant_buffer2,
                   supplicant_buffer);
    if (rc != 0)
        return rc;
    write_radius(supp->config);
    reload_supplicant(supp);
    return 0;
}

static te_errno
ds_supp_method_passwd_get(unsigned int gid, const char *oid,
                          char *value, const char *instance, 
                          const char *supp_inst, const char *method, ...)
{
    supplicant *supp = find_supplicant(instance);
    const char *password;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(supp_inst);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(supplicant_buffer, sizeof(supplicant_buffer),
             "tester.%s.password", method);
    retrieve_rp(supp->config, supplicant_buffer, &password);

    if (password == NULL)
        *value = '\0';
    else
        strcpy_dequote(value, password);

    return 0;
}

static te_errno
ds_supplicant_method_list(unsigned int gid, const char *oid,
                         char **list,
                         const char *instance, ...)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(instance);

    *list = strdup("eap-md5");
    if (*list == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

static te_errno
ds_supplicant_cur_method_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);
    const char *type;

    UNUSED(gid);
    UNUSED(oid);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    retrieve_rp(supp->config, "tester.allow_types", &type);

    if (type == NULL || strcmp(type, "all") == 0)
        *value = '\0';
    else
        strcpy(value, type);

    return 0;
}

static te_errno
ds_supplicant_cur_method_set(unsigned int gid, const char *oid,
                             const char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);

    UNUSED(gid);
    UNUSED(oid);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (*value == '\0')
        update_rp(supp->config, RP_ATTRIBUTE, "tester.allow_types", "all");
    else
        update_rp(supp->config, RP_ATTRIBUTE, "tester.allow_types", value);
    write_radius(supp->config);
    reload_supplicant(supp);
    return 0;
}

static te_errno
ds_supplicant_identity_get(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);
    const char *identity;

    UNUSED(gid);
    UNUSED(oid);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    retrieve_rp(supp->config, "tester.identity", &identity);

    if (identity == NULL)
        *value = '\0';
    else
        strcpy(value, identity);

    return 0;
}

static te_errno
ds_supplicant_identity_set(unsigned int gid, const char *oid,
                           char *value, const char *instance, ...)
{
    supplicant *supp = find_supplicant(instance);
    int rc;

    UNUSED(gid);
    UNUSED(oid);

    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = update_rp(supp->config, RP_ATTRIBUTE, "tester.identity", value);
    if (rc != 0)
        return rc;

    write_radius(supp->config);
    reload_supplicant(supp);
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
                            NULL,
                            NULL,
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
    NULL, /*(rcf_ch_cfg_add)ds_supplicant_add,*/
    NULL, /*(rcf_ch_cfg_del)ds_supplicant_del,*/
    NULL, /*ds_supplicant_list,*/
    NULL, NULL
};

te_errno
ta_unix_conf_supplicant_init()
{
    return rcf_pch_add_node("/agent/interface", &node_ds_supplicant);
}

/**
 * Get the name of interface from the name of interface resource
 * e.g. "/agent:Agt_A/interface:eth0" -> "eth0"
 *
 * @param name  Name of interface resource
 *
 * @return Name of interface.
 */
const char *
supplicant_get_name(const char *name)
{
    const char *instance = strrchr(name, ':');

    if (instance == NULL || *instance == '\0')
    {
        ERROR("%s(): invalid interface resource name '%s'",
              __FUNCTION__, name);
        return NULL;
    }
    return instance + 1;
}

te_errno
supplicant_grab(const char *name)
{
    const char *instance = supplicant_get_name(name);

    if (instance == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    RING("%s('%s') at '%s'", __FUNCTION__, name, instance);

    if (find_supplicant(instance) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (make_supplicant(instance) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

te_errno
supplicant_release(const char *name)
{
    const char *instance = supplicant_get_name(name);
    supplicant *supp;
    supplicant *prev = NULL;
    supplicant *iter;

    if (instance == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    supp = find_supplicant(instance);
    if (supp == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    for (iter = supplicant_list; iter != NULL; prev = iter, iter = iter->next)
    {
        if (supp == iter)
            break;
    }
    if (iter != NULL)
    {
        if (prev != NULL)
            prev->next = iter->next;
        else
            supplicant_list = iter->next;
    }

    if (supp->started)
        xsupplicant_stop(supp->ifname);

    destroy_rp(supp->config);
    free(supp->ifname);
    free(supp);

    return 0;
}
#endif
