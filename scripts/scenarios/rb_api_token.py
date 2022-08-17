#
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.

import os
from io import StringIO
from configparser import ConfigParser

def _take_from_hg(fname='~/.hgrc'):
    try:
        config = ConfigParser(strict=False)
        config.read(os.path.expanduser(fname))
        return config.get('reviewboard', 'token')
    except:
        return None

def _take_from_rbtools(fname='~/.reviewboardrc'):
    try:
        fp = StringIO()
        fp.write('[reviewboard]\n')
        fp.write(open(os.path.expanduser(fname)).read())
        fp.seek(0, os.SEEK_SET)

        config = ConfigParser(strict=False)
        config.readfp(fp)
        return config.get('reviewboard', 'API_TOKEN').strip('"')
    except:
        return None

def take_api_token():
    api_token = _take_from_hg()
    if not api_token:
        api_token = _take_from_rbtools()
    return api_token
