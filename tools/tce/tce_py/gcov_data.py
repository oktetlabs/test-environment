#!/usr/bin/env python3

import os

class _line(object):
    T_INFO = 0
    T_TEXT = 1
    T_EXT = 2

class _line_info(_line):
    def __init__(self, l):
        self.kind = self.T_INFO
        self.ecnt = "-"
        self.num = 0
        self.tag = l[0]
        self.value = l[1]

    def __str__(self):
        val = "%s:%s" % (self.tag, self.value)
        return "%9s:%5d:%s" % ("-", 0, val)

    @classmethod
    def parse(cls, s):
        l = s.split(":")

        if len(l) != 2:
            raise ValueError()

        return _line_info(l)

class _line_text(_line):
    def __init__(self, l):
        self.kind = self.T_TEXT
        self.ecnt = l[0]
        self.num = int(l[1])
        self.text = l[2]

    def __str__(self):
        return "%9s:%6d:%s" % (self.ecnt, self.num, self.text)

    @classmethod
    def parse(cls, l):
        l[0] = l[0].strip()
        l[1] = l[1].strip()
        l[2] = l[2].strip()

        if (l[0] == "-") and (l[1] == "0"):
            return _line_info.parse(l[2])

        return _line_text(l)

class _line_raw(_line):
    def __init__(self, data):
        self.kind = self.T_EXT
        self.data = data

    def __str__(self):
        return self.data

class gcov_line(_line):
    @classmethod
    def parse(cls, s):
        l = s.split(":", maxsplit=2)

        if len(l) == 3:
            return _line_text.parse(l)

        return _line_raw(s)

class gcov_data(object):
    def __init__(self, path):
        self.path = path
        self.data = []

    def set_path(self, path):
        self.path == path

    def set_source(self, source):
        lno = None
        line = None

        for i, s in enumerate(self.data):
            l = gcov_line.parse(s)

            if l.kind != l.T_INFO:
                continue

            if l.tag == "Source":
                lno = i
                line = l
                break

        if lno is None:
            raise ValueError("invalid format: %s" % self.path)

        l.value = source
        self.data[lno] = str(line)

def read(conf, path):
    base = conf["tcedir"]
    cov = gcov_data(path)
    p = os.path.join(base, path)

    with open(p, "r") as fp:
        for l in fp:
            l = l.rstrip()
            cov.data.append(l)

    return cov

def write(conf, cov):
    base = conf["tcedir"]
    path = cov.path
    p = os.path.join(base, path)

    with open(p, "w") as fp:
        for l in cov.data:
            fp.write(l)
            fp.write("\n")

def remove(conf, cov):
    base = conf["tcedir"]
    path = cov.path
    p = os.path.join(base, path)

    if os.path.exists(p):
        os.remove(p)
