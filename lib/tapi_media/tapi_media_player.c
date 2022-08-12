/** @file
 * @brief Generic Test API to media player routines
 *
 * Generic high level test API to control a media player.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "TAPI Media Player"

#include "tapi_media_player.h"
#include "tapi_media_player_mplayer.h"

#include "tapi_test.h"

#include "tapi_rpcsock_macros.h"
#include "tapi_rpc_unistd.h"

#include "te_alloc.h"


/* Name of RPC Server for media player process. */
#define PCO_MEDIA_PLAYER_NAME "pco_media_player"


/* See description in tapi_media_player.h. */
tapi_media_player *
tapi_media_player_create(const char               *ta,
                         tapi_media_player_client  client,
                         const char               *player)
{
    tapi_media_player *mp = NULL;
    rcf_rpc_server    *rpcs = NULL;
    te_errno           rc;

    if (client != TAPI_MEDIA_PLAYER_MPLAYER)
        return NULL;

    rc = rcf_rpc_server_create(ta, PCO_MEDIA_PLAYER_NAME, &rpcs);
    if (rc != 0)
        return NULL;

    mp = TE_ALLOC(sizeof(*mp));
    if (mp == NULL)
    {
        rcf_rpc_server_destroy(rpcs);
        return NULL;
    }

    mp->client = client;
    if (player != NULL)
    {
        mp->player = strdup(player);
        if (mp->player == NULL)
        {
            ERROR("%s:%d: Failed to allocate memory",
                  __FUNCTION__, __LINE__);
            rcf_rpc_server_destroy(rpcs);
            free(mp);
            return NULL;
        }
    }
    mp->rpcs = rpcs;
    mp->pid = -1;
    mp->stdin = -1;
    mp->stdout = -1;
    mp->stderr = -1;
    tapi_media_player_mplayer_init(mp);

    return mp;
}

/* See description in tapi_media_player.h. */
void
tapi_media_player_destroy(tapi_media_player *player)
{
    if (player == NULL)
        return;

    tapi_media_player_stop(player);

    if (player->stdout >= 0)
        RPC_CLOSE(player->rpcs, player->stdout);
    if (player->stderr >= 0)
        RPC_CLOSE(player->rpcs, player->stderr);

    rcf_rpc_server_destroy(player->rpcs);
    free(player->player);
    FREE_AND_CLEAN(player);
}

/* See description in tapi_media_player.h. */
te_errno
tapi_media_player_play(tapi_media_player *player,
                       const char        *source,
                       const char        *options)
{
    VERB("Start playback the \"%s\" with options: %s", source, options);

    if (player == NULL ||
        player->methods == NULL ||
        player->methods->play == NULL)
        return TE_EOPNOTSUPP;

    return player->methods->play(player, source, options);
}

/* See description in tapi_media_player.h. */
te_errno
tapi_media_player_stop(tapi_media_player *player)
{
    VERB("Stop playback");

    if (player == NULL ||
        player->methods == NULL ||
        player->methods->stop == NULL)
        return TE_EOPNOTSUPP;

    return player->methods->stop(player);
}

/* See description in tapi_media_player.h. */
te_errno
tapi_media_player_get_errors(tapi_media_player *player)
{
    if (player == NULL ||
        player->methods == NULL ||
        player->methods->get_errors == NULL)
        return TE_EOPNOTSUPP;

    return player->methods->get_errors(player);
}

/* See description in tapi_media_player.h. */
te_bool
tapi_media_player_check_errors(tapi_media_player *player)
{
    size_t   i;
    uint64_t num_errors;

    assert(player != NULL);

    num_errors = player->errors[0];
    for (i = 1; i < TE_ARRAY_LEN(player->errors); i++)
        num_errors += player->errors[i];

    return num_errors > 0;
}

/* See description in tapi_media_player.h. */
void
tapi_media_player_log_errors(const tapi_media_player *player)
{
    static const char *error_name[] =
    {
        [TAPI_MP_ERROR_NOT_FOUND] = "Media stream not found",
        [TAPI_MP_ERROR_EXHAUSTED_CACHE] = "Cache is exhausted",
        [TAPI_MP_ERROR_NO_RESPONSE] = "No response",
        [TAPI_MP_ERROR_BROKEN] = "Connection is broken",
    };
    te_string          dump = TE_STRING_INIT;
    size_t             i;

    te_string_append(&dump, "Found errors:\n");
    for (i = 0; i < TE_ARRAY_LEN(error_name); i++)
        te_string_append(&dump, "%s: %u errors\n",
                         error_name[i], player->errors[i]);
    RING(dump.ptr);
    te_string_free(&dump);
}
