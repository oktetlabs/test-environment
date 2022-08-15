#
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

import textwrap

class Comment(object):
    def __init__(self, base, start=''):
        self.base = base
        self.level = 0
        base.write('/*%s' % start)

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.base.write(' */')

    def write(self, *args):
        for arg in args:
            if type(arg) == int:
                self.level += arg
            elif arg == '':
                self.base.write(' *')
            else:
                level = 4 * self.level + 1
                self.base.write(' *%s%s' % (' ' * level, arg))

    def paragraph(self, key, value, limit=-1):
        if limit == -1:
            limit = len(key)
            line = '%s %s' % (key, value)
        else:
            fmt = '%%-%is %%s' % limit
            line = fmt % (key, value)
        lines = textwrap.wrap(line, width=80 - 3 - 4 * self.level)
        self.write(lines[0])
        if len(lines) >= 2:
            limit = limit + 1
            width = 80 - 3 - limit - 4 * self.level
            lines = textwrap.wrap(' '.join(lines[1:]), width=width)
            for line in lines:
                self.write('%s%s' % (' ' * limit, line))

class StepGenerator(object):
    def __init__(self, base):
        self.base = base
        self.prev_level = 0

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return

    def _get_prefix(self, level, prev_level):
        if level < 1:
           raise ValueError('Wrong level value: %s' % level)
        elif level == 1:
            step = 'STEP'
        elif level == 2:
            step = 'SUBSTEP'
        elif prev_level + 1 == level:
            step = 'STEP_PUSH'
        elif prev_level == level:
            step = 'STEP_NEXT'
        elif prev_level - 1 == level:
            step = 'STEP_POP'
        else:
            raise ValueError('Wrong prev_level/level combination: %s/%s'
                             % (level, prev_level))
        return ' ' * 4 * level + 'TEST_' + step + '("';

    def _paragraph_write(self, prefix, lines):
        if len(lines) > 1:
            for line in lines[0:-1]:
                self.base.write(prefix + line + ' "')
                prefix = ' ' * (len(prefix) - 1) + '"'
        self.base.write(prefix + lines[-1] + '");')

    def paragraph(self, level, value):
        while level > 2 and level < self.prev_level - 1:
            prefix = self._get_prefix(self.prev_level - 1, self.prev_level)
            self._paragraph_write(prefix, [''])
            self.prev_level -= 1
        prefix = self._get_prefix(level, self.prev_level)
        lines = textwrap.wrap(value, width=80 - 2 - len(prefix) - 1)
        self._paragraph_write(prefix, lines)
        self.prev_level = level

class Generator(object):
    def __init__(self):
        self.level = 0

    def comment(self, start=''):
        return Comment(self, start)

    def step(self):
        return StepGenerator(self)

    def write(self, *args):
        for arg in args:
            if type(arg) == int:
                self.level += arg
            elif not arg:
                print('')
            else:
                print('%s%s' % (' ' * 4 * self.level, arg))

    def label(self, label):
        self.write(-self.level, '%s:' % label, +self.level)
