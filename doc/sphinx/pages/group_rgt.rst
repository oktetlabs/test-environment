.. index:: pair: group; Report Generator Tool
.. _doxid-group__rgt:

Report Generator Tool
=====================

.. toctree::
	:hidden:



.. _doxid-group__rgt_1rgt_introduction:

Introduction
~~~~~~~~~~~~

All TE subsystems (except for :ref:`Dispatcher <doxid-group__te__engine__dispatcher>` as it's a shell script) and Agents generate log in **RAW** format. All the logs are gathered by the :ref:`Logger <doxid-group__te__engine__logger>` and written into tmp_raw_log file.

Report Generator Tool (RGT) is a software package that generates human-convenient representations of raw log format.

It supports:

* conversion into XML, HTML and plain text formats;

* run-time filtering;

* live and postponed log processing modes.





.. _doxid-group__rgt_1rgt_logs_processing:

Logs Processing Logic
~~~~~~~~~~~~~~~~~~~~~

The below diagram shows the logs processing logic used by the RGT.

.. image:: /static/image/rgt_processing_logic.png
	:alt: RGT log processing logic

**Coarse TXT report** is a report without smaller amount of logic (no expected results etc). However it can be generated on fly.





.. _doxid-group__rgt_1rgt_output_formats:

Output Formats
~~~~~~~~~~~~~~

As it was mentioned above there are two main output formats:

* HTML (single or multi-file)

* TXT



.. _doxid-group__rgt_1rgt_output_formats_txt:

Text Format
-----------

Usually the test package has a wrapper script called log.sh which is located in the scripts/ directory.

Example of text log:

.. code-block:: none


	ERROR  Tester  Run Path  16:15:50 592 ms
	Test path requested by user not found.
	Path: foobar-ts/foo_package/mytest

See  for details on the log format.

To generate text log one can usually use scripts/log.sh or log.sh wrapper script.





.. _doxid-group__rgt_1rgt_output_formats_html:

HTML Format
-----------

HTML log can be generated with scripts/html-log.sh or html-log.sh wrapper script. Or you can pass **--log-html=[output_dir_name]** to the dispatcher.sh.

This will generate a huge amount of html files in the output_dir_name or in html/ if no explicit path was specified. index.html is the staring point.

See example of the HTML log below:

.. image:: /static/image/rgt_html_report.png
	:alt: HTML report example

On the left there is a test tree. Tests are groupped by packages. Inside package there may be several sessions. See  for details on the file format.

If you click on a test on the left frame corresponding log will open on the right one.

**Log Filter** button allows you to filter the logs on particular page. This may not work well in certain browsers.







.. _doxid-group__rgt_1rgt_log_bundle:

Using raw log bundle
~~~~~~~~~~~~~~~~~~~~

Night testing can produce raw logs of large size (hundreds of megabytes or even gigabytes). Typically we want to view HTML log for just one iteration at a time, not for all the iterations simultaneously. However if we use log processing scheme described above, we have to process full raw log and it can take significant amount of time for large file.

To allow processing of just part of raw log related to selected test iteration, a method of splitting raw log into fragments and archiving (using **pixz**) these fragments into “raw log bundle” was introduced. With such raw log bundle, when we need to produce HTML log for a single test iteration, we can extract from the bundle only log fragments related to a given iteration, concatenate them into shortened raw log and process it with rgt-conv and rgt-xml2html-multi.

RGT contains the following scripts to work with such raw log bundles:

**1**. rgt-log-bundle-create

.. ref-code-block:: shell

	rgt-log-bundle-create --raw-log=log.raw[.bz2] \
	    --bundle=raw_log_bundle.tpxz

This creates raw log bundle raw_log_bundle.tpxz from specified raw log (possibly archived with bzip2).

**2**. rgt-log-bundle-get-original

.. ref-code-block:: shell

	rgt-log-bundle-get-original --bundle=raw_log_bundle.tpxz --output=log.raw

This recovers original raw log used to create raw_log_bundle.tpxz with rgt-log-bundle-create.

**3**. rgt-log-bundle-get-item

This script is used to obtain requested text or HTML logs from raw log bundle.

rgt-log-bundle-get-item is expected to be used in "Error 404" web server handler so that when you select another node in log tree (or move to another page of big HTML log) it automatically calls rgt-log-bundle-get-item to obtain requested file and redirects to it.

The script can be used in following ways:

**3.1**.

.. ref-code-block:: shell

	rgt-log-bundle-get-item --bundle=raw_log_bundle.tpxz \
	    --req-path=some_dir/log.txt

This produces log in text format.

**3.2**.

.. ref-code-block:: shell

	rgt-log-bundle-get-item --bundle=raw_log_bundle.tpxz \
	    --req-path=some_dir/html/

This will produce index files for HTML logs (such as index.html, files containing information about HTML logs tree, etc).

**3.3**.

.. ref-code-block:: shell

	rgt-log-bundle-get-item --bundle=raw_log_bundle.tpxz \
	    --req-path=some_dir/html/node_43.html

This will produce not only index files but also HTML log for a test iteration with TIN=43 (node_43.html).

**3.4**.

.. ref-code-block:: shell

	rgt-log-bundle-get-item --bundle=raw_log_bundle.tpxz \
	    --req-path=some_dir/html/node_1_0.html

This will produce not only index files but also HTML log for a log node with depth = 1 and seq = 0 (node_1_0.html, root log node). We use addressing by depth/seq (depth in log nodes tree and sequential number in the list of children of node's parent) for log nodes which do not have TIN (such as packages and sessions).

**3.5**. If HTML log is big (i.e. raw log from which it is produced has size of more than about 1Mb), it is by default split into pages and only the first page is produced by (3.3) and (3.4). In this case next pages can be obtained by calling rgt-log-bundle-get-item again with **req-path=.../[node_name]_pN.html** (where N is page number > 1). For example,

.. ref-code-block:: shell

	rgt-log-bundle-get-item --bundle=raw_log_bundle.tpxz \
	    --req-path=some_dir/html/node_43_p2.html

Also it is possible to request full HTML log in the single file by running

.. ref-code-block:: shell

	rgt-log-bundle-get-item --bundle=raw_log_bundle.tpxz \
	    --req-path=some_dir/html/node_43_all.html

Also rgt-log-bundle-get-item has optional **shared-url** and **docs-url** arguments which are passed to rgt-xml2html-multi (see its documentation or help for their meaning).

