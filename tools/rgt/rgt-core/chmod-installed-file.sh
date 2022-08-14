#!/bin/bash
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

# Script is intended to perform chmod with given option on
# given file located in the install directory

cd "${MESON_INSTALL_DESTDIR_PREFIX}"
chmod "${1}" "${2}"
