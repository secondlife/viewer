#!/usr/bin/env python3
"""Verify a material SPIR-V interface against its profile manifest dump."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections.abc import Iterable, Mapping, Sequence
from pathlib import Path
from typing import Any, NoReturn


class VerificationError(Exception):
    """A reflection input or interface failed validation."""


_RESOURCE_COLLECTIONS = (
    "ubos",
    "ssbos",
    "textures",
    "separate_images",
    "separate_samplers",
    "images",
    "storage_images",
    "atomic_counters",
    "acceleration_structures",
    "push_constants",
    "subpass_inputs",
    "specialization_constants",
)

_REFLECTION_FIELDS = {
    "entryPoints",
    "types",
    "inputs",
    "outputs",
    *_RESOURCE_COLLECTIONS,
}


def _fail(message: str) -> NoReturn:
    raise VerificationError(message)


def _object(value: Any, context: str) -> Mapping[str, Any]:
    if not isinstance(value, dict):
        _fail(f"{context} must be a JSON object")
    return value


def _array(value: Any, context: str) -> Sequence[Any]:
    if not isinstance(value, list):
        _fail(f"{context} must be a JSON array")
    return value


def _string(value: Any, context: str) -> str:
    if not isinstance(value, str):
        _fail(f"{context} must be a string")
    return value


def _integer(value: Any, context: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool):
        _fail(f"{context} must be an integer")
    return value


def _strings(value: Any, context: str) -> tuple[str, ...]:
    result = tuple(
        _string(item, f"{context}[{index}]")
        for index, item in enumerate(_array(value, context))
    )
    if len(result) != len(set(result)):
        _fail(f"{context} contains duplicates")
    return tuple(sorted(result))


def _load_json(path: Path, label: str) -> Mapping[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as stream:
            return _object(json.load(stream), label)
    except OSError as error:
        _fail(f"cannot read {label} {path}: {error}")
    except json.JSONDecodeError as error:
        _fail(f"invalid JSON in {label} {path}: {error}")


def _load_text(path: Path, label: str) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as error:
        _fail(f"cannot read {label} {path}: {error}")


def _format_rows(rows: Iterable[tuple[Any, ...]]) -> str:
    return "[" + ", ".join(repr(row) for row in sorted(rows)) + "]"


def _expect_equal(
    label: str, actual: set[tuple[Any, ...]], expected: set[tuple[Any, ...]]
) -> None:
    if actual == expected:
        return
    missing = expected - actual
    extra = actual - expected
    details: list[str] = []
    if missing:
        details.append(f"missing {_format_rows(missing)}")
    if extra:
        details.append(f"unexpected {_format_rows(extra)}")
    _fail(f"{label}: {'; '.join(details)}")


def _unique_rows(rows: Iterable[tuple[Any, ...]], label: str) -> set[tuple[Any, ...]]:
    materialized = list(rows)
    unique = set(materialized)
    if len(unique) != len(materialized):
        _fail(f"{label} contains duplicate entries")
    return unique


def _expectation_rows(
    expectation: Mapping[str, Any],
    field: str,
    scalar_fields: Sequence[str],
    *,
    stages: bool = False,
) -> set[tuple[Any, ...]]:
    result: list[tuple[Any, ...]] = []
    for index, raw in enumerate(_array(expectation.get(field), f"expectation.{field}")):
        item = _object(raw, f"expectation.{field}[{index}]")
        values: list[Any] = []
        allowed = set(scalar_fields)
        for name in scalar_fields:
            value = item.get(name)
            if name in {"location", "set", "binding", "size", "count"}:
                values.append(_integer(value, f"expectation.{field}[{index}].{name}"))
            else:
                values.append(_string(value, f"expectation.{field}[{index}].{name}"))
        if stages:
            values.append(
                _strings(item.get("stages"), f"expectation.{field}[{index}].stages")
            )
            allowed.add("stages")
        unknown = set(item) - allowed
        if unknown:
            _fail(f"expectation.{field}[{index}] has unknown fields {sorted(unknown)}")
        result.append(tuple(values))
    return _unique_rows(result, f"expectation.{field}")


def _reflection_items(
    reflection: Mapping[str, Any], field: str, module: str
) -> Sequence[Any]:
    value = reflection.get(field, [])
    return _array(value, f"{module} reflection.{field}")


def _reflection_io(
    reflection: Mapping[str, Any], field: str, module: str, *, expand_arrays: bool
) -> set[tuple[str, int, str]]:
    rows: list[tuple[str, int, str]] = []
    for index, raw in enumerate(_reflection_items(reflection, field, module)):
        item = _object(raw, f"{module} reflection.{field}[{index}]")
        name = _string(item.get("name"), f"{module} reflection.{field}[{index}].name")
        location = _integer(
            item.get("location"), f"{module} reflection.{field}[{index}].location"
        )
        value_type = _string(
            item.get("type"), f"{module} reflection.{field}[{index}].type"
        )
        count = 1
        if "array" in item:
            dimensions = _array(
                item["array"], f"{module} reflection.{field}[{index}].array"
            )
            if not expand_arrays or len(dimensions) != 1:
                _fail(
                    f"{module} reflection.{field}[{index}] has an unsupported array shape"
                )
            literal_context = (
                f"{module} reflection.{field}[{index}].array_size_is_literal"
            )
            literal_dimensions = _array(
                item.get("array_size_is_literal"), literal_context
            )
            if len(literal_dimensions) != len(dimensions):
                _fail(f"{literal_context} must match the array dimensions")
            for dimension, literal in enumerate(literal_dimensions):
                if not isinstance(literal, bool):
                    _fail(f"{literal_context}[{dimension}] must be a boolean")
                if not literal:
                    _fail(f"{literal_context}[{dimension}] must be true")
            count = _integer(
                dimensions[0], f"{module} reflection.{field}[{index}].array[0]"
            )
            if count <= 0:
                _fail(
                    f"{module} reflection.{field}[{index}] has a non-positive array size"
                )
        rows.extend((name, location + offset, value_type) for offset in range(count))
    return _unique_rows(rows, f"{module} reflection.{field}")


def _entry_point(reflection: Mapping[str, Any], module: str) -> tuple[str, str]:
    entries = _reflection_items(reflection, "entryPoints", module)
    if len(entries) != 1:
        _fail(
            f"{module} reflection must contain exactly one entry point, found {len(entries)}"
        )
    entry = _object(entries[0], f"{module} reflection.entryPoints[0]")
    return (
        _string(entry.get("name"), f"{module} reflection.entryPoints[0].name"),
        _string(entry.get("mode"), f"{module} reflection.entryPoints[0].mode"),
    )


def _resources(
    reflections: Mapping[str, Mapping[str, Any]],
    field: str,
    *,
    include_name: bool = False,
    include_size: bool = False,
    include_type: bool = False,
) -> set[tuple[Any, ...]]:
    combined: dict[tuple[Any, ...], set[str]] = {}
    seen_in_module: set[tuple[str, tuple[Any, ...]]] = set()
    for module, reflection in reflections.items():
        for index, raw in enumerate(_reflection_items(reflection, field, module)):
            item = _object(raw, f"{module} reflection.{field}[{index}]")
            if "array" in item or "array_size_is_literal" in item:
                _fail(
                    f"{module} reflection.{field}[{index}] is an unsupported descriptor array"
                )
            key: list[Any] = []
            if include_name:
                key.append(
                    _string(
                        item.get("name"), f"{module} reflection.{field}[{index}].name"
                    )
                )
            key.extend(
                [
                    _integer(
                        item.get("set"), f"{module} reflection.{field}[{index}].set"
                    ),
                    _integer(
                        item.get("binding"),
                        f"{module} reflection.{field}[{index}].binding",
                    ),
                ]
            )
            if include_size:
                key.append(
                    _integer(
                        item.get("block_size"),
                        f"{module} reflection.{field}[{index}].block_size",
                    )
                )
            if include_type:
                key.append(
                    _string(
                        item.get("type"), f"{module} reflection.{field}[{index}].type"
                    )
                )
            frozen_key = tuple(key)
            module_key = (module, frozen_key)
            if module_key in seen_in_module:
                _fail(
                    f"{module} reflection.{field} contains duplicate resource {frozen_key!r}"
                )
            seen_in_module.add(module_key)
            combined.setdefault(frozen_key, set()).add(module)
    return {(*key, tuple(sorted(stages))) for key, stages in combined.items()}


def _reject_unexpected_resource_categories(
    reflections: Mapping[str, Mapping[str, Any]],
) -> None:
    permitted_nonempty = {"ubos", "textures"}
    for module, reflection in reflections.items():
        unknown = set(reflection) - _REFLECTION_FIELDS
        if unknown:
            _fail(f"{module} reflection has unknown top-level fields {sorted(unknown)}")
        for field in _RESOURCE_COLLECTIONS:
            items = _reflection_items(reflection, field, module)
            if items and field not in permitted_nonempty:
                _fail(
                    f"{module} reflection has unexpected {field}: {len(items)} entr{'y' if len(items) == 1 else 'ies'}"
                )


_OP_NAME = re.compile(r'^\s*OpName\s+(%\S+)\s+"((?:[^"\\]|\\.)*)"\s*$')
_OP_INTERPOLATION = re.compile(
    r"^\s*OpDecorate\s+(%\S+)\s+(Flat|NoPerspective|Centroid|Sample)\s*$"
)
_OP_LOCATION = re.compile(r"^\s*OpDecorate\s+(%\S+)\s+Location\s+(\d+)\s*$")


def _disassembly_decorations(
    text: str, module: str
) -> tuple[dict[str, str], set[tuple[str, str]], dict[str, int]]:
    names: dict[str, str] = {}
    interpolations: set[tuple[str, str]] = set()
    locations: dict[str, int] = {}
    for line in text.splitlines():
        if match := _OP_NAME.match(line):
            identifier, encoded_name = match.groups()
            try:
                name = json.loads(f'"{encoded_name}"')
            except json.JSONDecodeError as error:
                _fail(f"{module} disassembly has an invalid OpName string: {error}")
            if identifier in names:
                _fail(f"{module} disassembly names {identifier} more than once")
            names[identifier] = name
        elif match := _OP_INTERPOLATION.match(line):
            identifier, interpolation = match.groups()
            if (identifier, interpolation) in interpolations:
                _fail(
                    f"{module} disassembly decorates {identifier} {interpolation} more than once"
                )
            interpolations.add((identifier, interpolation))
        elif match := _OP_LOCATION.match(line):
            identifier, raw_location = match.groups()
            if identifier in locations:
                _fail(f"{module} disassembly gives {identifier} more than one Location")
            locations[identifier] = int(raw_location)
    return names, interpolations, locations


def _verify_interpolation_decorations(
    expectation: Mapping[str, Any], disassemblies: Mapping[str, str]
) -> None:
    expected_by_module: dict[str, set[tuple[str, int, str]]] = {
        "vertex": set(),
        "fragment": set(),
    }
    raw_interfaces = _array(
        expectation.get("flat_interfaces"), "expectation.flat_interfaces"
    )
    for index, raw in enumerate(raw_interfaces):
        item = _object(raw, f"expectation.flat_interfaces[{index}]")
        name = _string(item.get("name"), f"expectation.flat_interfaces[{index}].name")
        location = _integer(
            item.get("location"), f"expectation.flat_interfaces[{index}].location"
        )
        modules = _strings(
            item.get("modules"), f"expectation.flat_interfaces[{index}].modules"
        )
        if not modules or set(modules) - set(expected_by_module):
            _fail(
                f"expectation.flat_interfaces[{index}].modules must contain vertex and/or fragment"
            )
        unknown = set(item) - {"name", "location", "modules"}
        if unknown:
            _fail(
                f"expectation.flat_interfaces[{index}] has unknown fields {sorted(unknown)}"
            )
        for module in modules:
            expected_by_module[module].add((name, location, "Flat"))

    for module, text in disassemblies.items():
        names, interpolations, locations = _disassembly_decorations(text, module)
        actual: set[tuple[str, int, str]] = set()
        for identifier, interpolation in interpolations:
            if identifier not in names:
                _fail(
                    f"{module} disassembly has {interpolation} on unnamed ID {identifier}"
                )
            if identifier not in locations:
                _fail(
                    f"{module} disassembly has {interpolation} on {names[identifier]!r} without Location"
                )
            actual.add((names[identifier], locations[identifier], interpolation))
        _expect_equal(
            f"{module} interpolation decorations", actual, expected_by_module[module]
        )


def verify(
    expectation: Mapping[str, Any],
    vertex: Mapping[str, Any],
    fragment: Mapping[str, Any],
    vertex_disassembly: str,
    fragment_disassembly: str,
) -> None:
    allowed_expectation_fields = {
        "schema",
        "entry_points",
        "vertex_inputs",
        "interstage_variables",
        "uniform_blocks",
        "combined_image_samplers",
        "fragment_outputs",
        "push_constant_ranges",
        "flat_interfaces",
    }
    unknown = set(expectation) - allowed_expectation_fields
    if unknown:
        _fail(f"expectation has unknown fields {sorted(unknown)}")
    if _integer(expectation.get("schema"), "expectation.schema") != 1:
        _fail("expectation.schema must be 1")

    expected_entries = _expectation_rows(
        expectation, "entry_points", ("module", "name", "stage")
    )
    actual_entries = {
        ("vertex", *_entry_point(vertex, "vertex")),
        ("fragment", *_entry_point(fragment, "fragment")),
    }
    _expect_equal("entry points", actual_entries, expected_entries)

    expected_vertex_inputs = _expectation_rows(
        expectation, "vertex_inputs", ("name", "location", "type")
    )
    actual_vertex_inputs = _reflection_io(
        vertex, "inputs", "vertex", expand_arrays=False
    )
    _expect_equal("vertex inputs", actual_vertex_inputs, expected_vertex_inputs)

    vertex_outputs = _reflection_io(vertex, "outputs", "vertex", expand_arrays=False)
    fragment_inputs = _reflection_io(
        fragment, "inputs", "fragment", expand_arrays=False
    )
    expected_interstage = _expectation_rows(
        expectation, "interstage_variables", ("name", "location", "type")
    )
    _expect_equal("vertex outputs", vertex_outputs, expected_interstage)
    _expect_equal("fragment inputs", fragment_inputs, expected_interstage)

    expected_fragment_outputs = _expectation_rows(
        expectation, "fragment_outputs", ("name", "location", "type")
    )
    actual_fragment_outputs = _reflection_io(
        fragment, "outputs", "fragment", expand_arrays=True
    )
    _expect_equal(
        "fragment outputs", actual_fragment_outputs, expected_fragment_outputs
    )

    reflections = {"vertex": vertex, "fragment": fragment}
    expected_blocks = _expectation_rows(
        expectation, "uniform_blocks", ("name", "set", "binding", "size"), stages=True
    )
    _expect_equal(
        "uniform blocks",
        _resources(reflections, "ubos", include_name=True, include_size=True),
        expected_blocks,
    )

    expected_samplers = _expectation_rows(
        expectation,
        "combined_image_samplers",
        ("name", "set", "binding", "type"),
        stages=True,
    )
    _expect_equal(
        "combined image samplers",
        _resources(reflections, "textures", include_name=True, include_type=True),
        expected_samplers,
    )

    push_constants = _array(
        expectation.get("push_constant_ranges"), "expectation.push_constant_ranges"
    )
    if push_constants:
        _fail("the material profile expectation must not contain push constants")
    _reject_unexpected_resource_categories(reflections)
    _verify_interpolation_decorations(
        expectation,
        {"vertex": vertex_disassembly, "fragment": fragment_disassembly},
    )


def _arguments(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compare material SPIR-V reflection with a profile manifest-derived expectation."
    )
    parser.add_argument(
        "--expectation",
        type=Path,
        required=True,
        help="JSON emitted by llshadermanifest_dump",
    )
    parser.add_argument(
        "--vertex-reflection",
        type=Path,
        required=True,
        help="spirv-cross --reflect JSON for the vertex module",
    )
    parser.add_argument(
        "--fragment-reflection",
        type=Path,
        required=True,
        help="spirv-cross --reflect JSON for the fragment module",
    )
    parser.add_argument(
        "--vertex-disassembly",
        type=Path,
        required=True,
        help="spirv-dis text for the vertex module",
    )
    parser.add_argument(
        "--fragment-disassembly",
        type=Path,
        required=True,
        help="spirv-dis text for the fragment module",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    arguments = _arguments(sys.argv[1:] if argv is None else argv)
    try:
        verify(
            _load_json(arguments.expectation, "expectation"),
            _load_json(arguments.vertex_reflection, "vertex reflection"),
            _load_json(arguments.fragment_reflection, "fragment reflection"),
            _load_text(arguments.vertex_disassembly, "vertex disassembly"),
            _load_text(arguments.fragment_disassembly, "fragment disassembly"),
        )
    except VerificationError as error:
        print(f"material reflection verification failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
