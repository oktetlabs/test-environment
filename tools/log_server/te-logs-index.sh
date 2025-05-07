#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2007-2025 OKTET Labs Ltd. All rights reserved.
#
# CGI script to index log storage directories and provide fake entries
# for log files which may be generated from available raw logs.
#
# Uses the following CGI environment varialbes:
#   - REQUEST_URI
#   - CONTEXT_PREFIX
#   - CONTEXT_DOCUMENT_ROOT
#

#############################
# Print fake (to be generated) or real directory entry.
# Arguments:
#   Directory name
# Outputs:
#   HTML fragment to stdout.
#############################
function print_dir_entry()
{
    local entry="$1"
    local href="${entry}"

    if test ! -e "${entry}" ; then
        icon="/icons/link.gif"
    else
        icon="/icons/folder.gif"
    fi
    printf "<img src=\"%s\" alt=\"[DIR]\"> " "$icon"
    printf "<a href=\"./%s/\">%-32s %10s\n" "${href}" "${entry}/</a>" "&lt;DIR&gt;"
}

#############################
# Print fake (to be generated) or real file entry.
# Arguments:
#   File name
# Outputs:
#   HTML fragment to stdout.
#############################
function print_file_entry()
{
    local entry="$1"
    local href="${entry}"

    if test ! -e "${entry}" ; then
        icon="/icons/link.gif"
        size=""
    else
        size="$(ls -1hs "${entry}" | awk '{print $1}')"
        if file "${entry}" | grep -qw compressed ; then
            icon="/icons/compressed.gif"
        elif file "${entry}" | grep -qw text ; then
            icon="/icons/text.gif"
        else
            icon="/icons/unknown.gif"
        fi
    fi

    printf "<img src=\"%s\" alt=\"[   ]\"> " "$icon"
    printf "<a href=\"./%s\">%-32s %10s\n" "${href}" "${entry}</a>" "$size"
}

printf "Content-Type: text/html\r\n\r\n"

# Substitute spaces in URI
REQUEST_URI="${REQUEST_URI//%20/ }"
# Substitute colons in URI path (nothing else is expected)
REQUEST_URI="${REQUEST_URI//%3A/:}"

directory="${REQUEST_URI/#${CONTEXT_PREFIX}/${CONTEXT_DOCUMENT_ROOT}}"
cd "$directory"

echo "<html><head><title>Index of ${REQUEST_URI}</title></head><body>"

if test -r "HEADER.html" ; then
    cat "HEADER.html"
else
    echo "<h2>Index of ${REQUEST_URI}</h2>"
fi

echo "<pre><hr />"

print_dir_entry ".."
if [[ -r log.raw.bz2 || -r log.raw.xz || -r raw_log_bundle.tpxz ]] ; then
    if test ! -d html ; then
        print_dir_entry "html"
    fi
    if test ! -e log.txt ; then
        print_file_entry "log.txt"
    fi
    if [[ -r raw_log_bundle.tpxz ]] ; then
        if test ! -e log_gist.raw ; then
            print_file_entry "log_gist.raw"
        fi
        if test ! -e bublik.xml ; then
            print_file_entry "bublik.xml"
        fi
        if test ! -e bublik.json ; then
            print_file_entry "bublik.json"
        fi
    fi
fi
for i in * ; do
    test "$i" = "." -o "$i" = ".." -o "$i" = "HEADER.html" && continue
    i="${i/#.\//}"
    if test -d "$i" ; then
        print_dir_entry "$i"
    else
        print_file_entry "$i"
    fi
done

echo "<hr /></pre></body></html>"

exit 0
