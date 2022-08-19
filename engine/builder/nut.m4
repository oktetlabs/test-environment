dnl SPDX-License-Identifier: Apache-2.0
dnl Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.

dnl Test Environment
dnl
dnl Nut configuration macros definitions

changequote([,])

dnl Specifies parameters for NUT bootable image building.
dnl May be called once for each NUT image name. Source location may
dnl be the same for several NUT image names.
dnl
dnl Parameters:
dnl       NUT image name
dnl       build script pathname (absolute or related to the source
dnl                              directory pathname, if specified)
dnl       source directory pathname (optional)
dnl       additional parameters to the building script (optional)
dnl
define([TE_NUT],
[
[
if test -n "$NUT_$1_SCRIPT" ; then
    echo "Definition of the NUT $1 appears twice" >&2 ;
    exit 1 ;
fi
]
NUTS="$NUTS $1"
SCRIPT="$2"
if test -z "$SCRIPT" ; then
    echo "Build script pathname for the NUT $1 must be specified" >&2 ;
    exit 1 ;
fi
NUT_$1_SOURCES="$3"
if test "${SCRIPT:0:1}" != "/" -a -n "$3" ; then
    NUT_$1_SCRIPT="$3/$SCRIPT"
else
    NUT_$1_SCRIPT="$SCRIPT"
fi
NUT_$1_PARMS="$4"
])

