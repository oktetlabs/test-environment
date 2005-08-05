dnl Test Environment
dnl
dnl Nut configuration macros definitions
dnl
dnl Copyright (C) 2005 Test Environment authors (see file AUTHORS in the 
dnl root directory of the distribution).
dnl
dnl Test Environment is free software; you can redistribute it and/or 
dnl modify it under the terms of the GNU General Public License as 
dnl published by the Free Software Foundation; either version 2 of 
dnl the License, or (at your option) any later version.
dnl 
dnl Test Environment is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
dnl MA  02111-1307  USA
dnl
dnl Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
dnl
dnl $Id$
 
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

dnl Request TCE for the NUT sources.
dnl
dnl Parameters:
dnl       NUT image name
dnl       list of Test Agents for TCE gathering
dnl       
define([TE_NUT_TCE],
[
[
if test -z "$NUT_$1_SCRIPT" ; then
    echo "Definition of the NUT $1 has not appeared yet" >&2 ;
    exit 1 ;
fi
]
NUT_$1_TCE=yes
NUT_$1_TCE_TA="$NUT_$1_TCE_TA $2"
])

dnl Specifiy sources for TCE gathering for the NUT.
dnl
dnl Parameters:
dnl       NUT image name
dnl       path to sources for TCE (absolute or relative to NUT sources)
dnl       
define([TE_NUT_TCE_SRC],
[
[
if test -z "$NUT_$1_SCRIPT" ; then
    echo "Definition of the NUT $1 has not appeared yet" >&2 ;
    exit 1 ;
fi
]
NUT_$1_TCE_SRC="$NUT_$1_TCE_SRC $2"
])

dnl Specifiy sources to be excluded from TCE gathering for the NUT.
dnl
dnl Parameters:
dnl       NUT image name
dnl       path to sources for TCE (absolute or relative to NUT sources)
dnl       
define([TE_NUT_TCE_SRC_EXCLUDE],
[
[
if test -z "$NUT_$1_SCRIPT" ; then
    echo "Definition of the NUT $1 has not appeared yet" >&2 ;
    exit 1 ;
fi
]
NUT_$1_TCE_SRC_EXCLUDE="$NUT_$1_TCE_SRC_EXCLUDE $2"
])
