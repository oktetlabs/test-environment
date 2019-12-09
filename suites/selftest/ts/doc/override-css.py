# A small extention for able override css for doxyrest

import os


def on_builder_inited(app):
    css_file = 'override-doxyrest-sphinx_rtd_theme.css'
    app.add_stylesheet(css_file)


def setup(app):
    app.connect('builder-inited', on_builder_inited)
