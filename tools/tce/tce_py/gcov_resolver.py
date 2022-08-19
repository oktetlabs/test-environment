#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.

import sys
import os
import tce_py.gcov_info as gcov_info
import tce_py.gcov_data as gcov_data

class resolver(object):
    def __init__(self, conf):
        self.conf = conf
        self.covs = {}
        self.unres = set()

    def add(self, path):
        info = gcov_info.read(self.conf, path)
        gcov_info.resolve(self.conf, info)

        self.covs[info.path] = info

    def _remove(self, path):
        base = self.conf["tcedir"]
        p = os.path.join(base, path)

        os.remove(p)

    def update(self):
        for path in self.covs:
            info = self.covs[path]

            if info.kind == info.T_UNRES:
                self.unres.add(info.source)
                self._remove(info.path)
                continue

            if info.kind == info.T_BUILD:
                continue

            cov = gcov_data.read(self.conf, path)
            cov.set_source(info.source)
            gcov_data.write(self.conf, cov)

    def write_unres(self, fp):
        unres = list(self.unres)
        unres.sort()

        for s in unres:
            fp.write(s)
            fp.write("\n")

def resolve(conf):
    covs = resolver(conf)
    path = conf["tcedir"]

    for root, dirs, files in os.walk(path):
        for f in files:
            if not f.endswith(".gcov"):
                continue

            p = os.path.join(root, f)
            p = os.path.relpath(p, path)
            covs.add(p)

    covs.update()

    if "unres" in conf:
        p = os.path.join(path, conf["unres"])

        with open(p, "w") as fp:
            covs.write_unres(fp)
