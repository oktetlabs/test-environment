..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
  te-parent: te

.. _selftest:

Self-testing system
===================

test-environment has a self-testing system that solves the following tasks:

* Checking API health.
* Checking the build on different platforms, analyzing warnings.
* Providing examples of tests that use up-to-date API.

.. note:: Self-testing system lives in ${TE_BASE}/suites/selftest, and works
	only with the meson build.

The suite is run on every change to TE, so a commit that breaks the build or an
existing test is caught quickly.

Because it is built and run continuously, it is also the best place to look for
a working example of anything: see
:ref:`Minimal Test Suite <doxid-group__te__ts_1te_ts_min>` in the Test Suite
guide, which is built around it.

Policy:

* After each change, the self-test system must complete successfully.
* If you change an existing API, it is very desirable to update the tests existing for it.
* It is not required (but recommended) to add tests when adding new API or modifying existing API without tests.
* When adding new API, say in the pull request whether you intend to cover it
  with tests. See ``CONTRIBUTING.md`` in the root of the checkout for how
  changes are submitted and reviewed.
