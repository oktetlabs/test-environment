..
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. _selftest:

Self-testing system
===================

test-environment has a self-testing system that solves the following tasks:

* Checking API health.
* Checking the build on different platforms, analyzing warnings.
* Providing examples of tests that use up-to-date API.

.. note:: Self-testing system works only with Meson build.

The self-test system runs every night via `jenkins-selftest`_ and after every change in TE.
If the commit caused a problem with the build or the test was broken you will get an email with the appropriate content.

.. note:: Self-testing system lives in ${TE_BASE}/suites/selftest.

Policy:

* After each change, the self-test system must completed successfully.
* If you change an existing API, it is very desirable to update the tests existing for it.
* It is not required (but recommended) to add tests when adding new API or modifying existing API without tests.
* There should be a mention of whether the tests are planned to be covered in reviewboard Testing Done section when adding new API.

.. _jenkins-selftest: https://oktetlabs.ru/prj/ol/jenkins/job/TE/job/te-selftests/
