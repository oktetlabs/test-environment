#! /bin/sh

# Test Environment
#
# Script for starting of test suites
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
# Author Elena Vengerova <Elena.Vengerova@oktet.ru>
# 
# $Id: $
#

trap "exit 1" SIGINT

QUIET=no
case $1 in
    -q) QUIET=yes; shift 1 ;;
esac

# Get a sane screen width
[ -z "${COLUMNS:-}" ] && COLUMNS=80

[ -z "${CONSOLETYPE:-}" ] && [ -x /sbin/consoletype ] && \
    CONSOLETYPE="`/sbin/consoletype`"

if [ -f /etc/sysconfig/i18n -a -z "${NOLOCALE:-}" ] ; then
  . /etc/sysconfig/i18n
  if [ "$CONSOLETYPE" != "pty" ]; then
        case "${LANG:-}" in
                ja_JP*|ko_KR*|zh_CN*|zh_TW*)
                        export LC_MESSAGES=en_US
                        ;;
                *)
                        export LANG
                        ;;
        esac
  else
        export LANG
  fi
fi

RESULT_STYLE=color
RES_COL=70
MOVE_TO_COL="echo -en \\033[${RES_COL}G"
SETCOLOR_SUCCESS="echo -en \\033[0;32m"
SETCOLOR_FAILURE="echo -en \\033[1;31m"
SETCOLOR_WARNING="echo -en \\033[1;33m"
SETCOLOR_NORMAL="echo -en \\033[0;39m"

if [ "$CONSOLETYPE" = "serial" ]; then
    RESULT_STYLE=
    MOVE_TO_COL="echo -en \\t"
    SETCOLOR_SUCCESS=
    SETCOLOR_FAILURE=
    SETCOLOR_WARNING=
    SETCOLOR_NORMAL=
fi
  
echo_failure() {
  $MOVE_TO_COL
  [ "$RESULT_STYLE" = "color" ] && echo -n "[ "
  [ "$RESULT_STYLE" = "color" ] && $SETCOLOR_FAILURE
  echo -n $"FAILED"
  [ "$RESULT_STYLE" = "color" ] && $SETCOLOR_NORMAL
  [ "$RESULT_STYLE" = "color" ] && echo  -n " ]"
  echo -ne "\n"
  return 1
}

echo_passed() {
  $MOVE_TO_COL
  [ "$RESULT_STYLE" = "color" ] && echo -n "[ "
  [ "$RESULT_STYLE" = "color" ] && $SETCOLOR_SUCCESS
  echo -n $"PASSED"
  [ "$RESULT_STYLE" = "color" ] && $SETCOLOR_NORMAL
  [ "$RESULT_STYLE" = "color" ] && echo -n " ]"
  echo -ne "\n"
  return 1
}


# Process include
gcc -E -x c $1 > ${TE_TMP}/tester.conf 2>/dev/null


SUITES=
awk '
BEGIN { suite = 1; package = 1; suite_n=2 ; package_n=2 ; test_n = 1; } 
/#/   { next; }
/%suite/ \
{ 
    if (NF != 2)
    {
        printf("INCORRECT=YES\n");
        exit;
    }
    suite=suite_n;
    suite_n++;
    package=1;
    package_n = 2;
    test = 1;
    test_n = 2;
    printf("SUITES=\"${SUITES} %d\"\n",  suite) ; 
    printf("SUITEDIR_%d=%s\n", suite, $2); 
    printf("SUITE_%d_PACKAGES=\"${SUITE_%d_PACKAGES} %d\"\n", 
           suite, suite, package); 
    next;
}
/%package/ \
{ 
    if (NF != 2)
    {
        printf("INCORRECT=YES\n");
        exit;
    }
    package=package_n;
    package_n++;
    test = 1; 
    test_n = 2;
    printf("SUITE_%d_PACKAGES=\"${SUITE_%d_PACKAGES} %d\"\n", 
           suite, suite, package); 
    printf("PKGDIR_%d_%d=%s\n", suite, package, $2); 
    next;
}
{
    if (NF == 0)
        next;
    test=test_n;
    test_n++;
    printf("SUITE_%d_%d_TESTS=\"${SUITE_%d_%d_TESTS} %d\"\n",
           suite, package, suite, package, test);
    printf("TEST_%d_%d_%d_NAME=\"%s\"\n", suite, package, test, $1);
    printf("TEST_%d_%d_%d_PARMS=\"%s\"\n", suite, package, test, 
           gensub($1, " ", 1));
    next;
}' ${TE_TMP}/tester.conf > ${TE_TMP}/suites

rm ${TE_TMP}/tester.conf
. ${TE_TMP}/suites
rm ${TE_TMP}/suites

if test -n "${INCORRECT}" ; then
    echo Incorrect TESTER configuration file
    exit 1;
fi

# Valgrind options for MemCheck
VG_OPTIONS="--num-callers=8"
# Valgrind options for threads safety
#VG_OPTIONS="--tool=helgrind --num-callers=8"

passed=0
failed=0
for i in $SUITES ; do
    SUITEDIR=`eval echo '$SUITEDIR_'$i` ;        
    PACKAGES=`eval echo '$SUITE_'$i'_PACKAGES'` ;
    for j in ${PACKAGES} ; do
        PKGDIR=`eval echo '$PKGDIR_'$i'_'$j` ;
        TESTS=`eval echo '$SUITE_'$i'_'$j'_TESTS'` ; 
        for k in $TESTS ; do
            TEST_NAME=`eval echo '$TEST_'$i'_'$j'_'$k'_NAME'` ;
            TEST_PARMS=`eval echo '$TEST_'$i'_'$j'_'$k'_PARMS'` ;
            if [ "${QUIET}" == "no" ]; then
                echo -n "Starting test ${SUITEDIR}/${PKGDIR}/${TEST_NAME}"
            fi
            if test -n "$VG_TESTS" ; then
                valgrind $VG_OPTIONS \
                ${TE_INSTALL_SUITE}/${SUITEDIR}/${PKGDIR}/${TEST_NAME} \
                ${TEST_PARMS} 2>vg.test.${SUITEDIR}.${PKGDIR}.${TEST_NAME}
            else
                ${TE_INSTALL_SUITE}/${SUITEDIR}/${PKGDIR}/${TEST_NAME} \
                ${TEST_PARMS}
            fi
            if test $? -ne 0 ; then
                let failed++ ;
                result=failed ;
            else
                let passed++ ;
                result=passed ;
            fi
            te_log_message Engine Tester \
                "Test ${SUITEDIR}/${PKGDIR}/${TEST_NAME} ${result}"
            if [ "${QUIET}" == "no" ]; then
                if [ "${result}" = "passed" ]; then
                    echo_passed
               else
                    echo_failure
               fi
            fi
        done
    done
done
te_log_message Engine Tester "PASSED $passed, FAILED $failed"
echo "PASSED $passed, FAILED $failed"
