/** @file
 * @brief Test Environment substitution of stdint.h
 *
 * Definition of standard integer types.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_STDINT_H__
#define __TE_STDINT_H__

/*
 * It should be included only after including of config.h where
 * constant HAVE_STDINT_H is defined.
 */

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else

#ifndef int8_t
#define int8_t          char
#endif

#ifndef uint8_t
#define uint8_t         unsigned char
#endif

#ifndef int16_t
#define int16_t         short int
#endif

#ifndef uint16_t
#define uint16_t        unsigned short int
#endif

#ifndef int32_t
#define int32_t         int
#endif

#ifndef uint32_t
#define uint32_t        unsigned int
#endif

#ifndef int64_t
#define int64_t         long long int
#endif

#ifndef uint64_t
#define uint64_t        unsigned long long int
#endif

#endif

#endif /* !__TE_STDINT_H__ */
