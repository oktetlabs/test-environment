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
# umask the unpack script runs with; a case may set it for its run
UNPACK_UMASK=022

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
    out="$(cd "${c}" && umask "${UNPACK_UMASK}" \
           && "${UNPACK}" -i incoming -s storage -B bad 2>&1)" \
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
pop_no_bundle() { rm raw_log_bundle.tpxz; }
# Entry names are matched as strings, not as patterns
pop_bypass_link() { ln -s /etc/passwd lnk; }
pop_bypass_option() {
    echo x >"--to-command=touch\$IFS'PWNED'|trc-stats.txt"
}
pop_empty() { :; }
pop_empty_append() { head -c 10240 /dev/zero >"$1"; }

run_case "no attachments (as before)" ok pop_none
run_case "unrelated top-level file" reject pop_unrelated
run_case "only metadata" reject pop_no_bundle
if ! grep -q 'missing mandatory: raw_log_bundle.tpxz$' <<<"${LAST_OUT}" ; then
    echo "FAIL: only metadata: missing log bundle not reported"
    failed=1
fi
run_case "pattern-like symlink name" reject pop_bypass_link \
    -P --transform='s,^lnk$,./attachments/node_id8/x|trc-stats.txt,'
run_case "option-like name" reject pop_bypass_option

pop_valid() {
    mkdir -p attachments/node_id8
    echo '[]' >attachments/node_id8/attachments.json
    echo console >attachments/node_id8/trex-console.txt
    chmod 4755 attachments/node_id8/trex-console.txt
}
pop_dir_only() { mkdir -p attachments/node_id8; }
pop_dotdot() {
    mkdir -p attachments/node_id8
    echo x >attachments/node_id8/ok.txt
    echo x >evil
}
pop_abs() { echo x >abs; }
pop_symlink() {
    mkdir -p attachments/node_id8
    ln -s /etc/passwd attachments/node_id8/link
}
pop_symlink_dir() { mkdir -p attachments; ln -s /tmp attachments/node_id8; }
pop_hardlink() {
    mkdir -p attachments/node_id8
    echo x >attachments/node_id8/a
    ln attachments/node_id8/a attachments/node_id8/b
}
pop_fifo() { mkdir -p attachments/node_id8; mkfifo attachments/node_id8/f; }
pop_badname() {
    mkdir -p attachments/node_id8
    echo x >"attachments/node_id8/bad name"
}
pop_badchar() {
    mkdir -p attachments/node_id8
    echo x >'attachments/node_id8/a$b'
}
pop_newline() {
    mkdir -p attachments/node_id8
    echo x >$'attachments/node_id8/a\nb'
}
pop_backslash() {
    mkdir -p attachments/node_id8
    echo x >'attachments/node_id8/a\b'
}
pop_longname() {
    mkdir -p attachments/node_id8
    echo x >"attachments/node_id8/$(printf 'a%.0s' $(seq 129))"
}
pop_badnode() { mkdir -p attachments/other; echo x >attachments/other/a; }
pop_nonum() { mkdir -p attachments/node_id; echo x >attachments/node_id/a; }
pop_toodeep() {
    mkdir -p attachments/node_id8/sub
    echo x >attachments/node_id8/sub/a
}
pop_toplevel_file() { mkdir -p attachments; echo x >attachments/a; }
pop_attach_file() { echo x >attachments; }
pop_top_symlink() { ln -s /etc/passwd trc-stats.txt; }
# Archived after meta_data.json, so stored as a hard link to it
pop_top_hardlink() { ln meta_data.json trc-stats.txt; }
pop_prefixed() {
    mkdir -p attachments/node_id8
    echo x >attachments/node_id8/a
}
pop_rooted() { mkdir -p attachments/node_id8; echo x >attachments/node_id8/a; }
pop_double_slash() { mkdir -p d; echo x >d/f; }
pop_samename() {
    mkdir -p attachments/node_id8
    echo x >attachments/node_id8/a
}
pop_samename_append() {
    rm attachments/node_id8/a
    ln -s /etc/passwd attachments/node_id8/a
    tar -rf "$1" attachments/node_id8/a
}
pop_odd_dir_mode() {
    mkdir -p attachments/node_id8
    echo x >attachments/node_id8/a
}
pop_private() {
    mkdir -p attachments/node_id8
    echo x >attachments/node_id8/a
}

# Check the modes under attachments/ of the case that passed last.
# Arguments:
#   case name
#   expected mode of files
#   expected mode of directories
check_attach_modes() {
    local name="$1"
    local fmode="$2"
    local dmode="$3"
    local s
    local bad

    [[ -n "${CASE_DIR}" ]] || return
    s="$(find "${CASE_DIR}/storage" -type d -name 'cfg-*')"
    bad="$(cd "${s}" && find attachments \( -type f ! -perm "${fmode}" \
               -o -type d ! -perm "${dmode}" \) -printf '%m %p\n')"
    if [[ -n "${bad}" ]] ; then
        echo "FAIL: ${name}: modes other than ${fmode}/${dmode}:"
        echo "${bad}" | sed 's/^/    /'
        failed=1
    fi
}

run_case "valid attachments" ok pop_valid
if [[ -n "${CASE_DIR}" ]] ; then
    s="$(find "${CASE_DIR}/storage" -type d -name 'cfg-*')"
    for f in attachments.json trex-console.txt ; do
        if [[ ! -f "${s}/attachments/node_id8/${f}" ]] ; then
            echo "FAIL: ${f} missing in storage"
            failed=1
        fi
    done
    if [[ -n "$(find "${s}/attachments" -perm /7000)" ]] ; then
        echo "FAIL: special permission bits kept"
        failed=1
    fi
    [[ -f "${s}/.done" ]] || { echo "FAIL: no done marker"; failed=1; }
fi
run_case "attachments dir only" ok pop_dir_only
run_case "directory with odd mode" ok pop_odd_dir_mode --mode=a-x
if [[ -n "${CASE_DIR}" ]] ; then
    s="$(find "${CASE_DIR}/storage" -type d -name 'cfg-*')"
    if [[ ! -x "${s}/attachments/node_id8" \
          || ! -f "${s}/attachments/node_id8/a" ]] ; then
        echo "FAIL: odd directory mode kept"
        failed=1
    fi
fi
run_case "files stored private" ok pop_private --mode=go-rwx
check_attach_modes "files stored private" 644 755
UNPACK_UMASK=027 run_case "restrictive umask" ok pop_private --mode=go-rwx
check_attach_modes "restrictive umask" 640 750
run_case "dotdot entry" reject pop_dotdot -P \
    --transform='s,^evil$,attachments/../evil,'
run_case "absolute entry" reject pop_abs -P --transform='s,^abs$,/abs/path,'
run_case "./attachments entry" reject pop_prefixed -P \
    --transform='s,^attachments/,./attachments/,'
run_case "/attachments entry" reject pop_rooted -P \
    --transform='s,^attachments/,/attachments/,'
run_case "doubled slash" reject pop_double_slash -P \
    --transform='s,^d/f$,attachments/node_id8//f,;s,^d$,attachments/node_id8,'
run_case "symlink file" reject pop_symlink
run_case "symlink dir" reject pop_symlink_dir
run_case "file then symlink of same name" reject pop_samename
run_case "hardlink" reject pop_hardlink
run_case "fifo" reject pop_fifo
run_case "space in name" reject pop_badname
run_case "bad char in name" reject pop_badchar
run_case "newline in name" reject pop_newline
run_case "backslash in name" reject pop_backslash
run_case "too long name" reject pop_longname
run_case "non node_id dir" reject pop_badnode
run_case "node_id without digits" reject pop_nonum
run_case "too deep" reject pop_toodeep
run_case "file directly in attachments" reject pop_toplevel_file
run_case "attachments is a file" reject pop_attach_file
run_case "allowed top-level name as symlink" reject pop_top_symlink
run_case "allowed top-level name as hardlink" reject pop_top_hardlink
run_case "empty tar" reject pop_empty
if ! grep -q 'missing mandatory' <<<"${LAST_OUT}" ; then
    echo "FAIL: empty tar: no missing mandatory message"
    failed=1
fi

exit "${failed}"
