# Process this file with autoconf to produce a configure script.

# Test Environment
#
# Root configure
#
# Copyright (C) 2003 Test Environment authors (see file AUTHORS in the 
# root directory of the distribution).
#
# Test Environment is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as 
# published by the Free Software Foundation; either version 2 of 
# the License, or (at your option) any later version.
# 
# Test Environment is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
# MA  02111-1307  USA
#
# Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>

AC_PREREQ(2.53)

AC_INIT(test_environment, 0.8, Elena.Vengerova@oktetlabs.ru)

AC_CONFIG_AUX_DIR(aux)

AM_INIT_AUTOMAKE([foreign -Wall])

AC_PROG_INSTALL

# Auxiliary functions

# Checks if a host tool exists and visible.
no_host_tool() 
{
    TMP=`which $1 2>/dev/null` ;
    test -z "$TMP"  ;
}

# Filter out several occurance of one word
filter_var()
{
    val=`eval echo '$'$1`
    val=`for i in ${val} ; do echo $i ; done | sort | uniq`
    val=`echo $val`
    eval $1="\"$val\""
}

# Run configure and make install-data for the host platform
configure_package()
{
    if test "${kind}" != "TA" ; then 
        mkdir -p ${package} ;
        cd ${package} ;
        PREFIX=$prefix/$host ;
    else
        PREFIX=$prefix/agents
    fi
    PARMS=`eval echo '$TE_BS_'${kind}'_'${package}'_PARMS'`
    ADD_CFLAGS=`eval echo '$TE_BS_'${kind}'_'${package}'_CFLAGS'`
    ADD_LDFLAGS=`eval echo '$TE_BS_'${kind}'_'${package}'_LDFLAGS'`

    # add --target option if the user has provided it to us
    if test -n "${target_alias}"; then
        OPT_TARGET="--target=${target_alias}"
    else
        OPT_TARGET=""
    fi
    
    echo ${PKGSRC}/configure -q --prefix=$PREFIX ${OPT_TARGET} ${PARMS} ${TE_BS_HOST_PARMS}
    
    TE_CFLAGS="-DDEBUG -Wall -W -O0 -g -ggdb ${ADD_CFLAGS} ${TE_BS_HOST_CFLAGS}" \
    TE_LDFLAGS="${ADD_LDFLAGS} $TE_BS_HOST_LDFLAGS" \
    TE_LDADD=`eval echo '$TE_BS_'${kind}'_'${package}'_LDADD'` \
    TE_DEPENDENCIES=`eval echo '$TE_BS_'${kind}'_'${package}'_DEPENDENCIES'` \
    eval ${PKGSRC}/configure '-q --prefix=$PREFIX ${OPT_TARGET}' ${PARMS} ${TE_BS_HOST_PARMS}

    if test $? -ne 0 ; then
        rm ../../Makefile.conf
        exit 1 ;
    fi
    if test "${kind}" != "TA" ; then 
        make install-data
        cd ../
    fi
}

if test -z "$TE_BASE" ; then
    CONFIGURE=`which $0` ;
    export TE_BASE=${CONFIGURE/\/configure/} ;
fi

AC_CANONICAL_HOST

TE_BUILD=`pwd`

if test "$prefix" = "NONE" ; then
    prefix=/usr/local ;
fi    

# Put initial values to control variables

# TE_BS_TOOLS_CONFIGURE - what tools directories should be configured
# and built by "make tools" command.
if test -z "$with_tools"  ; then
# FINAL    TE_BS_TOOLS_CONFIGURE="rgt tsc ndn" ;
    TE_BS_TOOLS_CONFIGURE="rgt" ;
else
    TE_BS_TOOLS_CONFIGURE="$with_tools" ;
fi
 
# TE_BS_TOOLS - what tools should be built by "make all" command.
TE_BS_TOOLS=

# List of NUTs to be built by "make all" command and default list
# of NUTs built by command "make nuts".
TE_BS_NUTS=
TE_BS_NUTLIBS=

# Lists of host libraries and executables
TE_BS_HOST_PARMS=
TE_BS_HOST_LDFLAGS=
TE_BS_HOST_CFLAGS=
TE_BS_HOST_EXEC=

# List of cross-compilation platforms
TE_BS_PLATFORMS=

# List of mandatory libraries
TE_BS_LIB=empty

# List of external libraries
TE_BS_EL=

# List of TEN applications
TE_BS_APP=empty

# List of Test Agents
TE_BS_TA=

# Parse configuration file
if test -n "$with_config" -a "$no_create" != "yes" ; then
    cat ${TE_BASE}/engine/builder/builder.m4 $with_config | m4 > ${TE_BUILD}/builder.conf.processed
else     
    touch ${TE_BUILD}/builder.conf.processed
fi

# Dummy loop to avoid breaking in the case of error
while test 1 ; do
    . ${TE_BUILD}/builder.conf.processed
    break ;
done
if test -n "$TE_BS_CONF_ERR" ; then
    AC_MSG_ERROR([$TE_BS_CONF_ERR])
fi


# Check and update control variables

if echo "${TE_BS_TOOLS_CONFIGURE}" | grep -q rgt ; then 
    if no_host_tool rgt ; then
        TE_BS_TOOLS="${TE_BS_TOOLS} rgt"  ;
    fi ;
fi
if test -z "$with_tools" ; then
    TE_BS_TOOLS_CONFIGURE="${TE_BS_TOOLS_CONFIGURE} ${TE_BS_TOOLS}"
fi
filter_var TE_BS_TOOLS_CONFIGURE
filter_var TE_BS_TOOLS

if test "$TE_BS_LIB" = empty ; then
    TE_BS_LIB="ipc bsapi loggerten rcfapi conf_oid confapi" ;
fi

if test "$TE_BS_APP" = empty ; then
    TE_BS_APP="builder rcf logger tester configurator" ;
fi

# Add --enable-tce option for Test Agents
for i in $TE_BS_NUTS $$TE_BS_NUTLIBS ; do
    NAT_TATYPE=`eval echo '$NUT_'${i}'_TCE_TATYPE'` ;
    if test -n "$NAT_TATYPE" ; then
        for j in $TE_BS_TA ; do
            if test $j = $NAT_TATYPE ; then
                PARMS=`eval echo '$TE_BS_TA_'${NAT_TATYPE}'_PARMS'`" --enable-tce" ;
                eval TE_BS_TA_${NAT_TATYPE}_PARMS='${PARMS}' ;
                break 1 ;
            fi
        done
    fi
done

# Checks for host programs.
for i in $TE_BS_HOST_EXEC ; do 
    if no_host_tool $i ; then
        AC_MSG_ERROR([No host tool $i])
    fi
done ;

# Generate temporary Makefile.conf
echo define import_variables > Makefile.conf.new

echo TE_BS_comm_net_agent_TA="yes ; \\" >> Makefile.conf.new
echo TE_BS_conf_oid_TA="yes ; \\" >> Makefile.conf.new

echo TE_BS_TOOLS_CONFIGURE="\""${TE_BS_TOOLS_CONFIGURE}"\" ; \\" >> Makefile.conf.new
echo TE_BS_TOOLS="\""$TE_BS_TOOLS"\" ; \\" >> Makefile.conf.new
for i in $TE_BS_TOOLS_CONFIGURE ; do
    for k in PARMS CFLAGS LDFLAGS ; do
        val=`eval echo '$TE_BS_TOOL_'${i}'_'$k` 
        echo TE_BS_TOOL_${i}_$k="\""$val"\" ; \\" >> Makefile.conf.new
    done
done

echo TE_BS_NUTS="\""$TE_BS_NUTS"\" ; \\" >> Makefile.conf.new
echo TE_BS_NUTLIBS="\""$TE_BS_NUTLIBS"\" ; \\" >> Makefile.conf.new
for i in $TE_BS_NUTS $TE_BS_NUTLIBS ; do
    for k in SOURCES SCRIPT PARMS ST_LOCATION ST_STRING \
             TCE_TANAME TCE_TATYPE TCE_TYPE TCE_FMT TCE_SOURCES ; do
        val=`eval echo '$NUT_'${i}'_'$k` 
        echo NUT_${i}_$k="\""$val"\" ; \\" >> Makefile.conf.new
    done
done

for k in PARMS CFLAGS LDFLAGS EXEC ; do
    val=`eval echo '$TE_BS_HOST_'$k` ;
    echo TE_BS_HOST_$k="\""$val"\" ; \\" >> Makefile.conf.new
done

echo TE_BS_PLATFORMS="\""$TE_BS_PLATFORMS"\" ; \\" >> Makefile.conf.new
for i in $TE_BS_PLATFORMS ; do
    pname=`echo $i | sed s/-/_/g`
    PARMS=`eval echo '$'$pname'_PARMS'`
    echo ${pname}_PARMS="\""$PARMS"\" ; \\" >> Makefile.conf.new
    CFLAGS=`eval echo '$'$pname'_CFLAGS'`
    echo ${pname}_CFLAGS="\""$CFLAGS"\" ; \\" >> Makefile.conf.new
    LDFLAGS=`eval echo '$'$pname'_LDFLAGS'`
    echo ${pname}_LDFLAGS="\""$LDFLAGS"\" ; \\" >> Makefile.conf.new
done

echo TE_BS_LIB="\""$TE_BS_LIB"\" ; \\" >> Makefile.conf.new
echo TE_BS_EL="\""$TE_BS_EL"\" ; \\" >> Makefile.conf.new
for i in $TE_BS_LIB $TE_BS_EL ; do
    for k in PARMS CFLAGS LDFLAGS ; do
        val=`eval echo '$TE_BS_LIB_'${i}'_'$k` 
        echo TE_BS_LIB_${i}_$k="\""$val"\" ; \\" >> Makefile.conf.new
    done
done

echo TE_BS_APP="\""$TE_BS_APP"\" ; \\" >> Makefile.conf.new
for i in $TE_BS_APP ; do
    for k in SOURCES PARMS CFLAGS LDFLAGS LDADD DEPENDENCIES; do
        val=`eval echo '$TE_BS_APP_'${i}'_'$k` 
        echo TE_BS_APP_${i}_$k="\""$val"\" ; \\" >> Makefile.conf.new
    done
done

echo TE_BS_TA="\""$TE_BS_TA"\" ; \\" >> Makefile.conf.new
for i in $TE_BS_TA ; do
    for k in SOURCES PARMS CFLAGS LDFLAGS PLATFORM RCFLIB LDADD DEPENDENCIES EL; do
        val=`eval echo '$TE_BS_TA_'${i}'_'$k` 
        echo TE_BS_TA_${i}_$k="\""$val"\" ; \\" >> Makefile.conf.new
    done
done


echo >> Makefile.conf.new
echo endef >> Makefile.conf.new

# Compare new Makefile.conf with new one to avoid unnecessary re-configuring
# (however is Makefile is not generated in the root directory yet
# configuring is necessary in any case).

if test -e Makefile.conf ; then
    if diff -q Makefile.conf Makefile.conf.new > /dev/null ; then 
        rm Makefile.conf.new
        AC_MSG_NOTICE([no parameters are changed])
        exit 0
    fi
fi    

# Parameters are changed: save new Makefile.conf, generate header file for RCF 
# and configure subdirectories according to control variables.

mv Makefile.conf.new Makefile.conf

AC_CONFIG_SUBDIRS(doc)

DUMMY=

# Configure packages
if test -n "$TE_BS_LIB" -o -n "$TE_BS_EL"; then
    kind=LIB
    mkdir -p lib ;
    cd lib ;
    for package in $DUMMY $TE_BS_LIB $TE_BS_EL ; do
        PKGSRC=${TE_BASE}/lib/${package} ;
        configure_package ;
    done
    cd ..
fi
LIB_SUBDIRS="$TE_BS_LIB $TE_BS_EL"
AC_SUBST(LIB_SUBDIRS)

# Configure TEN applications
if test -n "$TE_BS_APP" ; then
    kind=APP
    mkdir -p engine ;
    cd engine ;
    for package in $DUMMY $TE_BS_APP ; do
        PKGSRC=${TE_BASE}/engine/${package} ;
        configure_package ;
    done
    cd ..
fi
APP_SUBDIRS="$TE_BS_APP"
AC_SUBST(APP_SUBDIRS)

# Configure tools
if test -n "${TE_BS_TOOLS_CONFIGURE}" ; then
    kind=TOOL
    mkdir -p tools ;
    cd tools ;
    for package in $DUMMY ${TE_BS_TOOLS_CONFIGURE} ; do
        PKGSRC=${TE_BASE}/tools/${package} ;
        configure_package;
    done
    cd ..
fi
TOOLS_SUBDIRS="${TE_BS_TOOLS_CONFIGURE}" 
AC_SUBST(TOOLS_SUBDIRS)

# Configure Test Agents
TA_SUBDIRS=
TA_REMOTE_SUBDIRS=
if test -n "$TE_BS_TA" ; then
    mkdir -p agents ;
    cd agents ;
    kind=TA
    for package in $DUMMY $TE_BS_TA ; do
        PLATFORM=`eval echo '$TE_BS_TA_'${package}'_PLATFORM'` ;
        if test "$PLATFORM" = "$host" ; then
            PKGSRC=`eval echo '$TE_BS_TA_'${package}'_SOURCES'` ;
            if test "${PKGSRC:0:1}" = "/" ; then
                cd ${PKGSRC} ;
                TA_REMOTE_SUBDIRS="${TA_REMOTE_SUBDIRS} ${PKGSRC}" ;
            else
                TA_SUBDIRS="${TA_SUBDIRS} ${PKGSRC}" ;
                mkdir -p $PKGSRC ;
                cd $PKGSRC ;
                PKGSRC=${TE_BASE}/agents/${PKGSRC} ;
            fi ;
            configure_package ;
        fi
        cd ${TE_BUILD}/agents ;
    done
    cd ..
fi
AC_SUBST(TA_SUBDIRS)
AC_SUBST(TA_REMOTE_SUBDIRS)

AC_SUBST(host)

AM_CONDITIONAL(SUITE_SCRIPTS_PROVIDED, test -n "$SUITE_SCRIPTS")
SUITE_SCRIPTS_EXTRA_DIST=
for i in ${SUITE_SCRIPTS} ; do
    SUITE_SCRIPTS_EXTRA_DIST="${SUITE_SCRIPTS_EXTRA_DIST} `basename $i`"
done
AC_SUBST(SUITE_SCRIPTS_EXTRA_DIST)
AC_SUBST(SUITE_SCRIPTS)

if test -n "$SUITE_SOURCES" ; then
    SUITE_SUBDIR=suites
else
    SUITE_SUBDIR=
fi    
AC_SUBST(SUITE_SUBDIR)

AC_CONFIG_FILES(Makefile include/Makefile lib/Makefile 
                engine/Makefile tools/Makefile agents/Makefile)

AC_OUTPUT
