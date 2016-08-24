/** @file
 * @brief Test API to 'mplayer' routines
 *
 * Test API to control 'mplayer'.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI MPlayer"

#include "tapi_media_player_mplayer.h"

#include "tapi_rpc_unistd.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpcsock_macros.h"


/* MPlayer error messages mapping. */
typedef struct error_mapping {
      const char              *msg;     /* Error message. */
      tapi_media_player_error  error;   /* Error code. */
} error_mapping;

/* Map of error messages corresponding to them codes. */
static error_mapping errors[] =
{
    { "No stream found to handle url", TAPI_MP_ERROR_NOT_FOUND },
    { "Cache empty", TAPI_MP_ERROR_EXHAUSTED_CACHE },
    { "nop_streaming_read error", TAPI_MP_ERROR_NO_RESPONSE },
    { "connection timeout", TAPI_MP_ERROR_NO_RESPONSE },
    { "connect error", TAPI_MP_ERROR_BROKEN },
    { NULL, 0 }
};


/**
 * Get default MPlayer name. Note return value should be freed with
 * @b free(3) when it is no longer needed.
 *
 * @param rpcs          RPC server handle.
 *
 * @return MPlayer name
 *
 * @todo Bug 8823: Get a player name for specified agent.
 */
static char *
get_default_player(rcf_rpc_server *rpcs)
{
    UNUSED(rpcs);

    return strdup("mplayer");
}


/**
 * Reset an error counters.
 *
 * @param player    Player handle.
 */
static void
reset_errors(tapi_media_player *player)
{
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(player->errors); i++)
        player->errors[i] = 0;
}

/**
 * Play a media file.
 *
 * @param player    Player handle.
 * @param source    Audio or video file to play (link or local pathname).
 * @param options   Custom options to pass to player run command line
 *                  or @c NULL to use default ones.
 *
 * @return Status code.
 */
static te_errno
play(tapi_media_player *player, const char *source, const char *options)
{
    te_string         cmd = TE_STRING_INIT;
    te_errno          rc;
    static const char default_opts[] = "-cache 32 "
                                       "-vo null "
                                       "-ao null "
                                       "-benchmark "
                                       "-identify";


    if (source == NULL)
    {
        ERROR("Source is not provided");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (options == NULL)
        options = default_opts;

    /* Close a current opened file descriptors. */
    if (player->stdout >= 0)
        RPC_CLOSE(player->rpcs, player->stdout);
    if (player->stderr >= 0)
        RPC_CLOSE(player->rpcs, player->stderr);

    reset_errors(player);

    rc = te_string_append(&cmd, "%s %s %s", player->player, options,
                          source);
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    player->pid = rpc_te_shell_cmd(player->rpcs, cmd.ptr, -1,
                                   &player->stdin, &player->stdout,
                                   &player->stderr);
    if (player->pid < 0)
        rc = TE_EFAIL;

    te_string_free(&cmd);

    return TE_RC(TE_TAPI, rc);
}

/**
 * Stop playback.
 *
 * @param player        Player handle.
 *
 * @return Status code.
 */
static te_errno
stop(tapi_media_player *player)
{
    int rc;

    if (player->pid < 0)
        return 0;

    if (player->stdin >= 0)
        RPC_CLOSE(player->rpcs, player->stdin);

    rc = rpc_ta_kill_death(player->rpcs, player->pid);
    if (rc != 0)
        rc = TE_ECHILD;
    player->pid = -1;

    return TE_RC(TE_TAPI, rc);
}

/**
 * Parse player @b stderr stream and count playback errors, the counters are
 * located in @p player.errors.
 *
 * @param player    Player handle.
 *
 * @return Status code.
 */
static te_errno
get_errors(tapi_media_player *player)
{
    te_string err_buf = TE_STRING_INIT;
    size_t    i;
    te_errno  rc;

    rc = tapi_rpc_append_fd_to_te_string(player->rpcs, player->stderr,
                                         &err_buf);
    if (rc != 0)
        return TE_RC(TE_TAPI, rc);
    VERB("MPlayer stderr:\n%s", err_buf.ptr);

    for (i = 0; errors[i].msg != NULL; i++)
    {
        const char *ptr = err_buf.ptr;

        while ((ptr = strstr(ptr, errors[i].msg)) != NULL)
        {
            player->errors[errors[i].error]++;
            ptr++;
        }
    }

    /*
     * If cache is enabled then "Cache empty" message appears usually
     * before playback is started. It is not error.
     */
    if (player->errors[TAPI_MP_ERROR_EXHAUSTED_CACHE] > 0)
        player->errors[TAPI_MP_ERROR_EXHAUSTED_CACHE]--;

    te_string_free(&err_buf);

    return 0;
}


/**
 * mplayer specific methods.
 */
static tapi_media_player_methods mplayer_methods = {
    .play = play,
    .stop = stop,
    .get_errors = get_errors
};


/* See description in tapi_media_player_mplayer.h. */
void
tapi_media_player_mplayer_init(tapi_media_player *player)
{
    if (player->player == NULL)
        player->player = get_default_player(player->rpcs);

    assert(player->player != NULL);

    player->methods = &mplayer_methods;
}
