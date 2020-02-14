.. index:: pair: group; Terminology
.. _doxid-group__terminology:

Terminology
===========

.. toctree::
	:hidden:



.. _doxid-group__terminology_1te_abbreviations:

Abbreviations
~~~~~~~~~~~~~

====  ======================================

====  ======================================

API   Application Programming Interface
CLI   Command Line Interface
CSAP  Communication Service Access Point
      Device Under Testing
LAN   Local Area Network

RCF   Remote Control Facility
RGT   Report Generator Tool
TA    Test Agent
TAD   Traffic Application Domain of the RCFG
TCE   Test Coverage Estimation
TE    Test Environment
TEN   TE Engine
      Tester
====  ======================================





.. _doxid-group__terminology_1terminology:

Terminology
~~~~~~~~~~~

==================================  =================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================

==================================  =================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================

Communication Service Access Point  An object, which could be created on a Test Agent to send/receive some data (packets, frames, cells, CLI commands, etc.) A set of primitives to create and destroy CSAP, receive and send data is associated with each type of CSAP.
Device Under Test (DUT)             Station where a tested hardware/software is located.
RCF Application Domain              A semantically independent set of services provided by RCF (for example, TA configuration service). A set of API functions, protocol commands and routines of TA libraries for command handling, corresponds to a particular application domain. Some application domains may be unsupported by some Test Agents.
Test                                A complete sequence of actions required to achieve a specific purpose (e.g., a check that tested system provides a required functionality, complies to a standard, etc.) and producing a verdict pass/fail (possibly accompanied by additional data).
Test Package                        Group of tightly related tests or test packages, which may share internal libraries and usually run together (one-by-one or simultaneously). Test Package may consist of one test. It may have a prologue (performing some initialization) and epilogue (releasing resources and restoring TE configuration). @tdt_termTest Agent @tdt_defAn application running on the NUT or other station and performing some actions (configuring NUT or itself, interacting with the tested system, sending/receiving packets, etc.) according to instructions provided by the Test Engine. All interactions with the tested system should be performed via Test Agent only.
Test Engine                         Set of applications performing testing of tested subsystem according to Test Package(s) and configuration specified by a user. It is responsible for:
Test Protocol                       Protocol used for Test Engine and Test Agent interaction.
Test Environment (TE)               Software product, which includes:
Test Environment Subsystem          A mandatory and logically separate module (which may include software, data, documents) of the Test Environment responsible for one of main services provided by TE. The TE Subsystem can provide service to:
==================================  =================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================

