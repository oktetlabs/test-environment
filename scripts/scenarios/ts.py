#
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

import os
import sys
import logging
import xml.etree.ElementTree as ET

from scenarios.scenarioobject import *

class Packages(object):
    log = logging.getLogger(__name__)

    def __init__(self, base, sorting=False):
        self.base = os.path.normpath(base)
        self.sorting = sorting

        self.groups = self._parse(('', None))

        self.groups['group'] = os.path.basename(self.base)
        self.context = Context({'groups': [self.groups]})

    def _parse_node(self, root):
        scripts, packages = [], []
        description = None
        for child in root:
            s, p = self._parse_node(child)
            scripts += s
            packages += p
            if child.tag == 'package':
                packages.append((child.attrib['name'], child.attrib.get('src')))
            elif child.tag == 'script':
                script = child.attrib['name']
                if script.endswith('prologue'):   continue
                if script.endswith('epilogue'):   continue
                if script.startswith('prologue'): continue
                if script.startswith('epilogue'): continue
                scripts.append(script)
        return scripts, packages

    def _parse_summary(self, filename):
        fp = open(filename, 'r')
        for line in fp:
            line = line.strip()
            if '* @page ' in line:
                line = line.replace('/** @page ', '', 1).\
                    replace('* @page ', '', 1).strip()
                items = line.split(' ', 1)
                if len(items) == 1:
                    return items[0], ''
                return items[0], items[1].strip()

    def _parse_script(self, name, sub):
        filename = os.path.join(self.base, sub, name + '.c')
        test_ref, summary = self._parse_summary(filename)
        test_ref = os.path.join(*test_ref.split('-'))
        path = os.path.join(sub, name)
        if test_ref != path and test_ref != os.path.normpath(path):
            self.log.warning('Invalid test reference %s '
                '(expected %s)',
                test_ref, path)
        return {
            "test": name,
            "summary": summary,
        }

    def _parse(self, name, sub=''):
        name, package_xml = name
        if not package_xml:
            sub = os.path.join(sub, name, "package.xml")
        else:
            sub = os.path.join(sub, package_xml)
        sub, package_xml = os.path.split(sub)

        tree = ET.parse(os.path.join(self.base, sub, package_xml))
        description = tree.getroot().find('description').text

        scripts, packages = self._parse_node(tree.getroot())

        tests = [self._parse_script(s, sub) for s in sorted(set(scripts))]
        groups = [self._parse(package, sub) for package in sorted(packages)]
        return {
            "group": name,
            "summary": description,
            "objective": "",
            "tests": tests,
            "groups": groups,
        }

    def merge(self, context):
        src = self.context.groups[0]
        dst = context.groups[0]
        if src.group != dst.group:
            self.log.error('Invalid test suite name: "%s" '
                '(expected "%s")',
                src.group, dst.group)
            sys.exit(-1)
        self._merge_group(src, dst)

        if self.sorting:
            self._sort_groups(dst)

    def _merge_group(self, src, dst):
        if src.summary != dst.summary:
            self.log.warning('Invalid group summary: "%s" '
                '(expected "%s" for group "%s")',
                src.group, dst.group, src.group)

        for group in src.groups:
            self._add_group(group, dst)

        for test in src.tests:
            self._add_test(test, dst)

    def _add_group(self, group, dst):
        for cur in dst.groups:
            if cur.group == group.group:
                self._merge_group(group, cur)
                return
        dst.groups.append(group)

    def _add_test(self, test, dst):
        for cur in dst.tests:
            if cur.test == test.test:
                if cur.summary != test.summary:
                    self.log.warning('Invalid test summary: "%s" '
                        '(expected "%s" for test "%s")',
                        cur.summary, test.summary, cur.test)
                if not test.summary:
                    cur.summary = test.summary
                return
        dst.tests.append(test)

    def _sort_groups(self, cur):
        if hasattr(cur, 'groups'):
            cur.groups.sort(key=lambda a: a.group)
            for groups in cur.groups:
                self._sort_groups(groups)

        if hasattr(cur, 'tests'):
            cur.tests.sort(key=lambda a: a.test)
