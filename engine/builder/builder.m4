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
dnl $Id: builder.m4,v 1.4 2003/12/08 15:15:51 oleg Exp $
 
changequote([,])

dnl Declares the list of tools to be built by "make all" command.
dnl May be called several times.
dnl
dnl Parameters:
dnl       list of names of directories in ${TE_BASE}/tools 
dnl               separated by spaces (may be empty)
dnl
define([TE_TOOLS],
[
TE_BS_TOOLS="$TE_BS_TOOLS $1"
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
NUT_$1_TCE_SOURCES="$NUT_$1_TCE_SOURCES -c \"$3\" $2"
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
dnl
define([TE_PLATFORM],
[
if echo $TE_BS_PLATFORMS | grep -q $1 ;
then
    TE_BS_CONF_ERR="platform $1 is specified twice" ; 
    break ; 
fi
if test "$1" = "$host" ;
then
    TE_BS_HOST_PARMS="$2" ;
    TE_BS_HOST_CFLAGS="$3" ;
    TE_BS_HOST_LDFLAGS="$4" ;
else
    TE_BS_PLATFORMS="$TE_BS_PLATFORMS $1" ;
    patsubst($1, -, _)_PARMS="$2" ;
    patsubst($1, -, _)_CFLAGS="$3" ;
    patsubst($1, -, _)_LDFLAGS="$4" ;
fi
])

dnl Specifies list of mandatory libraries to be built (may be empty). 
dnl May be called only once. If the macro is not called, all mandatory
dnl libraries are built.
dnl Libraries should be listed in order "independent first".
dnl
dnl Parameters:
dnl       list of directory names in ${TE_BASE}/lib separated by spaces
dnl
define([TE_LIB],
[
[
if test "$TE_BS_LIB" != "empty" ;
then
    TE_BS_CONF_ERR="macro TE_LIB should be called only once" ; 
    break ; 
fi
]
TE_BS_LIB="$1"
])

dnl Specifies additional parameters to be passed to configure script of the
dnl mandatory or external library (common for all platforms). May be called 
dnl once for each library.
dnl
dnl Parameters:
dnl       name of the directory in ${TE_BASE}/lib
dnl       additional parameters to configure script (may be empty)
dnl       additional compiler flags
dnl       additional linker flags
dnl
define([TE_LIB_PARMS],
[
[
if test "${TE_BS_$1_LIB_PARMS_SPECIFIED}" = "yes";
then
    TE_BS_CONF_ERR="parameters for the library $1 are specified twice" ; 
    break ; 
fi
]
TE_BS_LIB_$1_PARMS="$2"
TE_BS_LIB_$1_CFLAGS="$3"
TE_BS_LIB_$1_LDFLAGS="$4"
TE_BS_$1_LIB_PARMS_SPECIFIED=yes
])

dnl Specifies external libraries necessary for TEN applications, Test Agents
dnl and Test Suites. May be called once.
dnl Libraries should be listed in order "independent first".
dnl
dnl Parameters:
dnl       list of directory names in ${TE_BASE}/lib separated by spaces
dnl
define([TE_EL],
[
[
if test -n "$TE_BS_EL" ; 
then
    TE_BS_CONF_ERR="macro TE_EL should be called only once" ; 
    break ; 
fi
]
TE_BS_EL="$1"
])

dnl Specifies list of TEN applications to be built (may be empty). 
dnl May be called only once. If the macro is not called, all TEN applications
dnl are built.
dnl
dnl Parameters:
dnl       list of directory names in ${TE_BASE}/engine separated by spaces
dnl
define([TE_APP],
[
[
if test "$TE_BS_APP" != "empty" ;
then
    TE_BS_CONF_ERR="macro TE_APP should be called only once" ; 
    break ; 
fi
]
TE_BS_APP="$1"
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
dnl       list of external libraries names (names of directories 
dnl              in ${TE_BASE}/lib in the order "independent last")
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
TE_BS_APP_$1_CFLAGS="$3"
TE_BS_APP_$1_LDFLAGS="$4"
[
DUMMY=
if test -n "$5" ;
then
    for i in $DUMMY $5 ; do
        TE_BS_APP_$1_LDADD="${TE_BS_APP_$1_LDADD} -l$i" ;
    done
    for i in $DUMMY $5 ; do
        TE_BS_APP_$1_DEPENDENCIES="${TE_BS_APP_$1_DEPENDENCIES} \$(DESTDIR)/\$(prefix)/lib/lib${i}.a" ;
    done
fi
]
TE_BS_$1_APP_PARMS_SPECIFIED=yes
])

dnl Specifies parameters for the Test Agent. Can be called once for each TA.
dnl
dnl Parameters:
dnl       type of the Test Agent 
dnl       sources location (name of the directory in ${TE_BASE}/agents or
dnl       full absolute path to sources)
dnl       platform (may be empty)
dnl       additional parameters to configure script (may be empty)
dnl       additional compiler flags
dnl       additional linker flags
dnl       list of external libraries names 
dnl               (names of directories in ${TE_BASE}/lib)
dnl       
define([TE_TA_TYPE],
[
[
if test -n "${TE_BS_TA_$1_SOURCES}" ;
then
    TE_BS_CONF_ERR="configuration for TA $1 is specified twice" ; 
    break ; 
fi
]
TE_BS_TA="$TE_BS_TA $1"
TE_BS_TA_$1_SOURCES=$2
TE_BS_TA_$1_PARMS="$4"
TE_BS_TA_$1_CFLAGS="$5"
TE_BS_TA_$1_LDFLAGS="$6"
[
DUMMY=
if test -z "$3" ; then
    platform=${host};
else
    platform=$3;
fi
TE_BS_TA_$1_PLATFORM=$platform
if test -n "$7" ;
then
    for i in $DUMMY $7 ; do
        TE_BS_TA_$1_LDADD="${TE_BS_TA_$1_LDADD} -l$i" ;
	for j in $DUMMY $TE_BS_LIB $TE_BS_EL ; do
            if test $j = $i ; then
                if test ${platform} = ${host} ; then 
                TE_BS_TA_$1_DEPENDENCIES="${TE_BS_TA_$1_DEPENDENCIES} \$(DESTDIR)/\$(prefix)/../${platform}/lib/lib${i}.a" ;
                else
                    TE_BS_TA_$1_DEPENDENCIES="${TE_BS_TA_$1_DEPENDENCIES} \$(DESTDIR)/\$(prefix)/${platform}/lib/lib${i}.a" ;
                fi
                TE_BS_TA_$1_EL="${i} ${TE_BS_TA_$1_EL}" ;
		break 1;
	    fi
        done
    done
fi
]
])
