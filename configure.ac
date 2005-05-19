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

AC_PREREQ([2.59])

AC_INIT([test_environment], [0.9], [Elena.Vengerova@oktetlabs.ru])

AC_CONFIG_AUX_DIR([auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall dist-bzip2])

AC_PROG_INSTALL

CONFIGURE=`which $0` ;
TE_BASE_DIR=`dirname $CONFIGURE`
pushd $TE_BASE_DIR >/dev/null
export TE_BASE=`pwd` 
popd >/dev/null

#AC_CANONICAL_HOST

# Do not use config.guess to avoid problems with 64-bit architecture
# and i686 tools.
host=`${TE_BASE}/engine/builder/te_discover_host`

TE_BUILD=`pwd`

if test "$prefix" = "NONE" ; then
    prefix=/usr/local ;
fi    

TE_INSTALL=$prefix

# Put initial values to control variables

# List of all platforms
TE_BS_PLATFORMS=

# TE_BS_TOOLS - what tools should be built by "make all" command.
TE_BS_TOOLS=rgt

# List of NUTs to be built by "make all" command.
TE_BS_NUTS=
TE_BS_NUTLIBS=

# List of TEN applications
TE_BS_APP="builder rcf logger tester configurator"

# List of Test Agents
TE_BS_TA=

# List of suites
TE_BS_SUITES=

if test -z "${TE_INSTALL_NUT}" ; then 
    TE_INSTALL_NUT=${TE_INSTALL}/nuts ; 
fi ; 

if test -z "$TE_INSTALL_SUITE" ; then
    TE_INSTALL_SUITE=${TE_INSTALL}/suites
fi

# Parse configuration file
echo TE_BASE=$TE_BASE > ${TE_BUILD}/builder.conf.processed.new
echo TE_BUILD=$TE_BUILD >> ${TE_BUILD}/builder.conf.processed.new
echo TE_INSTALL=$TE_INSTALL >> ${TE_BUILD}/builder.conf.processed.new
echo TE_INSTALL_NUT=$TE_INSTALL_NUT >> ${TE_BUILD}/builder.conf.processed.new
echo TE_INSTALL_SUITE=$TE_INSTALL_SUITE >> ${TE_BUILD}/builder.conf.processed.new
echo host=$host >> ${TE_BUILD}/builder.conf.processed.new

AC_ARG_VAR(with_config)

if test -z "$with_config" -a -e ${srcdir}/conf/builder.conf ; then
    # Called from make distcheck
    with_config=${srcdir}/conf/builder.conf
fi    

AC_SUBST([TE_CONF_DIR], [`dirname $with_config`])
TE_BUILDER_CONF="`pwd`/builder.conf"
cp $with_config ${TE_BUILDER_CONF}
AC_SUBST([TE_BUILDER_CONF])

if test -n "$with_config" ; then
    cat ${TE_BASE}/engine/builder/builder.m4 $with_config | m4 >> ${TE_BUILD}/builder.conf.processed.new
else        
    # Fill default configuration
    HOST=`echo $host | sed s/-/_/g`
    TE_BS_PLATFORMS=$HOST
    echo TE_BS_PLATFORMS=$TE_BS_PLATFORMS >> ${TE_BUILD}/builder.conf.processed.new
    eval ${HOST}_LIBS="ipc bsapi loggerten rpcxdr rcfapi conf_oid confapi tad"
    echo ${HOST}_LIBS=\"ipc bsapi loggerten rpcxdr rcfapi conf_oid confapi tad\" >> ${TE_BUILD}/builder.conf.processed.new
    echo TE_BS_APP=\"$TE_BS_APP\" >> ${TE_BUILD}/builder.conf.processed.new
fi
echo 'TE_PATH=${TE_INSTALL}/${host}/bin' >> ${TE_BUILD}/builder.conf.processed.new

if test -e ${TE_BUILD}/builder.conf.processed ; then
    if ! diff -q ${TE_BUILD}/builder.conf.processed ${TE_BUILD}/builder.conf.processed.new > /dev/null ; then 
        cd ${TE_BUILD}    
        rm -rf `ls | grep -v builder.conf`
        cd $OLDPWD
        no_create=""
    fi
fi    

mv ${TE_BUILD}/builder.conf.processed.new ${TE_BUILD}/builder.conf.processed

# Dummy loop to avoid breaking in the case of error
while test 1 ; do
    . ${TE_BUILD}/builder.conf.processed
    break ;
done
if test -n "$TE_BS_CONF_ERR" ; then
    AC_MSG_ERROR([$TE_BS_CONF_ERR])
fi

# Checks for host programs.
for i in $TE_BS_HOST_EXEC ; do 
    TMP=`which $i 2>/dev/null` ;
    if test -z "$TMP"  ; then
        AC_MSG_ERROR([No host tool $i])
    fi
done ;

WORK_DIR=`pwd`
cd ${TE_BASE}/doc
if test ! -e configure ; then
    echo "Calling aclocal/autoheader/autoconf/automake in `pwd`"
    aclocal -I ${TE_BASE}/auxdir || exit 1 ;
    autoconf || exit 1 ;
    automake || exit 1 ;
fi
cd ${WORK_DIR}
AC_CONFIG_SUBDIRS(doc)

DUMMY=

# Process libraries
mkdir -p include
mkdir -p lib
LIB_SUBDIRS=
LIB_DIST_SUBDIRS=
for p in ${TE_BS_PLATFORMS} ; do
    PLATFORM=`eval echo '$'${p}'_PLATFORM'`
    
    if test $PLATFORM == $host ; then 
        ${TE_BASE}/engine/builder/te_configure_package INCLUDE include \
                include $host ${TE_BASE}/include $prefix || exit 1
    fi
    
    LIBS=`eval echo '$'${p}'_LIBS'`
    for package in $LIBS ; do
        PKGNAME=${p}_$package
        PKGSRC=`eval echo '$TE_BS_LIB_'${PKGNAME}'_SOURCES'`
        if test -z "$PKGSRC" ; then 
            PKGSRC=${TE_BASE}/lib/$package ; 
        fi
        
        cd lib
        mkdir -p $PKGNAME
        NAME=`basename $PKGSRC`
        if ! test -e $NAME ; then
            ln -s $PKGNAME $NAME
        fi
        cd ..
        
        if test $PLATFORM != $host ; then continue ; fi
        
        if test -z "$no_create" ; then
            ${TE_BASE}/engine/builder/te_configure_package LIB $package \
            $PKGNAME $PLATFORM $PKGSRC $prefix || exit 1
            cd lib/$PKGNAME
            make install-data || exit 1
            cd ../..
        fi
        LIB_SUBDIRS="$LIB_SUBDIRS $PKGNAME"
    done
done
for i in `find lib -maxdepth 1 -type l` ; do 
    LIB_DIST_SUBDIRS="$LIB_DIST_SUBDIRS `basename $i`" ;
done
AC_SUBST([LIB_SUBDIRS])
AC_SUBST([LIB_DIST_SUBDIRS])

# Configure TEN applications
if test -z "$no_create" ; then
    for package in $DUMMY $TE_BS_APP ; do
        ${TE_BASE}/engine/builder/te_configure_package APP $package $package \
            $host ${TE_BASE}/engine/$package $prefix || exit 1
    done
fi    
APP_SUBDIRS="$TE_BS_APP"
AC_SUBST([APP_SUBDIRS])

# Configure tools
if test -z "$no_create" ; then
    for package in $DUMMY ${TE_BS_TOOLS} ; do
        ${TE_BASE}/engine/builder/te_configure_package TOOL $package $package \
            $host ${TE_BASE}/tools/$package $prefix || exit 1
    done
fi    
TOOLS_SUBDIRS="${TE_BS_TOOLS}" 
AC_SUBST([TOOLS_SUBDIRS])

# Configure Test Agents
TA_SUBDIRS=
for package in $DUMMY $TE_BS_TA ; do
    PLATFORM=`eval echo '$TE_BS_TA_'${package}'_PLATFORM'` ;
    if test "$PLATFORM" != "$host" ; then continue ; fi
    PKGSRC=`eval echo '$TE_BS_TA_'${package}'_SOURCES'` ;
    if test -z "$PKGSRC" ; then 
        PKGSRC=${TE_BASE}/agents/$package ; 
    fi
    if test -z "$no_create" ; then
        ${TE_BASE}/engine/builder/te_configure_package TA $package $package \
            $host $PKGSRC $prefix || exit 1
    fi
    TA_SUBDIRS="$TA_SUBDIRS $package"
done
AC_SUBST([TA_SUBDIRS])

AC_SUBST([host])

AM_CONDITIONAL(SUITE_SCRIPTS_PROVIDED, test -n "$SUITE_SCRIPTS")
SUITE_SCRIPTS_EXTRA_DIST=
for i in ${SUITE_SCRIPTS} ; do
    SUITE_SCRIPTS_EXTRA_DIST="${SUITE_SCRIPTS_EXTRA_DIST} `basename $i`"
done
AC_SUBST([SUITE_SCRIPTS_EXTRA_DIST])
AC_SUBST([SUITE_SCRIPTS])

AC_SUBST([TE_BS_NUTS])
AC_SUBST([TE_BS_NUTLIBS])

if test -z "$no_create" ; then
    mkdir -p ${TE_BUILD}/suites
    echo "TE_BS_SUITES=$TE_BS_SUITES" > ${TE_BUILD}/suites/Makefile.suites
fi    

AC_CONFIG_FILES(Makefile lib/Makefile 
                engine/Makefile tools/Makefile agents/Makefile suites/Makefile)

AC_OUTPUT

