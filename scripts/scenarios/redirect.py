#
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

import sys
from io import StringIO

def stdout2str(function):
    def wrapper(*args, **kwargs):
        stdout_old = sys.stdout
        sys.stdout = StringIO()

        function(*args, **kwargs)

        sys.stdout.seek(0)
        buf = sys.stdout.read()
        sys.stdout = stdout_old
        return buf
    return wrapper
