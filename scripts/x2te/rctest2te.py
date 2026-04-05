#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2025 OKTET Ltd. All rights reserved.

import os
import re
import argparse
import subprocess
from lib import texml
from textwrap import dedent


class RDMAcoreDef:
    bin_arg_name = "rdma_core_bin"
    group_arg_name = "rdma_core_group"
    name_arg_name = "rdma_core_name"
    env_arg_name = "env"
    dir_name = "rdma_core_test"


def rdma_core_get_node(line):
    """
    Parse and get node name and parent by line

    rdma_core_bin --list_tests has format:
    tests.Group1.Group2.test

    :param line: Line to parse
    :return: tuple (node_name, node_parent) or None if cannot match
    """
    result = re.search(r'tests\.([^\.]+)\.([^\.]+)\.(.*)$', line)
    if result:
        return result.group(3), f'{result.group(1)}.{result.group(2)}'
    return None


def rdma_core_list_tests_gen(rdma_core_bin):
    """
    Generator of next node in rdma_core_bin

    :param rdma_core_bin: Path to rmda-core-bin
    :return: dict with name and parent
    """
    parent = None
    result = subprocess.run([rdma_core_bin, "--list-tests"],
                            capture_output=True, text=True, check=False)
    # Return code 5 is for 'no tests run'
    if result.returncode not in (0, 5):
        raise RuntimeError(f"Unexpected exit code for {rdma_core_bin}:"
                           f"{result.returncode}")
    for line in result.stdout.splitlines():
        if not line:
            continue
        node = rdma_core_get_node(line)
        if not node:
            print(f"WARNING: Cannot parse line, will be skip: {line}")
            continue
        node_name, parent_node_name = node
        parent = parent_node_name
        yield {"name": node_name, "parent": parent}


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


def generate_pkg(args, rdma_core_tests):
    """
    Generate package(s).xml by command line args

    :param args: command line args
    """
    rdma_core_bin = os.path.split(args.rdma_core_bin)[1]
    rdma_core_bin2 = RDMAcoreDef.dir_name
    basedir = os.path.join(args.pkg_dir, rdma_core_bin2)
    if not os.path.exists(basedir):
        os.mkdir(basedir)

    root_pkg_filename = os.path.join(basedir, "package.xml")
    root_pkg = get_te_xml("package", root_pkg_filename,
                          description=args.description, mailto=args.mailto)

    for parent, tests in rdma_core_tests.items():
        if parent is None:
            continue
        pkg_filename = os.path.join(basedir, f"package.{parent}.xml")
        root_pkg.add_run_package(name=parent,
                                 src=f"package.{parent}.xml")
        descr = f'RDMA core {parent} unit test'
        pkg = get_te_xml("package", pkg_filename,
                         description=descr, mailto=args.mailto)
        for test in tests:
            test_args = [
                (RDMAcoreDef.bin_arg_name, None),
                (RDMAcoreDef.group_arg_name, None),
                (RDMAcoreDef.name_arg_name, test["name"]),
            ]
            pkg.add_run_script(args.script_name, test["name"], test_args)

        pkg.add_session_arg(RDMAcoreDef.bin_arg_name)
        pkg.add_session_arg(RDMAcoreDef.env_arg_name)
        pkg.add_session_arg(RDMAcoreDef.group_arg_name, parent)
        pkg.save(pkg_filename)

    root_pkg.add_session_arg(RDMAcoreDef.bin_arg_name, rdma_core_bin)
    root_pkg.save(root_pkg_filename)


def generate_trc(args, rdma_core_tests):
    basedir = os.path.join(args.trc_dir, RDMAcoreDef.dir_name)
    if not os.path.exists(basedir):
        os.mkdir(basedir)

    root_trc_filename = os.path.join(basedir, "trc.xml")
    root_trc = get_te_xml("trc", root_trc_filename,
                          name=RDMAcoreDef.dir_name, type="package")

    for parent, tests in rdma_core_tests.items():
        if parent is None:
            continue
        trc_filename = os.path.join(basedir, f"trc.{parent}.xml")
        root_trc.include_file(path=f"trc.{parent}.xml")

        trc = get_te_xml("trc", trc_filename, name=parent, type="package")
        for test in tests:
            test_args = [
                (RDMAcoreDef.bin_arg_name, None),
                (RDMAcoreDef.group_arg_name, None),
                (RDMAcoreDef.name_arg_name, test["name"]),
            ]
            trc.add_test(test["name"], "script", texml.TRCResult.PASSED,
                         args=test_args)
        trc.save(trc_filename)

    root_trc.save(root_trc_filename)


def main():
    description = dedent("""\
        Tool for generation package.xml (passed for all tests) from
        RDMA core binaries.

        NOTE: You can modify generated package(s).xml; the script does not
        touch your modifications if you run this script again.
    """)

    examples = dedent("""\
        ./rctest2te.py --rdma-core-bin=/path/to/run_test.py
                       --script-name=run_test
                       --pkg-dir=ts-name/rdma-core-dir-name
                       --mailto=example@oktetlabs.ru
                       --description="Wrap RDMA core in TE"

        This command will create dir "ts-name/rdma-core-name/rdma_core_bin" and
        generate package.xml,
        package.{rdma_core_group11}.{rdma_core_group12}.xml, ...
        package.{rdma_core_groupN1}.{rdma_core_groupN2}.xml in
        ts-name/rdma-core-name/rdma_core_bin
    """)
    parser = argparse.ArgumentParser(
        description=description,
        usage=examples,
    )

    parser.add_argument("-r", "--rdma-core-bin", required=True,
                        help="RDMA core binary")
    parser.add_argument("-s", "--script-name", required=True,
                        help="Script name for package.xml")
    parser.add_argument("-p", "--pkg-dir",
                        help="package.xml output dir")
    parser.add_argument("-t", "--trc-dir",
                        help="trc.xml output dir")
    parser.add_argument("-m", "--mailto",
                        default="autogenerated@arknetworks.am",
                        help="email in package.xml")
    parser.add_argument("-d", "--description",
                        help="Specific description package.xml")

    args = parser.parse_args()

    rdma_core_tests = {}
    for test in rdma_core_list_tests_gen(args.rdma_core_bin):
        rdma_core_tests.setdefault(test["parent"], [])
        rdma_core_tests[test["parent"]].append(test)

    generate_pkg(args, rdma_core_tests)
    generate_trc(args, rdma_core_tests)



if __name__ == "__main__":
    main()
