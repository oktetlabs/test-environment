/** @file
 * @brief Agent support library
 *
 * Dynamic symbol lookup
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Alexandra N. Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER      "Agent library"

#include "agentlib.h"

#include <stdlib.h>

#if defined(ENABLE_DLFCN_LOOKUP) && defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#if defined(HAVE_SYS_QUEUE_H)
#include <sys/queue.h>
#else
#include "te_queue.h"
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_alloc.h"
#include "logger_api.h"

typedef struct symbol_table_list {
    LIST_ENTRY(symbol_table_list)  next;
    const rcf_symbol_entry *entries;
} symbol_table_list;

static LIST_HEAD(, symbol_table_list) symbol_tables = 
    LIST_HEAD_INITIALIZER(symbol_tables);

/* See description in agentlib.h */
te_errno
rcf_ch_register_symbol_table(const rcf_symbol_entry *entries)
{
    symbol_table_list *table = TE_ALLOC(sizeof(*table));

    if (table == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    table->entries = entries;
    LIST_INSERT_HEAD(&symbol_tables, table, next);

    return 0;
}

static void *
dl_lookup_sym(const char *name)
{
#if defined(ENABLE_DLFCN_LOOKUP)
    void *addr;

    dlerror();
    addr = dlsym(RTLD_DEFAULT, name);
    
    if (addr == NULL)
    {
        const char *err = dlerror();
        if (err != NULL)
            ERROR("Cannot lookup symbol %s: %s", name, err);
    }
    return addr;
#else
    UNUSED(name);
    return NULL;
#endif
}

static const char *
dl_lookup_addr(const void *addr)
{
#if defined(ENABLE_DLFCN_LOOKUP) && defined(HAVE_DLADDR)
    Dl_info info;
    
    dlerror();
    if (!dladdr(addr, &info))
    {
        ERROR("Cannot resolve address %p: %s", addr, dlerror());
        return NULL;
    }
    return info.dli_sname;
#else
    UNUSED(addr);
    return NULL;
#endif /* ENABLE_DLFCN_LOOKUP && HAVE_DLADDR */
}

/* See description in agentlib.h */
void *
rcf_ch_symbol_addr(const char *name, te_bool is_func)
{
    const symbol_table_list *table_iter;
    
    LIST_FOREACH(table_iter, &symbol_tables, next)
    { 
        const rcf_symbol_entry *iter;

        for (iter = table_iter->entries; iter->name != NULL; iter++)
        {
            if (is_func == iter->is_func && 
                strcmp(name, iter->name) == 0)
            {
                return iter->addr;
            }
        }
    }

    return dl_lookup_sym(name);
}

/* See description in agentlib.h */
const char *
rcf_ch_symbol_name(const void *addr)
{
    const symbol_table_list *table_iter;
    
    LIST_FOREACH(table_iter, &symbol_tables, next)
    { 
        const rcf_symbol_entry *iter;

        for (iter = table_iter->entries; iter->name != NULL; iter++)
        {
            if (iter->addr == addr)
            {
                return iter->name;
            }
        }
    }
    return dl_lookup_addr(addr);
}
