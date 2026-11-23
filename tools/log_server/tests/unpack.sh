#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Labs Ltd. All rights reserved.
#
# Test that publish-logs-unpack.sh publishes valid tars and rejects
# unsafe ones. Works only inside a temporary directory, does not use
# the network and does not request Bublik import.

UNPACK="$(cd "$(dirname "$0")/.." && pwd)/publish-logs-unpack.sh"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT

failed=0
n=0

# Make a base tar content in $1 with a unique start time suffix $2.
mk_base() {
    local dir="$1"
    local sec="$2"

    mkdir -p "${dir}"
    cat >"${dir}/meta_data.json" <<JSON
{"version": 1, "metas": [
  {"name": "CAMPAIGN_DATE", "value": "2026-11-23"},
  {"name": "TS_NAME", "value": "ts"},
  {"name": "CFG", "value": "cfg"},
  {"name": "START_TIMESTAMP", "value": "2026-11-23T10:00:${sec}+00:00"}
]}
JSON
    echo dummy >"${dir}/raw_log_bundle.tpxz"
}

# Run one case.
# Arguments:
#   case name
#   expected result: ok or reject
#   function that populates the content directory (argument: dir)
#   extra tar options (optional, e.g. -P)
run_case() {
    local name="$1"
    local expect="$2"
    local populate="$3"
    shift 3
    local c="${WORK}/case${n}"
    local sec
    local out
    local rc=0

    # Set only by an ok case that passes, for checks after it
    CASE_DIR=

    sec="$(printf '%02d' "${n}")"
    n=$((n + 1))
    mkdir -p "${c}/incoming" "${c}/storage" "${c}/bad" "${c}/content"
    mk_base "${c}/content" "${sec}"
    ( cd "${c}/content" && ${populate} ) \
        || { echo "SETUP FAIL: ${name}"; failed=1; return; }
    ( cd "${c}/content" \
      && find . -mindepth 1 -maxdepth 1 -printf '%P\0' | sort -z \
      | tar "$@" -cf "${c}/incoming/run.tar" --owner=0 --group=0 \
            --null -T - ) \
        || { echo "SETUP FAIL: ${name}"; failed=1; return; }

    # Optional hook may replace the tar (arguments: tar path)
    if declare -F "${populate}_append" >/dev/null ; then
        ( cd "${c}/content" && "${populate}_append" "${c}/incoming/run.tar" ) \
            || { echo "SETUP FAIL: ${name}"; failed=1; return; }
    fi
    touch -d '10 minutes ago' "${c}/incoming/run.tar"

    # Run inside the case directory, so that any stray file created by
    # a command run from the archive stays there.
    out="$(cd "${c}" && "${UNPACK}" -i incoming -s storage -B bad 2>&1)" \
        || rc=$?
    LAST_OUT="${out}"

    if [[ -n "$(find "${c}" -name PWNED -print -quit)" ]] ; then
        echo "FAIL: ${name} (a command from the archive was run)"
        failed=1
        return
    fi

    local published
    published="$(find "${c}/storage" -type f | wc -l)"
    if [[ "${expect}" == ok ]] ; then
        if [[ "${rc}" -ne 0 || "${published}" -eq 0 \
              || -e "${c}/bad/run.tar" || -e "${c}/incoming/run.tar" ]] ; then
            echo "FAIL: ${name} (rc=${rc}, published=${published})"
            echo "${out}" | sed 's/^/    /'
            failed=1
            return
        fi
        if grep -q '^tar:' <<<"${out}" ; then
            echo "FAIL: ${name} (tar complained)"
            echo "${out}" | sed 's/^/    /'
            failed=1
            return
        fi
        CASE_DIR="${c}"
    else
        if [[ "${published}" -ne 0 || ! -e "${c}/bad/run.tar" \
              || -e "${c}/incoming/run.tar" ]] ; then
            echo "FAIL: ${name} (not rejected, published=${published})"
            echo "${out}" | sed 's/^/    /'
            failed=1
            return
        fi
        # No directory may be created in storage either
        if [[ -n "$(find "${c}/storage" -mindepth 1 -print -quit)" ]] ; then
            echo "FAIL: ${name} (storage not empty)"
            failed=1
            return
        fi
    fi
    echo "PASS: ${name}"
}

pop_none() { :; }
pop_unrelated() { echo x >extra.txt; }
# Entry names are matched as strings, not as patterns
pop_bypass_link() { ln -s /etc/passwd lnk; }
pop_bypass_option() {
    echo x >"--to-command=touch\$IFS'PWNED'|trc-stats.txt"
}

run_case "no attachments (as before)" ok pop_none
run_case "unrelated top-level file" reject pop_unrelated
run_case "pattern-like symlink name" reject pop_bypass_link \
    -P --transform='s,^lnk$,./attachments/node_id8/x|trc-stats.txt,'
run_case "option-like name" reject pop_bypass_option

exit "${failed}"
