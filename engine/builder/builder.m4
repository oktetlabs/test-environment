dnl Test Environment
dnl
dnl Builder configuration macros definitions
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

TE_HOST_DEFINED=

dnl Specifies a host platform. Should be the first macro in the
dnl configuration file.
dnl
dnl Parameters:
dnl       host platform or empty
dnl
define([TE_HOST],
[
if test "$TE_HOST_DEFINED" ; then
    TE_BS_CONF_ERR="host platform is specified in the incorrect place" ; 
    break ; 
fi    
if test "$1" ; then
    host=$1
fi    
TE_HOST_DEFINED=yes
])


dnl Declares a platform for and specifies platform-specific 
dnl parameters for configure script as well platform-specific 
dnl CFLAGS and LDFLAGS. 
dnl May be called once for each platform (including host platform).
dnl
dnl Parameters:
dnl       canonical host name
dnl       configure parameters (may be empty)
dnl       additional compiler flags
dnl       additional linker flags
dnl       list of all libraries to be built
dnl
define([TE_PLATFORM],
[
TE_HOST_DEFINED=yes
PLATFORM=$1
if test -z "$PLATFORM" ; then
    PLATFORM=$host
fi    
PLATFORM_NAME=`echo $PLATFORM | sed s/-/_/g`
TE_BS_PLATFORMS="$TE_BS_PLATFORMS $PLATFORM_NAME"
eval `echo ${PLATFORM_NAME}_PARMS=\"$2\"`
eval `echo ${PLATFORM_NAME}_CFLAGS=\"$3\"`
eval `echo ${PLATFORM_NAME}_LDFLAGS=\"$4\"`
eval `echo ${PLATFORM_NAME}_LIBS=\"$5\"`
eval `echo ${PLATFORM_NAME}_PLATFORM=$PLATFORM`
])

dnl Specifies additional parameters to be passed to configure script of the
dnl mandatory or external library (common for all platforms). May be called 
dnl once for each library.
dnl
dnl Parameters:
dnl       name of the library
dnl       platform
dnl       source directory
dnl       additional parameters to configure script (may be empty)
dnl       additional compiler flags
dnl       additional linker flags
dnl
define([TE_LIB_PARMS],
[
[
TE_HOST_DEFINED=yes
PLATFORM=$2
SOURCES=$3
if test -z "$PLATFORM" ; then
    PLATFORM=$host
fi
PLATFORM_NAME=`echo $PLATFORM | sed s/-/_/g`
eval `echo TE_BS_LIB_${PLATFORM_NAME}_$1_PLATFORM=$PLATFORM`

if test -z "$SOURCES" ; then 
    SOURCES=${TE_BASE}/lib/$1 ; 
elif test "${SOURCES:0:1}" != "/" ; then 
    SOURCES=${TE_BASE}/lib/$SOURCES ; 
fi
if ! test -d "$SOURCES" ; then
    TMP=${TE_BASE}/lib/`basename $SOURCES`
    if test -d "$TMP" ; then
        SOURCES=$TMP
    fi
fi
]
eval `echo TE_BS_LIB_${PLATFORM_NAME}_$1_SOURCES=$SOURCES`
eval `echo TE_BS_LIB_${PLATFORM_NAME}_$1_PARMS=\"$4\"`
eval `echo TE_BS_LIB_${PLATFORM_NAME}_$1_CFLAGS=\"$5\"`
eval `echo TE_BS_LIB_${PLATFORM_NAME}_$1_LDFLAGS=\"$6\"`
])


dnl Declares the list of engine applications to be built by "make all" command.
dnl May be called only once.
dnl
dnl Parameters:
dnl       list of names of directories in ${TE_BASE}/engine
dnl               separated by spaces (may be empty)
dnl
define([TE_APP],
[
TE_HOST_DEFINED=yes
TE_BS_APPS="$1"
])

dnl Specifies additional parameters to be passed to configure script 
dnl of the application and list of external libraries to be linked with
dnl the application. May be called once for each TEN application.
dnl
dnl Parameters:
dnl       name of the directory in ${TE_BASE}/engine
dnl       additional parameters to configure script (may be empty)
dnl       additional compiler flags
dnl       additional linker flags
dnl       list of libraries names (names of directories 
dnl       in ${TE_BASE}/lib in the order "independent last")
dnl
define([TE_APP_PARMS],
[
[
TE_HOST_DEFINED=yes
if test "${TE_BS_$1_APP_PARMS_SPECIFIED}" = "yes";
then
    TE_BS_CONF_ERR="parameters for the TEN application $1 are specified twice" ; 
    break ; 
fi
]
TE_BS_APP_$1_PARMS="$2"
TE_BS_APP_$1_CFLAGS="$3"
TE_BS_APP_$1_LDFLAGS="$4"
TE_BS_APP_$1_LIBS="$5"
TE_BS_$1_APP_PARMS_SPECIFIED=yes
])

dnl Declares the list of tools to be built by "make all" command.
dnl May be called only once.
dnl
dnl Parameters:
dnl       list of names of directories in ${TE_BASE}/tools 
dnl               separated by spaces (may be empty)
dnl
define([TE_TOOLS],
[
TE_HOST_DEFINED=yes
TE_BS_TOOLS="$1"
])

dnl Specifies additional parameters to be passed to configure script of the
dnl tool. May be called once for each tool.
dnl
dnl Parameters:
dnl       name of the directory in ${TE_BASE}/tools
dnl       additional parameters to configure script (may be empty)
dnl       additional compiler flags
dnl       additional linker flags
dnl
define([TE_TOOL_PARMS],
[
[
TE_HOST_DEFINED=yes
if test "${TE_BS_$1_TOOL_PARMS_SPECIFIED}" = "yes";
then
    TE_BS_CONF_ERR="parameters for the tool $1 are specified twice" ; 
    break ; 
fi
]
TE_BS_TOOL_$1_PARMS="$2"
TE_BS_TOOL_$1_CFLAGS="$3"
TE_BS_TOOL_$1_LDFLAGS="$4"
TE_BS_$1_TOOL_PARMS_SPECIFIED=yes
])

dnl Requests for checking of program presence. May be called several times.
dnl
dnl Parameters:
dnl       program name
dnl
define([TE_HOST_EXEC],
[
TE_HOST_DEFINED=yes
TE_BS_HOST_EXEC="$TE_BS_HOST_EXEC $1"
])


dnl Specifies parameters for the Test Agent. Can be called once for each TA.
dnl
dnl Parameters:
dnl       type of the Test Agent 
dnl       platform (may be empty)
dnl       sources location (name of the directory in ${TE_BASE}/agents or
dnl               full absolute path to sources)
dnl       additional parameters to configure script (may be empty)
dnl       additional compiler flags
dnl       additional linker flags
dnl       list of external libraries names 
dnl               (names of directories in ${TE_BASE}/lib)
dnl       
define([TE_TA_TYPE],
[
[
TE_HOST_DEFINED=yes
if test -n "${TE_BS_TA_$1_SOURCES}" ;
then
    TE_BS_CONF_ERR="configuration for TA $1 is specified twice" ; 
    break ; 
fi
]
SOURCES=$3
if test -z "$SOURCES" ; then 
    SOURCES=${TE_BASE}/agents/$1 ; 
elif test "${SOURCES:0:1}" != "/" ; then 
    SOURCES=${TE_BASE}/agents/$SOURCES ; 
fi
if ! test -d "$SOURCES" ; then
    TMP=${TE_BASE}/lib/`basename $SOURCES`
    if test -d "$TMP" ; then
        SOURCES=$TMP
    fi
fi
TE_BS_TA="$TE_BS_TA $1"
TE_BS_TA_$1_SOURCES=$SOURCES
TE_BS_TA_$1_PARMS="$4"
TE_BS_TA_$1_CFLAGS="$5"
TE_BS_TA_$1_LDFLAGS="$6"
TE_BS_TA_$1_LIBS="$7"
[
if test -z "$2" ; then
    PLATFORM=${host};
else
    PLATFORM=$2;
fi
TE_BS_TA_$1_PLATFORM=$PLATFORM
]
])

dnl Specifies the suite.
dnl
dnl Parameters:
dnl       suite name
dnl       sources location - name of the directory in ${TE_BASE}
dnl       
define([TE_SUITE],
[
TE_HOST_DEFINED=yes
ADD=yes
for i in $TE_BS_SUITES ; do 
    if test x"$i" = x"$1" ; then
        ADD=no
        break
    fi
done
if test "$ADD"="yes" ; then
    TE_BS_SUITES="$TE_BS_SUITES $1" 
fi
]
)
