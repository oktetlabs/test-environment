.. _add_doc:

Documentation
=============

Full pipeline you can read from `doxyrest`_ repository

Requirement tools:

* doxygen
* doxyrest (>=2.0)
* sphinx (>=1.7.0)
* sphinx_rtd_theme

Structure

* ``doc/sphinx/index.rst`` is main reStructuredText.
* ``doc/shpinx/static/image`` folder for custom images.
* ``doc/sphinx/static/css`` folder for you custom css.

To add your own, you need to put in `RST`_ format you need put
``my_doc.rst`` into ``doc/shpinx/pages`` and reference your file
from the main file ``doc/sphinx/index.rst``

To generate Sphinx documentation from scratch you need

.. code:: shell

    cd ${TE_BASE}
    export DOXYREST_PREFIX="<path-to-doxyrest>"
    ./gen_doxygen --type=sphinx

.. note:: You can download last doxyrest release `here`_

    .. code:: shell

        wget $DOXYREST_RELEASE_LINK
        tar -xf doxyrest-<version>.tar.xz
        export DOXYREST_PREFIX=$(pwd)/doxyrest-<version>

If you debug your RST you can use

.. code:: shell

    sphinx-build -j auto -v -c doc/ doc/sphinx doc/generated/html

.. _doxyrest: https://github.com/vovkos/doxyrest
.. _RST: https://www.sphinx-doc.org/es/master/usage/restructuredtext/basics.html
.. _here: https://github.com/vovkos/doxyrest/releases