#!/bin/bash
# 
# Test Environment
#
# Test Suite Compiler (light version, for OKTET Labs users only). 
#
# Usage: 
#
#    tsc <filename>
#
# Script should be started in the test directory where configure.ac
# should be placed. 
#
# If configure.ac is already there, it is updated. Otherwise it is created. 
# If mainpage.dox is created or updated. Doxyfile is created or is not touched.
#
# <filename> is name of file with test suite description. It should look like:
# 
# <Author name> <Author family name> 
#
# AM_CPPFLAGS=<value>
# AM_LDFLAGS=<value>
# MYLDADD=<value>
# MYDEPS=<value>
#
# dir D1/.../Dn Package description
# test_name     Test description
# test_name     Test description
# ...
# dir D1/.../Dn Package description
# test_name     Test description
# ...
#
# Directories are created if they are not exist. 
# Makefile.am, package.dox and package.xml are created if they are not exist. 
# Otherwise they are updated. 
# Files for new tests are created if they are not exist.
#
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
# Author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
# 
# $Id: $
#

get_dir()
{
    CURR_DIR=`pwd`
    DIR=${CURR_DIR#${START_DIR}}
    DIR=${DIR#/}
    if test -n "${DIR}" ; then 
        DIR=${DIR}"/"
    fi
}

get_redline()
{
    if test -z "${GET_REDLINE}" ; then
        svn co https://svn.oktetlabs.ru/svnroot/oktetlabs/qms/trunk/redline/ \
               /tmp/redline || echo exit 1
        GET_REDLINE=yes
    fi        
}

create_package_dox()
{
    if test ! -e package.dox ; then
        get_redline
        CURR_DIR=
        echo Creating ${DIR}package.dox
        cp /tmp/redline/OKTL-TMPL-0000054-TS_package.dox package.dox
        cat package.dox | awk --assign descr="$1" \
                              --assign package=`basename $DIR` \
                              --assign author_name=$AUTHOR_NAME \
                              --assign author_fname=${AUTHOR_FNAME} \
        '/@page/ { printf("/** @page %s %s\n", package, descr); next ; } \
         /@ref/ { next; }
         /@author/ { printf("@author %s %s <%s.%s@oktetlabs.ru>\n", \
                            author_name, author_fname, \
                            athor_name, author_fname); next; } \
         { print $0 ; }' > tmp
         mv tmp package.dox
    fi
}

create_package_xml()
{
    if test ! -e package.xml ; then
        echo Creating ${DIR}package.xml

        cat > package.xml << EOF
<?xml version="1.0"?>
<package version="1.0">
    
    <description>$1</description>
    
    <author mailto="${AUTHOR_NAME}.${AUTHOR_FNAME}@oktetlabs.ru"/>

    <session>

    </session>
</package>
EOF

    fi
}

create_makefile_am()
{
    if test ! -e Makefile.am ; then
        echo Creating ${DIR}Makefile.am
        cat > Makefile.am << EOF
SUBDIRS =

EXTRA_DIST = package.dox

tetestdir=\$(prefix)/@PACKAGE_NAME@/\$(subdir)

tetest_DATA = tests-info.xml

dist_tetest_DATA = package.xml

tests-info.xml: \$(srcdir)/*.c
	@TE_PATH@/te_tests_info.sh \$(srcdir) > tests-info.xml

CLEANFILES = tests-info.xml

tetest_PROGRAMS =
EOF
 
        echo >> Makefile.am
        cat ${START_DIR}/Makefile.am.inc >> Makefile.am
    fi
}

if test -z "$1" ; then
    echo Test Suite description is not provided
    exit 1
fi    

START_DIR=`pwd`
MAIN_SUITE_NAME=`basename $START_DIR`
AUTHOR_NAME=`finger $USER | awk '{ print $4 ; nextfile ; }'`
AUTHOR_FNAME=`finger $USER | awk '{ print $5 ; nextfile ; }'`

# Extract user-specific part of Makefile.am from the TS description
cat $1 | awk '/^dir/ { nextfile; } { print $0 ; }' > Makefile.am.inc


# Create root files, if necessary
if test ! -e mainpage.dox ; then
    get_redline
    echo Create mainpage.dox
    cp /tmp/redline/OKTL-TMPL-0000053-TS_mainpage.dox mainpage.dox
fi

if test ! -e Doxyfile ; then
    get_redline
    echo Creating Doxyfile
    cp /tmp/redline/OKTL-TMPL-0000052-TS_Doxyfile Doxyfile
fi

if test ! -e configure.ac ; then

svn co https://svn.oktetlabs.ru/svnroot/prj/oktetlabs/test-environment/src/trunk/auxdir
cat > configure.ac << EOF
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT([${MAIN_SUITE_NAME}], 1.4.0, ${AUTHOR_NAME}.${AUTHOR_FNAME}@oktetlabs.ru)

AC_CONFIG_SRCDIR([mainpage.dox])

AC_CONFIG_AUX_DIR([auxdir])

AM_INIT_AUTOMAKE([1.8.5 -Wall foreign])

AC_CANONICAL_HOST
AC_SUBST([host])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_INSTALL])
AC_SUBST([TE_PATH])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_INSTALL])
AC_ARG_VAR([TE_PATH])

AC_CONFIG_HEADERS([package.h])

AC_CONFIG_FILES([\\
Makefile \\
])

AC_OUTPUT
EOF
fi    

create_package_xml "Test Suite ${MAIN_SUITE_NAME}"

create_makefile_am

update_package_xml()
{
    NAME=`basename $1`
    FILE=`dirname $1`/package.xml
    echo Adding $2 $NAME to $FILE
    
    cat $FILE | awk ' \
    /<\/session>/ { if (s != "") { print s ; } s = $0 ; next; } \
    /<\/package>/ { nextfile ; } \
    {  \
        if ($1 == "") { print $0 ; next ; } \
        if (s != "") { print s ; printf("\n"); } s = "" ; \
        print $0 ; \
    }' >tmp
    mv tmp $FILE
    cat >> $FILE << EOF
        <run>
            <$2 name="$NAME"/>
        <run>
             
    </session>
</package>
EOF
}

update_package_dox()
{
    NAME=`basename $1`
    DIRNAME=`dirname $1`

    
    if test ${DIRNAME} = "." ; then
        echo Adding $NAME reference to ${START_DIR}/mainpage.dox
        cat ${START_DIR}/mainpage.dox | grep -v '*/' >tmp
        echo '- @ref '$NAME >>tmp
        echo '*/' >>tmp
        mv tmp ${START_DIR}/mainpage.dox
    else        
        echo Adding $NAME reference to ${START_DIR}/package.dox
        cat ${DIRNAME}/package.dox | grep -v '*/' >tmp
        echo '-# @ref '$NAME >>tmp
        echo '*/' >>tmp
        mv tmp ${DIRNAME}/package.dox
    fi
}

update_makefile_am_subdir()
{
    FILE=`dirname ${DIR}`/Makefile.am
    SUBDIR=`basename ${DIR}`
    echo Adding subdirectory $SUBDIR to ${FILE}
    cat ${FILE} | awk --assign name=${SUBDIR} '\
    /^SUBDIRS/ { printf("%s %s\n", $0, name); next; } \
    { print $0 ; }' > tmp
    mv tmp ${FILE}
}

update_makefile_am_test()
{
    echo Adding test $1 to ${DIR}Makefile.am
    cat ${DIR}Makefile.am | awk --assign name=$1 '\
    /^tetest_PROGRAMS/ { printf("%s %s\n", $0, name); next; } \
    { print $0 ; next ; }' > tmp

    echo >> tmp
    echo $1_SOURCES = $1.c >> tmp
    echo $1_LDADD = '$(MYLDADD)' >> tmp
    echo $1_DEPENDENCIES = '$(MYDEPS)' >> tmp
    mv tmp ${DIR}Makefile.am
}

create_test()
{
    if test ! -e ${TEST_NAME}.c ; then
        echo Creating ${DIR}${TEST_NAME}.c
        SUITE_NAME=`basename ${DIR}`
        
        cat > ${DIR}${TEST_NAME}.c << EOF
/* 
 * ${TEST_NAME}
 * ${TEST_DESCR}
 *
 * Copyright (C) 2005 OKTET Labs, St.-Petersburg, Russia
 * 
 * \$Id: $
 */

/** @page ${TEST_NAME}  ${TEST_DESCR}
 *
 * @objective 
 *
 * @param p             description
 *
 * @pre 
 *
 * @par Scenario
 * -# Step
 *
 * @author ${AUTHOR_NAME} ${AUTHOR_FNAME} <${AUTHOR_NAME}.${AUTHOR_FNAME}@oktetlabs.ru>
 */

#define TE_TEST_NAME  "${DIR}${TEST_NAME}"

int
main(int argc, char *argv[])
{
    return 0;
}
EOF

    fi
}

process_line()
{
    if test "$1" = "dir" ; then
        shift 1
        DIR=$1/ ;
        mkdir -p ${START_DIR}/$1
        cd ${START_DIR}/$1 ;
        shift 1
        create_package_xml "$*"
        create_package_dox "$*"
        create_makefile_am
        cd ${START_DIR}
        update_makefile_am_subdir
        update_package_dox ${DIR}
        update_package_xml ${DIR} package
        return
    fi
    if test -z "$DIR" ; then
        return;
    fi
    if test -z "$1" ; then
        return;
    fi
    update_makefile_am_test $1
    update_package_dox ${DIR}$1
    update_package_xml ${DIR}$1 test
    TEST_NAME=$1
    shift 1
    TEST_DESCR="$*"
    create_test 
}

# Go via directories creating necessary files

DIR=
cat $1 | while read msg ; do process_line $msg ; done

rm -f Makefile.am.inc
