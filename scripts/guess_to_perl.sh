#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

. `dirname \`which $0\``/guess.sh >/dev/null

echo "\$sh_path=\"$PATH\";"
echo "\$sh_ld_library_path=\"$LD_LIBRARY_PATH\";"

exit 0
