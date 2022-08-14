import os
import datetime

from scenarios.generator import Generator
from scenarios.redirect  import stdout2str

class TestGenerator(Generator):
    def __init__(self, test, author, rights):
        super().__init__()
        self.test = test.test
        self.groups = test.groups
        self.author = author
        self.rights = rights
        self.location = list(test.get_location())

    def copyright(self):
        with self.comment() as c:
            has_objective = False
            for group in self.groups:
                if group.objective:
                    c.write(group.objective)
                    has_objective = True
            if not has_objective and self.test.summary:
                c.write(self.test.summary)
            year = datetime.datetime.now().year
            if self.rights:
                c.write('', self.rights.format(year=year))
            else:
                c.write('', 'Copyright (C) 2004-{year} OKTET Labs Ltd. '\
                        'All rights reserved.'.format(year=year))
        self.write('')

    def _doxygen_values(self, values):
        if not values:
            return
        for value in values:
            if hasattr(value, 'comment'):
                comment = ' (%s)' % value.comment
            else:
                comment = ''
            if type(value.value) == int:
                yield '@c %s%s' % (value.value, comment)
            else:
                yield '%s%s' % (value.value, comment)

    def _doxygen_params(self, c):
        if not hasattr(self.test, 'params'):
            return
        length = len('@param ') + \
            max([len(p.param) for p in self.test.params])
        for p in self.test.params:
            description = p.description
            if hasattr(p, 'values'):
                description = description.rstrip('.:') + ':'
            c.paragraph('@param %s' % p.param, description, length)
            if hasattr(p, 'values'):
                for value in self._doxygen_values(p.values):
                    c.paragraph('%6s' % '-', value)
        c.write('')

    def _doxygen_scenario(self, c):
        if not hasattr(self.test, 'steps'):
            return
        c.write('@par Scenario:')
        c.write('')

    def doxygen(self):
        with self.comment('*') as c:
            page = '-'.join(self.location[1:])
            c.write('@page %s %s' % (page, self.test.summary))
            c.write('')

            if hasattr(self.test, 'objective'):
                c.paragraph('@objective', self.test.objective)
                c.write('')

            if hasattr(self.test, 'note'):
                c.paragraph('@note', self.test.note)
                c.write('')

            self._doxygen_params(c)
            self._doxygen_scenario(c)

            if hasattr(self.test, 'type'):
                c.write('@type %s', self.test.type)
                c.write('')

            if self.author:
                c.write('@author %s' % self.author)
            else:
                c.write('@author')
        self.write('')

    @staticmethod
    def _steps(steps, current_level=1):
        def _sub(step):
            if not hasattr(step, 'sub'):
                return []
            return TestGenerator._steps(step.sub, current_level+1)

        for step in steps:
            yield (current_level, step.step)
            for x in _sub(step):
                yield x

    def main(self):
        self.write('int', 'main(int argc, char *argv[])', '{', +1)
        self.write('TEST_START;', '')
        if hasattr(self.test, 'steps'):
            old_level = self.level
            self.level = 0
            s = self.step()
            for level, step in self._steps(self.test.steps):
                s.paragraph(level, step)
            self.level = old_level
            self.write('')
        self.write('TEST_SUCCESS;', '')
        self.label('cleanup')
        self.write('', 'TEST_END;')
        self.write(-1, '}')

    @staticmethod
    @stdout2str
    def generate(test, author=None, rights=None):
        g = TestGenerator(test, author, rights)
        g.copyright()
        g.doxygen()
        g.write('#define TE_TEST_NAME "%s"' % '/'.join(g.location[1:]), '')
        g.main()
