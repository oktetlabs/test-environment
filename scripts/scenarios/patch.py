#
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

import os
import difflib

from scenarios.testgenerator import TestGenerator
from scenarios.testreader    import TestReader
from scenarios.redirect      import stdout2str

def diff_new(filename, data):
    filename_in  = '/dev/null'
    filename_out = os.path.join('b', filename)
    data_in  = ''
    data_out = [x + '\n' for x in data.split('\n')]
    print('diff --git %s %s' % (filename_in, filename_out))
    print('new file mode 100644')
    print(''.join(difflib.unified_diff(data_in, data_out,
        fromfile=filename_in, tofile=filename_out)))

def diff_change(filename_in, filename_out, data_in, data_out):
    filename_in  = os.path.join('a', filename_in)
    filename_out = os.path.join('b', filename_out)
    data_in  = [x + '\n' for x in data_in.split('\n')]
    data_out = [x + '\n' for x in data_out.split('\n')]
    diff = ''.join(difflib.unified_diff(data_in, data_out,
        fromfile=filename_in, tofile=filename_out))
    if diff:
        print('diff --git %s %s' % (filename_in, filename_out))
        print(diff)

@stdout2str
def patch(reader, author=None, rights=None):
    diff_new('scenarios.yaml', reader.dump_short())
    for key, test in sorted(reader.tests.items()):
        if hasattr(test.test, 'steps'):
            filename = '%s.c' % os.path.join(*test.get_location())
#            assert not os.path.exists(os.path.join(
#                args.test_suite, filename)), \
#                'Test already exists: %s' % filename
            diff_new(filename, TestGenerator.generate(
                    test, author, rights))

def create_patch(test_suite, data=None, filename=None, author=None,
                 rights=None):
    reader = TestReader()
    assert data or filename, 'Please define data or filename'
    if data:
        reader.load_data(data)
    else:
        reader.load_file(filename)
    reader.fill_objective(test_suite)
    return patch(reader, author, rights)
