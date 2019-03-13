/** @file
 * @brief Wrapper for net/if.h
 *
 * Wrapper for net/if.h need for resolve confict with linux/if.h and net/if.h
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "conf_net_if_wrapper.h"

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

int
conf_net_if_wrapper_if_nametoindex(const char *if_name)
{
    return if_nametoindex(if_name);
}
