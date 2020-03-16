
.. toctree::

Test Environment
================

This is the documentation for `Test Environment`_ last updated |today|.

.. `Test Environment`_: https://oktetlabs.ru/hg/oktetlabs/test-environment

Introduction
~~~~~~~~~~~~

OKTET Labs Test Environment (TE) is a software product that is intended to ease
creating automated test suites. The history of TE goes back to 2000 year when
the first prototype of software was created. At that time the product was used
for testing SNMP MIBs and CLI commands. Two years later (in 2002) software was
extended to support testing of IPv6 protocol.

Few years of intensive usage the software in testing projects showed that a
deep re-design was necessary to make the architecture flexible and extandable
for new and new upcoming features. In 2003 year it was decided that the redesign
should be fulfilled. Due to the careful and well-thought design decisions made
in 2003 year, the overall TE architecture (main components and interconnections
between them) are still valid even though a lot of new features has been added
since then.

Conventions
~~~~~~~~~~~

Throught the documentation the following conventions are used:

* directory paths or file names marked as: ``/usr/bin/bash``, ``Makefile``;
* program names (scripts or binary executables) marked as: ``gdb``, ``dispatcher.sh``;
* program options are maked as: ``help``, ``-q``;
* environment variables are marked as: ``TE_BASE``, ``${PATH}``;
* different directives in configuration files are marked as: ``TE_LIB_PARMS``, ``include``;
* configuration pathes as maked as: ``/agent/interface``, ``/local:/net:A``;
* names of different attributes (mainly names of XML element attributes) are marked as: ``type``, ``src``;
* values of different attributes are marked as: ``unix``, ``${TE_BASE}/suites/ipv6-demo``;
* different kind of commands or modes are marked as: ``Unregister``, ``live``.

Where to start
~~~~~~~~~~~~~~

If you want to known details about the Test Environment architecture it's worth
reading :ref:`Test Environment <doxid-group__te>` section.

If you need to build and use TE you should head to :ref:`TE: User Guide <doxid-group__te__user>`.

If you want to learn more about test suites, their sources structure and how to
create a new continue with :ref:`Test Suite <doxid-group__te__ts>`.

.. toctree::
    :hidden:

    pages/group_terminology
    pages/group_te_ts
    pages/group_te_user
    generated/group_te_engine
    generated/group_te_agents
    generated/group_te_tools
    pages/selftest
    pages/add_doc
    generated/global
