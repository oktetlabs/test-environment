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

#include "te_config.h"

#include "logger_api.h"
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


/* see description in tapi_acse.h */
te_errno
tapi_acse_manage_acs(const char *ta, const char *acs_name,
                     acse_op_t opcode, ...)
{
    va_list  ap;
    te_errno rc = 0;

    if (ACSE_OP_ADD == opcode)
    {
        rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, 0),
                                  "/agent:%s/acse:/acs:%s", ta, acs_name);
    }

    va_start(ap, opcode);

    while (1)
    {
        char *name = va_arg(ap, char *);
        if (VA_END_LIST == name)
            break;
        /* integer parameters */
        if (strcmp(name, "port") == 0)
        {
            int val;
        }
        else /* string parameters */
        {
        }
    }

    va_end(ap);

    return 0;
}
