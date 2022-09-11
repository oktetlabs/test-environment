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
