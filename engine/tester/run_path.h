/** @file
 * @brief Tester Subsystem
 *
 * Run path definitions.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_ENGINE_TESTER_RUN_PATH_H__
#define __TE_ENGINE_TESTER_RUN_PATH_H__

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif 


/** Run path specified in command line */
typedef struct tester_run_path {
    TAILQ_ENTRY(tester_run_path)    links;  /**< List links */

    char                   *path;   /**< Full path */
    char                   *pos;    /**< Current position in the path */
} tester_run_path;

/** Head of the list with run paths */
typedef TAILQ_HEAD(tester_run_paths, tester_run_path) tester_run_paths;


/**
 * Create a new run path item and insert it in the list.
 *
 * @param paths     A list of run path items
 * @param path      Path content
 *
 * @return Status code.
 * @retval 0        Success.
 * @retval ENOMEM   Memory allocation failure.
 */
extern int tester_run_path_new(tester_run_paths *paths, const char *path);

/**
 * Clone set of run path items.
 *
 * @param paths     A set run path items to be cloned
 * @param new_paths A set run path items for clones
 *
 * @return Status code.
 */
extern int tester_run_paths_clone(const tester_run_paths *paths,
                                  tester_run_paths *new_paths);

/**
 * Free run path item.
 *
 * @param path      A run path item
 */
extern void tester_run_path_free(tester_run_path *path);

/**
 * Free list of run paths.
 *
 * @param paths     A list of run paths
 */
extern void tester_run_paths_free(tester_run_paths *paths);


/* Forward */
struct tester_ctx;

/**
 * Move forward on run path.
 * The function updates Tester context on success.
 *
 * @param ctx       Tester context
 * @param name      Name of the node
 *
 * @return Status code.
 * @retval 0        The node is on run path
 * @retval ENOENT   The node is not on run path
 */
extern int tester_run_path_forward(struct tester_ctx *ctx, const char *name);

#endif /* !__TE_ENGINE_TESTER_RUN_PATH_H__ */
