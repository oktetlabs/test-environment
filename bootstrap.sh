#!/bin/bash
#
# Clean up TE sources to build from scratch:
# 1. Delete all 'configure' scripts marked as ignored in Subversion.
#    It is sufficient to force dispatcher.sh to call aclocal, autoconf,
#    automake in all such directories.
#
# TE_BASE is required to be set. All actions are performed in this
# directory. The script uses Subversion to make sure that 'configure'
# to be removed in marked as ignored by user.
#
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
#
# $Id$
#

if test -d .svn; then
    for i in `find . -name configure -or -name \*.in -or \
                     -name aclocal.m4 -or -name autom4te.cache` ; do
        i_status=`/usr/bin/svn status $i` || \
        echo "Failed to set SVN status of $i"
        # Remove only files which are ignored by Subversion
        if test "x$i_status" = "xI${i_status/#I/}" ; then
            rm -rf $i
        fi
    done
elif test -d .hg; then
    python <<EOF
import os
import shutil
import fnmatch
from mercurial import ui, hg, commands

u = ui.ui()
r = hg.repository(u, '.')

# Execute command status and transform result to "list" of couples
def status(*args, **opts):
    u.pushbuffer()
    commands.status(u, r, *args, **opts)
    data = u.popbuffer()
    if not data:
        return
    for line in data.strip('\n').split('\n'):
        yield line.split(" ", 1)

# Remove ignored files with special names
for _, path in status(ignored=True, include=[
        'glob:**/configure',
        'glob:**/*.in',
        'glob:**/aclocal.m4']):
    os.remove(path)

# Remove directories with special name which contain only ignored files
for dname, _, _ in os.walk('.'):
    if os.path.basename(dname) == 'autom4te.cache':
        try:
            assert not any([t != 'I' for t, _ in status(dname, all=True)])
            shutil.rmtree(dname)
        except:
            pass
EOF
elif test -d .git; then
    # Remove files ignored by Git, excluding user's config files
    for i in $(git ls-files --others --ignored --exclude-standard) ; do
        test "$i" = ".reviewboardrc" || rm --force $i
    done
else
    echo "Unknown type of repository"
    exit 1
fi
