[SPDX-License-Identifier: Apache-2.0]::
[Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.]::

## Introduction

This document supplements the general Coding Style defined in
[C Style Guide](https://github.com/oktetlabs/styleguides/blob/master/cguide.md).

It contains a collection of guidelines and best practices that
new contributions to TE should follow. Note that TE may contain a lot of
old legacy code that does not comply with these guidelines; in many
cases it may be more important to stay consistent with the old code
than to adhere to these recommendations strictly.

## Naming conventions

Names of externally visible entities should start with a prefix.
Below is the table of the most common prefixes (of course, if a name is
uppercase, the prefix will be uppercase as well):

|Prefix      |Description                              |Sample modules                          |
|------------|-----------------------------------------|----------------------------------------|
|`te_`       | code usable in the whole TE             | `lib/tools`, `include/te_defs.h`       |
|`tapi_`     | general TAPI functions                  | `lib/tapi*`                            |
|`tapi_cfg_` | TAPI wrappers around Configurator nodes | `lib/tapi/tapi_cfg_*`                  |
|`rpc_`      | Engine-side RPC wrappers                | `lib/tapi_rpc`                         |
|`cfg_`      | Engine side of the Configurator         | `engine/configurator`, `lib/confapi`   |
|`ta_`       | general agent-side code                 | `agents/*`, `lib/agentlib`, `lib/ta_*` |
|`rcf_`      | Engine and agent-side RCF code          | `engine/rcf`, `lib/rcf*`               |
|`tad`       | agent-side TAD code                     | `lib/tad`                              |
|`tester_`   | Tester code                             | `engine/tester`                        |
|`trc_`      | TRC-related code                        | `lib/trc`, `tools/trc`                 |

## Types

### Boolean

In TE `te_bool` should be used instead of C standard `bool`.
The corresponding Boolean constants are `TRUE` and `FALSE`,
not `true` and `false`.

This is due to purely historical reasons.
`te_bool` is guaranteed to be compatible with C standard `bool`.

## Error handling

### Status code

Functions that return statuses should return a value of `te_errno`
with a well-defined status code, not a negative value of `int`.

`te_errno` code should be composed of a proper module code and
an error code using the `TE_RC` macro. Low-level common functions
e.g. from `lib/tools` are allowed to return a bare error code with
no module.

A caller must not ignore the status code of such functions, unless
the documentation of a function explicitly allows it or if it is able
to ensure that no error is possible in a given context (that's most
probably only feasible for intra-module calls).

The caller should not compare the obtained status with an error code
value, i.e. the following is generally incorrect due to a possible
non-zero module code:

```
if (rc == TE_ENOENT)
```

`TE_RC_GET_ERROR()` should be used instead like this:

```
if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
```

However, a successful error code is always zero without a module code,
so the following check is correct:

```
if (rc == 0)
```

If a function may fail for different reasons, it's better to return
distinct error codes for different abnormal conditions, and not something
generic like `TE_EFAIL`.

Avoid returning errors that a caller would not be able to handle properly,
such as when the contract of a function is broken: such conditions are better
handled with `assert()` or `TE_FATAL_ERROR()` (see below).

If a function fails due to a system API error, it should propagate the
exact `errno` value with `TE_OS_RC()`, not substitute some ad hoc code.

#### Some specific error code gotchas

- `TE_EFAULT` --- despite its name, it's not a "generic error" code,
  it means "Bad address" and is more or less only appropriate if a pointer
  has unexpected `NULL` value.
- `TE_ENOMEM` and `TE_NOSPC` should not be used to indicate insufficient
  space in some buffer, `TE_ENOBUFS` exists for that purpose.
- `TE_EBADTYPE` despite its generic name should only be used to report
  an invalid/unsupported Configurator object type.
- `TE_EFAIL` is the code for a generic error, it should be used as a last
  resort if no other code suites better.
- `TE_EWOULDBLOCK` and `TE_EAGAIN` are different codes in TE, but their
  system equivalents may be equal; the same holds true for `TE_EDEADLOCK` vs
  `TE_EDEADLK`.
- `TE_EINVAL` is a code for reporting invalid arguments, however:
  - do not overuse argument validation in general (see below)
  - there are more specific codes that may indicate what's wrong more precisely,
    e.g. one may pick the most suitable element from the list below:
    | Code                 | Description                                                        |
    |----------------------|--------------------------------------------------------------------|
    | `TE_E2BIG`           | Argument list too long                                             |
    | `TE_EBADF`           | Bad file descriptor                                                |
    | `TE_EFAULT`          | Bad address                                                        |
    | `TE_ENOTBLK`         | Block device required                                              |
    | `TE_ENOTTY`          | Not a terminal                                                     |
    | `TE_EDOM`            | Argument out of domain of function (e.g. a divisor is zero)        |
    | `TE_ERANGE`          | Value is out of range                                              |
    | `TE_ENOSYS`          | Function not implemented                                           |
    | `TE_ENAMETOOLONG`    | File name too long                                                 |
    | `TE_ECHRNG`          | Channel number out of range                                        |
    | `TE_ELNRNG`          | Link number out of range                                           |
    | `TE_EBADR`           | Invalid request descriptor                                         |
    | `TE_EBADRQC`         | Invalid request code                                               |
    | `TE_EBADSLT`         | Invalid slot                                                       |
    | `TE_ENODATA`         | No data available                                                  |
    | `TE_EBADMSG`         | Not a data message                                                 |
    | `TE_EOVERFLOW`       | Value too large to be stored in data type                          |
    | `TE_ENOTUNIQ`        | Name not unique                                                    |
    | `TE_EILSEQ`          | Illegal byte sequence (may be used for various parsing conditions) |
    | `TE_EMSGSIZE`        | Message too long                                                   |
    | `TE_EPROTOTYPE`      | Protocol wrong type for socket                                     |
    | `TE_EPROTONOSUPPORT` | Protocol not supported                                             |
    | `TE_ESOCKTNOSUPPORT` | Socket type not supported                                          |
    | `TE_EOPNOTSUPP`      | Operation not supported                                            |
    | `TE_EPFNOSUPPORT`    | Protocol family not supported                                      |
    | `TE_EAFNOSUPPORT`    | Address family not supported                                       |
    | `TE_EADDRNOTAVAIL`   | Cannot assign requested address                                    |
    | `TE_EMEDIUMTYPE`     | Wrong medium type                                                  |
    | `TE_ETOOMANY`        | Too many objects have been already allocated                       |
    | `TE_EFMT`            | Invalid format                                                     |

    TE error codes follow closely POSIX error codes which are usually
    just an approximation of an actual abnormal condition, so full consistency
    is rarely possible, therefore it is recommended to document important
    error codes of individual functions using `@retval`.

    If a TE function has a system API equivalent (e.g. `te_strtol` vs `strtol`),
    it should use the same error codes as the system one.

### Fatal errors and non-local exits

There are other means to signal errors besides returning a status code.

#### `assert()`

It should be used to check for "impossible" conditions, i.e. logic errors,
not caused by the program environment.

`assert(0)` may be used to mark unreachable branches.

Note:
- `assert()` may be compiled out if `NDEBUG` macro is defined. It implies that:
  - the condition of `assert()` must never have any side effects;
  - `assert()` should never be used to guard against user errors.

- `assert()` message is printed to `stderr`, so it may not appear in TE logs.

#### `TE_FATAL_ERROR()`

It should be used to signal errors that the caller is unable to handle or that
are theoretically possible but too improbable to handle properly.
An archetypal example of such a condition is an Out-of-Memory error.

`TE_FATAL_ERROR()` is never compiled out and it logs its message via TE logger,
however, in the current implementation a `TE_FATAL_ERROR()` in the main agent
process would result in the message being lost.

#### `TEST_FAIL` and its kin

`TEST_FAIL` is normally used by test suites, however, it may also be used by
high-level TAPI functions. The advantage of using `TEST_FAIL` is that an error
in a high-level TAPI function would usually result in a test failure anyway,
so doing it directly from the TAPI spares some intermediate error propagation.

`TEST_FAIL` jumps can be intercepted with `TAPI_ON_JUMP`, however, it
obscures the control flow, so it should only be used in special cases, e.g.
when an intermediate function needs to release some precious resources,
like file descriptors at the agent side.

`CHECK_RC` may be used to fail a test inside a higher level function whenever
a lower-level function returned an error status.

These macros may never be used in code that may also be used at the agent side.

### RPC errors

TAPI RPC wrappers do not return error codes by default and call `TEST_FAIL`
instead in case of an error. However, if either `RPC_AWAIT_IUT_ERROR` or
`RPC_AWAIT_ERROR` is called on a given RPC server, an error would be returned.
The effect of these macros is one-shot, i.e. affecting only the next RPC
call for a given server. Higher-level TAPI functions that call RPC wrappers
may assume that `RPC_AWAIT_IUT_ERROR` is not enabled upon entry.

### Argument validation

Ensuring the program correctness is of course important. However, TE is a much
more controlled environment than e.g. the Linux kernel, therefore values of
arguments in most function calls do not come from the outside, so if they happen
to be invalid, it's a breach of contract, i.e. a programmer's errors, which
should be handled with an `assert()` or `TE_FATAL_ERROR`, not by returning
a error status which the caller would most probably be unable to handle in any
sane way.

The ideal way of dealing with invalid arguments is by having none, that is,
either use proper typing to exclude incorrect values at compile time, or
handle the whole range of values in some sensible way. E.g., instead of

```
te_errno function(unsigned int port_no)
{
    if (port_no > 65535)
       return TE_EINVAL
```

use:
```
void function(uint16_t port_no)
{
```

A important case where errors must be reported via status codes and not treated
as fatal is functions which parse string representations of something, because
strings usually do come from the external environment and callers of such
functions usually have no way to filter out incorrect strings beforehand.

One especially obnoxious anti-pattern that unfortunately permeates the TE code
base is checking for `NULL` pointers and returning an error, like:

```
if (arg == NULL)
{
    ERROR("arg cannot be NULL");
    return TE_EFAULT;
}
```

This adds nothing to program robustness and only results in code bloating and
clobbering. An ordinary POSIX run-time ensures that dereferencing a null pointer
is an immediate program crash with a core dump, so even `assert(arg != NULL)` is
in most cases redundant. Real pointer issues cannot be (alas) get rid of with
simple run-time checks, so use tools like `valgrind` to detect them.

In many cases a better way to deal with null pointers is to allow them,
i.e. to treat them as black holes on output and as some kind of empty value on
input, so this:
```
void function(size_t *p_len)
{
    size_t len = calculate_length();

    if (p_len != NULL)
        *p_len = len;
}
```
is usually better than:
```
te_errno function(size_t *p_len)
{
    if (p_len == NULL)
        return TE_EFAULT;

    *p_len = calculate_length();
}
```
and
```
te_string_append(&dest, "%s", te_str_empty_if_null(str));
```
may be preferred to:
```
if (str == NULL)
   return TE_EFAULT;
te_string_append(&dest, "%s", str);
```
