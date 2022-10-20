#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.

import os
import tce_py.sum_stat as sum_stat

class susum(object):
    def add(self, name, fname, stat):
        pass

class _reader(object):
    def __init__(self, base):
        self.base = base
        self.susum = susum()

    def read(self, name, sumfile):
        p = os.path.join(self.base, sumfile)
        stat = sum_stat.read(p)
        self.susum.add(name, sumfile, stat)

def read(conf):
    base = conf["base"]
    sums = conf["sums"]

    reader = _reader(base)

    for s in sums:
        reader.read(s["name"], s["file"])

    return reader.susum
