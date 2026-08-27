#!/usr/bin/env python3
"""Focused regression tests for material profile reflection validation."""

from __future__ import annotations

import copy
import unittest
from typing import Any

from verify_material_reflection import VerificationError, verify


def _fixture() -> tuple[dict[str, Any], dict[str, Any], dict[str, Any], str]:
    expectation = {
        "schema": 1,
        "entry_points": [
            {"module": "vertex", "name": "main", "stage": "vert"},
            {"module": "fragment", "name": "main", "stage": "frag"},
        ],
        "vertex_inputs": [{"name": "position", "location": 0, "type": "vec3"}],
        "interstage_variables": [
            {"name": "vary_position", "location": 0, "type": "vec3"},
            {"name": "vary_sign", "location": 2, "type": "float"},
        ],
        "uniform_blocks": [
            {
                "name": "MaterialParams",
                "set": 0,
                "binding": 0,
                "size": 272,
                "stages": ["vertex", "fragment"],
            }
        ],
        "combined_image_samplers": [
            {
                "name": "diffuseMap",
                "set": 1,
                "binding": 0,
                "type": "sampler2D",
                "stages": ["fragment"],
            }
        ],
        "fragment_outputs": [{"name": "frag_data", "location": 0, "type": "vec4"}],
        "push_constant_ranges": [],
        "flat_interfaces": [
            {
                "name": "vary_sign",
                "location": 2,
                "modules": ["vertex", "fragment"],
            }
        ],
    }
    shared_block = {
        "name": "MaterialParams",
        "set": 0,
        "binding": 0,
        "block_size": 272,
    }
    vertex = {
        "entryPoints": [{"name": "main", "mode": "vert"}],
        "inputs": [{"name": "position", "location": 0, "type": "vec3"}],
        "outputs": [
            {"name": "vary_position", "location": 0, "type": "vec3"},
            {"name": "vary_sign", "location": 2, "type": "float"},
        ],
        "ubos": [shared_block],
    }
    fragment = {
        "entryPoints": [{"name": "main", "mode": "frag"}],
        "inputs": [
            {"name": "vary_position", "location": 0, "type": "vec3"},
            {"name": "vary_sign", "location": 2, "type": "float"},
        ],
        "outputs": [{"name": "frag_data", "location": 0, "type": "vec4"}],
        "ubos": [shared_block],
        "textures": [
            {
                "name": "diffuseMap",
                "set": 1,
                "binding": 0,
                "type": "sampler2D",
            }
        ],
    }
    disassembly = "\n".join(
        (
            'OpName %smooth "vary_position"',
            "OpDecorate %smooth Location 0",
            'OpName %vary "vary_sign"',
            "OpDecorate %vary Flat",
            "OpDecorate %vary Location 2",
        )
    )
    return expectation, vertex, fragment, disassembly


class ReflectionGateTests(unittest.TestCase):
    def setUp(self) -> None:
        fixture = _fixture()
        self.expectation, self.vertex, self.fragment, self.disassembly = copy.deepcopy(
            fixture
        )

    def verify(self) -> None:
        verify(
            self.expectation,
            self.vertex,
            self.fragment,
            self.disassembly,
            self.disassembly,
        )

    def test_minimal_valid_fixture_passes(self) -> None:
        self.verify()

    def test_swapped_entry_point_stages_are_rejected(self) -> None:
        self.vertex["entryPoints"][0]["mode"] = "frag"
        self.fragment["entryPoints"][0]["mode"] = "vert"

        with self.assertRaisesRegex(VerificationError, "entry points"):
            self.verify()

    def test_descriptor_arrays_are_rejected(self) -> None:
        self.fragment["textures"][0].update(
            {"array": [2], "array_size_is_literal": [True]}
        )

        with self.assertRaisesRegex(VerificationError, "descriptor array"):
            self.verify()

    def test_unexpected_push_constants_are_rejected(self) -> None:
        self.fragment["push_constants"] = [{"name": "unexpected"}]

        with self.assertRaisesRegex(VerificationError, "unexpected push_constants"):
            self.verify()

    def test_missing_flat_decoration_is_rejected(self) -> None:
        self.disassembly = "\n".join(
            (
                'OpName %vary "vary_sign"',
                "OpDecorate %vary Location 2",
            )
        )

        with self.assertRaisesRegex(VerificationError, "interpolation decorations"):
            self.verify()

    def test_unexpected_interpolation_decorations_are_rejected(self) -> None:
        for interpolation in ("NoPerspective", "Centroid", "Sample"):
            with self.subTest(interpolation=interpolation):
                changed = f"{self.disassembly}\nOpDecorate %smooth {interpolation}"
                with self.assertRaisesRegex(
                    VerificationError, "interpolation decorations"
                ):
                    verify(
                        self.expectation,
                        self.vertex,
                        self.fragment,
                        changed,
                        self.disassembly,
                    )

    def test_interpolation_on_unlocated_data_is_rejected(self) -> None:
        self.disassembly += "\n".join(
            (
                "",
                'OpName %internal "internal"',
                "OpDecorate %internal NoPerspective",
            )
        )

        with self.assertRaisesRegex(VerificationError, "without Location"):
            self.verify()

    def test_diagnostic_outputs_cannot_satisfy_production_expectation(self) -> None:
        output_prototype = self.expectation["fragment_outputs"][0]
        diagnostic_outputs = [
            {**output_prototype, "location": output_prototype["location"] + offset}
            for offset in range(3)
        ]
        self.expectation["fragment_outputs"] = diagnostic_outputs
        self.fragment["outputs"] = [
            {
                **output_prototype,
                "array": [len(diagnostic_outputs)],
                "array_size_is_literal": [True],
            }
        ]
        self.verify()

        production_expectation = copy.deepcopy(self.expectation)
        last_output = production_expectation["fragment_outputs"][-1]
        production_expectation["fragment_outputs"].append(
            {**last_output, "location": last_output["location"] + 1}
        )

        with self.assertRaisesRegex(VerificationError, "fragment outputs"):
            verify(
                production_expectation,
                self.vertex,
                self.fragment,
                self.disassembly,
                self.disassembly,
            )

    def test_output_array_requires_literal_metadata(self) -> None:
        self.fragment["outputs"][0]["array"] = [1]

        with self.assertRaisesRegex(
            VerificationError, "array_size_is_literal must be a JSON array"
        ):
            self.verify()

    def test_output_array_rejects_malformed_literal_metadata(self) -> None:
        self.fragment["outputs"][0].update(
            {"array": [1], "array_size_is_literal": "true"}
        )

        with self.assertRaisesRegex(
            VerificationError, "array_size_is_literal must be a JSON array"
        ):
            self.verify()

    def test_output_array_rejects_mismatched_literal_metadata(self) -> None:
        self.fragment["outputs"][0].update({"array": [1], "array_size_is_literal": []})

        with self.assertRaisesRegex(VerificationError, "must match"):
            self.verify()

    def test_output_array_rejects_nonliteral_dimension(self) -> None:
        self.fragment["outputs"][0].update(
            {"array": [1], "array_size_is_literal": [False]}
        )

        with self.assertRaisesRegex(VerificationError, "must be true"):
            self.verify()


if __name__ == "__main__":
    unittest.main()
