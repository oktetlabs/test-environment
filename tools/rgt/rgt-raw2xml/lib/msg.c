/** @file
 * @brief Test Environment: RGT message.
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
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
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 * 
 * $Id$
 */

#include <string.h>
#include "rgt_msg.h"

te_bool
rgt_msg_valid(const rgt_msg *msg)
{
    return msg != NULL &&
           msg->entity != NULL && msg->user != NULL &&
           msg->fmt != NULL && msg->args != NULL;
}


te_bool
rgt_msg_is_control(const rgt_msg *msg)
{
    static const char   user[]      = "Control";

    return msg->user->len == sizeof(user) - 1 &&
           memcmp(msg->user->buf, user, sizeof(user) - 1) == 0;
}


te_bool
rgt_msg_is_tester_control(const rgt_msg *msg)
{
    static const char   entity[]    = "Tester";

    return msg->entity->len == sizeof(entity) - 1 &&
           memcmp(msg->entity->buf, entity, sizeof(entity) - 1) == 0 &&
           rgt_msg_is_control(msg);
}


