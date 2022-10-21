# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
#
# Test Coverage Estimation
#
# Internal functions for TCE Processing
# Do not use it directly!

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
