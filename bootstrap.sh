#!/bin/bash
#
# Clean up TE sources to build from scratch.
#
# TE_BASE is required to be set. All actions are performed in this
# directory.
#
# Copyright (C) 2004-2018 OKTET Labs.
#
# @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
#
#

if test -d .git; then
    # Remove files ignored by Git, excluding user's config files
    for i in $(git ls-files --others --ignored --exclude-standard) ; do
        [[ "$i" = ".reviewboardrc" ]] ||
        [[ "$i" = *".vscode/"* ]] ||
        rm --force $i
    done
else
    echo "Unknown type of repository"
    exit 1
fi
