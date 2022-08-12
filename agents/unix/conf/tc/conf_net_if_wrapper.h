/** @file
 * @brief Wrapper for net/if.h
 *
 * Wrapper for net/if.h need for resolve confict with linux/if.h and net/if.h
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TE_AGENTS_UNIX_CONF_CONF_NET_IF_WRAPPER_H_
#define __TE_AGENTS_UNIX_CONF_CONF_NET_IF_WRAPPER_H_

/**
 * Index of the network interface corresponding to the name @p if_name
 *
 * @param if_name       Interface name
 *
 * @return Index of the network interface
 */
extern int conf_net_if_wrapper_if_nametoindex(const char *if_name);

#endif /* __TE_AGENTS_UNIX_CONF_CONF_NET_IF_WRAPPER_H_ */
