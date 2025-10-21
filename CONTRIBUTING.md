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

Also see [README-Devel](README-Devel.md) for additional development
guidelines.

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
 - be prefixed with the component name (see below)
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

#### Component name

Normally a component name is a second-level directory name
(or a first-level one, when the second level does not exist), for example:

 - `agent/unix`
 - `dispatcher` for changes in `./dispatcher.sh` only
 - `doc/cm`
 - `engine/tester`
 - `include`
 - `lib/asn`
 - `suites/selftest`
 - `tool/trc`

If a patch touches code in several second-level directories,
then the most important one should be chosen as the component name.
In particular, if the patch implements a new RPC call, both client and
server parts, the "common denominator" directory shall be used, e.g.:

 - `lib/rpc_dpdk`
 - `lib/rpcxdr` (for changes in the base set of RPCs)

For changes that really affect large portions of code across directories,
there is a curated list of special component names:

 - `build` for generic changes in the build system
 - `common` for changes that affect everything. This should be reserved
   for cases like bulk copyright boilerplate update and the like.
 - `cs` for co-ordinated changes related to the Configurator subsystem.
   Note that changes to the model files and the implementation in an agent
   are better kept in separate patches with appropriate component names.
 - `doc` for generic changes in the documentation
 - `logger` for co-ordinated changes related to logging
 - `rcf` for co-ordinated changes in the RCF subsystem
 - `revert` for rollback commits (see below)

If a patch does not fall under any of the above categories, it is a good
idea to split it into several parts.

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

The changeset referenced by `Fixes` trailer must be accessible from the patch
commit --- this ensures the trailer always refers to a publicly visible
changeset. Sometimes this requirement may be too strong: it may be the case
that a public patch fixes a changeset that only exists in a private fork and
the contributor nevertheless believes this fact important enough to record.
For such situations, a `Fixes-Private` trailer may be used, which has the same
syntax as the `Fixes` trailer.

Helper trailers may contain references to an internal issue tracker (e.g.
Bugzilla, Redmine or Jira) which is not accessible outside. The
patch author is fully responsible for correctness of the reference.
For example:

```
OL-Redmine-Id: 12345
```

Reviewers may ask to add their references to issue tracker.

References to internal code review systems may be added in a similar way.

A reference to a public pull request or some other publicly available
discussion may be provided with the `Link: <url>` trailer, as it is done
by the Linux kernel community.


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
[Developer's Certificate of Origin](https://developercertificate.org/).
Ensure that you have read and fully understood it prior to applying the
signoff.

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

#### Referencing other commits

A commit message may reference other commits in the current repository or
related repositories, for example, if the commit is motivated by a certain
change in another project. The trailer `Ref` is used for that purpose:

```
Ref: [ROOT:ORGANIZATION/REPO@]HASH ("SUBJECT LINE")
```

The syntax is an extension of the syntax for `Fixes`, allowing cross-repository
references.
It is compatible with GitHub's [autolinking feature](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/autolinked-references-and-urls#commit-shas).

The `ROOT` component identifies the repository provider (so it is expected to be
a domain name, eg. `github.com` or `git.oktetlabs.ru`). Full URLs are not used,
because there is no standard syntax for commit-pointing Git URLs, they may vary
depending on the Git server implementation.

For example:

```
Ref: github.com:customer-org/device-driver@afaada7087af ("Extension API is implemented")
```

#### Patch priority (optional)

A patch priority may be specified with the help of a special trailer.

The purpose of it is fourfold:
- to help reviewers in prioritizing their backlog;
- to help maintainers in planning forthcoming releases;
- to help testsuite authors in assessing the need to upgrade to a new release;
- to provide a trackable way of pushing emergency patches,
  bypassing the standard review procedure.

The syntax and semantics of the trailer are modelled after the Debian's `Urgency`
control field (see https://www.debian.org/doc/debian-policy/ch-controlfields.html#urgency):

```
Urgency: LEVEL (OPTIONAL EXPLANATION IN PARENTHESES)
```

The following levels are defined:

- `low`: e.g. documentation changes, minor fixes, pure refactoring;
- `medium`: this is the default, so generally it should not be explicitly used;
- `high`: stuff that actually blocks the development of test suites would
  normally be on that level;
- `critical`: fixes for broken builds, other urgent bugfixes;
- `emergency`: patches that need to be applied ASAP, e.g. rollbacks of disastrous
  commits.

In general, if a patch is tied to an issue in a tracker, the urgency level would
correspond to a priority/severity of the associated issue.

For example:

```
Urgency: emergency (the build is totally broken)
```

Patches with the urgency level of `emergency` and `critical` may bypass
the normal review procedure and thus lack any of the review trailers
(however, following the standard procedure is still recommended).
In this case the explanation in `Urgency` trailer is **mandatory**.

#### Breaking changes

A change is breaking if testsuites using some part of TE need to be updated,
or else they would not compile, would crash or misbehave.

The presence of a breaking change must be indicated by the `Breaks:` trailer
containing a description of what exactly was broken. It may be a reference
to a particular symbol, a particular header or some other info that is
sufficient for testsuite maintainers to assess whether they are affected by
the change.

Changes that are generally non-breaking:

 - documentation and style changes
 - changes that do not affect the public API, e.g. if a better algorithm or
   a more robust implementation is employed
 - bugfixes (if a function crashed on some inputs and now it returns a proper
   error code, no user of this function is broken)
 - addition of new modules, provided they do not introduce new build
   dependencies
 - addition of new API functions to existing modules
 - addition of new fields to existing structures, when the exact binary layout
   of a structure is not important
 - backward-compatible changes in function signatures and object types
   (e.g. a replacement of `long long` to `int64_t`)

Changes that are generally breaking:

 - introduction of new build dependencies
 - removal of existing modules
 - removal of public API elements
 - changes in function signatures (addition or removal of parameters,
   type changes)
 - addition of function attributes, such as `format` or `deprecated`

#### Trailer order

The order of trailers is not rigidly fixed, but it must reflect to some
extent the development process of a patch. In particular:

 - the trailers that describe the substance of a patch must come first ---
   these are issue-referencing trailers (e.g. `OL-Redmine-Id`), `Link`,
   `Fixes`, `Breaks`, `Urgency`;
   the relative order of trailers within this group is not specified;
 - tribute trailers `Suggested-by` and `Reported-by`;
 - then `Signed-off-by` and `Co-developed-by` must come; the order of these
   trailers must reflect the history of contributions, that is, the most recent
   contributor must be mentioned last;
 - and at last `Reviewed-by`, `Acked-by` and `Tested-by` must come, again
   reflecting the timeline of the review process -- the most recent reviewer
   must be mentioned last.

Note that all `*-by` trailers should always come together, not separated
by other trailers.

If a patch is a result of a joint effort of several parties, the latter
two blocks may be interleaved, e.g. first come the author(s) of the original
patch, then internal reviewers of the first party, then co-authors from the
second party and then final reviewers.

In all cases the very last `Signed-off-by` must be either from the patch author
or from the patch committer.

#### Rollback commits

Rollback (aka revert) commits as produced by `git revert` are ordinary commits
with respect to the present Guidelines, that is, they should have a meaningful
description of why the rollback was needed, a `Signed-off-by` trailer and one or
more review trailers (e.g. `Reviewed-by`). It also may have any other trailers
that are necessary, e.g. the `OL-Redmine-Id` trailer.

The subject line of a rollback commit must have `revert:` as a component name
and then the remaining part of the subject line of the reverted commit.
The reference to the original commit must be specified as a `Fixes` trailer
(see below for an example).

Thus, it follows that the commit message generated by `git revert` does **not**
comply with the present Guidelines.
It is recommended to use `git revert --no-commit` which only updates the index
and then separately `git commit` with all relevant options.
There is also a script `scripts/ol_patch_revert`  in [qa-tools](https://github.com/oktetlabs/qa-tools)
that produces a conforming rollback commit in one shot.

Rollback commits that revert more than a single commit are not allowed by
these Guidelines.

#### Example

    component: fix segmentation fault on invalid input

    Input parameters were not validated and it resulted in segmentation
    fault which eats extra time on debugging. Add error logging to make
    debugging easier.

    Fixes: afaada7087af ("Import the first public release")
    OL-Redmine-Id: 12345
    Signed-off-by: John Doe <johnd@example.com>
    Reviewed-by: Jane Doe <janed@example.com>

A more elaborate example with a complex patch workflow:

    component: implement a super important feature

    Here comes the description of a very important and useful
    feature that has long been worked upon.

    Breaks: a lot of legacy testsuites
    OL-Redmine-Id: 12345
    Link: Link: https://github.com/oktetlabs/test-environment/pull/999
    Suggested-by: Gavin Armstrong <g.armstrong@example.come>
    Signed-off-by: James Baxter <jbaxter@example.com>
    Acked-by: Joanne Chappell <joanned@example.come>
    Co-developed-by: Margaret Doig  <mdoig@example.com>
    Signed-off-by: Margaret Doig <mdoig@example.com>
    Signed-off-by: Paul Farrow <paul.farrow@example.com>
    Reviewed-by: Ian Greenaway <ian-greenaway@example.com>
    Reviewed-by: Victor Harris <vharris@example.com>

Example of a rollback commit that would revert the first example above:

    revert: fix segmentation fault on invalid input

    It turned out the fix was worse than the bug itself.

    Fixes: 90dba6649d98 ("component: fix segmentation fault on invalid input")
    Urgency: high
    OL-Redmine-Id: 12453
    Signed-off-by: Jane Doe <janed@example.com>
    Reviewed-by: John Doe <johnd@example.com>

### Ensure compliance automatically

There is a separate repository for [tools](https://github.com/oktetlabs/qa-tools)
to ensure commit compliance.

It is strongly recommended to run `scripts/ol_install_hooks`
from there right after the clone of TE. This would install hook that would
prevent pushing the commits that do not comply with the present
Guidelines.

Also there is a script `scripts/ol_patch_add_trailer` that
can automatically add needed trailers such as `Reviewed-by` to
the series of patches at once.

## Submitting a patch series

When a series of patches is submitted for review via GitHub
or some other way, the pull request should contain a meaningful
description of the whole series and not just a concatenation
of all included commit messages. Trailers should be wiped out
from such a description, especially those containing emails.
