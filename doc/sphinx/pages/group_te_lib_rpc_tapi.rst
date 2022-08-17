..
  SPDX-License-Identifier: Apache-2.0
  Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

.. index:: pair: group; TAPI: Remote Procedure Calls (RPC)
.. _doxid-group__te__lib__rpc__tapi:

TAPI: Remote Procedure Calls (RPC)
==================================

.. toctree::
	:hidden:

	/generated/group_te_lib_rpcsock_macros.rst
	/generated/group_te_lib_rpc_rte_eal.rst
	/generated/group_te_lib_rpc_rte_ethdev.rst
	/generated/group_te_lib_rpc_rte_flow.rst
	/generated/group_te_lib_rpc_rte_mbuf.rst
	/generated/group_te_lib_rpc_rte_mempool.rst
	/generated/group_te_lib_rpc_rte_mbuf_ndn.rst
	/generated/group_te_lib_rpc_rte_ring.rst
	/generated/group_te_lib_rpc_aio.rst
	/generated/group_te_lib_rpc_dirent.rst
	/generated/group_te_lib_rpc_ifnameindex.rst
	/generated/group_te_lib_rpc_misc.rst
	/generated/group_te_lib_rpc_netdb.rst
	/generated/group_te_lib_rpc_winsock2.rst
	/generated/group_te_lib_rpc_dlfcn.rst
	/generated/group_te_lib_rpc_power_sw.rst
	/generated/group_te_lib_rpc_signal.rst
	/generated/group_te_lib_rpc_socket.rst
	/generated/group_te_lib_rpc_unistd.rst
	/generated/group_te_lib_rpc_stdio.rst



.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_introduction:

Introduction
~~~~~~~~~~~~

TAPI RPC library is an auxiliary library that hides the details of RPC encoding/decoding procedure. Its purpose is to provide an interface for tests that is almost equal to direct function calls made on test side.





.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework:

RPC Development Framework
~~~~~~~~~~~~~~~~~~~~~~~~~

In most cases a new test suite will require supporting a new set of RPC functions. In such cases you will need to extend RPC TAPI and here is a guidelines on how to do it.

Assume we would like to add RPC call for function **foobar()** that have the following prototype:

.. ref-code-block:: c

	enum foobar_type {
	    FOOBAR_TYPE_A,
	    FOOBAR_TYPE_B,
	};

	struct foobar_type1 {
	    enum foobar_type type;
	    int field;
	};

	int foobar(struct foobar_type1 *param1, const char *param2, int param3, int *param4_out);

Functions with array parameters are discussed in :ref:`Using arrays of variable length as in/out parameters of RPC calls<doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework_more_samples>`.

.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework_engine_side:

Support RPC call on Test Engine side
------------------------------------

First thing in supporting an RPC call is to define necessary data structures in include/tarpc.x.m4 file. According to the prototype of :ref:`rcf_rpc_call() <doxid-group__te__lib__rcfrpc_1ga3175cd6f2a0dce9edbc315c7ec1709af>` function we can see that to issue RPC we need to pass the following information:

* RPC name;

* RPC input argument;

* RPC output argument.

It means that for each function we need to define ONE data structure that will keep ALL input arguments and ONE data structure that will keep ALL output values (output arguments and return value). For our example this may look like the following:

.. ref-code-block:: c

	enum tarpc_foobar_type {
	    TARPC_FOOBAR_TYPE_A,
	    TARPC_FOOBAR_TYPE_B,
	};

	struct tarpc_foobar_type1 {
	    enum tarpc_foobar_type type;
	    tarpc_int field;
	};

	struct tarpc_foobar_in {
	    struct tarpc_in_arg common;

	    struct tarpc_foobar_type1 param1;
	    string                    param2<>;
	    tarpc_int                 param3;
	};

	struct tarpc_foobar_out {
	    struct tarpc_out_arg common;

	    tarpc_int param4;
	    tarpc_int retval;
	};

	/* Add foobar function in the list of RPC calls */
	program tarpc
	{
	    version ver0
	    {
	        ...
	        RPC_DEF(foobar)
	        ...
	    } = 1;
	} = 1;

Please note that each structure for input arguments should have struct :ref:`tarpc_in_arg <doxid-structtarpc__in__arg>` as the first field of the structure, each structure for output arguments should have struct :ref:`tarpc_out_arg <doxid-structtarpc__out__arg>` structure. The format of include/tarpc.x.m4 file complies with :ref:`XDR <doxid-structXDR>` format specified in RFC 1014.

Now we are ready to implement TAPI side for our **foobar()** function. Due to the fact that our target is to call **foobar()** function in the context of another process, we can't use the same name for the function implementation - it may conflict with such a function in our context. To sort this out we prefix all functions with **rpc\_** string.

Any TAPI RPC function should do the following things:

* define variable for IN arguments;

* define variable for OUT arguments;

* fill in fields of IN arguments variable;

* call :ref:`rcf_rpc_call() <doxid-group__te__lib__rcfrpc_1ga3175cd6f2a0dce9edbc315c7ec1709af>`;

* set corresponding output parameters and return value with values extracted from OUT arguments variable.

For the case of **foobar()** function the implementation of corresponding TAPI RPC function could be as following:

.. ref-code-block:: c

	int
	rpc_foobar(rcf_rpc_server *rpcs,
	           struct foobar_type1 *param1, const char *param2, int param3, int *param4_out)
	{
	    struct tarpc_foobar_in in;
	    struct tarpc_foobar_out out;

	    memset(&in, 0, sizeof(in));
	    memset(&out, 0, sizeof(out));

	    in.param1.type = param1_type_h2rpc(param1->type); /* Convert enum constants to RPC representation */
	    in.param1.field = param1->field; /* We can copy int value as is */
	    in.param2 = param2; /* Put a pointer to string (XRPC will calculate string length and copy it) */
	    in.param3 = param3; /* Copy int parameter as is */

	    rcf_rpc_call(rpcs, "foobar", &in, &out);

	    /* Check foobar() function returns value more or equal to -1 */
	    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(foobar, out.param4);

	    *param4_out = out.param4;

	    TAPI_RPC_LOG(rpcs, foobar, "%s:%d, %s, %d", "%d, %d",
	                 param1_type_h2str(param1->type), param1->field, param2, param3,
	                 out.param4, out.retval);

	    RETVAL_INT(foobar, out.retval);
	}

You should note the following things:

* an extra parameter ``rpcs`` has been added to **rpc_foobar()** function in order to be able to specify RPC Server on which to run **foobar()** function;

* enumeration type should be converted from host representation to corresponding RPC enumeration. This needs to be done in case such constants have different values on different hosts. For our example this is not obvious, but imagine we pass **errno** value, or protocol family type in socket API (``PF_INET``, ``PF_LOCAL``, etc.);

* parameters of integer type are not converted, but copied as is. This is obvious because there can be no conflicts between the value of this type of data on different hosts;

* parameters that represent string value are copied by XPRC library and we just put a pointer to the beginning of the string. Please note that to force XRPC library treat a parameter as a string to copy we shall specify that parameter in tarpc.x.m4 file in the following form:

  .. ref-code-block:: c

    string param_name<>;

* assuming we know the range of return value from a function, we can check the return value is in range with one of macros defined in ``lib/tapi_rpc/tapi_rpc_internal.h`` file. In our case we check that the return value is more or equal to -1 using :ref:`CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE() <doxid-tapi__rpc__internal_8h_1a2f7837d77b0450f140ef2b6bca0d739d>` macro. More generic macro to use is :ref:`CHECK_RETVAL_VAR() <doxid-tapi__rpc__internal_8h_1aae5b5613c9f12e0295f1a1c6d020dee4>`, but more likely you will find some predefined specific macro;

* right before returning a value from a function, we should provide log message about our function call. We specify value of input and output arguments and return value;

* return from a function should be done with one of macros defined in ``lib/tapi_rpc/tapi_rpc_internal.h`` file. In our case we return integer, which is why we use :ref:`RETVAL_INT() <doxid-tapi__rpc__internal_8h_1a5fa9a2e07acde98971d14a5fa8232b1b>` macro.

A lot of RPC data types and convert functions are defined under lib/rpc_types library. Mainly this library keeps data types to use while doing RPC calls for system functions with well-known prototypes (like libc functions or functions from POSIX API). The purpose of this library is to export header files analogue to system library files.

Assuming you are implementing RPC calls for some standard API, you should better have a look at lib/rpc_types library and create your own header (and if necessary C source) file with corresponding data types.





.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework_ta_side:

Support RPC call on Test Agent side
-----------------------------------

Now we are ready to implement server side for our **foobar()** function. Implementation of server side of RPC calls can be found under:

* for te_agents_unix - agents/unix/rpc/tarpc_server.c file;

* for te_agents_win - agents/win32/tarpc_server.c file.

To define a new RPC function call we should use TARPC_FUNC() macro.

For our sample the implementation can look like the following:

.. ref-code-block:: c

	TARPC_FUNC(foobar, {},
	{
	    struct foobar_type1 param1;

	    param1.type = param1_type_rpc2h(in->param1.type);
	    param1.field = in->param1.field;
	    MAKE_CALL(out->retval = func(&param1, in->param2, in->param3, &out->param4));
	}
	)

Please make sure that data types and values of arguments passed to a function call are native for the host where we run this function call. In our sample we created local variable ``param1`` of type ``foobar_type1`` in order to have correct value for the first argument of the function. Then we filled each field of this structure with native host values:

* ``foobar_type1::type`` field is set by means of helper function param1_type_rpc2h() that converts enum tarpc_foobar_type enumeration into native host enumeration of type enum foobar_type;

* ``foobar_type1::field`` field is set to the value got from RPC input argument structure. This is acceptable because it is just the value of integer type.

Please also note that for our sample we should better use ``func_ptr`` instead of ``func`` - refer to :ref:`Aspects of TARPC_FUNC() macro <doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework_ta_side_TARPC_FUNC>` section for the details.



.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework_ta_side_TARPC_FUNC:

Aspects of TARPC_FUNC() macro
+++++++++++++++++++++++++++++

In the context of code blocks represented as the second and the third argument of TARPC_FUNC() macro the following variables are defined:

* ``in`` of type **tarpc\_<func_name>_in** (for our **foobar** sample it would be **struct tarpc_foobar_in**) - keeps RPC input arguments for a call;

* ``out`` of type **tarpc\_<func_name>_out** (for our **foobar** sample it would be **struct tarpc_foobar_out**) - location for RPC output arguments of a call;

* ``func`` - pointer to an address of RPC function to be called. The type of this variable is :ref:`api_func <doxid-rpc__server_8h_1a926efd7c583bf0a5c162de5d3342895b>`, which assumes the first argument of the function is of integer type and the return value is of integer type. If your function does not match that prototype you shall use one of the following names while calling your function:

  * func_ptr;

  * func_void;

  * func_ret_ptr;

  * func_ptr_ret_ptr;

  * func_void_ret_ptr.

  The usage of the correct reference name is desired to fix compilation warning that might pop up due to implicit cast to a different function type. In our **foobar** sample we should have used func_ptr instead of ``func``, because **foobar()** function gets a pointer as the value of the first argument and returns an integer, which matches exactly :ref:`api_func_ptr <doxid-rpc__server_8h_1a0b20e8dd158dd4ee03cdb80e2d5bf452>` prototype. I.e. :ref:`MAKE_CALL() <doxid-rpc__server_8h_1a393830a65d04a95c1829f752bcfbcf04>` line in **foobar** sample would be:

  .. ref-code-block:: c

    MAKE_CALL(out->retval = func_ptr(&param1, in->param2, in->param3, &out->param4));

An address of function to be called (**func** variable in the content of TARPC_FUNC() macro) is searched among symbols of either user-specified library or among the symbols from an ordered set of objects consisting of the Test Agent image file, together with any objects loaded at program start-up as specified by that process image file (for example, shared libraries).

The place where to search for desired symbol is controlled with **in->common.use_libc** field that is tarpc_in_arg::use_libc, but from user point of view the type of library to use is controlled via RPC Server settings (read :ref:`Library name to resolve RPC call symbols <doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_usage_library_name>` for more information).

This feature allows you to provide alternative implementations of the same function in different dynamic libraries between which you can switch from test context.

Normally you would have functions statically linked with Test Agent. Regarding our **foobar** sample you could add **foobar()** function to an appropriate source file that will be later linked into Test Agent binary.

If you have a look at agents/unix/rpc/tarpc_server.c file you will find that together with TARPC_FUNC() code blocks it keeps implementation of some private functions to be called. For example check_port_is_free() function.







.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework_more_samples:

Using arrays of variable length as in/out parameters of RPC calls
-----------------------------------------------------------------

One thing that should also be explained is how to pass structures of variable length to and from RPC Server. Let's assume we need to pass some data buffer as input parameter and a function shall fill some output buffer in.

Let's implement an RPC function foo() that has the following prototype:

.. ref-code-block:: c

	void foo(uint8_t *in_buf, size_t in_buf_len, uint8_t *out_buf, size_t *out_buf_len);

include/tarpc.x.m4 file will have the following lines:

.. ref-code-block:: c

	struct tarpc_foo_in {
	    struct tarpc_in_arg common;

	    uint8_t in_buf<>; /* Input buffer content */
	    tarpc_size_t in_buf_len; /* The value of in_buf_len parameter */
	    uint8_t out_buf<>; /* Output buffer content */
	    tarpc_size_t out_buf_len<>; /* The value kept by out_buf_len address */
	};

	struct tarpc_foo_out {
	    struct tarpc_out_arg common;

	    uint8_t out_buf<>; /* Output buffer content */
	    tarpc_size_t out_buf_len<>; /* The value kept by out_buf_len address */
	};

Notes:

* We define ``rpc_foo_in::out_buf`` field in order to pass output buffer content to RPC server, i.e. on RPC Server side we would like to have an output buffer with the same pre-filled value as on RPC caller side. This is unnecessary in most of the cases and should be eliminated, but for example it can be useful when we want to check if a function call modifies output buffer or not in case a particular condition and how it modifies it.

* We define ``rpc_foo_in::out_buf_len`` field as an array in order to be able to track cases when ``out_buf_len`` value is ``NULL``. We could use two separate fields to get the same results:

  .. ref-code-block:: c

    tarpc_bool out_buf_len_null; /* TRUE when out_buf_len parameter is NULL */
    tarpc_size_t out_buf_len; /* The value of out_buf_len when it is not NULL */

* We define ``rpc_foo_in::out_buf`` array in ``rpc_foo_out`` data structure in order to be able to encode filled output buffer value;

* We define ``rpc_foo_in::out_buf_len`` field in ``rpc_foo_out`` data structure in order to pass updated value of ``out_buf_len`` parameter. We define it as an array in order to simplify server side implementation;

* We use the same names in in and out data structures for ``rpc_foo_in::out_buf`` and ``rpc_foo_in::out_buf_len`` fields to make it possible to use some auxiliary macros in implementation of server side.

TAPI implementation for our foo() function may look like the following:

.. ref-code-block:: c

	int
	rpc_foo(rcf_rpc_server *rpcs,
	        uint8_t *in_buf, size_t in_buf_len, uint8_t *out_buf, size_t *out_buf_len,
	        size_t rout_buf_len)
	{
	    struct tarpc_foo_in in;
	    struct tarpc_foo_out out;

	    memset(&in, 0, sizeof(in));
	    memset(&out, 0, sizeof(out));

	    /* Pass information about in_buf parameter */
	    in.in_buf.in_buf_len = (in_buf == NULL) ? 0 : in_buf_len;
	    in.in_buf.in_buf_val = in_buf;

	    /*
	     * Copy in_buf_len parameter independently on the value
	     * of in_buf parameter
	     */
	    in.in_buf_len = in_buf_len;

	    /*
	     * The size of out_buf buffer can be different from the value passed
	     * via out_buf_len parameter, which is why we added an extra
	     * parameter rout_buf_len. This parameter reflects the real size of
	     * out_buf that we need to transfer from RPC caller to RPC server.
	     */
	    in.out_buf.out_buf_len = (out_buf == NULL) ? 0 : rout_buf_len;
	    in.out_buf.out_buf_val = out_buf;

	    /*
	     * We use array data type to pass information whether
	     * the value of out_buf_len parameter is NULL or not.
	     * In our case out_buf_len_val is either NULL or a valid pointer.
	     * In case of NULL, the size of an array is 0, but when it is a valid
	     * data pointer, the size of an array will be 1, because we have only
	     * one element that needs to be passed to RPC server.
	     */
	    in.out_buf_len.out_buf_len_len = (out_buf_len == NULL) ? 0 : 1;
	    in.out_buf_len.out_buf_len_val = out_buf_len;

	    rcf_rpc_call(rpcs, "foo", &in, &out);

	    /*
	     * In case this was just a call without waiting for the result
	     * we should not copy output parameters (they are not ready yet).
	     */
	    if (RPC_IS_CALL_OK(rpcs) && rpcs->op != RCF_RPC_WAIT)
	    {
	        if (out_buf != NULL && out.out_buf.out_buf_val != NULL)
	            memcpy(out_buf, out.out_buf.out_buf_val, out.out_buf.out_buf_len));

	        if (out_buf_len != NULL)
	            *out_buf_len = out.out_buf_len.out_buf_len_val[0];
	    }

	    TAPI_RPC_LOG(rpcs, foo, "%p, %u, %p, %p", "",
	                 in_buf, in_buf_len, out_buf, out_buf_len);

	    RETVAL_VOID(foo);
	}

Please note the following:

* we do not make a copy of ``in_buf``, we just pass a pointer to its beginning, since it won't be changed or freed by the following functions;

* we encode ``rout_buf_len`` bytes of ``out_buf`` buffer to transfer between RPC client and server;

* we do not copy return value when the case was not blocking RPC start-up.

Server side implementation may look like the following:

.. ref-code-block:: c

	TARPC_FUNC(foo,
	{
	    COPY_ARG(out_buf);
	    COPY_ARG(out_buf_len);
	},
	{

	    /*
	     * Register out_buf for memory corruption in the out of buffer area
	     * (for the cases when real buffer length is more than the length
	     * passed via out_buf_len parameter).
	     */
	    INIT_CHECKED_ARG(out->out_buf.out_buf_val, out->out_buf.out_buf_len,
	                     (out->out_buf_len.out_buf_len_len == 0) ? 0 : out->out_buf_len.out_buf_len_val[0]);

	    MAKE_CALL(func(in->in_buf.in_buf_val, in->in_buf_len,
	                   (out->out_buf.out_buf_len == 0) ? NULL : out->out_buf.out_buf_val,
	                   (out->out_buf_len.out_buf_len_len == 0) ? NULL : out->out_buf_len.out_buf_len_val));
	}
	)

Notes:

* We copy ``out_buf`` and ``out_buf_len`` arguments from IN to OUT. You should note that for the case of RPC we must pass output arguments (if necessary) first in IN arguments data structure and then all the values function returns or updates are passed via OUT data structure (tarpc_foo_out structure). Because of this we need to copy the content of buffers that potentially could be updated by the function. Field names for such arguments should be the same in input and output arguments structures;

* We use INIT_CHECKED_ARG() macro to register our ``out_buf`` for validation against out of bound update - :ref:`MAKE_CALL() <doxid-rpc__server_8h_1a393830a65d04a95c1829f752bcfbcf04>` will automatically check that the data beyond specified buffer length is not updated;





.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework_errors:

Reporting errors from RPC calls
-------------------------------

Normally :ref:`MAKE_CALL() <doxid-rpc__server_8h_1a393830a65d04a95c1829f752bcfbcf04>` macro captures errno value and saves it in ``out->common._errno``. On Test Engine side you can get this error code from RPC server structure with the help of :ref:`RPC_ERRNO() <doxid-group__te__lib__rcfrpc_1ga98b23de539d6cac225a3d5c45ce8faa7>` macro (see :ref:`Handling expected errors <doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_usage_error_handling>`). So if you implement a simple RPC wrapper over a system function, such as socket(), you do not need to worry about error reporting.

However in case of complex RPC calls (when you have some function implemented on TA side which can fail in different places for various reasons), returning a negative value and some system errno code may not be specific enough. In this case you can use

.. ref-code-block:: c

	void te_rpc_error_set(te_errno err, const char *msg, ...);

to set an error code reported to the RPC caller, together with a string describing what happened. If this function is used, the error code set by it will be reported from :ref:`MAKE_CALL() <doxid-rpc__server_8h_1a393830a65d04a95c1829f752bcfbcf04>` instead of the errno value. Also there is

.. ref-code-block:: c

	te_errno te_rpc_error_get_num(void);

which returns the error code previously set by :ref:`te_rpc_error_set() <doxid-rpc__server_8h_1a9cb37f3bba0020bedb4e6999f4016098>`.

Please note that these functions work correctly only if they are called from the same thread in which :ref:`MAKE_CALL() <doxid-rpc__server_8h_1a393830a65d04a95c1829f752bcfbcf04>` is used.

On the test side the error data set with :ref:`te_rpc_error_set() <doxid-rpc__server_8h_1a9cb37f3bba0020bedb4e6999f4016098>` can be used in the following way:

.. ref-code-block:: c

	RPC_AWAIT_ERROR(rpcs);
	rc = rpc_func(rpcs, x, y, z);
	if (rc < 0)
	{
	    TEST_VERDICT("func() failed reporting error " RPC_ERROR_FMT,
	                 RPC_ERROR_ARGS(rpcs));
	}

In this case both the error code and the error string will be printed out in verdict. The error string description can also be obtained via :ref:`RPC_ERROR_MSG() <doxid-group__te__lib__rcfrpc_1gad24d189804e1369b4d639c6af15f8fee>` macro, while :ref:`RPC_ERRNO() <doxid-group__te__lib__rcfrpc_1ga98b23de539d6cac225a3d5c45ce8faa7>` macro can be used to access the error code.







.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_usage:

Some usage aspects of TAPI RPC library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_usage_error_handling:

Handling expected errors
------------------------

Each RPC call (TAPI RPC call) can finish with success or an error. Error condition is analyzed in the context of TAPI RPC function implementation. An error condition could be:

* timeout waiting for RPC call to finish;

* unexpected return code value or the value of some output parameters (controlled by TAPI RPC implementation logic);

* Test Agent crash.

By default if an error occurred during RPC call, TAPI RPC interrupts test execution and passes control to test termination block. Sometimes you would expect an error to be returned by a function and you would not want a test to be terminated. To tell TAPI RPC that you expect an error condition on RPC call you can use :ref:`RPC_AWAIT_IUT_ERROR() <doxid-group__te__lib__rcfrpc_1ga8fdf87f7fd729794cad82629cd2a4473>` macro. For example:

.. ref-code-block:: c

	/*
	 * Check that bind() returns an error when we try to bind a socket
	 * to an address that does not exist on a host.
	 */
	RPC_AWAIT_IUT_ERROR(tst_srv);
	rc = rpc_bind(tst_srv, sock_desc, SA(&alien_addr));
	if (rc != -1)
	    TEST_FAIL("bind() for an address not assigned to any host interface "
	              "returns %d instead of -1", rc);

	if (RPC_ERRNO(tst_srv) != RPC_EADDRNOTAVAIL)
	{
	    TEST_FAIL("bind() returns -1 when called for not assigned address, "
	              "but sets errno to %s instead of %s",
	              errno_rpc2str(RPC_ERRNO(tst_srv)),
	              errno_rpc2str(RPC_EADDRNOTAVAIL));
	}





.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_usage_non_blocking_calls:

Non blocking calls of RPC
-------------------------

In some tests you may have RPC calls that block until some external event happens and a test is the entity that should cause this external event. One possible solution would be to create a separate thread that blocks on RPC call while another thread takes care of necessary event generating. This approach could be the right choice for some complex test cases, but in most cases it is enough to use a non blocking RPC call with further calls to check/get return status. As soon as non blocking RPC call finishes we can continue our test sequence taking into account that there is a pending call on some RPC Server. The type of RPC call can be controlled with :ref:`rcf_rpc_server::op <doxid-structrcf__rpc__server_1a1abaf4ae738264d46d7ab8e08107b9bb>` field of RPC Server.

In the following example we do non blocking call for recv() function on one socket and then call send() function on another socket that causes recv() function to return.

.. ref-code-block:: c

	/*
	 * Tell rcv_srv to do non blocking call.
	 * It means that RPC Server of course blocks on this call,
	 * but RPC caller gets control immediately after launching
	 * this function on RPC Server side.
	 */
	rcv_srv->op = RCF_RPC_CALL;
	/* We do not check return value because this does not make sense right now */
	rpc_recv(rcv_srv, rx_sock, rx_buf, TST_BUF_LEN, 0);

	/* Make sure recv() call blocks */
	CHECK_RC(rcf_rpc_server_is_op_done(rcv_srv, &rcv_done));
	if (rcv_done)
	    TEST_FAIL("recv() call is expected to be blocked on incoming data event, but it is not");

	/* By default RPC call is run with RCF_RPC_CALL_WAIT, i.e. blocking call */
	rc = rpc_send(snd_srv, tx_sock, tx_buf, TST_BUF_LEN, 0);
	if (rc != TST_BUF_LEN)
	    TEST_FAIL("send() returns %d, but expected return value is %d", rc, TST_BUF_LEN);

	/* Check that recv() call finished */
	CHECK_RC(rcf_rpc_server_is_op_done(rcv_srv, &rcv_done));
	if (!rcv_done)
	    TEST_FAIL("recv() call is expected to return on incoming data, but it is not");

	/* Get results of recv() call */
	rcv_srv->op = RCF_RPC_WAIT;
	rc = rpc_recv(rcv_srv, rx_sock, rx_buf, TST_BUF_LEN, 0);
	if (rc != TST_BUF_LEN)
	    TEST_FAIL("recv() returns %d, but expected return value is %d", rc, TST_BUF_LEN);

	if (memcmp(rx_buf, tx_buf, TST_BUF_LEN) != 0)
	    TEST_FAIL("recv() returns buffer whose content does not match with "
	              "the content of transmitted buffer");





.. _doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_usage_library_name:

Library name to resolve RPC call symbols
----------------------------------------

By default RPC Server resolves symbols from Test Agent executable, which means it will take into account symbols of **libc** and other libraries linked together with Test Agent. Please note that here under the symbol to resolve we mean function whose address is set to **func** variable in the context of TARPC_FUNC() macro (read :ref:`Aspects of TARPC_FUNC() macro <doxid-group__te__lib__rpc__tapi_1tapi_rpc_lib_framework_ta_side_TARPC_FUNC>` for more information).

Here we will explain how to tell RPC Server to search RPC call symbol in user-specified library.

By default a newly created RPC Server searches for symbols in Test Agent binary, but as soon as you specify an alternative library it will use that library instead. To switch to an alternative library use :ref:`rcf_rpc_setlibname() <doxid-group__te__lib__rcfrpc_1ga93f11072f7e2bc2f87edce186c7d33ea>` function:

.. ref-code-block:: c

	rcf_rpc_server *rpc_srv;

	CHECK_RC(rcf_rpc_server_create(ta_name, "tst_srv", &rpc_srv));
	CHECK_RC(rcf_rpc_setlibname(rpc_srv, "/path/to/libmy_shared_library.so"));

You should keep in mind that on Test Agent side RPC Server will first call a code block defined under TARPC_FUNC() macro and inside this code block you can do a call to a function that was found by RPC Server in appropriate library.

Sometimes you may need to call a function from **libc** library without switching default dynamic library name. This can be done with the help of two fields of :ref:`rcf_rpc_server <doxid-structrcf__rpc__server>` data structure:

* :ref:`rcf_rpc_server::use_libc <doxid-structrcf__rpc__server_1a10dab8c239b2e2d1810a53d652afeb0f>` - to use **libc** library (and Test Agent global symbols) for name resolution in next RPC calls;

* :ref:`rcf_rpc_server::use_libc_once <doxid-structrcf__rpc__server_1a52fde4b7b1b9103d84da628f0f2b7414>` - to use **libc** library for name resolution only for the next RPC call.

For example:

.. ref-code-block:: c

	CHECK_RC(rcf_rpc_server_create(ta_name, "tst_srv", &rpc_srv));
	CHECK_RC(rcf_rpc_setlibname(rpc_srv, "/path/to/libmy_shared_library.so"));

	/* Call socket() function from libmy_shared_library.so */
	sock_1 = rpc_socket(rpc_srv, RPC_PF_INET, RPC_SOCK_STREAM, RPC_PROTO_DEF);

	/* Call socket() function from libc library */
	rpc_srv->use_libc_once = TRUE;
	sock_2 = rpc_socket(rpc_srv, RPC_PF_INET, RPC_SOCK_STREAM, RPC_PROTO_DEF);

	/* Next call will be again from libmy_shared_library.so */
	rpc_bind(rpc_srv, sock_1, bind_addr_1);

	/* To switch completely to libc we can do: */
	rpc_srv->use_libc = TRUE;
	rpc_bind(rpc_srv, sock_2, bind_addr_2);
	rpc_listen(rpc_srv, sock_2, 1);

	/* Now switch back to libmy_shared_library.so */
	rpc_srv->use_libc = FALSE;
	rpc_connect(rpc_srv, sock_1, bind_addr_2);

|	:ref:`Macros for socket API remote calls<doxid-group__te__lib__rpcsock__macros>`
|	:ref:`TAPI for RTE EAL API remote calls<doxid-group__te__lib__rpc__rte__eal>`
|	:ref:`TAPI for RTE Ethernet Device API remote calls<doxid-group__te__lib__rpc__rte__ethdev>`
|	:ref:`TAPI for RTE FLOW API remote calls<doxid-group__te__lib__rpc__rte__flow>`
|	:ref:`TAPI for RTE MBUF API remote calls<doxid-group__te__lib__rpc__rte__mbuf>`
|	:ref:`TAPI for RTE MEMPOOL API remote calls<doxid-group__te__lib__rpc__rte__mempool>`
|	:ref:`TAPI for RTE mbuf layer API remote calls<doxid-group__te__lib__rpc__rte__mbuf__ndn>`
|	:ref:`TAPI for RTE ring API remote calls<doxid-group__te__lib__rpc__rte__ring>`
|	:ref:`TAPI for asynchronous I/O calls<doxid-group__te__lib__rpc__aio>`
|	:ref:`TAPI for directory operation calls<doxid-group__te__lib__rpc__dirent>`
|	:ref:`TAPI for interface name/index calls<doxid-group__te__lib__rpc__ifnameindex>`
|	:ref:`TAPI for miscellaneous remote calls<doxid-group__te__lib__rpc__misc>`
|	:ref:`TAPI for name/address resolution remote calls<doxid-group__te__lib__rpc__netdb>`
|	:ref:`TAPI for remote calls of Winsock2-specific routines<doxid-group__te__lib__rpc__winsock2>`
|	:ref:`TAPI for remote calls of dynamic linking loader<doxid-group__te__lib__rpc__dlfcn>`
|	:ref:`TAPI for remote calls of power switch<doxid-group__te__lib__rpc__power__sw>`
|	:ref:`TAPI for signal and signal sets remote calls<doxid-group__te__lib__rpc__signal>`
|	:ref:`TAPI for socket API remote calls<doxid-group__te__lib__rpc__socket>`
|	:ref:`TAPI for some file operations calls<doxid-group__te__lib__rpc__unistd>`
|	:ref:`TAPI for standard I/O calls<doxid-group__te__lib__rpc__stdio>`


