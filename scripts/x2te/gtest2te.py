#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

import os
import re
import argparse
import subprocess
from lib import texml


class GTestDef:
    bin_arg_name = "gtest_bin"
    group_arg_name = "gtest_group"
    name_arg_name = "gtest_name"


def gtest_get_node(line):
    """
    Parse and get node type and name by line

    gtest_bin --gtest_list_test have format:
    Group1.
      Test1
      Test2
    Group2.
      Test1/0.  # TypeParam = TestIterateParam1<42>
      Test1/1.  # TypeParam = TestIterateParam1<100>

    :param line: Line to parse
    :return: tuple (node_type, node_name) or None if cannot match
    """

    # Search trash lines
    if re.match(r"^seed \d+$", line):
        return "trash", line

    # Search test name, example: '  Test1'
    if re.match(r"^ {2}.+$", line):
        return "test", re.sub(r" {2}", "", line)

    # Search group name, example: 'Group1'
    if re.match(r"^.+\.$", line):
        return "package", re.sub(r".$", "", line)

    # Search template test name, example:
    # '  Test1/0.  # TypeParam = TestInterateParam1<false>'
    if re.match(r"^.+.+\. {2}#.+$", line):
        return "test", re.sub(r". {2}# .+", "", line)


def gtest_list_tests_gen(gtest_bin):
    """
    Generator of next node in gtest_bin

    :param gtest_bin: Path to gtest-bin
    :return: dict with key type, name, parent
    """
    parent = None
    output = subprocess.check_output([gtest_bin, "--gtest_list_tests"])
    list_tests_str = str(output, encoding="utf-8")
    for line in filter(bool, list_tests_str.split("\n")):
        node = gtest_get_node(line)
        if not node:
            print("WARNING: Cannot parse line, will be skip: {}".format(line))
            continue
        node_type, node_name = node
        if node_type == "package":
            parent = node_name
            yield {"type": "package", "name": node_name, "parent": None}
        elif node_type == "test":
            yield {"type": "test", "name": node_name, "parent": parent}


def get_te_xml(xml_type, filename, **kwargs):
    """
    Get a new instance of TEXML or load if one already exists

    :param xml_type: Type of instance
    :param filename: Path to package.xml
    :param kwargs: Parameters for init PackageXML or TRCTest
    :return: PackageXML or TRCTest object
    """
    xmls = {
        "package": texml.PackageXML,
        "trc": texml.TRCTest,
    }

    if os.path.exists(filename):
        return xmls[xml_type].load(filename)
    return xmls[xml_type](**kwargs)


def generate_pkg(args, gtest_tests):
    """
    Generate package(s).xml by command line args

    :param args: command line args
    """
    gtest_bin = os.path.split(args.gtest_bin)[1]
    basedir = os.path.join(args.pkg_dir, gtest_bin)
    if not os.path.exists(basedir):
        os.mkdir(basedir)

    root_pkg_filename = os.path.join(basedir, "package.xml")
    root_pkg = get_te_xml("package", root_pkg_filename,
                          description=args.description, mailto=args.mailto)

    for parent, tests in gtest_tests.items():
        if parent is None:
            continue
        pkg_filename = os.path.join(basedir, "package.{}.xml".format(parent))
        root_pkg.add_run_package(name=parent,
                                 src="package.{}.xml".format(parent))

        pkg = get_te_xml("package", pkg_filename,
                         description=args.description, mailto=args.mailto)
        for test in tests:
            test_args = [
                (GTestDef.bin_arg_name, None),
                (GTestDef.group_arg_name, None),
                (GTestDef.name_arg_name, test["name"]),
            ]
            pkg.add_run_script(args.script_name, test["name"], test_args)

        pkg.add_session_arg(GTestDef.bin_arg_name)
        pkg.add_session_arg(GTestDef.group_arg_name, parent)
        pkg.save(pkg_filename)

    root_pkg.add_session_arg(GTestDef.bin_arg_name, gtest_bin)
    root_pkg.save(root_pkg_filename)


def generate_trc(args, gtest_tests):
    gtest_bin = os.path.split(args.gtest_bin)[1]
    basedir = os.path.join(args.trc_dir, gtest_bin)
    if not os.path.exists(basedir):
        os.mkdir(basedir)

    root_trc_filename = os.path.join(basedir, "trc.xml")
    root_trc = get_te_xml("trc", root_trc_filename,
                          name=gtest_bin, type="package")

    for parent, tests in gtest_tests.items():
        if parent is None:
            continue
        trc_filename = os.path.join(basedir, "trc.{}.xml".format(parent))
        root_trc.include_file(path="trc.{}.xml".format(parent))

        trc = get_te_xml("trc", trc_filename, name=parent, type="package")
        for test in tests:
            test_args = [
                (GTestDef.bin_arg_name, None),
                (GTestDef.group_arg_name, None),
                (GTestDef.name_arg_name, test["name"]),
            ]
            trc.add_test(test["name"], "script", texml.TRCResult.PASSED,
                         args=test_args)
        trc.save(trc_filename)

    root_trc.save(root_trc_filename)



def main():
    description = """\
Tool for generation package.xml (passed for all tests) from
GTest binaries.
NOTE: You can modify generated package(s).xml, script dot not touch
you modification if you run this script again.
"""
    examples = """\
./gtest2te.py --gtest-bin=/path/to/gtest_bin
              --script-name=run_test
              --pkg-dir=my-ts/my-gtest
              --mailto=example@oktetlabs.ru
              --description="Wrap GTest in TE"

This cmdline will be create dir "my-ts/my-gtest/gtest_bin" and generate
package.xml, package.{gtest_group1}.xml, ... package.{gtest_groupN}.xml in
my-ts/my-gtest/gtest_bin
"""
    parser = argparse.ArgumentParser(
        description=description,
        usage=examples,
    )

    parser.add_argument("-g", "--gtest-bin", required=True,
                        help="GTest binary")
    parser.add_argument("-s", "--script-name", required=True,
                        help="Script name for package.xml")
    parser.add_argument("-p", "--pkg-dir",
                        help="package.xml output dir")
    parser.add_argument("-t", "--trc-dir",
                        help="trc.xml output dir")
    parser.add_argument("-m", "--mailto", default="autogenerated@oktetlabs.ru",
                        help="email in package.xml")
    parser.add_argument("-d", "--description",
                        help="Specific description package.xml")

    args = parser.parse_args()

    gtest_tests = dict()
    for test in gtest_list_tests_gen(args.gtest_bin):
        gtest_tests.setdefault(test["parent"], [])
        gtest_tests[test["parent"]].append(test)

    generate_pkg(args, gtest_tests)
    generate_trc(args, gtest_tests)



if __name__ == "__main__":
    main()
