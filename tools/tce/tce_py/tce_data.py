#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.

import sys
import os
import tempfile
import subprocess as sp
import tce_py.cksum as cksum

class _header(object):
    def __str__(self):
        return "FILE %s %d %d %s %s %d" % (self.obj, self.cksum, self.bytes,
            self.source, "", self.runs)

    @classmethod
    def parse(cls, s):
        l = s.split(" ")

        if len(l) == 7:
            del l[5]

        if (len(l) != 6) or (l[0] != "FILE"):
            raise ValueError()

        hdr = cls()

        hdr.obj = l[1]
        hdr.cksum = int(l[2])
        hdr.bytes = int(l[3])
        hdr.source = l[4]
        hdr.runs = int(l[5])

        return hdr

class tce_data(object):
    def __init__(self, conf):
        self.conf = conf
        self.hdr = None
        self.lines = []

    def _cksum(self, source):
        build = self.conf["plat_build"]
        p = os.path.join(build, source)

        return cksum.cksum_file(p)

    def set_header(self, s):
        hdr = _header.parse(s)

        c = self._cksum(hdr.source)

        if c != hdr.cksum:
            raise ValueError()

        self.hdr = hdr

    @property
    def source(self):
        return self.hdr.source

    @property
    def cksum(self):
        return self.hdr.cksum

    def set_source(self, path):
        c = self._cksum(path)

        if c != self.hdr.cksum:
            raise ValueError()

        self.hdr.source = path

    def add_line(self, s):
        self.lines.append(s)

    def write(self, fp):
        fp.write(str(self.hdr) + "\n")

        for l in self.lines:
            fp.write(l + "\n")

    def _write_temp(self):
        tcedir = self.conf["tcedir"]

        p1 = tempfile.NamedTemporaryFile(mode="w", dir=tcedir)

        self.write(p1)
        p1.flush()

        return p1

    def _reload(self, fp):
        self.hdr = None
        self.lines = []

        l = fp.readline()
        l = l.rstrip()
        self.set_header(l)

        for l in fp:
            l = l.rstrip()
            self.lines.append(l)

    def merge(self, tce):
        if self.cksum != tce.cksum:
            raise ValueError()

        p1 = self._write_temp()
        p2 = tce._write_temp()

        with sp.Popen(["tce_merge", "sum", p1.name, p2.name],
                      stdout=sp.PIPE, stderr=sp.DEVNULL, text=True) as p:
            self._reload(p.stdout)
            rc = p.wait()

        p1.close()
        p2.close()

        if rc:
            raise ValueError()
