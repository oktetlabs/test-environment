#ifndef __COMMON_H
#define __COMMON_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif

#if HAVE_STRING_H
#  include <string.h>
#else
#  include <strings.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <time.h>


#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <glib.h>

#define i_isspace(x) isspace((int) (unsigned char) (x))

#endif
