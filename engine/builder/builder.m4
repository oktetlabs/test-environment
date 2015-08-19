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

TE_HOST=
 
changequote([,])

dnl Declares a platform for and specifies platform-specific 
dnl parameters for configure script as well platform-specific 
dnl CPPFLAGS, CFLAGS and LDFLAGS. 
dnl May be called once for each platform (including host platform).
dnl Host platform should appear first.
dnl
dnl Parameters:
dnl       platform name; may be empty for host platform (name "default" 
dnl           is used for it); shouldn't contain '-'
dnl       configure parameters (including --host for cross-compiling and 
dnl           variables in form VAR=VAL)
dnl       additional preprocessor flags
dnl       additional compiler flags
dnl       additional linker flags
dnl       list of all libraries to be built
dnl
define([TE_PLATFORM],
[
PLATFORM=$1
if test -z "$PLATFORM" ; then
    PLATFORM=default
fi    
for i in $TE_BS_PLATFORMS ; do
    if test $i = $PLATFORM ; then
        TE_BS_CONF_ERR="platform $1 is specified twice" ; 
        break 2 ; 
    fi
done    
if test -z "$TE_HOST" ; then
    TE_HOST=$PLATFORM
fi    
TE_BS_PLATFORMS="$TE_BS_PLATFORMS $PLATFORM"
eval `echo ${PLATFORM}_PARMS=\"$2\"`
eval `echo ${PLATFORM}_CPPFLAGS=\"$3\"`
eval `echo ${PLATFORM}_CFLAGS=\"$4\"`
eval `echo ${PLATFORM}_LDFLAGS=\"$5\"`
eval `echo ${PLATFORM}_LIBS=\"$6\"`
])

dnl Specifies list of external static libraries that should be 
dnl loaded via http from the server specified in TE_EXT_LIBS
dnl environment variable.
dnl
dnl Parameters:
dnl       platform name in TE_INSTALL
dnl       platform name on server
dnl       list of libraries
dnl
define([TE_EXT_LIBS],
[
PLATFORM=$1
if test -z "$PLATFORM" ; then
    PLATFORM=default
fi
DIR=$2
if test -z "$DIR" ; then
    DIR=i386-pc-linux
fi    
eval `echo ${PLATFORM}_EXT_LIBS_DIR=\"$DIR\"`
eval `echo ${PLATFORM}_EXT_LIBS=\"$3\"`
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
dnl       additional preprocessor flags
dnl       additional compiler flags
dnl       additional linker flags
dnl
define([TE_LIB_PARMS],
[
[
PLATFORM=$2
SOURCES=$3
if test -z "$PLATFORM" ; then
    PLATFORM=${TE_HOST}
fi
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
eval `echo TE_BS_LIB_${PLATFORM}_$1_SOURCES=$SOURCES`
eval `echo TE_BS_LIB_${PLATFORM}_$1_PARMS=\"$4\"`
eval `echo TE_BS_LIB_${PLATFORM}_$1_CPPFLAGS=\"$5\"`
eval `echo TE_BS_LIB_${PLATFORM}_$1_CFLAGS=\"$6\"`
eval `echo TE_BS_LIB_${PLATFORM}_$1_LDFLAGS=\"$7\"`
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
TE_BS_APPS="$1"
])

dnl Specifies additional parameters to be passed to configure script 
dnl of the application and list of external libraries to be linked with
dnl the application. May be called once for each TEN application.
dnl
dnl Parameters:
dnl       name of the directory in ${TE_BASE}/engine
dnl       additional parameters to configure script (may be empty)
dnl       additional preprocessor flags
dnl       additional compiler flags
dnl       additional linker flags
dnl       list of libraries names (names of directories 
dnl       in ${TE_BASE}/lib in the order "independent last")
dnl
define([TE_APP_PARMS],
[
[
if test "${TE_BS_$1_APP_PARMS_SPECIFIED}" = "yes";
then
    TE_BS_CONF_ERR="parameters for the TEN application $1 are specified twice" ; 
    break ; 
fi
]
TE_BS_APP_$1_PARMS="$2"
TE_BS_APP_$1_CPPFLAGS="$3"
TE_BS_APP_$1_CFLAGS="$4"
TE_BS_APP_$1_LDFLAGS="$5"
TE_BS_APP_$1_LIBS="$6"
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
TE_BS_TOOLS="$1"
])

dnl Specifies additional parameters to be passed to configure script of the
dnl tool. May be called once for each tool.
dnl
dnl Parameters:
dnl       name of the directory in ${TE_BASE}/tools
dnl       additional parameters to configure script (may be empty)
dnl       additional preprocessor flags
dnl       additional compiler flags
dnl       additional linker flags
dnl
define([TE_TOOL_PARMS],
[
[
if test "${TE_BS_$1_TOOL_PARMS_SPECIFIED}" = "yes";
then
    TE_BS_CONF_ERR="parameters for the tool $1 are specified twice" ; 
    break ; 
fi
]
TE_BS_TOOL_$1_PARMS="$2"
TE_BS_TOOL_$1_CPPFLAGS="$3"
TE_BS_TOOL_$1_CFLAGS="$4"
TE_BS_TOOL_$1_LDFLAGS="$5"
TE_BS_$1_TOOL_PARMS_SPECIFIED=yes
])

dnl Requests for checking of program presence. May be called several times.
dnl
dnl Parameters:
dnl       program name
dnl
define([TE_HOST_EXEC],
[
TE_BS_HOST_EXEC="$TE_BS_HOST_EXEC $1"
])


dnl Specifies parameters for the Test Agent. Can be called once for each TA.
dnl
dnl Parameters:
dnl       type of the Test Agent 
dnl       platform (should be empty for host platform)
dnl       sources location (name of the directory in ${TE_BASE}/agents or
dnl               full absolute path to sources)
dnl       additional parameters to configure script (may be empty)
dnl       additional preprocessor flags
dnl       additional compiler flags
dnl       additional linker flags
dnl       list of external libraries names 
dnl               (names of directories in ${TE_BASE}/lib)
dnl       
define([TE_TA_TYPE],
[
[
SOURCES=TE_BS_TA_$1_SOURCES
if test -n "${!SOURCES}" ;
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
dnl $1 could be "${TE_IUT_TA_TYPE}"
dnl For declosure of substitutions use declare
declare "TE_BS_TA_$1_SOURCES"=$SOURCES
declare "TE_BS_TA_$1_PARMS"="$4"
declare "TE_BS_TA_$1_CPPFLAGS"="$5"
declare "TE_BS_TA_$1_CFLAGS"="$6"
declare "TE_BS_TA_$1_LDFLAGS"="$7"
declare "TE_BS_TA_$1_LIBS"="$8"
[
if test -z "$2" ; then
    PLATFORM=${TE_HOST};
else
    PLATFORM=$2;
fi
declare "TE_BS_TA_$1_PLATFORM"=$PLATFORM
]
])

dnl Specifies additional parameters for test suite.
dnl The path to test suite is specified in tester.conf file.
dnl
dnl Parameters:
dnl       suite name - should be the same as specified in tester.conf
dnl       additional parameters to configure script (may be empty)
dnl       additional preprocessor flags
dnl       additional compiler flags
dnl       additional linker flags
dnl
define([TE_SUITE],
[
SUITE=$1
SUITE_NAME=`echo $SUITE | tr .- _`
DEFINED=`eval echo '$TE_BS_SUITE_'${SUITE_NAME}'_DEFINED'`
if test "${DEFINED}" ; then
    TE_BS_CONF_ERR="configuration for suite $1 is specified twice" ; 
    break ; 
fi
eval `echo TE_BS_SUITE_${SUITE_NAME}_DEFINED=yes`
eval `echo TE_BS_SUITE_${SUITE_NAME}_PARMS=\"$2\"`
eval `echo TE_BS_SUITE_${SUITE_NAME}_CPPFLAGS=\"$3\"`
eval `echo TE_BS_SUITE_${SUITE_NAME}_CFLAGS=\"$4\"`
eval `echo TE_BS_SUITE_${SUITE_NAME}_LDFLAGS=\"$5\"`
]
)

dnl Execute arbitrary shell script specified as the first argument.
dnl
dnl Parameters:
dnl       shell script to be executed
dnl
define([TE_SHELL],
[
$1
]
)
