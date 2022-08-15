#!/usr/bin/env python3
# Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.

import sys
import os
import tce_py.gcov_data as gcov_data

class gcov_info(object):
    T_UNRES = 0
    T_BUILD = 1
    T_INST = 2

    def __init__(self, path):
        self.path = path
        self.source = None
        self.kind = self.T_UNRES

class _reader(object):
    def __init__(self, conf):
        self.conf = conf

    def read(self, path):
        info = gcov_info(path)

        b = self.conf["tcedir"]
        p = os.path.join(b, path)

        with open(p, "r") as fp:
            for l in fp:
                line = gcov_data.gcov_line.parse(l)

                if line.kind != line.T_INFO:
                    break

                if line.tag == "Source":
                    info.source = line.value
                    break

        if not info.source:
            raise ValueError("invalid format: %s" % info.path)

        return info

def read(conf, path):
    rd = _reader(conf)

    return rd.read(path)

class _resolver(object):
    def __init__(self, conf):
        self.conf = conf

    def _resolve_abs(self, info):
        build = self.conf["build"]
        inst = self.conf["inst"]
        hinst = self.conf["host_inst"]
        s = info.source

        hinst = os.path.normpath(hinst)

        if not s.startswith(hinst):
            return

        s = s[len(hinst):]
        s = inst + s

        info.kind = info.T_INST
        info.source = s

    def resolve(self, info):
        pbuild = self.conf["plat_build"]
        s = info.source

        if os.path.isabs(s):
            self._resolve_abs(info)
        else:
            info.kind = info.T_BUILD

        if info.kind == info.T_UNRES:
            return

        s = info.source
        p = os.path.join(pbuild, s)

        if not os.path.isfile(p):
            raise ValueError("fail to resolve %s" % info.path)

def resolve(conf, info):
    rs = _resolver(conf)

    rs.resolve(info)
