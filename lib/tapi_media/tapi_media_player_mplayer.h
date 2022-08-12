/** @file
 * @brief Test API to 'mplayer' routines
 *
 * @defgroup tapi_media_player_mplayer Test API to control the mplayer media player
 * @ingroup tapi_media
 * @{
 *
 * Test API to control 'mplayer'.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TAPI_MEDIA_PLAYER_MPLAYER_H__
#define __TAPI_MEDIA_PLAYER_MPLAYER_H__

#include "tapi_media_player.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize media player access point hooks.
 *
 * @param player    The player access point
 */
extern void tapi_media_player_mplayer_init(tapi_media_player *player);

/**@} <!-- END tapi_media_player_mplayer --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TAPI_MEDIA_PLAYER_MPLAYER_H__ */
