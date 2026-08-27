# scenario.py: markdown test scenarios

Test scenarios are written as `package.md` files kept next to
`package.xml` in a test suite. This tool turns them into C test
stubs and keeps the two in sync:

    scenario.py generate <package>/<test>   # write the C stub
    scenario.py check                       # report drift, non-zero exit
    scenario.py list --pending              # the backlog
    scenario.py steps <test>.c              # annotated steps from the built source

Run it from the suite; the test root is `-t` (default: the
current directory, or its `ts/` subdirectory when present).

The dialect reference - file naming, heading forms, step and
substep semantics, the paragraph vs blockquote distinction,
inline code span resolution - is the "Test scenarios in markdown"
page of the TE documentation, `doc/sphinx/pages/scenario_md.rst`
in this repository.

Development: `pyproject.toml` carries the ruff and mypy
configuration (oktet-dev Python style guide); self-tests live in
`tests/` and run with pytest from this directory.
