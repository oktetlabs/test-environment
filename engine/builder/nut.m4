dnl Test Environment
dnl
dnl Nut configuration macros definitions
dnl
dnl Copyright (C) 2003 Test Environment authors (see file AUTHORS in the 
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
dnl Moreover the NUT image name is included to the list of NUT images
dnl to be built by "make all" command.
dnl May be called once for each NUT image name. Source location may
dnl be the same for several NUT image names.
dnl
dnl Parameters:
dnl       NUT image name
dnl       source directory pathname
dnl       building script pathname
dnl       additional parameters to the building script 
dnl       list of Test Agents for TCE gathering
dnl       list of TCE sources
dnl       
define([TE_NUT],
[
[
if test -n "$NUT_$1_SOURCES" ;
then
    echo "Definition of the NUT $1 appears twice" ;
    exit 1 ;
fi
]
NUTS="$NUTS $1"
NUT_$1_SOURCES=$2
SCRIPT=$3
if test "${SCRIPT:0:1}" != "/" ; then 
    NUT_$1_SCRIPT=$2/$3
else    
    NUT_$1_SCRIPT=$3
fi
NUT_$1_PARMS="$4"
NUT_$1_TCE_TA="$5"
NUT_$1_TCE_SRC="$6"
])
