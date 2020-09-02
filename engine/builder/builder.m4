dnl Test Environment
dnl
dnl Builder configuration macros definitions
dnl
dnl Copyright (C) 2003-2018 OKTET Labs.
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
dnl       flag to force local build (local or remote)
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
eval `echo ${PLATFORM}_LOCAL_BUILD=\"$7\"`
])

dnl Declares a platform for and specifies platform-specific
dnl
dnl Parameters:
dnl       platform name; may be empty for host platform (name "default"
dnl           is used for it); shouldn't contain '-'
dnl       build system to use; autotools or meson
dnl
define([TE_PLATFORM_BUILD],
[
PLATFORM=$1
if test -z "$PLATFORM" ; then
    PLATFORM=default
fi
case "$2" in
    autotools|meson|"") ;;
    *)
        TE_BS_CONF_ERR="wrong build system $2 for platform $1" ;
        break 2 ;;
esac
eval `echo ${PLATFORM}_BUILD=\"$2\"`
])

dnl Declares an external component for a platform.
dnl An external component is built before the platform itself,
dnl so that TE libraries could make use of
dnl
dnl Parameters:
dnl       extension name
dnl       platform name; may be empty for host platform (name "default"
dnl           is used for it); shouldn't contain '-'
dnl       extension sources (absolute pathname or relative to TE_BASE/lib)
dnl       source preparation script (always run on the engine side).
dnl           The script is run in the sources directory as specified above.
dnl           The following env variables are exported:
dnl           - EXT_BUILDDIR: build directory path, as specified in the next
dnl                           argument
dnl       build directory (absolute or relative to source path)
dnl       build script (may be run on the engine or agent side)
dnl           The script is run in the build directory, as specified above.
dnl           The following env variables are exported:
dnl           - TE_PREFIX: the installation prefix of TE build
dnl                        for the current platform
dnl           - EXT_SOURCES: absolute path to the sources
dnl       list of headers to copy to TE build tree
dnl       list of libraries to copy to TE build tree
dnl       list of environment variables to export to the build script
dnl       (this only matters for remote builds)
dnl
define([TE_PLATFORM_EXT],
[[
EXTNAME="$1"
case "$EXTNAME" in
     [^a-zA-Z]*)
        TS_BS_CONF_ERR="extension name does not start with a letter" ;
        break ;
        ;;
     *[^a-zA-Z0-9_]*)
        TS_BS_CONF_ERR="extension name contain illegal characters" ;
        break ;
        ;;
esac
PLATFORM="$2"
SOURCES="$3"
if test -z "$PLATFORM" ; then
    PLATFORM=${TE_HOST}
fi
if test -z "$SOURCES" ; then
    SOURCES=${TE_BASE}/lib/${EXTNAME} ;
elif test "${SOURCES:0:1}" != "/" ; then
    SOURCES=${TE_BASE}/lib/$SOURCES ;
fi
if ! test -d "$SOURCES" ; then
    TE_BS_CONF_ERR="source path for platform extension ${EXTNAME} does not exist or is not a directory" ;
    break ;
fi
BUILDDIR="$5"
if test -z "$BUILDDIR"; then
   BUILDDIR="${TE_BUILD}/platforms/${PLATFORM}/lib/${EXTNAME}"
fi
EXTLIST="TE_BS_EXT_${PLATFORM}"
declare "$EXTLIST"="${!EXTLIST} ${EXTNAME}"
declare "${EXTLIST}_${EXTNAME}_SOURCES"="$SOURCES"
declare "${EXTLIST}_${EXTNAME}_PREPARE"="$4"
declare "${EXTLIST}_${EXTNAME}_BUILDDIR"="${BUILDDIR}"
declare "${EXTLIST}_${EXTNAME}_BUILD"="$6"
declare "${EXTLIST}_${EXTNAME}_INSTALL_HEADERS"="$7"
declare "${EXTLIST}_${EXTNAME}_INSTALL_LIBS"="$8"
declare "${EXTLIST}_${EXTNAME}_ENV_VARS"="$9"
]])

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

dnl Declares an external application for the agent host
dnl An external component is built after the agent itself.
dnl
dnl Parameters:
dnl       extension name
dnl       platform (should be empty for host platform)
dnl       list of TA types (as defined by TE_TA_TYPE macro)
dnl       extension sources (absolute pathname or relative to TE_BASE/apps)
dnl       source preparation script (always run on the engine side)
dnl           The script is run in the sources directory as specified above.
dnl           The following env variables are exported:
dnl           - EXT_BUILDDIR: build directory path, as specified in the next
dnl                           argument
dnl       build directory (absolute or relative to source path)
dnl       list of agent libraries to link the app with
dnl           This is a list of entity names (i.e. without lib prefix and .a suffix),
dnl           as in TE_TA_TYPE which is
dnl           used to construct the value of TE_LDFLAGS variable (see below).
dnl           Listing a library here won't per se create a build dependency and
dnl           won't cause the library to be built
dnl       build script (may be run on the engine or agent side)
dnl           The script is run in the build directory, as specified above.
dnl           The following env variables are exported:
dnl           - TE_PREFIX: the installation prefix of TE build
dnl                        for the current platform
dnl           - TE_CPPFLAGS: gcc preprocess flags to use TE-provided headers
dnl           - TE_LDFLAGS: linker flags to use TE libraries
dnl           - EXT_SOURCES: absolute path to the sources
dnl           - TE_TA_TYPES: list of TA types
dnl           - TE_AGENTS_INST: agents installation directory - this directory
dnl                             contains all the agents specified in TE_TA_TYPES
dnl       list of executables to copy to TA agent directory
dnl       list of environment variables to export to the build script
dnl       (this only matters for remote builds)
dnl
define([TE_TA_APP],
[[
APPNAME="$1"
case "$APPNAME" in
     [^a-zA-Z]*)
        TS_BS_CONF_ERR="application name does not start with a letter" ;
        break ;
        ;;
     *[^a-zA-Z0-9_]*)
        TS_BS_CONF_ERR="application name contain illegal characters" ;
        break ;
        ;;
esac
PLATFORM=$2
if test -z "$PLATFORM" ; then
    PLATFORM=${TE_HOST}
fi
TATYPES="$3"
SOURCES="$4"
if test -z "$SOURCES" ; then
    SOURCES=${TE_BASE}/apps/${APPNAME} ;
elif test "${SOURCES:0:1}" != "/" ; then
    SOURCES=${TE_BASE}/apps/$SOURCES ;
fi
if ! test -d "$SOURCES" ; then
    TE_BS_CONF_ERR="source path for agent extension ${APPNAME} does not exist or is not a directory" ;
    break ;
fi
BUILDDIR="$6"
if test -z "$BUILDDIR"; then
    BUILDDIR="${TE_BUILD}/${PLATFORM}/apps/${APPNAME}"
fi
TA_APPS="TE_BS_TA_APPS_${PLATFORM}"
declare "$TA_APPS"="${!TA_APPS} ${APPNAME}"
declare "${TA_APPS}_${APPNAME}_TATYPES"="$TATYPES"
declare "${TA_APPS}_${APPNAME}_SOURCES"="$SOURCES"
declare "${TA_APPS}_${APPNAME}_PREPARE"="$5"
declare "${TA_APPS}_${APPNAME}_BUILDDIR"="$BUILDDIR"
declare "${TA_APPS}_${APPNAME}_LIBS"="$7"
declare "${TA_APPS}_${APPNAME}_BUILD"="$8"
declare "${TA_APPS}_${APPNAME}_INSTALL_BIN"="$9"
declare "${TA_APPS}_${APPNAME}_ENV_VARS"="$10"
]])

dnl Specifies TCE-enabled components
dnl Note that the macro just specifies the location of sources and
dnl object files to be used by TCE processing tools.
dnl It does *not* cause the component to be compiled with TCE-enabling options,
dnl nor does it cause TCE reports to be generated
dnl
dnl Parameters
dnl        Component name
dnl        Component kind:
dnl        - library
dnl        - agent (TCE-ing agents is necessary when a TCE-ed library is statically
dnl                 linked into it)
dnl        - app
dnl        Target platform (for libraries) or agent type (for agents and apps)
dnl        Source directory (absolute or relative to TE_BASE/lib, TE_BASE/agents, TE_BASE/apps,
dnl            depending on the kind of the entity). Default is the name of the component.
dnl        Build directory (absolute or relative to
dnl            TE_BUILD/platforms/PLATFORM/{agents,lib} or
dnl            TE_BUILD/apps/AGTYPE depending on the kind of the entity).
dnl            Default is the name of the component.
dnl        Source pattern to exclude (a regexp)

define([TE_TCE],
[[
COMPONENT="$1"
KIND="$2"
PLATFORM="$3"
SOURCES="$4"
BUILDDIR="$5"
EXCLUDESRC="$6"

case "$KIND" in
     app)
        # For applications and agents the argument is the agent type,
        # and we need to infer the platform for the corresponding agent definition
        AGTYPE="$PLATFORM"
        AGENT_PLATFORM_VAR="TE_BS_TA_${AGTYPE}_PLATFORM"
        PLATFORM="${!AGENT_PLATFORM_VAR}"
        ;;
     agent)
        # Essentially, the same logic as in the previous branch,
        # but for the agent its type is the name of the component,
        # and the platform argument should normally left empty
        AGTYPE="$COMPONENT"
        if test -z "$PLATFORM"; then
           AGENT_PLATFORM_VAR="TE_BS_TA_${AGTYPE}_PLATFORM"
           PLATFORM="${!AGENT_PLATFORM_VAR}"
        fi
        ;;
     library)
        AGTYPE=""
        if test -z "$PLATFORM"; then
           PLATFORM="${TE_HOST}"
        fi
        ;;
     *)
        TE_BS_CONF_ERR="invalid kind of ${COMPONENT}: ${KIND}" ;
        break ;
        ;;
esac
if test -z "$PLATFORM"; then
   TE_BS_CONF_ERR="TCE component ${COMPONENT} refers to undefined agent type ${AGTYPE}" ;
   break ;
fi
if test -z "$SOURCES" ; then
    SOURCES="$COMPONENT"
fi
if test "${SOURCES:0:1}" != "/" ; then
   case "$KIND" in
       library)
         SOURCES=${TE_BASE}/lib/$SOURCES ;
         ;;
       agent)
         SOURCES=${TE_BASE}/agents/$SOURCES ;
         ;;
       app)
         SOURCES=${TE_BASE}/apps/$SOURCES ;
         ;;
  esac
fi
if ! test -d "$SOURCES" ; then
    TE_BS_CONF_ERR="source path (${SOURCES}) for TCE component ${COMPONENT} does not exist or is not a directory" ;
    break ;
fi
if test -z "$BUILDDIR"; then
   BUILDDIR="${COMPONENT}"
fi
if test "${BUILDDIR:0:1}" != "/" ; then
   case "$KIND" in
       library)
         BUILDDIR=${TE_BUILD}/platforms/${PLATFORM}/lib/${BUILDDIR} ;
         ;;
       agent)
         BUILDDIR=${TE_BUILD}/platforms/${PLATFORM}/agents/${BUILDDIR} ;
         ;;
       app)
         BUILDDIR=${TE_BUILD}/apps/${BUILDDIR}/${AGTYPE} ;
         ;;
  esac
fi

TCELIST="TE_BS_TCE_${PLATFORM}"
declare "$TCELIST"="${!TCELIST} ${COMPONENT}"
declare "${TCELIST}_${COMPONENT}_KIND"="$KIND"
declare "${TCELIST}_${COMPONENT}_AGTYPE"="$AGTYPE"
declare "${TCELIST}_${COMPONENT}_SOURCES"="$SOURCES"
declare "${TCELIST}_${COMPONENT}_BUILDDIR"="$BUILDDIR"
declare "${TCELIST}_${COMPONENT}_EXCLUDESRC"="$EXCLUDESRC"
]])

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
