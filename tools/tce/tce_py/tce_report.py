#!/usr/bin/env python3
# Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.

import sys
import os
import tce_py.cksum as cksum
import tce_py.tce_data as tce_data

class report(object):
    def __init__(self, conf):
        self.conf = conf
        self.tce_list = {}

    def get_list(self):
        tce_list = list(self.tce_list.values())

        tce_list.sort(key=lambda item : item.source)

        return tce_list

    def add(self, tce):
        s = tce.source

        if s not in self.tce_list:
            self.tce_list[s] = tce
            return

        prev = self.tce_list[s]
        prev.merge(tce)

    def merge(self, rep, tce):
        rep_build = rep.conf["plat_build"]
        build = self.conf["plat_build"]
        s = tce.source

        if s not in self.tce_list:
            p = os.path.join(build, s)
            c = cksum.cksum_file(p)
        else:
            prev = self.tce_list[s]
            c = prev.cksum

        if tce.cksum != c:
            p = os.path.join(rep_build, s)
            p = os.path.relpath(p, build)
            tce.set_source(p)

        self.add(tce)

def _read_list(conf, fp):
    tce_list = []
    tce = None

    for l in fp:
        l = l.rstrip()

        if l.startswith("FILE "):
            if tce:
                tce_list.append(tce)

            tce = tce_data.tce_data(conf)
            tce.set_header(l)
            continue

        if tce:
            tce.add_line(l)
            continue

        raise ValueError("invalid format: %s" % self.path)

    if tce:
        tce_list.append(tce)

    return tce_list

def read(conf, path):
    with open(path, "r") as fp:
        tce_list = _read_list(conf, fp)

    rep = report(conf)

    for tce in tce_list:
        rep.add(tce)

    return rep

def write(rep, path):
    tce_list = rep.get_list()

    with open(path, "w") as fp:
        for tce in tce_list:
            tce.write(fp)

def merge(mrep, rep):
    for tce in rep.tce_list.values():
        mrep.merge(rep, tce)
