#! /bin/sh
#
# Windows Test Agent
# Build standalone RPC server using native compiler
# Agruments: list of RPC server source files (pathnames).
# Pre-processor flags may be passed via RPCSERVER_CPPFLAGS 
# environment variable.
#
# Copyright (C) 2006 Test Environment authors (see file AUTHORS in
# the root directory of the distribution).
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
#
# Author: Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
#
# $Id:  $
#

if test -z ${TE_WIN32_BUILD_HOST} ; then
    exit 0 
fi  

cat >cl.m4 <<EOF
changequote([,])

define([INCLUDE],[#include <\$1>])
EOF

FILES= 
for i in $* ; do 
    FILENAME=`basename ${i}` 
    FILES="${FILES} ${FILENAME}"
    ${CC} ${RPCSERVER_CPPFLAGS} -P -E ${i} -o ${FILENAME}.tmp ; 
    cat cl.m4 ${FILENAME}.tmp | m4 > ${FILENAME} ; 
    rm ${FILENAME}.tmp ; 
done

LIBRARIES="ws2_32.lib mswsock.lib wsock32.lib iphlpapi.lib"

rm cl.m4

scp ${FILES} ${TE_WIN32_BUILD_HOST}:
rm ${FILES}
ssh ${TE_WIN32_BUILD_HOST} "\
export PATH=/usr/local/bin:/usr/bin:/bin:/usr/X11R6/bin:/cygdrive/c/WINDOWS/system32:/cygdrive/c/WINDOWS:/cygdrive/c/WINDOWS/System32/Wbem:/bin:/cygdrive/e/SDK/Bin:/cygdrive/e/P/msvs.net/Common7/IDE:/cygdrive/e/P/msvs.net/VC7/BIN:/cygdrive/e/P/msvs.net/Common7/Tools:/cygdrive/e/P/msvs.net/Common7/Tools/bin/prerelease:/cygdrive/e/P/msvs.net/Common7/Tools/bin:/cygdrive/e/P/msvs.net/SDK/v1.1/bin:/cygdrive/c/WINDOWS/Microsoft.NET/Framework/v1.1.4322 ;\
export LIB='E:\P\SDK\Lib;;E:\P\msvs.net\VC7\ATLMFC\LIB;E:\P\msvs.net\VC7\LIB;E:\P\msvs.net\VC7\PlatformSDK\lib\prerelease;E:\P\msvs.net\VC7\PlatformSDK\lib;E:\P\msvs.net\SDK\v1.1\lib;E:\P\msvs.net\SDK\v1.1\Lib' ; \
export INCLUDE='E:\P\SDK\Include;;E:\P\msvs.net\VC7\ATLMFC\INCLUDE;E:\P\msvs.net\VC7\INCLUDE;E:\P\msvs.net\VC7\PlatformSDK\include\prerelease;E:\P\msvs.net\VC7\PlatformSDK\include;E:\P\msvs.net\SDK\v1.1\include;E:\P\msvs.net\SDK\v1.1\include\' ; \
cl -o tawin32tmpl_rpcserver ${FILES} /link ${LIBRARIES}"
ssh ${TE_WIN32_BUILD_HOST} rm ${FILES} 
scp ${TE_WIN32_BUILD_HOST}:tawin32tmpl_rpcserver.exe .
ssh ${TE_WIN32_BUILD_HOST} rm -f tawin32tmpl_rpcserver.exe *.obj

exit 0
