[SPDX-License-Identifier: Apache-2.0]::
[Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.]::

# Contributing code to Test Environment

The document provides the guidelines for submitting changes to Test
Environment.

If you want your changes to be accepted in the mainline:

 1. The code must comply with the project's coding style.
 2. The code must meet the license requirements.
 3. A changeset summary and body must comply with requirements for it.
 4. Each changeset must be testable and tested.
 5. All changes must pass maintainers' review and be accepted by the
    maintainer.

Submission is done using pull requests.


## Coding style

Coding style for C, Shell and Python is defined in the separate Git
repository [StyleGuides](https://github.com/oktetlabs/styleguides).

Meson build system control files (`meson.build` and
`meson_options.txt`) should:

 - use lower case and underscore for words separation
 - use 4 spaces to indent
 - have no space after opening and before closing parentheses
 - have spaces after opening and before closing square brackets
 - follow the style of a corresponding file


## License

All new code must be licensed under Apache 2.0.

All new files must have SPDX-License-Identifier in the first (e.g. C, Meson)
or the second line (e.g. Shell, Python and other shebang executables, XML,
YAML).

The next line after the SPDX tag should contain a copyright notice.
It could be non OKTET Labs Ltd copyright (e.g. your employer copyright).


## Commit messages


### Summary line

The summary line should:

 - highlight the scope and the impact of the change
 - be up to 60 characters
 - be lowercase apart from acronyms and proper names
 - be prefixed with the component name, for example:
    - `agent/unix`
    - `build` (for generic changes in the build system which affect various
      components)
    - `doc/cm`
    - `engine/tester`
    - `include`
    - `lib/asn`
    - `lib/rpc_dpdk` (when you define an RPC call and implement both
      client and server sides in one patch)
    - `suites/selftest`
    - `tool/trc`
 - start with the imperative of a verb, for example:

   ```
   lib/asn: fix build with GCC 12
   ```

   ```
   agent/unix: support VLAN interfaces control
   ```

 - be human-readable and contain no file and symbol names from the sources

Note that it is prohibited to use ticket tracker references (e.g.
"Bug 12345: " or "#12345") in the summary line. Use corresponding helper
trailer instead.


### Body

The commit message body should consist of several sections in the order
specified below.

Note that there should be no empty lines in trailer sections and between
them.


#### Description (optional)

The body of the message must reply to the question why the change is
necessary. Even when a new feature is added, it must shed light on why
the feature is necessary/useful. If you fix a build warning, don't
forget to provide compiler version and other environment details.

The description may be absent if the summary line already does the job.

If the change is tricky, the description may provide explanations which are
not suitable for the code.

The text of the description should wrap at 72 characters.


#### Helper trailers (if applicable)

If the patch fixes an issue introduced by an earlier changeset, it should
have `Fixes:` trailer which must be formatted using:

```
git config alias.fixline "log -1 --abbrev=12 --format='Fixes: %h (\"%s\")'" # once
```

```
git fixline <SHA>
```

Helper trailers may contain references to an internal issue tracker (e.g.
Bugzilla, Redmine or Jira) which is not accessible outside. The
patch author is fully responsible for correctness of the reference.
For example:

```
OL-Redmine-Id: 12345
```

Reviewers may ask to add their references to issue tracker.

References to internal code review systems may be added in a similar way.


#### Co-authorship trailers (if applicable)

Patch authors are encouraged to add the following trailers, if applicable,
to give credit to people who find and report bugs (`Reported-by:`),
suggest a main idea/design implemented in the patch (`Suggested-by:`) or
take part in the development (`Co-developed-by:`). Every co-developer
trailer must be immediately followed by a `Signed-off-by:` of the associated
co-author (see below).


#### Signoff (mandatory)

The commit message must have a `Signed-off-by:` line which is added using:

```
git commit --signoff # or -s
```

The purpose of the signoff is explained in the
[Developer’s Certificate of Origin](https://www.kernel.org/doc/html/latest/process/submitting-patches.html#developer-s-certificate-of-origin-1-1)
section of the Linux kernel guidelines. Ensure that you have read and fully
understood it prior to applying the signoff.

#### Review trailers (mandatory)

Review trailers (`Reviewed-by:`, `Acked-by:` and `Tested-by:`) are based on
a review process which may take place internally using company services or
publicly on GitHub.

In case of an internal review it is the pull request submitter who is
responsible to add review trailers based on internal review feedback.

In case of a public review either the submitter or the accepting
maintainer should add applicable review trailers.

`Acked-by:` means that the reviewer acknowledges the overall idea/design of
the patch, but has not reviewed the code in details.

`Reviewed-by:` means that the reviewer has inspected the code in details in
addition to overall design approval.

`Tested-by:` means that the reviewer has tried the changeset or entire patch
series, checked that it does the job and found no regressions.


#### Example

    component: fix segmentation fault on invalid input

    Input parameters were not validated and it resulted in segmentation
    fault which eats extra time on debugging. Add error logging to make
    debugging easier.

    Fixes: afaada7087af ("Import the first public release")
    OL-Redmine-Id: 12345
    Signed-off-by: John Doe <johnd@example.com>
    Reviewed-by: Jane Doe <janed@example.com>
