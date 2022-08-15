..
  Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

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
* ``doc/dox_resources/images`` folder for custom images.
* ``doc/sphinx/static/css`` folder for you custom css.

To add your own page, you need to put your page in `RST`_ format
into ``doc/shpinx/pages`` and reference your file
from the main file ``doc/sphinx/index.rst``

To generate Sphinx documentation from scratch you need to run these commands:

.. code:: shell

    cd ${TE_BASE}
    export DOXYREST_PREFIX="<path-to-doxyrest>"
    ./gen_docs

.. note:: You can download last doxyrest release `here`_

    .. code:: shell

        wget $DOXYREST_RELEASE_LINK
        tar -xf doxyrest-<version>.tar.xz
        export DOXYREST_PREFIX=$(pwd)/doxyrest-<version>

The documentation is generated in 3 steps:

1. ``doxygen`` goes through ``${TE_BASE}`` and generates documentation in xml format.
2. ``doxyrest`` converts the xml files to rst pages.
3. ``sphinx-build`` generates html pages based on the rst pages.

Use ``-e`` flag not to generate global namespace.
Otherwise, a page containing links to all functions, enums, structs,
etc. will be created, which is very time- and resource-consuming.

.. code:: shell

    ./gen_docs -e


If you don't want the rst pages to be rebuild by ``doxygen`` or ``doxyrest``,
you can run the third step only using ``-s`` flag. In this case, your rst pages
will remain untouched unless you change them manually. You can use the flag
along with ``-e``.

.. code:: shell

    ./gen_docs -s

This is equivalent to

.. code:: shell

    rm -rf doc/generated/html
    sphinx-build -j auto -q doc/sphinx doc/generated/html

.. _doxyrest: https://github.com/vovkos/doxyrest
.. _RST: https://www.sphinx-doc.org/es/master/usage/restructuredtext/basics.html
.. _here: https://github.com/vovkos/doxyrest/releases
