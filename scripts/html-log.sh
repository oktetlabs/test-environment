#! /bin/bash
#
# Script to generate html test log.
#
# Copyright (C) 2012-2022 OKTET Labs, St.-Petersburg, Russia
#
#

HTML_OPTION=false EXEC_NAME=$0 `dirname \`which $0\``/log.sh --html "$@"
