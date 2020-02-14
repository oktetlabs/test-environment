.. index:: pair: group; Packet Sniffer
.. _doxid-group__sniffer:

Packet Sniffer
==============

.. toctree::
	:hidden:

The main purpose of the sniffer framework is to provide infrastructure for sniffing on the network interfaces and storing information about the packets sent/received there.



.. _doxid-group__sniffer_1sniffer_launch:

How to launch
~~~~~~~~~~~~~

To use the sniffer framework make sure that the three following points are performed.



.. _doxid-group__sniffer_1sniffer_build:

Build options
-------------

To compile agents with support of the sniffer framework should be added option **with-sniffers** in two places. As described **TA** options and library options for the **rcfpch** library. See example:

.. ref-code-block:: cpp

	TE_LIB_PARMS([rcfpch], [], [], [--with-sniffers], [], [], [])
	TE_TA_TYPE([linux], [], [unix], [--with-sniffers=yes], [], [], [], [])

If are used a few platforms or agents types the options should be added for each agent and platform. If the TE is built with sniffer framework and some agents do not support the sniffer framework, then this agents should be built with **with-sniffers=sniffers_dummy** option. See example:

.. ref-code-block:: cpp

	TE_LIB_PARMS([rcfpch], [], [], [--with-sniffers], [], [], [])
	# Agent with Power Unit support, it do not support the sniffer framework.
	TE_TA_TYPE([power_ctl], [], [power-ctl], [--with-tad=tad_dummy --with-sniffers=sniffers_dummy],
	           [], [], [], [comm_net_agent])

	TE_LIB_PARMS([rcfpch], [linux_other], [], [--with-sniffers], [], [], [])
	TE_TA_TYPE([linux64], [linux_other], [unix], [--with-sniffers=yes], [], [], [], [])





.. _doxid-group__sniffer_1sniffer_logger_config:

Logger config
-------------

The **Logger** config file has mandatory part. This is mandatory, even if you do not use the sniffer framework (due to the design of the framework). The part contains default and user settings for sniffer polling process. A more detailed description of the **Logger** configurations see below in the **Logger settings** :ref:`Logger settings <doxid-group__sniffer_1sniffer_logger_settings>` section.





.. _doxid-group__sniffer_1sniffer_conf_subtrees:

Configurator objects subtree
----------------------------

Config file of the **Configurator** should contains the following objects subtrees:

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	<history>
	  <register>
	    <object oid="/agent/sniffer_settings" access="read_only" type="none"/>
	    <object oid="/agent/sniffer_settings/enable" access="read_write" type="integer"/>
	    <object oid="/agent/sniffer_settings/filter_exp_str" access="read_write" type="string"/>
	    <object oid="/agent/sniffer_settings/filter_exp_file" access="read_write" type="string"/>
	    <object oid="/agent/sniffer_settings/snaplen" access="read_write" type="integer"/>
	    <object oid="/agent/sniffer_settings/tmp_logs" access="read_only" type="none"/>
	    <object oid="/agent/sniffer_settings/tmp_logs/path" access="read_write" type="string"/>
	    <object oid="/agent/sniffer_settings/tmp_logs/file_size" access="read_write" type="integer"/>
	    <object oid="/agent/sniffer_settings/tmp_logs/total_size" access="read_write" type="integer"/>
	    <object oid="/agent/sniffer_settings/tmp_logs/rotation" access="read_write" type="integer"/>
	    <object oid="/agent/sniffer_settings/tmp_logs/overfill_meth" access="read_write" type="integer"/>
	  </register>
	  <register>
	    <object oid="/agent/interface/sniffer" access="read_create" type="integer"/>
	    <object oid="/agent/interface/sniffer/enable" access="read_write" type="integer"/>
	    <object oid="/agent/interface/sniffer/filter_exp_str" access="read_write" type="string"/>
	    <object oid="/agent/interface/sniffer/filter_exp_file" access="read_write" type="string"/>
	    <object oid="/agent/interface/sniffer/snaplen" access="read_write" type="integer"/>
	    <object oid="/agent/interface/sniffer/tmp_logs" access="read_only" type="none"/>
	    <object oid="/agent/interface/sniffer/tmp_logs/sniffer_space" access="read_write" type="integer"/>
	    <object oid="/agent/interface/sniffer/tmp_logs/file_size" access="read_write" type="integer"/>
	    <object oid="/agent/interface/sniffer/tmp_logs/rotation" access="read_write" type="integer"/>
	    <object oid="/agent/interface/sniffer/tmp_logs/overfill_meth" access="read_write" type="integer"/>
	  </register>
	</history>

After each launch of the TE, automatically generates a file **сs.сonf.sniffer**. That is used to launch sniffers configured by the **Dispatcher** options. If there is no sniffers launched by **Dispatcher**, then the file will contains only xml header and the open/close history tags.







.. _doxid-group__sniffer_1sniffer_use_cases:

Simple use cases
~~~~~~~~~~~~~~~~

Sniffer can be started by few ways. Here some examples to launch sniffer. By default all capture logs location is **TE_LOG_DIR/caps**. A detailed description about setting up and running sniffers see below in the respective sections.

* Example to start the sniffer on the agent **Agt_B** for interface **eth0** using the **Dispatcher** options:

  .. ref-code-block:: cpp

  	./dispatcher.sh --sniff=Agt_B/eth0

* It is possible to setup the started sniffer from cli. Now the sniffer will be have name **clisniff** and filter expression "ip and udp":

  .. ref-code-block:: cpp

  	./dispatcher.sh --sniff=Agt_B/eth0 --sniff-name=clisniff --sniff-filter="ip and udp"

* To add and launch a sniffer from Configurator, should to describe the sniffer in the conf file of Configurator. There must be added the sniffer instance. To launch the sniffer **enable** field in the sniffer subtree should be set to 1. The example to add and launch the sniffer on the agent **Agt_A** for interface **lo** with name **mysniffer** :

  .. ref-code-block:: cpp

  	<add>
  	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer" value="0"/>
  	</add>
  	<set>
  	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer/enable:" value="1"/>
  	</set>

* It is possible to start a sniffer from Test API. The example demonstrates how to add/del a new sniffer with name **newsniffer** for agent **Agt_B** on the interface **lo**. The sniffer will be use the filter expression "<b>ip and udp</b>" and overfill handle method for temporary files - **rotation**.

  .. ref-code-block:: cpp

  	#include "tapi_sniffer.h"

  	tapi_sniffer_id    *snif;
  	snif = tapi_sniffer_add("Agt_B", "lo", "newsniffer", "ip and udp", 0);
  	tapi_sniffer_del(snif);





.. _doxid-group__sniffer_1sniffer_configuration:

Configuration
~~~~~~~~~~~~~

Configurations of sniffer framework divided on two main parts. A first part is worked on the agent side. It includes options responsible to add and configuration of sniffer processes. The management performed via the **Configurator**. There are few ways to create and configure sniffers. This can be done via the command line of **Dispatcher**, via the config file of **Configurator** or directly from a test application using TAPI. The second part of the framework is worked on the **TEN** side in the **Logger** subsystem. It performs collect of logs from all sniffers. Setting of this part happens at start of the **Logger**. Configuration for it can be passed by means of a configuration file or using command line options of the **Dispatcher**. By default capture logs location is **TE_LOG_DIR/caps**.



.. _doxid-group__sniffer_1sniffer_cli:

Command line options for Dispatcher
-----------------------------------

Additional command line options for dispatcher.sh are supported. It is possible to add sniffers and set their settings by command line options. The same settings of the **Logger** can be changed here. ==============================================  =====================================================================================

==============================================  =====================================================================================

–sniff-not-feed-conf                          Do not feed the sniffer configuration file to Configurator.
–sniff=<TA/iface>                             Run sniffer on
–sniff-filter=<filter>                        Add for the sniffer filter(tcpdump-like syntax). See &#039;man 7 pcap-filter&#039;.
–sniff-name=<name>                            Add for the sniffer a human-readable name.
–sniff-snaplen=<val>                          Add for the sniffer restriction on maximum number of bytes to capture for one packet.
By default: unlimited.
–sniff-space=<val>                            Add for the sniffer restriction on maximum overall size of temporary files in Mb.
By default: 64Mb.
–sniff-fsize=<val>                            Add for the sniffer restriction on maximum size of the one temporary file in Mb.
By default: 16Mb.
–sniff-rotation=<x>                           Add for the sniffer restriction on number of temporary files. This option excluded by
the
–sniff-ofill-drop                             Change overfill handle method of temporary files for the sniffer to tail drop.
By default overfill handle method is rotation.
–sniff-log-dir=<path>                         Path to the
By default used: TE_LOG_DIR/caps.
–sniff-log-name <pattern>
- %a : agent name
- %u : user name
- %i : interface name
- %s : sniffer name
- %n : sniffer session sequence number
By default
–sniff-log-size=<val>                         Maximum
By default: 256 Mb.
–sniff-log-fsize=<val>                        Maximum
By default: 64 Mb.
–sniff-log-ofill-drop                         Change overfill handle method to tail drop.
By default overfill handle method is rotation.
–sniff-log-period=<val>                       Period of taken logs from agents in milliseconds.
By default: 200 msec.
–sniff-log-conv-disable                       Disable capture logs conversion and merge with the main log.
==============================================  =====================================================================================

Example how to add and configure a sniffer:

.. ref-code-block:: cpp

	./dispatcher.sh --sniff=Agt_B/eth0 --sniff-filter="ip and udp" --sniff-name=clisniff
	--sniff-snaplen=500 --sniff-space=300 --sniff-fsize=3 --sniff-rotation=3

Example how to configure **TEN** side capture logs polling settings and location:

.. ref-code-block:: cpp

	./dispatcher.sh --sniff-log-name=%n_%i_%s_%a --sniff-log-size=500
	--sniff-log-fsize=100 --sniff-log-period=150 --sniff-log-dir=/tmp

If are you launch the sniffers by the command line options for an interfaces, should be sure that the interfaces are configuring, while Configurator processing the conf files. Otherwise, if the interfaces are configured later, for example in the prolog, should to use command line option **sniff-not-feed-conf** to configure and launch the sniffers later. To activate sniffers configurations at the right time can be used the **confapi**. Use the following code to launch command line sniffers when it is needed (e.g. in prolog):

.. ref-code-block:: cpp

	char *te_sniff_csconf = getenv("TE_SNIFF_CSCONF");
	if (te_sniff_csconf != NULL)
	{
	    CHECK_RC(cfg_process_history(te_sniff_csconf, NULL));
	}





.. _doxid-group__sniffer_1sniffer_env:

Environments
------------

Environment **TE_SNIFF_TSHARK_OPTS** can be used to pass arbitrary options to **tshark** utility. **tshark** is used for processing binary capture, so its options can influence to printed in logs data.

For example, the following option can be used to print absolute TCP sequence and ACK numbers instead of relative (by default):

.. ref-code-block:: cpp

	TE_SNIFF_TSHARK_OPTS="-o tcp.relative_sequence_numbers:FALSE"





.. _doxid-group__sniffer_1sniffer_logger_settings:

Logger settings
---------------

Polling process and result capture logs location is configured by XML file. By default used **logger.conf** from the **conf/** directory, but config file path may be changed by **Dispatcher** command line option **conf-logger=<filename>**. The same from command line may be changed other settings of **Logger**. The command line options have a higher priority. Settings described in the table below. ======================================  ===================================================================

======================================  ===================================================================

snif_fname
- %a : agent name
- %u : user name
- %i : interface name
- %s : sniffer name
- %n : sniffer session sequence number
By default
snif_path                               Path to the
By default: TE_LOG_DIR/caps
snif_max_fsize                          Max file size for one sniffer in Mb.
By default: 64 Mb.
snif_space                              Max total capture files size for one sniffer in Mb.
By default: 256 Mb.
snif_rotation                           Rotate logger side temporary logs across
snif_overall_size                       Max total files size for all sniffers in Mb (Is not supported now).
By default: unlimited.
snif_ovefill_meth                       Overfill handle method:
0 - rotation(default)
1 - tail drop.
snif_period                             Period of taken logs from agents in milliseconds.
By default: 200 msec.
======================================  ===================================================================

Configuration file contains the set of default settings and set of user setting. Example of user settings setup is below.

.. ref-code-block:: cpp

	<userSnifferSets>
	     <snif_fname value="%a_%i_%n_%s"/>
	     <snif_path value="/home/andrey/work/trunk/caps"/>
	     <snif_max_fsize value="250"/>
	     <snif_space value="500"/>
	     <snif_rotation value="0"/>
	     <snif_overall_size value="40"/>
	     <snif_ovefill_meth value="0"/>
	     <snif_period value="150"/>
	</userSnifferSets>





.. _doxid-group__sniffer_1sniffer_configurator_trees:

Configurator trees
------------------

Two configuration subtrees are added to the agent configuration model. Generic configuration subtree /agent/sniffer_settings/ which represents agent-wide sniffer configuration. Sniffers uses some of these fields by default if the value to them is not defined personally. ==============================================  ===========================================================

==============================================  ===========================================================

/agent/sniffer_settings                         Sniffer object.
/agent/sniffer_settings/enable                  Enable the sniffer settings lock.
/agent/sniffer_settings/filter_exp_str          Filter expression string, by default: empty.
/agent/sniffer_settings/filter_exp_file         Filter file contains expression tcpdump-like filter syntax.
/agent/sniffer_settings/snaplen                 Maximum packet capture size for all sniffers.
By default unlimited.
/agent/sniffer_settings/tmp_logs                Dump files settings.
/agent/sniffer_settings/tmp_logs/path           Path to temporary capture files.
/agent/sniffer_settings/tmp_logs/total_size     Max total capture files size for all agent sniffers, Mb.
By defualt 256 Mb.
/agent/sniffer_settings/tmp_logs/file_size      Max capture file size in Mb, by default 16 Mb.
/agent/sniffer_settings/tmp_logs/rotation       Rotate agent side temporary logs across
By defaul
/agent/sniffer_settings/tmp_logs/overfill_meth  Overfill handle method:
0 - rotation(default)
1 - tail drop.
==============================================  ===========================================================

Example of subtree:

.. ref-code-block:: cpp

	<set>
	  <instance oid="/agent:Agt_A/sniffer_settings:/filter_exp_str:" value=""/>
	  <instance oid="/agent:Agt_A/sniffer_settings:/snaplen:" value="300"/>
	  <instance oid="/agent:Agt_A/sniffer_settings:/tmp_logs:/file_size:" value="5"/>
	  <instance oid="/agent:Agt_A/sniffer_settings:/tmp_logs:/path:" value="tmp/"/>
	  <instance oid="/agent:Agt_A/sniffer_settings:/tmp_logs:/total_size:" value="3"/>
	  <instance oid="/agent:Agt_A/sniffer_settings:/tmp_logs:/rotation:" value="4"/>
	  <instance oid="/agent:Agt_A/sniffer_settings:/tmp_logs:/overfill_meth:" value="0"/>
	</set>

Plus a per-interface /agent/interface/sniffer/ subtree which is responsible for configuration of a specific sniffer instances is added. Each sniffer instance in the configurato subtree has a **sniffer name** which is by design (cause it's a name of the object instance) uniq across the interface. ===============================================  ===========================================================

===============================================  ===========================================================

/agent/interface/sniffer                         Sniffer name contains read only value of
/agent/interface/sniffer/enable                  Enable the sniffer.
/agent/interface/sniffer/filter_exp_str          Filter expression string, by default: empty.
/agent/interface/sniffer/filter_exp_file         Filter file contains expression tcpdump-like filter syntax.
/agent/interface/sniffer/snaplen                 Maximum packet capture size in bytes, by default unlimited.
/agent/interface/sniffer/tmp_logs                Dump files settings subtree.
/agent/interface/sniffer/tmp_logs/sniffer_space  Max total dump files size for the sniffer in Mb.
By defualt 64 Mb.
/agent/interface/sniffer/tmp_logs/file_size      Max capture file size (Mb), by default is 16 Mb.
/agent/interface/sniffer/tmp_logs/rotation       Rotate agent side temporary logs across
By defaul
/agent/interface/sniffer/tmp_logs/overfill_meth  Overfill handle method:
0 - rotation(default)
1 - tail drop.
===============================================  ===========================================================

Example of subtree:

.. ref-code-block:: cpp

	<add>
	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer" value="0"/>
	</add>
	<set>
	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer/filter_exp_str:"
	                value="ip or udp or tcp"/>
	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer/snaplen:" value="300"/>
	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer/tmp_logs:/sniffer_space:" value="6"/>
	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer/tmp_logs:/file_size:" value="2"/>
	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer/tmp_logs:/rotation:" value="3"/>
	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer/tmp_logs:/overfill_meth:" value="1"/>
	  <instance oid="/agent:Agt_A/interface:lo/sniffer:mysniffer/enable:" value="1"/>
	</set>



* If the same settings declarated in both subtrees (file_size, overfill_type, snaplen), use the one that declarated in the interface subtree.

* **sniffer_space**, **file_size** and **rotation** are restricting the parameters. By default this parameters means that sniffer capture files can take no more than 64Mb space on the disk, size of the one file can not exceed 16 Mb and can be created not more than 4 files.





.. _doxid-group__sniffer_1sniffer_tapi:

Sniffer Test API
----------------

The Test API provides ability add/remove sniffers, suspend/resume the work of sniffers, insert mark packets to logs. It is possible to manage of sniffers via **Configurator** TAPI. Note, during the sniffer is working, its settings cannot be changed. The complete list and detailed description functions of Test API you can find in ``lib/tapi/tapi_sniffer.h`` file.

The example below demonstrates how to add a new sniffer with name **newsniffer** for agent **Agt_B** on the interface **lo**. The sniffer will be use the filter expression **"ip and udp"** and overfill handle method for temporary files - **rotation**. After starting the sniffer is called a request to insert a marker packet. Then the work of the sniffer is suspended and renewable. And at the end the sniffer is removed.

.. ref-code-block:: cpp

	#include "tapi_sniffer.h"

	tapi_sniffer_id    *snif;

	snif = tapi_sniffer_add("Agt_B", "lo", "newsniffer", "ip", TRUE); /* Add new sniffer */
	tapi_sniffer_mark(NULL, snif, "My first marker packet.");         /* Insert marker packet */
	tapi_sniffer_stop(snif);                                          /* Suspend the sniffer work*/
	tapi_sniffer_start(snif);                                         /* Renew the sniffer work */
	tapi_sniffer_del(snif)                                            /* Delete the sniffer */

