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
fi

if test "${SOURCES:0:1}" != "/" ; then 
    SOURCES=${TE_BASE}/lib/$SOURCES ; 
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

dnl NUT images

dnl Specifies parameters for NUT bootable image building.
dnl Moreover the NUT image name is included to the list of NUT images
dnl to be built by "make all" command.
dnl May be called once for each NUT image name. Source location may
dnl be the same for several NUT image names.
dnl
dnl Parameters:
dnl       NUT image name
dnl       LIB (if NUT is a library and should be built before engine
dnl             and Test Agents) or PRG otherwise
dnl       source directory (absolute or relative from ${TE_BASE})
dnl       building script location directory (absolute or 
dnl             relative from ${TE_BASE})
dnl       additional parameters to the building script (may be empty)
dnl
dnl Optional parameters:
dnl       full filename of the image in the storage 
dnl       storage configuration string
dnl       
define([TE_NUT],
[
[
TE_HOST_DEFINED=yes
if test -n "$NUT_$1_SOURCES" ;
then
    TE_BS_CONF_ERR="Warning: definition of the NUT $1 appears twice" ;
    break ;
fi
]
if test "$2" = "LIB" ; then
    TE_BS_NUTLIBS="$TE_BS_NUTLIBS $1"
else
    TE_BS_NUTS="$TE_BS_NUTS $1"
fi
SOURCES=$3 ;
if test "${SOURCES:0:1}" != "/" ; then
    SOURCES=${TE_BASE}/${SOURCES}
fi
NUT_$1_SOURCES=${SOURCES}
SCRIPT=$4
if test "${SCRIPT:0:1}" != "/" ; then
    SCRIPT=${TE_BASE}/${SCRIPT}
fi
NUT_$1_SCRIPT=${SCRIPT}/te_build_nut
NUT_$1_PARMS="$5"
NUT_$1_ST_LOCATION=$6
NUT_$1_ST_STRING="$7"
])

dnl Specifies common TCE parameters for the NUT. May be called once for 
dnl each NUT image name.
dnl
dnl Parameters:
dnl       NUT image name
dnl       Test Agent name
dnl       Test Agent type
dnl       TCE type: branch, loop, multi, relational, routine, call or all
dnl               (see OKT-HLD-0000037-TE_TCE for details).
dnl       TCE data format (FILE or LOGGER)
dnl
define([TE_NUT_TCE],
[
[
TE_HOST_DEFINED=yes
if test -n "$NUT_$1_TCE_TANAME" ;
then
    TE_BS_CONF_ERR="TCE parameters for the NUT $1 are specified twice" ;
    break ;
fi
]
NUT_$1_TCE_TANAME=$2
NUT_$1_TCE_TATYPE=$3
[
TMP=
ALL=
for i in $4 ; do
    if echo $TMP | grep -q $i ;
    then
        TE_BS_CONF_ERR="incorrect TCE type expression for the NUT $1" ;
        break 2 ;
    fi
        
    case $i in
        all) ALL=yes ;;
        branch) ;;
        loop) ;;
        multi) ;;
        relational) ;;
        routine) ;;
        call) ;;
        *) 
            TE_BS_CONF_ERR="wrong TCE type for the NUT $1" ; 
            break 2 ; 
        ;;
    esac
    
    if test -z "$TMP" ;
    then
        TMP=$i ;
    else
        TMP=$TMP",$i" ;
    fi
done    
if test "$ALL" = "yes" -a "$TMP" != "all"  ;
then
    TE_BS_CONF_ERR="incorrect TCE type expression for the NUT $1" ;
    break;
fi
if test -z "$TMP" ;
then
    TE_BS_CONF_ERR="TCE type is not specified for the NUT $1" ;
    break;
fi
NUT_$1_TCE_TYPE="-t "$TMP
]
[
case $5 in
    FILE) ;;
    LOGGER) ;;
    *) 
        TE_BS_CONF_ERR="wrong TCE data format for the NUT $1" ; 
        break ; 
        ;;
esac
]
NUT_$1_TCE_FMT="-x "$5
])

dnl Specifies NUT sources to be instrumented. May be called several times.
dnl 
dnl Parameters:
dnl       NUT image name
dnl       list of files/directories to be instrumented separated by spaces; 
dnl               if a directory is specified, all .c and .h files in it
dnl               and in all its subdirectories are instrumented; 
dnl               TCE data array and auxiliary routine definitions are 
dnl               appended to the first .c file met during macros processing
dnl       additional compiler flags to be used for sources compilation
dnl               
define([TE_NUT_TCE_SOURCES],
[
TE_HOST_DEFINED=yes
NUT_$1_TCE_SOURCES="$NUT_$1_TCE_SOURCES -c \"$3\" $2"
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
fi

if test "${SOURCES:0:1}" != "/" ; then 
    SOURCES=${TE_BASE}/agents/$SOURCES ; 
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
    if $i = $1 ; then
        ADD=no
        break
    fi
done
if test "$ADD"="yes" ; then
    TE_BS_SUITES="$TE_BS_SUITES $1" 
fi
]
)
