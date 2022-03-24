#!/usr/bin/env python3

import sys
import os
import json
import tce_py.tce_report as report

def _read_conf(path):
    with open(path, "r") as fp:
        conf = json.load(fp)

    p = os.path.abspath(path)
    conf["tcedir"] = os.path.dirname(p)

    return conf

def main(argv):
    if len(argv) != 5:
        raise ValueError("invalid cmdline")

    mconf = _read_conf(argv[1])
    mrep = argv[2]
    conf = _read_conf(argv[3])
    rep = argv[4]

    mr = report.read(mconf, mrep)
    r = report.read(conf, rep)

    report.merge(mr, r)

    report.write(mr, mrep)

    return 0

if __name__ == "__main__":
    argv = sys.argv
    rc = 1

    try:
        rc = main(argv)
    except Exception as e:
        sys.stderr.write("%s: fail to resove gcov: %s\n" % (argv[0], e))

    sys.exit(rc)
