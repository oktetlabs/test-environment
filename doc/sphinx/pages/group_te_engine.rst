..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
  te-parent: te

.. index:: pair: group; Test Engine
.. _doxid-group__te__engine:

Test Engine
===========

.. include:: _toctree/te_engine.inc

.. _doxid-group__te__engine_1te_eng_introduction:

Introduction
~~~~~~~~~~~~

Test Engine is a set of software components that provide essential features of Test Environment. It is unlikely that you will need to update any of Test Engine components, but more likely you will need to implement some helper libraries that utilize services provided by Test Engine or you will need to add some functionality in Test Agents.

.. image:: /static/image/ten_decomposition.png
	:alt: High Level decomposition of Test Engine Components

Test Engine consists of the following components:

* :ref:`Dispatcher <doxid-group__te__engine__dispatcher>`, which is responsible for configuring and starting of another subsystems;

* :ref:`Builder <doxid-group__te__engine__builder>`, which is responsible for preparing libraries and executables for Test Agents and TE Subsystems as well as NUT bootable images and building the tests;

* :ref:`Configurator <doxid-group__te__engine__conf>`, which is responsible for configuring the environment, providing configuration information to tests and for recovering the configuration after failures. Moreover it supports some TEN-local database used by Test Packages to store shared data;

* :ref:`Remote Control Facility (RCF) <doxid-group__te__engine__rcf>`, which is responsible for starting Test Agents and for all interactions between :ref:`Test Engine <doxid-group__te__engine>` and Test Agents on behalf of other subsystems and tests;

* :ref:`Tester <doxid-group__te__engine__tester>`, which is responsible for running a set of Test Packages specified by a user in the mode specified by a user (one-by-one, simultaneous, debugging);

* :ref:`Logger <doxid-group__te__engine__logger>`, which provides logging facilities for Test Environment (:ref:`Test Engine <doxid-group__te__engine>` and Test Agents) and tests, and log processing tools for users.

The following diagram gives more detailed information on relations between Test Environment components:

.. image:: /static/image/ten_interconnections.png
	:alt: Interconnections of Test Engine components
