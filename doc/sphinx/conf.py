# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# http://www.sphinx-doc.org/en/master/config

import sys
import os

doxyrest_prefix = os.environ["DOXYREST_PREFIX"]
doxyrest_ext_path = os.path.join(doxyrest_prefix, "share", "doxyrest", "sphinx")
sys.path.append(doxyrest_ext_path)

this_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(this_dir)

# -- Project information -----------------------------------------------------
project = 'Test Environment'
copyright = '2019, OKTET Labs'
author = 'OKTET Labs'

# The full version, including alpha/beta/rc tags
release = '1.0'

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
html_static_path = ['generated/_static']
