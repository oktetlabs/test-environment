#! /bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Script to generate html test log.
#
# Copyright (C) 2012-2022 OKTET Labs Ltd. All rights reserved.

HTML_OPTION=false EXEC_NAME=$0 `dirname \`which $0\``/log.sh --html "$@"
