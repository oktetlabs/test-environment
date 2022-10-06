#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.

import os

class formatter(object):
    _THE = """\
<html>
<head><title>TCE Super Summary</title></head>
<body>
<table border="1">
    <th>Component</th>
    <th>Type</th>
    <th>Name</th>
    <th>Files</th>
    <th>Lines Executed</th>
    <th>Branches executed</th>
"""
    _FIL = 3
    _LIN = 4
    _BRA = 5
    _FOO = """\
</table>
</body>
</html>
"""
    _ROW = """\
    <tr>
        <td>%s</td>
        <td>%s</td>
        <td>%s</td>
        <td>%s</td>
        <td>%s</td>
        <td>%s</td>
"""

    @classmethod
    def header(cls):
        return cls._THE

    @classmethod
    def footer(cls):
        return cls._FOO

    @classmethod
    def _cnt(cls, ce, cn):
        ce = ce * 100.0 / cn if cn else 100.0
        return "%.2f%% of %d" % (ce, cn)

    @classmethod
    def _row(cls, level, name, fname=None, stat=None):
        ss = ["", "", "", "", "", ""]

        if fname:
            name = "<a href=\"%s\">%s</a>" % (fname, name)
        ss[level] = name

        if stat:
            ss[cls._FIL] = "%d" % stat.fn
            ss[cls._LIN] = cls._cnt(stat.le, stat.ln)
            ss[cls._BRA] = cls._cnt(stat.be, stat.bn)

        return cls._ROW % tuple(ss)

    @classmethod
    def comp(cls, name, fname=None, stat=None):
        return cls._row(0, name, fname, stat)

    @classmethod
    def type(cls, name, fname=None, stat=None):
        return cls._row(1, name, fname, stat)

    @classmethod
    def name(cls, name, fname, stat):
        return cls._row(2, name, fname, stat)

def _write(susum, fp):
    fmt = formatter

    fp.write(fmt.header())

    for c in susum:
        fp.write(fmt.comp(c.name))

        for t in c:
            fp.write(fmt.type(t.name))

            for n in t:
                fp.write(fmt.name(n.name, n.fname, n.stat))

            if t.fname:
                fp.write(fmt.name("Total", t.fname, t.stat))

        if c.fname:
            fp.write(fmt.type("Total", c.fname, c.stat))

    fp.write(fmt.footer())

def write(conf, susum, fname):
    base = conf["base"]
    p = os.path.join(base, fname)

    with open(p, "w") as fp:
        _write(susum, fp)
