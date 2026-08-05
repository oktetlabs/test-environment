#
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# http://www.sphinx-doc.org/en/master/config

import sys
import os
import re
import subprocess
from urllib.parse import urlsplit

doxyrest_prefix = os.environ["DOXYREST_PREFIX"]
doxyrest_ext_path = os.path.join(doxyrest_prefix, "share", "doxyrest", "sphinx")
sys.path.append(doxyrest_ext_path)

this_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(this_dir)

# -- Project information -----------------------------------------------------
project = 'Test Environment'
copyright = '2019, OKTET Labs'
author = 'OKTET Labs'

# TE is developed continuously and has no release numbering, so there is
# no version to show.
release = ''

# -- The repository these pages were generated from ---------------------------
# The user guide has to tell the reader where to get TE. That answer is a
# property of the tree the documentation is built from, not text to freeze
# into a page: a fork, a mirror or a private clone has to document itself,
# not the repository it was copied from.

te_base = os.path.dirname(os.path.dirname(this_dir))

# Used when the tree has no origin, e.g. unpacked from an archive.
te_public_repo = 'https://github.com/ts-factory/test-environment.git'


def _clone_url():
    """URL this tree was cloned from."""
    try:
        url = subprocess.run(
            ['git', '-C', te_base, 'config', '--get', 'remote.origin.url'],
            capture_output=True, text=True, check=True).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        url = ''

    return url or te_public_repo


def _browse_url(url):
    """The same repository in a form a browser can open, if there is one."""
    scp = re.match(r'^(?:[^@/]+@)?([^/:]+):(?!//)(.+)$', url)
    if scp:
        host, path = scp.group(1), scp.group(2)
    else:
        parts = urlsplit(url)
        if parts.scheme in ('http', 'https'):
            return re.sub(r'\.git$', '', url)
        if parts.scheme not in ('ssh', 'git') or not parts.hostname:
            return None
        host, path = parts.hostname, parts.path.lstrip('/')

    return 'https://%s/%s' % (host, re.sub(r'\.git$', '', path))


te_clone_url = _clone_url()
_te_browse_url = _browse_url(te_clone_url)

# A standalone URI is recognised wherever it appears, which would turn the
# clone command into a link inside its code block; escaping the scheme colon
# keeps that one plain text.
_te_plain_url = te_clone_url.replace(':', '\\:', 1)

# Defined in the epilog rather than the prolog: appended to a document these
# cannot come between a file and its title.
rst_epilog = """
.. |te_clone_url| replace:: %s
.. |te_repository| replace:: %s
""" % (_te_plain_url,
       '`%s <%s>`__' % (_te_browse_url, _te_browse_url) if _te_browse_url
       else '``%s``' % te_clone_url)

# -- General configuration ---------------------------------------------------
# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'doxyrest',
    'cpplexer',
    'override_css',
]

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = [
    'generated/index.rst',
]

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'sphinx_rtd_theme'

html_logo = 'static/image/te-logo.png'

html_theme_options = {
    'logo_only': True,
    'style_nav_header_background': '#fcfcfc',
    'titles_only': True,
    'collapse_navigation': False,
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['static']
