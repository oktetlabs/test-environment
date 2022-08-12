/** @file
 * @brief Generic Test API to media player routines
 *
 * @defgroup tapi_media_player Test API to control a media player
 * @ingroup tapi_media
 * @{
 *
 * Generic high level test API to control a media player.
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 */

#ifndef __TAPI_MEDIA_PLAYER_H__
#define __TAPI_MEDIA_PLAYER_H__

#include "te_errno.h"
#include "rcf_rpc.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Supported media players list.
 */
typedef enum tapi_media_player_client {
    TAPI_MEDIA_PLAYER_MPLAYER,      /**< mplayer media player */
} tapi_media_player_client;

/**
 * List of possible playback errors.
 */
typedef enum tapi_media_player_error {
    TAPI_MP_ERROR_NOT_FOUND,         /**< File not found on server. */
    TAPI_MP_ERROR_EXHAUSTED_CACHE,   /**< Playback cache is exhausted. */
    TAPI_MP_ERROR_NO_RESPONSE,       /**< No response from remote server. */
    TAPI_MP_ERROR_BROKEN,            /**< Connection is broken. */
    TAPI_MP_ERROR_MAX,               /**< Not error, but elements number. */
} tapi_media_player_error;

/*
 * Forward declaration of media player access point.
 */
struct tapi_media_player;
typedef struct tapi_media_player tapi_media_player;


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
typedef te_errno (* tapi_media_player_method_play)(
                                            tapi_media_player *player,
                                            const char        *source,
                                            const char        *options);

/**
 * Stop playback.
 *
 * @param player        Player handle.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_media_player_method_stop)(
                                            tapi_media_player *player);

/**
 * Parse player @b stderr stream and count playback errors, the counters are
 * located in @p player.errors.
 *
 * @param player    Player handle.
 *
 * @return Status code.
 */
typedef te_errno (* tapi_media_player_method_get_errors)(
                                            tapi_media_player *player);

/**
 * Methods to operate the server.
 */
typedef struct tapi_media_player_methods {
    tapi_media_player_method_play       play;
    tapi_media_player_method_stop       stop;
    tapi_media_player_method_get_errors get_errors;
} tapi_media_player_methods;

/**
 * Media player access point.
 */
typedef struct tapi_media_player {
    tapi_media_player_client         client;    /**< Player client class. */
    rcf_rpc_server                  *rpcs;      /**< RPC server handle. */
    char                            *player;    /**< Player name to pass to
                                                     command line. */
    const tapi_media_player_methods *methods;   /**< Methods to operate the
                                                     player.*/
    tarpc_pid_t pid;    /**< Player process PID. */
    int         stdin;  /**< File descriptor to write to stdin stream. */
    int         stdout; /**< File descriptor to read from stdout stream. */
    int         stderr; /**< File descriptor to read from stderr stream. */
    uint32_t    errors[TAPI_MP_ERROR_MAX];  /**< Errors counters. */
} tapi_media_player;


/**
 * Create media player access point. Start aux RPC server and initialize
 * hooks. Note, @b tapi_media_player_destroy must be called to release
 * resources.
 *
 * @param ta        Test agent name.
 * @param client    Program to play media.
 * @param player    Pathname to the player, or @c NULL to use default for
 *                  specified client.
 *
 * @return Acccess point.
 *
 * @sa tapi_media_player_destroy
 */
extern tapi_media_player *tapi_media_player_create(
                                        const char               *ta,
                                        tapi_media_player_client  client,
                                        const char               *player);

/**
 * Destroy media player access point: stop playback, stop RPC server, and
 * release resources.
 *
 * @param player The player access point (can be @c NULL to call in cleanup)
 *
 * @sa tapi_media_player_create
 */
extern void tapi_media_player_destroy(tapi_media_player *player);

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
extern te_errno tapi_media_player_play(tapi_media_player *player,
                                       const char        *source,
                                       const char        *options);

/**
 * Stop playback.
 *
 * @param player        Player handle.
 *
 * @return Status code.
 */
extern te_errno tapi_media_player_stop(tapi_media_player *player);

/**
 * Parse player @b stderr stream and count playback errors, the counters are
 * located in @p player.errors.
 *
 * @param player    Player handle.
 *
 * @return Status code.
 */
extern te_errno tapi_media_player_get_errors(tapi_media_player *player);

/**
 * Check if there were errors during media playback. (Actually just check if
 * counters are zero).
 *
 * @param player    Player access point.
 *
 * @return @c TRUE if there are errors in counters.
 */
extern te_bool tapi_media_player_check_errors(tapi_media_player *player);

/**
 * Print a number of errors sorted by type which were occured during media
 * playback using RING function.
 *
 * @param player    Player access point.
 */
extern void tapi_media_player_log_errors(const tapi_media_player *player);

/**@} <!-- END tapi_media_player --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_MEDIA_PLAYER_H__ */
