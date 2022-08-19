#!/usb/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.

import os
import subprocess as sp

def _chksum(path):
    with sp.Popen(["cksum", path], stdout=sp.PIPE, stderr=sp.DEVNULL) as p:
        l = p.stdout.read()
        rc = p.wait()

    if rc != 0:
        raise ValueError()

    l = l.split(maxsplit=1)

    return int(l[0])

def cksum_file(path):
    try:
        val = _chksum(path)
    except Exception as e:
        raise ValueError("fail to locate %s" % path)

    return val
