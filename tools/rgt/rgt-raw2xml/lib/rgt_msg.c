/** @file
 * @brief Test Environment: RGT message.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
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


