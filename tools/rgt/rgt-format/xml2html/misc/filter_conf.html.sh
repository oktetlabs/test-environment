#!/bin/bash
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

cat "${1}" | sed 's,\@\@DATADIR\@\@,'"${2}"'/..,g;'
