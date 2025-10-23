#!/usr/bin/env python3
"""
@file gen_tut_to_doctest.py
@date   2025-02-18
@brief doctest: unit tests for TUT to doctest generator

$LicenseInfo:firstyear=2025&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2025, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""

from __future__ import annotations

import argparse
import datetime
import pathlib
import re
import textwrap
from typing import Iterable, List, Tuple


TEST_RE = re.compile(
    r"template\s*<>\s*template\s*<>\s*void\s+"
    r"(?P<object>[A-Za-z_]\w*)::test<\s*(?P<index>\d+)\s*>\s*\(\s*\)\s*{",
    re.MULTILINE,
)

INCLUDE_RE = re.compile(r"^\s*#\s*include\s+[<\"].+[>\"]\s*$", re.MULTILINE)


def extract_block(source: str, start_index: int) -> Tuple[str, int]:
    """Extract a block delimited by braces starting at start_index."""
    depth = 0
    end_index = start_index
    for idx in range(start_index, len(source)):
        char = source[idx]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                end_index = idx
                break
    # body without surrounding braces
    body = source[start_index + 1 : end_index]
    return body, end_index + 1


def collect_includes(source: str) -> List[str]:
    includes = INCLUDE_RE.findall(source)
    cleaned: List[str] = []
    filtered_keywords = ("lltut.h", "tut/")
    windows_excludes = (
        "sys/wait.h",
        "unistd.h",
        "llallocator.h",
        "llallocator_heap_profile.h",
        "llmemtype.h",
        "lllazy.h",
        "netinet/in.h",
        "StringVec.h",
    )
    for line in includes:
        if any(keyword in line for keyword in filtered_keywords):
            continue
        if any(keyword in line for keyword in windows_excludes):
            cleaned.append("// " + line.strip() + "  // not available on Windows")
            continue
        cleaned.append(line.strip())
    return cleaned


def discover_tests(source: str) -> List[Tuple[str, str, str]]:
    tests: List[Tuple[str, str, str]] = []
    for match in TEST_RE.finditer(source):
        object_name = match.group("object")
        index = match.group("index")
        body, end_index = extract_block(source, match.end() - 1)
        snippet = source[match.start() : end_index]
        tests.append((object_name, index, snippet))
    return tests


def make_case_name(stem: str, object_name: str, index: str) -> str:
    return f"{stem}::{object_name}_test_{index}"


def generate_doctest_file(
    src_path: pathlib.Path, dst_path: pathlib.Path, includes: Iterable[str], tests: List[Tuple[str, str, str]]
) -> None:
    timestamp = datetime.datetime.utcnow().isoformat(timespec="seconds") + "Z"
    lines: List[str] = []
    lines.append("// ---------------------------------------------------------------------------")
    lines.append(f"// Auto-generated from {src_path.name} at {timestamp}")
    lines.append("// This file is a TODO stub produced by gen_tut_to_doctest.py.")
    lines.append("// ---------------------------------------------------------------------------")
    lines.append('#include "doctest.h"')
    lines.append('#include "ll_doctest_helpers.h"')
    lines.append('#include "tut_compat_doctest.h"')
    for inc in includes:
        lines.append(inc)
    lines.append("")
    lines.append('TUT_SUITE("llcommon")')
    lines.append("{")

    if not tests:
        lines.append("    TUT_CASE(\"{0}::no_tests_detected\")".format(src_path.stem))
        lines.append("    {")
        lines.append('        DOCTEST_FAIL("TODO: no TUT tests discovered in source file");')
        lines.append("    }")
    else:
        for object_name, index, snippet in tests:
            case_name = make_case_name(src_path.stem, object_name, index)
            lines.append(f'    TUT_CASE("{case_name}")')
            lines.append("    {")
            lines.append(
                f'        DOCTEST_FAIL("TODO: convert {src_path.name}::{object_name}::test<{index}> from TUT to doctest");'
            )
            lines.append("        // Original snippet:")
            snippet_comment = textwrap.indent(textwrap.dedent(snippet).rstrip(), "        // ")
            lines.extend(snippet_comment.splitlines())
            lines.append("    }")
            lines.append("")
    lines.append("}")
    lines.append("")

    dst_path.parent.mkdir(parents=True, exist_ok=True)
    dst_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate doctest stubs from TUT sources.")
    parser.add_argument("--src", required=True, help="Directory containing TUT sources.")
    parser.add_argument("--dst", required=True, help="Directory where doctest files will be written.")
    args = parser.parse_args()

    src_dir = pathlib.Path(args.src).resolve()
    dst_dir = pathlib.Path(args.dst).resolve()

    if not src_dir.exists():
        raise SystemExit(f"Source directory {src_dir} does not exist")

    dst_dir.mkdir(parents=True, exist_ok=True)

    mappings: List[Tuple[pathlib.Path, pathlib.Path]] = []

    for cpp_path in sorted(src_dir.glob("*.cpp")):
        source = cpp_path.read_text(encoding="utf-8")
        includes = collect_includes(source)
        tests = discover_tests(source)

        dest_filename = cpp_path.stem + "_doctest.cpp"
        dest_path = dst_dir / dest_filename

        generate_doctest_file(cpp_path, dest_path, includes, tests)
        mappings.append((cpp_path.relative_to(src_dir), dest_path.relative_to(dst_dir)))

    index_lines = [f"{src} -> {dst}" for src, dst in mappings]
    (dst_dir / "generated.index").write_text("\n".join(index_lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
