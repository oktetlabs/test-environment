.. index:: pair: group; Console Log Level Configuration
.. _doxid-group__console__ll:

Console Log Level Configuration
===============================

.. toctree::
	:hidden:



.. _doxid-group__console__ll_1console_ll_intro:

Introduction
~~~~~~~~~~~~

The kernel routine **printk()** will only print a message on the console, if it has a **loglevel** less than the value of the variable **console_loglevel**. The **Test Environment** provides possibility of configure the **console_loglevel** variable on the system, where is launched **agent**. For more information about kernel log levels see the **printk(9)**, **klogctl(3)** and **/proc/sys/kernel/printk** documentation.



.. _doxid-group__console__ll_1console_ll_intro_table:

Table of levels
---------------

======  ============  ================================
Number  Name          Description
======  ============  ================================
0       KERN_EMERG    System is unuseable
1       KERN_ALERT    Action must be taken immediately
2       KERN_CRIT     Critical conditions
3       KERN_ERR      Error conditions
4       KERN_WARNING  Warning conditions
5       KERN_NOTICE   Normal but significant condition
6       KERN_INFO     Informational
7       KERN_DEBUG    Debug-level messages
======  ============  ================================







.. _doxid-group__console__ll_1console_ll_obj:

Configurator object
~~~~~~~~~~~~~~~~~~~

There is a new object to set log level value. The object **console_loglevel** is located on the **/agent/sys** subtree. Make sure that the objects are registered:

.. ref-code-block:: cpp

	<?xml version="1.0"?>
	<history>
	  <register>
	    <object oid="/agent/sys" access="read_only" type="none"/>
	    <object oid="/agent/sys/console_loglevel" access="read_write" type="integer"/>
	  </register>
	</history>





.. _doxid-group__console__ll_1console_ll_howto:

How to use
~~~~~~~~~~

<note>The **loglevel** value should be a digit in the range 1-8. See the [[te:console_loglevel::table_of_levels\|Table of levels]] section. </note>



.. _doxid-group__console__ll_1console_ll_howto_configurator:

Configurator
------------

To set a console log level - write the level number as value field of instance. See the following example.

.. ref-code-block:: cpp

	<set>
	  <instance oid="/agent:Agt_A/sys:/console_loglevel:" value="4"/>
	</set>





.. _doxid-group__console__ll_1console_ll_howto_tapi:

Test API
--------

Also can be used the Test API function shown below.

.. ref-code-block:: cpp

	/**
	 * Set the console log level
	 *
	 * @param agent    Test agent name
	 * @param level    Console log level (See printk(9))
	 *
	 * @return Status code
	 * @retval 0            Success
	 */
	te_errno
	tapi_cfg_set_loglevel(const char *agent, int level);





.. _doxid-group__console__ll_1console_ll_howto_eg:

Example
-------

The example demonstrates how to change the console log level from **test**.

.. ref-code-block:: cpp

	#include "tapi_cfg.h"

	tapi_cfg_set_loglevel("Agt_A", 5);

