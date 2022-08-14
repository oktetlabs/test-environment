#! /bin/sh
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.
#
# Script to generate set of TRC tags for the host.
#
#

SSH="ssh -qxTn -o BatchMode=yes"

#
# Generate usage to standard output and exit
#
usage() {
cat <<EOF
USAGE: trc-tags.sh [-d|-s|--help] [<hostname>]

If <hostname> is not specified, tags for local host are generated.

Options:
    -d          Generate set of options for TE Dispatcher
    -s          Generate set of options for TRC standalone tool
    --help      Display this help and exit

EOF
exit 1
}

# Process options
while test -n "$1" ; do
    case "$1" in
        --help) usage ;;
        -d) prefix="--trc-tag=" ;;
        -s) prefix="--tag=" ;;
        *)  host="$1" ;;
    esac
    shift 1
done

test -z "$host" && host="${TE_IUT}"
if test -n "${TE_IUT_KEY}" ; then
    SSH="${SSH} -i ${TE_IUT_KEY}"
elif test -n "${TE_SSH_KEY}" ; then
    SSH="${SSH} -i ${TE_SSH_KEY}"
fi

if test -z "$host" ; then
    ssh=""
    host="$(hostname -s)"
else
    ssh="${SSH} ${host}"
fi

# Kernel name without upper case letters
kname="$(${ssh} uname -s | tr '[A-Z]' '[a-z]')"
test -n "$kname" || exit 1
tags="${kname}:2"

# Kernel release
if test x"$kname" = x"sunos" ; then
    krel="$(${ssh} uname -rv | tr ' ' '-')"
else
    krel="$(${ssh} uname -r)"
fi

# Extract major, minor and patch numbers
krel_part=""
while test 1 ; do
    krel_part="$(echo $krel | \
                 sed -e "s/\(${krel_part}${krel_part:+[.-]}[^.-]*\).*/\1/")"
    tags="${kname}-${krel_part} ${tags}"
    test "$krel_part" = "$krel" && break
done

tags="${host} ${tags}"

# Red Hat Linux kernel detection
el="$(echo ${krel} | sed 's/.*\.\(el[0-9]\+\)\(\.\|_\|rt\|\>\).*/\1/')"
if test "x$(echo ${el} | grep '^el[0-9]\+$')" != "x" ; then
    el="$(echo ${el} | sed 's/^el/el:/')"
    tags="${tags} ${el}"
fi

if test $(${ssh} uname -a | grep -c "Ubuntu") -ne 0 ; then
    tags="${tags} ubu";
fi
if test "x$(echo ${tags} | grep powerpc64)" != "x" || \
   test "x$(echo ${tags} | grep ppc64)" != "x"; then
    tags="${tags} ppc64";
fi

for tag in ${tags} ; do
    result="${result} ${prefix}${tag}"
done

echo ${result}
