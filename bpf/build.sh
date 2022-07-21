#!/bin/bash
#
# This script can be used to build some or all of the programs.
# It can be used inside TE_TA_APP macro in build.conf, see README.md.
#

declare -a progs
declare -a opts

all_progs=(tc_delay.c tc_drop.c tc_dup.c)

function usage() {
    cat <<EOF
Usage: build.sh [<options>]
  --progs=<list of names>       Which BPF programs to build
                                (comma-separated list of file names
                                 without extensions)

Other options are passed to te_build_bpf script.
EOF
}


function process_opts() {
    while [[ "$#" -gt 0 ]] ; do
        case "$1" in
            "") ;; # Ignore empty arguments

            -h | --help) usage ; exit 0 ;;

            --progs=*)
                progs_list="${1#--progs=}"
                for prog in ${progs_list//,/ } ; do
                    progs+=("${prog}.c")
                done
                ;;

            *)
                opts+=("$1")
                ;;
        esac
        shift 1
    done

    if [[ ${#progs[@]} -eq 0 ]] ; then
        progs=("${all_progs[@]}")
    fi
}

function main() {
    process_opts "$@"

    te_build_bpf "${opts[@]}" "${progs[@]}"
}

main "$@"
