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
