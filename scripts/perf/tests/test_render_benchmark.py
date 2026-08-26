from __future__ import annotations

from contextlib import redirect_stdout
import copy
from io import StringIO
import json
import os
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET


PERF_DIR = Path(__file__).resolve().parents[1]
import sys

sys.path.insert(0, str(PERF_DIR))

import render_benchmark as benchmark


class RendererBenchmarkTests(unittest.TestCase):
    def setUp(self) -> None:
        self.fixture = benchmark.load_json(PERF_DIR / "fixtures" / "renderer-result-v1.json")

    def test_all_scenario_manifests_validate(self) -> None:
        manifests = sorted((PERF_DIR / "scenarios").glob("*.json"))
        self.assertEqual(6, len(manifests))
        for path in manifests:
            with self.subTest(path=path.name):
                benchmark.validate_manifest(benchmark.load_json(path))

    def test_all_requested_settings_exist(self) -> None:
        settings_path = PERF_DIR.parents[1] / "indra" / "newview" / "app_settings" / "settings.xml"
        known_settings = {element.text for element in ET.parse(settings_path).iter("key")}
        self.assertEqual(set(), set(benchmark.OPERATIONAL_SETTINGS) - known_settings)
        for path in sorted((PERF_DIR / "scenarios").glob("*.json")):
            manifest = benchmark.load_json(path)
            with self.subTest(path=path.name):
                self.assertEqual(set(), set(manifest["settings"]) - known_settings)

    def test_manifest_requires_capture_contract(self) -> None:
        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        del manifest["capture"]["duration_seconds"]
        with self.assertRaisesRegex(benchmark.BenchmarkError, "duration_seconds"):
            benchmark.validate_manifest(manifest)

    def test_percentile_uses_linear_interpolation(self) -> None:
        self.assertEqual(2.5, benchmark.percentile([1, 2, 3, 4], 0.5))
        self.assertAlmostEqual(3.85, benchmark.percentile([1, 2, 3, 4], 0.95))

    def test_leap_command_is_encoded_as_llsd_string_notation(self) -> None:
        command = "/tmp/viewer plugin.py --config '/tmp/path with spaces/config.json'"
        notation = benchmark.llsd_string_notation(command)
        self.assertTrue(notation.startswith('"'))
        self.assertEqual(command, json.loads(notation))

    def test_operational_settings_disable_slurl_handoff(self) -> None:
        self.assertTrue(benchmark.OPERATIONAL_SETTINGS["AllowMultipleViewers"])
        self.assertFalse(benchmark.OPERATIONAL_SETTINGS["SLURLPassToOtherInstance"])
        self.assertIn("--noaudio", benchmark.OPERATIONAL_SWITCHES)
        self.assertIn("--nonotifications", benchmark.OPERATIONAL_SWITCHES)
        self.assertIn("--novoice", benchmark.OPERATIONAL_SWITCHES)

    def test_warm_runs_have_an_unmeasured_prime(self) -> None:
        self.assertEqual([0, 1, 2, 3], benchmark.benchmark_run_numbers("warm", 3))
        self.assertEqual([1, 2, 3], benchmark.benchmark_run_numbers("cold", 3))

    def test_runner_rejects_zero_repeats(self) -> None:
        parser = benchmark.build_parser()
        with tempfile.TemporaryDirectory() as temp_name:
            output_dir = Path(temp_name) / "results"
            args = parser.parse_args([
                "run",
                "--viewer", "/viewer/SecondLife",
                "--manifest", str(PERF_DIR / "scenarios" / "steady-warm-v1.json"),
                "--credential-file", str(Path(temp_name) / "does-not-exist"),
                "--slurl", "secondlife://example/128/128/25",
                "--hardware-label", "fixture-hardware",
                "--output-dir", str(output_dir),
                "--repeats", "0",
                "--dry-run",
            ])
            with self.assertRaisesRegex(benchmark.BenchmarkError, "repeats must be positive"):
                args.handler(args)
            self.assertFalse(output_dir.exists())

    def test_summary_includes_tail_and_resource_deltas(self) -> None:
        summary = benchmark.summarize_result(self.fixture)
        self.assertEqual(4, summary["sample_count"])
        self.assertEqual(11.5, summary["frame_time_ms"]["median"])
        self.assertEqual(30.0, summary["frame_time_ms"]["worst"])
        self.assertEqual(6144.0, summary["counter_deltas"]["texture_upload_bytes"])
        self.assertEqual(1.0, summary["counter_deltas"]["shader_compile_count"])

    def test_result_schema_rejects_invalid_runs(self) -> None:
        invalid = copy.deepcopy(self.fixture)
        invalid["status"] = "invalid"
        invalid["failure_reason"] = "asset load incomplete"
        with self.assertRaisesRegex(benchmark.BenchmarkError, "asset load incomplete"):
            benchmark.validate_result(invalid)
        benchmark.validate_result(invalid, require_valid=False)

    def test_comparison_rejects_changed_context(self) -> None:
        changed = copy.deepcopy(self.fixture)
        changed["context"]["width"] = 1920
        changed["run"]["settings_hash"] = "different"
        mismatches = benchmark.comparison_mismatches([self.fixture, changed])
        self.assertIn("context.width", mismatches)
        self.assertIn("run.settings_hash", mismatches)

    def test_comparison_allows_extension_difference_between_backends_only(self) -> None:
        zink = copy.deepcopy(self.fixture)
        zink["context"]["backend_label"] = "zink"
        zink["context"]["detected_backend"] = "zink"
        zink["context"]["gl_extensions_hash"] = "zink-extensions"
        self.assertEqual({}, benchmark.comparison_mismatches([self.fixture, zink]))

        changed_native = copy.deepcopy(self.fixture)
        changed_native["context"]["gl_extensions_hash"] = "changed-native-extensions"
        mismatches = benchmark.comparison_mismatches([self.fixture, changed_native])
        self.assertIn("context.gl_extensions_hash[native-gl]", mismatches)

    def test_report_applies_cross_backend_noise_threshold(self) -> None:
        results = []
        for run_number in range(1, benchmark.MIN_COMPARISON_REPEATS + 1):
            native = copy.deepcopy(self.fixture)
            native["run"]["run_number"] = run_number
            results.append(native)
            zink = copy.deepcopy(self.fixture)
            zink["run"]["run_number"] = run_number
            zink["context"]["backend_label"] = "zink"
            zink["context"]["detected_backend"] = "zink"
            zink["context"]["gl_extensions_hash"] = "zink-extensions"
            for frame in zink["frames"]:
                frame["frame_time_ms"] += 5.0
            results.append(zink)
        report = benchmark._format_markdown(results, None)
        self.assertIn("Zink median run p95", report)
        self.assertIn("meaningful", report)

    def test_report_requires_five_repeats_for_a_decision(self) -> None:
        native = copy.deepcopy(self.fixture)
        zink = copy.deepcopy(self.fixture)
        zink["context"]["backend_label"] = "zink"
        zink["context"]["detected_backend"] = "zink"
        zink["context"]["gl_extensions_hash"] = "zink-extensions"
        report = benchmark._format_markdown([native, zink], None)
        self.assertIn("Decision is indeterminate", report)
        self.assertIn("5 per backend are required", report)

    def test_sanitize_removes_private_fields_recursively(self) -> None:
        unsafe = copy.deepcopy(self.fixture)
        unsafe["context"]["machine_id"] = "machine-secret"
        unsafe["run"]["username"] = "account-secret"
        unsafe["frames"][0]["parcel"] = "private-place"
        safe = benchmark.sanitize(unsafe)
        serialized = json.dumps(safe)
        self.assertNotIn("machine-secret", serialized)
        self.assertNotIn("account-secret", serialized)
        self.assertNotIn("private-place", serialized)
        benchmark.validate_result(safe)

    def test_checked_in_fixture_is_valid_and_private(self) -> None:
        benchmark.validate_result(self.fixture)
        self.assertEqual([], benchmark.find_private_paths(self.fixture))
        self.assertEqual(self.fixture["summary"], benchmark.summarize_result(self.fixture))

    def test_dry_run_does_not_open_credentials_and_redacts_login(self) -> None:
        parser = benchmark.build_parser()
        with tempfile.TemporaryDirectory() as temp_name:
            args = parser.parse_args([
                "run",
                "--viewer", "/viewer/SecondLife",
                "--manifest", str(PERF_DIR / "scenarios" / "steady-warm-v1.json"),
                "--credential-file", str(Path(temp_name) / "does-not-exist"),
                "--slurl", "secondlife://example/128/128/25",
                "--hardware-label", "fixture-hardware",
                "--output-dir", temp_name,
                "--repeats", "1",
                "--dry-run",
            ])
            output = StringIO()
            with redirect_stdout(output):
                self.assertEqual(0, args.handler(args))
            rendered = output.getvalue()
            self.assertIn("<redacted>", rendered)
            self.assertNotIn("does-not-exist", rendered)

    @unittest.skipIf(os.name == "nt", "POSIX credential modes do not apply on Windows")
    def test_credentials_must_be_private(self) -> None:
        with tempfile.TemporaryDirectory() as temp_name:
            credential = Path(temp_name) / "account.txt"
            credential.write_text("first last password\n", encoding="utf-8")
            credential.chmod(0o644)
            with self.assertRaisesRegex(benchmark.BenchmarkError, "group or other"):
                benchmark._read_credentials(credential)
            credential.chmod(0o600)
            self.assertEqual(("first", "last", "password"), benchmark._read_credentials(credential))


if __name__ == "__main__":
    unittest.main()
