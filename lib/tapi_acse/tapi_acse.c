/** @file
 * @brief Test API for ACSE usage
 *
 * Implementation of Test API to ACSE.
 *
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI ACSE"

#include "te_config.h"

#include "logger_api.h"
#include "rcf_common.h"
#include "tapi_acse.h"
#include "tapi_cfg_base.h"


const int va_end_list_var = 10;
void * const va_end_list_ptr = &va_end_list_ptr;

/* see description in tapi_acse.h */
te_errno
tapi_acse_start(const char *ta)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                "/agent:%s/acse:", ta);
}


/* see description in tapi_acse.h */
te_errno
tapi_acse_stop(const char *ta)
{
    te_errno rc;
    char     buf[256];

    TE_SPRINTF(buf, "/agent:%s/acse:", ta);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0), "%s", buf)) == 0)
        return cfg_synchronize(buf, TRUE);

    return rc;
}

static inline int
acse_is_int_var(const char *name)
{
    return
        ((0 == strcmp(name, "port"))     || 
         (0 == strcmp(name, "ssl"))      ||
         (0 == strcmp(name, "enabled"))  ||
         (0 == strcmp(name, "cr_state")) ||
         (0 == strcmp(name, "cwmp_state"))  );
}



/** generic internal method for ACSE manage operations */
static inline te_errno
tapi_acse_manage_vlist(const char *ta, const char *acs_name,
                     const char *cpe_name,
                     acse_op_t opcode, va_list  ap)
{
    te_errno gen_rc = 0;

    char cpe_name_buf[RCF_MAX_PATH] = "";

    if (cpe_name != NULL)
        snprintf(cpe_name_buf, RCF_MAX_PATH, "/cpe:%s", cpe_name);

    if (ACSE_OP_ADD == opcode)
    {
        gen_rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, 0),
                          "/agent:%s/acse:/acs:%s%s", ta, acs_name,
                          cpe_name_buf);
    }

    while (1)
    {
        char *name = va_arg(ap, char *);
        char buf[RCF_MAX_PATH];
        te_errno rc = 0;

        snprintf(buf, RCF_MAX_PATH, "/agent:%s/acse:/acs:%s%s/%s:",
                 ta, acs_name, cpe_name_buf, name);

        if (VA_END_LIST == name)
            break;

        if (ACSE_OP_OBTAIN == opcode)
        { 
            cfg_val_type type;

            if (acse_is_int_var(name))
            {/* integer parameters */
                int *p_val = va_arg(ap, int *);
                type = CVT_INTEGER;
                rc = cfg_get_instance_fmt(&type, p_val, "%s", buf);
            }
            else /* string parameters */
            {
                char **p_val = va_arg(ap, char **);
                type = CVT_STRING;
                rc = cfg_get_instance_fmt(&type, p_val, "%s", buf);
            }
        }
        else
        {
            if (acse_is_int_var(name))
            {/* integer parameters */
                int val = va_arg(ap, int);
                rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, val), "%s", buf);
            }
            else /* string parameters */
            {
                char *val = va_arg(ap, char *);
                rc = cfg_set_instance_fmt(CFG_VAL(STRING, val), "%s", buf);
            }
        }
        if (0 == gen_rc) /* store in 'gen_rc' first TE errno */
            gen_rc = rc;
    } 

    return gen_rc;
}



te_errno
tapi_acse_manage_cpe(const char *ta, const char *acs_name,
                     const char *cpe_name,
                     acse_op_t opcode, ...)
{
    va_list  ap;
    te_errno rc;
    va_start(ap, opcode);
    rc = tapi_acse_manage_vlist(ta, acs_name, cpe_name, opcode, ap);
    va_end(ap);
}



/* see description in tapi_acse.h */
te_errno
tapi_acse_manage_acs(const char *ta, const char *acs_name,
                     acse_op_t opcode, ...)
{
    va_list  ap;
    te_errno rc;
    va_start(ap, opcode);
    rc = tapi_acse_manage_vlist(ta, acs_name, NULL, opcode, ap);
    va_end(ap);
}

