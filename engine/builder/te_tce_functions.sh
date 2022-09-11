# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
#
# Test Coverage Estimation
#
# Internal functions for TCE Processing
# Do not use it directly!

. "${TE_BUILD}/builder.conf.processed"
. "${SCRIPT_DIR}/te_functions"

TCE_WS="${TE_BUILD}/tce_workspace"

#######################################
# Die with a message.
# Arguments:
#   Arguments to message.
# Outputs:
#   Outputs arguments into STDERR.
# Returns:
#   This function never returns.
#######################################
function tce_die() {
    echo "ERROR: TCE: $*" >&2
    exit 1
}

#######################################
# Provide directory to process TCE.
# Globals:
#   TCE_WS
# Arguments:
#   None
# Returns:
#   0 if succeeds, non-zero on error.
#######################################
function tce_clean_workspace() {
    [[ -d "${TCE_WS}" ]] \
        && rm --interactive=never -r "${TCE_WS}" 2> /dev/null
    mkdir "${TCE_WS}" 2> /dev/null
}

#######################################
# Locate base directories for TA builds.
# Globals:
#   TE_BS_PLATFORMS
#   TE_BUILD
#   tce_build_base
# Arguments:
#   None
# Returns:
#   0 if succeeds, non-zero on error.
#######################################
function tce_set_build_base() {
    local type=
    local ws_file=
    local base=
    declare -gA tce_build_base

    for type in ${TE_BS_PLATFORMS} ; do
        ws_file="${TE_BUILD}/platforms/${type}/${type}.ws"
        base="$(cat "${ws_file}" 2> /dev/null)" || return 1
        tce_build_base["${type}"]="${base}"
    done
}

#######################################
# List TA components TCE should be processed.
# Globals:
#   TE_BS_TCE_${type}
#   TCE_LIST
# Arguments:
#   The TA type to list components for.
# Outputs:
#   TA components if any.
#######################################
function tce_list_components() {
    local type="$1"
    local list="${TCE_LIST}"
    local _list=

    if [[ "${list}" == "all" ]] ; then
        _list="TE_BS_TCE_${type}"
        list="${!_list}"
    fi
    echo "${list}"
}
