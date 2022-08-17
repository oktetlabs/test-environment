#
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

import os
import yaml

from scenarios.scenarioobject import *

class BaseDumper(yaml.Dumper):
    def __init__(self, stream, **kwargs):
        super().__init__(stream, **kwargs)

        old = self.serialize_node
        def serialize_node(node, *args, **kwargs):
            old(node, *args, **kwargs)
            if type(node) == yaml.MappingNode:
                for key, value in node.value:
                    if type(key) == yaml.ScalarNode and key.value == 'test':
                        self.write_indicator('\n', False)
                        break
        self.serialize_node = serialize_node

class FullDumper(BaseDumper):
    def __init__(self, stream, **kwargs):
        super().__init__(stream, **kwargs)
        self.add_user_representer(Context)
        self.add_user_representer(Group)
        self.add_user_representer(Sub)
        self.add_user_representer(Test)
        self.add_user_representer(Param)
        self.add_user_representer(Step)
        self.add_user_representer(Value)

    def add_user_representer(self, cls):
        self.add_representer(cls, cls.full_represent)

class ShortDumper(BaseDumper):
    def __init__(self, stream, **kwargs):
        super().__init__(stream, **kwargs)
        self.add_user_representer(Context)
        self.add_user_representer(Group)
        self.add_user_representer(Sub)
        self.add_user_representer(Test)
        self.add_user_representer(Param)
        self.add_user_representer(Step)
        self.add_user_representer(Value)

    def add_user_representer(self, cls):
        self.add_representer(cls, cls.short_represent)

class TestItem(object):
    def __init__(self, test, groups):
        self.test = test
        self.groups = groups

        self.name = '.'.join(self.get_location())

    def get_location(self):
        for group in self.groups:
            if type(group) == Group:
                yield group.group
        yield self.test.test

    def _extract_objective(self, filename):
        fp = open(filename, 'r')
        for line in fp:
            line = line.strip()
            if line.startswith('* @objective '):
                yield line.replace('* @objective ', '', 1).strip()
                break
        for line in fp:
            line = line.strip()
            if line == '*' or line == '*/' or line.startswith('* @'):
                return
            yield line[1:].strip()

    def fill_objective(self, test_suite):
        if hasattr(self.test, 'objective'):
            return
        filename = '%s.c' % os.path.join(test_suite, *self.get_location())
        self.test.objective = ' '.join(self._extract_objective(filename))

    def cut(self):
        def safe_delattr(obj, field):
            if hasattr(obj, field):
                delattr(obj, field)

        safe_delattr(self.test, 'objective')
        safe_delattr(self.test, 'steps')
        safe_delattr(self.test, 'params')
        location = list(self.get_location())
        self.test.ref = '-'.join(location[1:])

class TestReader(object):
    def __init__(self):
        self.context = ''

    def load_data(self, data):
        self.tests = {}
        self.context = Context(yaml.load(data))
        self._update_tests()

    def load_file(self, fname):
        self.load_data(open(fname))

    def import_context(self, context):
        self.tests = {}
        self.context = context
        self._update_tests()

    def _iter_groups(self, groups, base=[]):
        for group in groups:
            current = base + [group]
            yield current, group

            if hasattr(group, 'sub'):
                for x in self._iter_groups(group.sub, current):
                    yield x

            if hasattr(group, 'groups'):
                for x in self._iter_groups(group.groups, current):
                    yield x

    def _update_tests(self):
        for groups, group in self._iter_groups(self.context.groups):
            if not hasattr(group, 'tests'):
                continue
            for test in group.tests:
                test = TestItem(test, groups)
                assert test.name not in self.tests
                self.tests[test.name] = test

    def fill_objective(self, test_suite):
        for test in self.tests.values():
            test.fill_objective(test_suite)

    def dump_full(self):
        return yaml.dump(self.context,
            default_flow_style=False, Dumper=FullDumper)

    def dump_short(self):
        reader = copy.deepcopy(self)
        for test in reader.tests.values():
            filename = '%s.c' % os.path.join(*test.get_location())
            test.test.add_field('filename', filename)
        return yaml.dump(reader.context,
            default_flow_style=False, Dumper=ShortDumper)
