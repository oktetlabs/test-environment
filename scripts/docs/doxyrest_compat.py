#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Make Doxygen's XML readable by doxyrest again.

Doxygen up to 1.8 wrote a member's full ``<memberdef>`` into the compound of
the *file* it is declared in, and let a group that owns the member carry only
a lightweight ``<member refid=.../>`` reference.  Doxygen 1.9 inverted that:
the definition moved into the *group* and the file kept the reference.

doxyrest builds its member map from the file compounds and then walks each
group to mark which of those members belong to it.  Under the new layout it
finds no members in the files, so no member is ever attached to a group - and
since every documented TE symbol lives inside a ``@defgroup``/``@{ @}`` block,
that silently removed the whole API reference from the manual.  Prose,
structures and group pages were still generated, so the output looked
plausible; only the functions, macros, typedefs and enumerations were gone,
along with every ``:ref:`` pointing at one.

Copy each grouped ``<memberdef>`` back into its file compound, replacing the
reference stub there.  The group keeps its copy, so doxyrest sees the layout
it expects from both sides and renders the member on the group page, with the
global namespace listing cross-referencing it.

Nothing happens when the definitions are already in the file compounds, so
this stays correct against an older Doxygen.
"""
from __future__ import annotations

import argparse
import copy
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def file_compounds(xml_dir: Path) -> dict[str, tuple[Path, ET.ElementTree,
                                                     ET.Element]]:
    """Every file compound, keyed by the source path it documents."""
    found = {}
    for path in sorted(xml_dir.glob("*.xml")):
        if path.name in ("index.xml", "Doxyfile.xml"):
            continue
        try:
            tree = ET.parse(path)
        except ET.ParseError as exc:
            print(f"{path}: {exc}", file=sys.stderr)
            continue
        compound = tree.getroot().find("compounddef")
        if compound is None or compound.get("kind") != "file":
            continue
        location = compound.find("location")
        if location is not None and location.get("file"):
            found[location.get("file")] = (path, tree, compound)
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xml_dir", help="Doxygen XML output directory")
    args = parser.parse_args()

    xml_dir = Path(args.xml_dir)
    if not xml_dir.is_dir():
        print(f"{xml_dir}: not a directory", file=sys.stderr)
        return 1

    files = file_compounds(xml_dir)
    copied = 0
    touched: dict[Path, ET.ElementTree] = {}

    for group_path in sorted(xml_dir.glob("group__*.xml")):
        group = ET.parse(group_path).getroot().find("compounddef")
        if group is None or group.get("kind") != "group":
            continue
        for section in group.findall("sectiondef"):
            for member in section.findall("memberdef"):
                location = member.find("location")
                if location is None:
                    continue
                target = files.get(location.get("file"))
                if target is None:
                    continue
                path, tree, compound = target
                for file_section in compound.findall("sectiondef"):
                    for stub in list(file_section.findall("member")):
                        if stub.get("refid") != member.get("id"):
                            continue
                        file_section.remove(stub)
                        file_section.append(copy.deepcopy(member))
                        touched[path] = tree
                        copied += 1

    for path, tree in touched.items():
        tree.write(path, encoding="UTF-8", xml_declaration=True)

    print(f"{Path(sys.argv[0]).name}: moved {copied} member definitions into "
          f"{len(touched)} file compounds", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
