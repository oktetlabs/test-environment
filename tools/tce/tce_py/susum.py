#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.

import os
import tce_py.sum_stat as sum_stat

class _sunit(object):
    def __init__(self, sunod):
        cc = list(sunod._child.values())
        cc.sort(key = lambda x : x.name)
        self._child = cc

    def __next__(self):
        if not len(self._child):
            raise StopIteration()

        n = self._child[0]
        del self._child[0]
        return n

class _sunod(object):
    def __init__(self, name = ""):
        self._name = name
        self._value = None
        self._child = dict()

    def add(self, c):
        if c not in self._child:
            self._child[c] = _sunod(c)
        return self._child[c]

    def set(self, fname, stat):
        self._value = (fname, stat)

    def __iter__(self):
        return _sunit(self)

    @property
    def name(self):
        return self._name

    @property
    def fname(self):
        return self._value[0] if self._value else None

    @property
    def stat(self):
        return self._value[1] if self._value else None

class susum(object):
    def __init__(self):
        self._sunod = _sunod()

    def add(self, name, fname, stat):
        cc = name.split("/")

        n = self._sunod

        for c in cc:
            n = n.add(c)

        n.set(fname, stat)

    def __iter__(self):
        return _sunit(self._sunod)


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
