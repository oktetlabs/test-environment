#!/usr/bin/env python3

import sys
import os
import json
import tce_py.gcov_resolver as resolver

def main(argv):
    if len(argv) != 2:
        raise ValueError("invalid cmdline")

    path = os.path.abspath(argv[1])

    with open(path, "r") as fp:
        conf = json.load(fp)

    conf["tcedir"] = os.path.dirname(path)

    resolver.resolve(conf)

    return 0

if __name__ == "__main__":
    argv = sys.argv
    rc = 1

    try:
        rc = main(argv)
    except Exception as e:
        sys.stderr.write("%s: fail to resove gcov: %s\n" % (argv[0], e))

    sys.exit(rc)
