#
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

# A small extension for able override css for doxyrest

import os


def on_builder_inited(app):
    css_file = 'override-doxyrest-sphinx_rtd_theme.css'
    app.add_css_file(css_file)


def setup(app):
    app.connect('builder-inited', on_builder_inited)

    # The extension registers one stylesheet once and keeps no per-document
    # state, so it does not stop Sphinx from reading sources in parallel.
    return {
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
