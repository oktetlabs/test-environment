# /bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

. `dirname \`which $0\``/guess.sh

rgt-conv --no-cntrl-msg --mode=live -f ${TE_LOG_RAW}
