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

SRCDIR=$1

shift 1

# Build SUN RPC library
cp -r ${SRCDIR}/lib/rpc .
tar czf rpc.tgz rpc
scp rpc.tgz ${TE_WIN32_BUILD_HOST}:
rm -rf rpc rpc.tgz
ssh ${TE_WIN32_BUILD_HOST} bash -l -c "\"
tar xzf rpc.tgz ; \
cd rpc ; \
make ; \
mv librpc.lib .. ; \
cd .. ; \
rm -rf rpc rpc.tgz ;\""
rm -f librpc.lib
scp ${TE_WIN32_BUILD_HOST}:librpc.lib .
if ! test -e librpc.lib ; then
    exit 1 ;
fi
rm librpc.lib


# Apply pre-processor

cat >cl.m4 <<EOF
changequote($$,$$)

define($$INCLUDE$$,$$#include <\$1>$$)
EOF

FILES= 
for i in $* ; do 
    FILENAME=`basename ${i}` 
    FILES="${FILES} ${FILENAME}"
    ${CC} ${RPCSERVER_CPPFLAGS} -P -E ${i} -o ${FILENAME}.tmp ; 
    cat cl.m4 ${FILENAME}.tmp | m4 > ${FILENAME} ; 
    rm ${FILENAME}.tmp ; 
    indent ${FILENAME} ;
done
rm -f *~ cl.m4

# Build the result

LIBRARIES="ws2_32.lib mswsock.lib wsock32.lib iphlpapi.lib"

scp ${FILES} ${TE_WIN32_BUILD_HOST}: 
rm ${FILES}
ssh ${TE_WIN32_BUILD_HOST} bash -l -c "\"
cl -o tawin32tmpl_rpcserver.exe ${FILES} /link librpc.lib ${LIBRARIES}\""
ssh ${TE_WIN32_BUILD_HOST} rm ${FILES} 
scp ${TE_WIN32_BUILD_HOST}:tawin32tmpl_rpcserver.exe .
ssh ${TE_WIN32_BUILD_HOST} rm -f tawin32tmpl_rpcserver.exe *.obj librpc.lib

exit 0
