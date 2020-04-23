.. index:: pair: group; Test Agents: Windows Test Agent
.. _doxid-group__te__agents__win:

Test Agents: Windows Test Agent
===============================

.. toctree::
	:hidden:



.. _doxid-group__te__agents__win_1te_agent_win_introduction:

Introduction
~~~~~~~~~~~~

The implementation of Test Agent for Windows relies on Cygwin framework. Currently the functionality of this type of Agent is limited to:

* RPC Server functionality (POSIX socket API and Winsock API);

* Network services configuration tree (configuration of IP interfaces and IP routes);

* Process/thread creation feature;

* Reboot support.





.. _doxid-group__te__agents__win_1te_agent_win_source:

Source organization
~~~~~~~~~~~~~~~~~~~

The sources of Windows Test Agent located under agents/win32 directory of TE tree:

* agents/win32/tarpc_server.c - implementation of RPC Server calls;

* agents/win32/win32.c - implementation of entry point of Test Agent as well as functions of RCF PCH interface;

* agents/win32/win32conf.c - implementation of configuration nodes for Windows Test Agent.





.. _doxid-group__te__agents__win_1te_agent_win_conf_tree:

Supported configuration tree
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Windows Test Agent supports minimized version of basic configuration doc/cm/cm_base.xml:

.. image:: /static/image/ta_win32_conf_tree.png
	:alt: Configuration subtree supported by Windows Test Agent

