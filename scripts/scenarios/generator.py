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

class ShortComment(object):
    def __init__(self, base):
        self.base = base
        self.last = None
        self.prefix = '/*'

    def __enter__(self):
        return self

    def __exit__(self, *args):
        if not self.last:
            return
        END = ' */'
        level, value = self.last
        prefix = self._get_prefix(level)
        value = value + ('.' * len(END))
        lines = textwrap.wrap(value, width=80 - 2 - len(prefix) - 1)
        lines[-1] = lines[-1][:-len(END)] + END
        self._paragraph_write(prefix, lines)

    def _get_prefix(self, level):
        if self.prefix == '/*':
            return '-' * level
        return '  ' * level + '-#'

    def _paragraph_write(self, prefix, lines):
        self.__write(prefix + ' ' + lines[0])

        prefix = ' ' * len(prefix)
        for line in lines[1:]:
            self.__write(prefix + ' ' + line)

    def __write(self, line):
        self.base.write('%s%s' % (self.prefix, line))
        self.prefix = ' *'

    def paragraph(self, level, value):
        if self.last:
            last_level, last_value = self.last
            prefix = self._get_prefix(last_level)
            lines = textwrap.wrap(last_value, width=80 - 2 - len(prefix) - 1)
            self._paragraph_write(prefix, lines)
        self.last = (level, value)

class Generator(object):
    def __init__(self):
        self.level = 0

    def comment(self, start=''):
        return Comment(self, start)

    def short_comment(self):
        return ShortComment(self)

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
