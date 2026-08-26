from __future__ import annotations

from contextlib import redirect_stdout
import copy
from io import StringIO
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import xml.etree.ElementTree as ET


PERF_DIR = Path(__file__).resolve().parents[1]
import sys

sys.path.insert(0, str(PERF_DIR))

import render_benchmark as benchmark
import render_benchmark_leap as collector


class RendererBenchmarkTests(unittest.TestCase):
    def setUp(self) -> None:
        self.fixture = benchmark.load_json(PERF_DIR / "fixtures" / "renderer-result-v3.json")
        self.operator_args = [
            "--workload-id", "fixture-steady-scene",
            "--power-source", "ac",
            "--low-power-mode", "off",
            "--thermal-state", "nominal",
            "--scene-events", "none",
            "--ui-state", "approved",
            "--camera-state", "approved",
        ]

    def match_result_to_manifest(self, result: dict[str, object], manifest: dict[str, object]) -> None:
        result["run"]["manifest_hash"] = benchmark.canonical_hash(manifest)
        result["run"]["settings_hash"] = benchmark.canonical_hash(manifest["settings"])
        result["validity"]["policy"] = copy.deepcopy(manifest["validity"])
        result["validity"]["policy_hash"] = benchmark.canonical_hash(manifest["validity"])

    def appearance_attribution(self, classification: str = "ready") -> dict[str, object]:
        attribution: dict[str, object] = {
            "classification": classification,
            "avatar_valid": True,
            "cof_present": True,
            "cof_complete": True,
            "cof_change_in_progress": False,
            "required_links_resolved": {
                "shape": True,
                "skin": True,
                "hair": True,
                "eyes": True,
            },
            "required_wearables_delivered": {
                "shape": True,
                "skin": True,
                "hair": True,
                "eyes": True,
            },
            "avatar_loaded": True,
        }
        if classification == "avatar-unavailable":
            attribution["avatar_valid"] = False
            attribution["avatar_loaded"] = False
        elif classification == "cof-incomplete":
            attribution["cof_complete"] = False
            attribution["avatar_loaded"] = False
        elif classification == "required-link-missing-or-unresolved":
            attribution["required_links_resolved"]["shape"] = False
            attribution["avatar_loaded"] = False
        elif classification == "wearable-delivery-pending-or-failed":
            attribution["required_wearables_delivered"]["hair"] = False
            attribution["avatar_loaded"] = False
        elif classification == "avatar-later-blocker":
            attribution["avatar_loaded"] = False
        return attribution

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

    def test_manifest_requires_normalized_display_contract(self) -> None:
        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        manifest["settings"]["WindowWidth"] = 1920
        with self.assertRaisesRegex(benchmark.BenchmarkError, "display contract mismatch"):
            benchmark.validate_manifest(manifest)

        manifest["settings"]["WindowWidth"] = benchmark.BENCHMARK_BACKING_WIDTH
        manifest["settings"]["UIScaleFactor"] = 0.5
        with self.assertRaisesRegex(benchmark.BenchmarkError, "must be derived"):
            benchmark.validate_manifest(manifest)

        del manifest["settings"]["UIScaleFactor"]
        manifest["settings"]["RenderHiDPI"] = False
        with self.assertRaisesRegex(benchmark.BenchmarkError, "RenderHiDPI"):
            benchmark.validate_manifest(manifest)

    def test_manifest_requires_scene_validity_contract(self) -> None:
        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        del manifest["validity"]["asset_mode"]
        with self.assertRaisesRegex(benchmark.BenchmarkError, "asset_mode"):
            benchmark.validate_manifest(manifest)

        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        manifest["validity"]["settle_seconds"] = 31
        with self.assertRaisesRegex(benchmark.BenchmarkError, "fit inside"):
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
        self.assertFalse(benchmark.OPERATIONAL_SETTINGS["FirstLoginThisInstall"])
        self.assertFalse(benchmark.OPERATIONAL_SETTINGS["MigrateCacheDirectory"])
        self.assertFalse(benchmark.OPERATIONAL_SETTINGS["SLURLPassToOtherInstance"])
        self.assertIn("--noaudio", benchmark.OPERATIONAL_SWITCHES)
        self.assertIn("--nonotifications", benchmark.OPERATIONAL_SWITCHES)
        self.assertIn("--novoice", benchmark.OPERATIONAL_SWITCHES)

    def test_warm_runs_have_an_unmeasured_prime(self) -> None:
        self.assertEqual([0, 1, 2, 3], benchmark.benchmark_run_numbers("warm", 3))
        self.assertEqual([1, 2, 3], benchmark.benchmark_run_numbers("cold", 3))
        self.assertEqual(
            [0, 0],
            benchmark.benchmark_run_numbers("warm", 5, warm_prime_attempts=2, prime_only=True),
        )

    def test_disposable_cache_selection_is_complete_and_precreated(self) -> None:
        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        with tempfile.TemporaryDirectory() as temp_name:
            state = Path(temp_name) / "state"
            settings = benchmark.benchmark_state_settings(manifest["settings"], state, "warm")
            self.assertTrue((state / "user").is_dir())
            self.assertTrue((state / "cache").is_dir())
            self.assertEqual(str(state / "cache"), settings["CacheLocation"])
            self.assertEqual(settings["CacheLocation"], settings["NewCacheLocation"])
            self.assertFalse(settings["MigrateCacheDirectory"])
            self.assertFalse(settings["PurgeCacheOnStartup"])
            self.assertEqual("ready", benchmark._cache_probe(state / "cache"))

            (state / "cache").rmdir()
            benchmark.benchmark_state_settings(manifest["settings"], state, "warm", initialize=False)
            self.assertFalse((state / "cache").exists())
            self.assertEqual("root-missing", benchmark._cache_probe(state / "cache"))

    def test_cache_probe_reports_cleanup_failure(self) -> None:
        original_unlink = Path.unlink
        unlink_calls = 0

        def fail_final_cleanup(path: Path, *args: object, **kwargs: object) -> None:
            nonlocal unlink_calls
            unlink_calls += 1
            if unlink_calls == 3:
                raise OSError("simulated cleanup failure")
            original_unlink(path, *args, **kwargs)

        with tempfile.TemporaryDirectory() as temp_name:
            with patch.object(Path, "unlink", new=fail_final_cleanup):
                self.assertEqual("cleanup-failed", benchmark._cache_probe(Path(temp_name)))

    def test_collector_reapplies_requested_settings_at_runtime(self) -> None:
        class FakeAPI:
            def __init__(self) -> None:
                self.requests: list[tuple[str, dict[str, object]]] = []

            def request(self, pump: str, data: dict[str, object]) -> dict[str, object]:
                self.requests.append((pump, data))
                return {"value": data["value"]}

        api = FakeAPI()
        collector.apply_requested_settings({"RenderShadowDetail": 2}, api)
        self.assertEqual(
            [("LLViewerControl", {
                "op": "set",
                "group": "Global",
                "key": "RenderShadowDetail",
                "value": 2,
            })],
            api.requests,
        )

    def test_collector_requests_display_normalization_after_startup(self) -> None:
        class FakeAPI:
            def __init__(self) -> None:
                self.requests: list[tuple[str, dict[str, object]]] = []

            def request(self, pump: str, data: dict[str, object]) -> dict[str, object]:
                self.requests.append((pump, data))
                return {"accepted": True}

        api = FakeAPI()
        collector.normalize_renderer_display(api)
        self.assertEqual(
            [("LLStats", {"op": "normalizeRendererDisplay"})],
            api.requests,
        )

    def test_appearance_sampling_is_prime_only_and_retains_the_last_blocker(self) -> None:
        classifications = [
            "cof-incomplete",
            "wearable-delivery-pending-or-failed",
            "ready",
        ]
        diagnostics = [
            {
                "scene_state": {
                    "self_avatar_loaded": classification == "ready",
                },
                "appearance": self.appearance_attribution(classification),
            }
            for classification in classifications
        ]
        class FakeAPI:
            def __init__(self, values: list[dict[str, object]]) -> None:
                self.values = values

            def renderer_diagnostic_state(self) -> dict[str, object]:
                return self.values.pop(0)

        api = FakeAPI(copy.deepcopy(diagnostics))
        samples = [collector.sample_appearance_attribution(api) for _ in diagnostics]
        summary = collector.summarize_appearance_attribution(
            [scene for scene, _ in samples],
            [appearance for _, appearance in samples],
        )
        self.assertIsNotNone(summary)
        self.assertEqual("wearable-delivery-pending-or-failed", summary["classification"])

        inconsistent = copy.deepcopy(diagnostics[-1])
        inconsistent["scene_state"]["self_avatar_loaded"] = False
        with self.assertRaisesRegex(collector.ProtocolError, "does not match"):
            collector.sample_appearance_attribution(FakeAPI([inconsistent]))

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
                *self.operator_args,
                "--output-dir", str(output_dir),
                "--repeats", "0",
                "--dry-run",
            ])
            with self.assertRaisesRegex(benchmark.BenchmarkError, "repeats must be positive"):
                args.handler(args)
            self.assertFalse(output_dir.exists())

    def test_prime_only_rejects_a_cold_cache_manifest(self) -> None:
        parser = benchmark.build_parser()
        with tempfile.TemporaryDirectory() as temp_name:
            output_dir = Path(temp_name) / "results"
            args = parser.parse_args([
                "run",
                "--viewer", "/viewer/SecondLife",
                "--manifest", str(PERF_DIR / "scenarios" / "cold-streaming-v1.json"),
                "--credential-file", str(Path(temp_name) / "does-not-exist"),
                "--slurl", "secondlife://example/128/128/25",
                "--hardware-label", "fixture-hardware",
                *self.operator_args,
                "--output-dir", str(output_dir),
                "--prime-only",
                "--dry-run",
            ])
            with self.assertRaisesRegex(benchmark.BenchmarkError, "require a warm-cache manifest"):
                args.handler(args)
            self.assertFalse(output_dir.exists())

    def test_multiple_warm_primes_require_prime_only_mode(self) -> None:
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
                *self.operator_args,
                "--output-dir", str(output_dir),
                "--warm-prime-attempts", "2",
                "--dry-run",
            ])
            with self.assertRaisesRegex(benchmark.BenchmarkError, "require --prime-only"):
                args.handler(args)
            self.assertFalse(output_dir.exists())

    def test_readiness_output_requires_two_shared_cache_launches(self) -> None:
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
                *self.operator_args,
                "--output-dir", str(output_dir),
                "--prime-only",
                "--readiness-output", str(Path(temp_name) / "readiness.json"),
                "--dry-run",
            ])
            with self.assertRaisesRegex(benchmark.BenchmarkError, "at least two warm primes"):
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

    def test_old_result_schema_is_rejected(self) -> None:
        old = copy.deepcopy(self.fixture)
        old["schema_version"] = 2
        with self.assertRaisesRegex(benchmark.BenchmarkError, "unsupported result schema 2"):
            benchmark.validate_result(old)

    def test_result_requires_display_geometry(self) -> None:
        missing = copy.deepcopy(self.fixture)
        del missing["context"]["backing_scale_x"]
        with self.assertRaisesRegex(benchmark.BenchmarkError, "backing_scale_x"):
            benchmark.validate_result(missing)

        malformed = copy.deepcopy(self.fixture)
        malformed["context"]["logical_width"] = "640"
        with self.assertRaisesRegex(benchmark.BenchmarkError, "logical_width"):
            benchmark.validate_result(malformed)

    def test_result_must_match_requested_settings_and_resolution(self) -> None:
        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        result = copy.deepcopy(self.fixture)
        self.match_result_to_manifest(result, manifest)
        result["context"]["effective_settings"] = copy.deepcopy(manifest["settings"])
        benchmark.validate_result_against_manifest(result, manifest)

        result["context"]["width"] = 1920
        with self.assertRaisesRegex(benchmark.BenchmarkError, "actual width=1920"):
            benchmark.validate_result_against_manifest(result, manifest)

        result["context"]["width"] = manifest["settings"]["WindowWidth"]
        result["context"]["effective_settings"]["RenderFarClip"] = 64.0
        with self.assertRaisesRegex(benchmark.BenchmarkError, "RenderFarClip=64.0"):
            benchmark.validate_result_against_manifest(result, manifest)

    def test_diagnostic_validation_allows_only_failed_scene_gates(self) -> None:
        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        result = copy.deepcopy(self.fixture)
        self.match_result_to_manifest(result, manifest)
        result["context"]["effective_settings"] = copy.deepcopy(manifest["settings"])
        result["validity"]["observed"]["background_frame_count"] = 1
        result["validity"]["gates"] = benchmark.validity_gate_results(
            result["validity"]["workload_id"],
            result["validity"]["operator"],
            result["validity"]["observed"],
            result["validity"]["policy"],
        )
        result["status"] = "invalid"
        result["failure_reason"] = "scene validity gates failed: focus"
        result.pop("summary")
        benchmark.validate_result(result, require_valid=False)
        benchmark.validate_result_against_manifest(result, manifest, require_all_gates=False)
        with self.assertRaisesRegex(benchmark.BenchmarkError, "do not pass"):
            benchmark.validate_result_against_manifest(result, manifest)

    def test_diagnostic_validation_still_requires_frame_samples(self) -> None:
        invalid = copy.deepcopy(self.fixture)
        invalid["status"] = "invalid"
        invalid["failure_reason"] = "scene validity gates failed: assets"
        invalid["frames"] = []
        with self.assertRaisesRegex(benchmark.BenchmarkError, "no numeric frame_time_ms"):
            benchmark.validate_result(invalid, require_valid=False)

    def test_display_contract_accepts_1x_and_2x_geometry(self) -> None:
        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        retina = copy.deepcopy(self.fixture)
        self.match_result_to_manifest(retina, manifest)
        retina["context"]["effective_settings"] = copy.deepcopy(manifest["settings"])
        benchmark.validate_result_against_manifest(retina, manifest)

        standard = copy.deepcopy(retina)
        standard["context"].update({
            "backing_scale_x": 1.0,
            "backing_scale_y": 1.0,
            "configured_ui_scale": 1.0,
            "logical_width": 1280,
            "logical_height": 720,
        })
        benchmark.validate_result_against_manifest(standard, manifest)

    def test_display_contract_rejects_scale_and_geometry_mismatches(self) -> None:
        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        result = copy.deepcopy(self.fixture)
        self.match_result_to_manifest(result, manifest)
        result["context"]["effective_settings"] = copy.deepcopy(manifest["settings"])
        result["context"]["effective_display_scale_x"] = 2.0
        result["context"]["logical_height"] = 720
        with self.assertRaisesRegex(benchmark.BenchmarkError, "effective_display_scale_x"):
            benchmark.validate_result_against_manifest(result, manifest)
        with self.assertRaisesRegex(benchmark.BenchmarkError, "logical y geometry"):
            benchmark.validate_result_against_manifest(result, manifest)

    def test_comparison_rejects_changed_context(self) -> None:
        changed = copy.deepcopy(self.fixture)
        changed["context"]["width"] = 1920
        changed["run"]["settings_hash"] = "different"
        mismatches = benchmark.comparison_mismatches([self.fixture, changed])
        self.assertIn("context.width", mismatches)
        self.assertIn("run.settings_hash", mismatches)

        changed_scale = copy.deepcopy(self.fixture)
        changed_scale["context"]["backing_scale_x"] = 1.0
        mismatches = benchmark.comparison_mismatches([self.fixture, changed_scale])
        self.assertIn("context.backing_scale_x", mismatches)

        changed_workload = copy.deepcopy(self.fixture)
        changed_workload["validity"]["workload_id"] = "another-steady-scene"
        mismatches = benchmark.comparison_mismatches([self.fixture, changed_workload])
        self.assertIn("validity.workload_id", mismatches)

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

    def test_sanitize_removes_raw_destination_and_view(self) -> None:
        unsafe = {"destination": "private-place", "view": {"origin": [1, 2, 3]}, "safe": True}
        self.assertEqual({"safe": True}, benchmark.sanitize(unsafe))

    def test_appearance_projection_is_fixed_and_fails_closed(self) -> None:
        private_text = "secret account inventory and /private/path"
        projected = benchmark.safe_appearance_attribution({
            "classification": private_text,
            "avatar_valid": 1,
            "cof_present": True,
            "cof_complete": private_text,
            "cof_change_in_progress": False,
            "required_links_resolved": {
                "shape": True,
                "skin": private_text,
                "extra-private-part": private_text,
            },
            "required_wearables_delivered": private_text,
            "avatar_loaded": True,
            "raw_log": private_text,
        })
        serialized = json.dumps(projected)
        self.assertEqual("unknown", projected["classification"])
        self.assertFalse(projected["avatar_valid"])
        self.assertFalse(projected["cof_complete"])
        self.assertEqual(
            {"shape": True, "skin": False, "hair": False, "eyes": False},
            projected["required_links_resolved"],
        )
        self.assertNotIn(private_text, serialized)
        self.assertNotIn("extra-private-part", serialized)
        self.assertNotIn("raw_log", serialized)

        for classification in sorted(benchmark.APPEARANCE_CLASSIFICATIONS - {"unknown"}):
            with self.subTest(classification=classification):
                facts = self.appearance_attribution(classification)
                self.assertEqual(
                    classification,
                    benchmark.safe_appearance_attribution(facts)["classification"],
                )

        contradictory = self.appearance_attribution("ready")
        contradictory["classification"] = "cof-incomplete"
        self.assertEqual(
            "unknown",
            benchmark.safe_appearance_attribution(contradictory)["classification"],
        )
        malformed_classification = self.appearance_attribution()
        malformed_classification["classification"] = [private_text]
        self.assertEqual(
            "unknown",
            benchmark.safe_appearance_attribution(malformed_classification)["classification"],
        )

    def test_optional_appearance_does_not_change_gates_or_policy_hash(self) -> None:
        result = copy.deepcopy(self.fixture)
        original_gates = copy.deepcopy(result["validity"]["gates"])
        original_policy_hash = result["validity"]["policy_hash"]
        result["validity"]["observed"]["appearance"] = self.appearance_attribution()
        benchmark.validate_result(result)
        self.assertEqual(original_gates, result["validity"]["gates"])
        self.assertEqual(original_policy_hash, result["validity"]["policy_hash"])

        result["validity"]["observed"]["appearance"]["inventory_id"] = "private"
        with self.assertRaisesRegex(benchmark.BenchmarkError, "privacy-safe contract"):
            benchmark.validate_result(result)

        numeric_boolean = copy.deepcopy(self.fixture)
        numeric_boolean["validity"]["observed"]["appearance"] = self.appearance_attribution(
            "unknown"
        )
        numeric_boolean["validity"]["observed"]["appearance"]["avatar_valid"] = 0
        with self.assertRaisesRegex(benchmark.BenchmarkError, "privacy-safe contract"):
            benchmark.validate_result(numeric_boolean)

    def test_result_rejects_tampered_scene_gates(self) -> None:
        tampered = copy.deepcopy(self.fixture)
        tampered["validity"]["observed"]["destination_ready"] = False
        with self.assertRaisesRegex(benchmark.BenchmarkError, "gates do not match"):
            benchmark.validate_result(tampered)

    def test_scene_gate_failures_are_machine_checkable(self) -> None:
        cases = {
            "placement": ("destination_ready", False),
            "focus": ("background_frame_count", 1),
            "assets": ("texture_fetch_requests_max", 1),
            "population": ("visible_avatars_max", 2),
            "ui": ("modal_dialog_max", 1),
        }
        for expected_gate, (field, value) in cases.items():
            with self.subTest(gate=expected_gate):
                observed = copy.deepcopy(self.fixture["validity"]["observed"])
                observed[field] = value
                gates = benchmark.validity_gate_results(
                    self.fixture["validity"]["workload_id"],
                    self.fixture["validity"]["operator"],
                    observed,
                    self.fixture["validity"]["policy"],
                )
                self.assertFalse(gates[expected_gate])

    def test_scene_gate_rejects_missing_view_fingerprint(self) -> None:
        observed = copy.deepcopy(self.fixture["validity"]["observed"])
        observed["view_hash"] = ""
        gates = benchmark.validity_gate_results(
            self.fixture["validity"]["workload_id"],
            self.fixture["validity"]["operator"],
            observed,
            self.fixture["validity"]["policy"],
        )
        self.assertFalse(gates["camera"])

    def test_missing_observation_fails_standard_gates(self) -> None:
        observed = copy.deepcopy(self.fixture["validity"]["observed"])
        del observed["view_hash"]
        gates = benchmark.validity_gate_results(
            self.fixture["validity"]["workload_id"],
            self.fixture["validity"]["operator"],
            observed,
            self.fixture["validity"]["policy"],
        )
        self.assertEqual(set(benchmark.VALIDITY_GATE_NAMES), set(gates))
        self.assertFalse(any(gates.values()))

    def test_rejection_message_extracts_only_known_failed_gates(self) -> None:
        rejected = copy.deepcopy(self.fixture)
        rejected["validity"]["gates"]["focus"] = False
        rejected["validity"]["gates"]["private-location"] = False
        self.assertEqual(["focus"], benchmark.failed_validity_gate_names(rejected))

    def test_private_logs_reduce_to_readiness_categories(self) -> None:
        with tempfile.TemporaryDirectory() as temp_name:
            log_root = Path(temp_name) / "user" / "logs"
            log_root.mkdir(parents=True)
            (log_root / "SecondLife.log").write_text(
                "private account and destination Failure in vf.write()\n"
                "Self is clouded due to missing one or more required body parts: SHAPE\n",
                encoding="utf-8",
            )
            self.assertEqual(
                "asset-cache-write",
                benchmark._first_log_category(log_root, benchmark.CACHE_FAILURE_SIGNATURES),
            )
            self.assertEqual(
                "required-bodyparts-missing",
                benchmark._first_log_category(log_root, benchmark.AVATAR_BLOCKER_SIGNATURES),
            )

    def test_readiness_does_not_reuse_a_failure_from_an_older_log(self) -> None:
        with tempfile.TemporaryDirectory() as temp_name:
            log_root = Path(temp_name) / "logs"
            log_root.mkdir(parents=True)
            viewer_log = log_root / "SecondLife.log"
            viewer_log.write_text("Failure in vf.write()\n", encoding="utf-8")
            cursor = benchmark._viewer_log_cursor(log_root)
            with viewer_log.open("a", encoding="utf-8") as stream:
                stream.write("clean second launch\n")
            self.assertIsNone(
                benchmark._first_log_category(
                    log_root, benchmark.CACHE_FAILURE_SIGNATURES, cursor
                )
            )
            viewer_log.rename(log_root / "SecondLife.old")
            viewer_log.write_text("clean rotated launch\n", encoding="utf-8")
            self.assertIsNone(
                benchmark._first_log_category(
                    log_root, benchmark.CACHE_FAILURE_SIGNATURES, cursor
                )
            )
            with viewer_log.open("a", encoding="utf-8") as stream:
                stream.write("Failure in vf.write()\n")
            self.assertEqual(
                "asset-cache-write",
                benchmark._first_log_category(
                    log_root, benchmark.CACHE_FAILURE_SIGNATURES, cursor
                ),
            )

    def test_readiness_report_contains_no_timing_or_private_log_text(self) -> None:
        rejected = copy.deepcopy(self.fixture)
        rejected["validity"]["observed"]["self_avatar_loaded"] = False
        rejected["validity"]["observed"]["texture_fetch_requests_max"] = 1
        rejected["validity"]["gates"] = benchmark.validity_gate_results(
            rejected["validity"]["workload_id"],
            rejected["validity"]["operator"],
            rejected["validity"]["observed"],
            rejected["validity"]["policy"],
        )
        with tempfile.TemporaryDirectory() as temp_name:
            state = Path(temp_name) / "state"
            benchmark.benchmark_state_settings({}, state, "warm")
            log_root = state / "user" / "logs"
            log_root.mkdir(parents=True)
            (log_root / "SecondLife.log").write_text(
                "secret-user at secret-place Failure in vf.write()\n"
                "Self is clouded because lower textures not baked\n",
                encoding="utf-8",
            )
            before = benchmark.cache_lifecycle_facts(state)
            attempt = benchmark.readiness_attempt(1, "rejected", rejected, before, state)
            output = Path(temp_name) / "readiness.json"
            benchmark.write_readiness_report(output, [attempt])
            report = benchmark.load_json(output)
            serialized = json.dumps(report)
            self.assertFalse(report["readiness_passed"])
            self.assertFalse(report["cache_reuse_passed"])
            self.assertFalse(report["all_scene_gates_passed"])
            self.assertEqual(["assets", "avatar"], report["target_gates"])
            self.assertFalse(report["retained_timing"])
            self.assertEqual(0, report["valid_measured_repeats"])
            self.assertNotIn("frames", serialized)
            self.assertNotIn("summary", serialized)
            self.assertNotIn("secret-user", serialized)
            self.assertNotIn("secret-place", serialized)
            self.assertNotIn(temp_name, serialized)
            self.assertEqual(
                "asset-cache-write",
                report["attempts"][0]["first_cache_failure"],
            )
            self.assertEqual(
                {"assets": False, "avatar": False},
                report["attempts"][0]["target_gates"],
            )
            self.assertFalse(report["attempts"][0]["assets"]["queues_settled"])
            self.assertEqual("lower-bake-missing", report["attempts"][0]["avatar"]["blocker"])

    def test_readiness_report_projects_malformed_scalars_to_safe_types(self) -> None:
        malformed = copy.deepcopy(self.fixture)
        private_text = "private-account at /private/location from raw log"
        malformed["validity"]["observed"]["texture_fetch_requests_max"] = private_text
        malformed["validity"]["gates"]["assets"] = private_text
        with tempfile.TemporaryDirectory() as temp_name:
            state = Path(temp_name) / "state"
            benchmark.benchmark_state_settings({}, state, "warm")
            attempt = benchmark.readiness_attempt(1, "invalid-artifact", malformed, {}, state)
            output = Path(temp_name) / "readiness.json"
            benchmark.write_readiness_report(output, [attempt])
            report = benchmark.load_json(output)
            serialized = json.dumps(report)
            self.assertNotIn(private_text, serialized)
            self.assertIsNone(report["attempts"][0]["target_gates"]["assets"])
            self.assertIsNone(
                report["attempts"][0]["assets"]["observed"]["texture_fetch_requests_max"]
            )

    def test_readiness_prefers_fixed_appearance_attribution_over_log_text(self) -> None:
        result = copy.deepcopy(self.fixture)
        result["validity"]["observed"]["self_avatar_loaded"] = False
        appearance = self.appearance_attribution("wearable-delivery-pending-or-failed")
        appearance["required_wearables_delivered"]["hair"] = False
        result["validity"]["observed"]["appearance"] = appearance
        result["validity"]["gates"] = benchmark.validity_gate_results(
            result["validity"]["workload_id"],
            result["validity"]["operator"],
            result["validity"]["observed"],
            result["validity"]["policy"],
        )
        with tempfile.TemporaryDirectory() as temp_name:
            state = Path(temp_name) / "state"
            benchmark.benchmark_state_settings({}, state, "warm")
            log_root = state / "user" / "logs"
            log_root.mkdir(parents=True)
            (log_root / "SecondLife.log").write_text(
                "Self is clouded because lower textures not baked\n",
                encoding="utf-8",
            )
            attempts = [
                benchmark.readiness_attempt(index, "rejected", result, {}, state)
                for index in (1, 2)
            ]
            output = Path(temp_name) / "readiness.json"
            benchmark.write_readiness_report(output, attempts)
            report = benchmark.load_json(output)
            self.assertEqual(
                "wearable-delivery-pending-or-failed",
                report["attempts"][0]["avatar"]["blocker"],
            )
            self.assertEqual(
                "wearable-delivery-pending-or-failed",
                report["attempts"][1]["avatar"]["appearance"]["classification"],
            )
            self.assertFalse(report["retained_timing"])
            self.assertEqual(0, report["valid_measured_repeats"])

    def test_readiness_can_pass_while_other_scene_gates_remain_explicit(self) -> None:
        rejected = copy.deepcopy(self.fixture)
        rejected["validity"]["observed"]["background_frame_count"] = 1
        rejected["validity"]["gates"] = benchmark.validity_gate_results(
            rejected["validity"]["workload_id"],
            rejected["validity"]["operator"],
            rejected["validity"]["observed"],
            rejected["validity"]["policy"],
        )
        with tempfile.TemporaryDirectory() as temp_name:
            state = Path(temp_name) / "state"
            benchmark.benchmark_state_settings({}, state, "warm")
            before_first = benchmark.cache_lifecycle_facts(state)
            asset_root = state / "cache" / "cache"
            asset_root.mkdir()
            (asset_root / "asset").write_bytes(b"cached")
            first = benchmark.readiness_attempt(
                1,
                "readiness-passed",
                rejected,
                before_first,
                state,
                prepare_reuse=True,
            )
            self.assertEqual("ready", first["sentinel_install"])
            before_second = benchmark.cache_lifecycle_facts(state)
            self.assertEqual("ready", before_second["requested_sentinel"])
            second = benchmark.readiness_attempt(
                2, "readiness-passed", rejected, before_second, state
            )
            self.assertTrue(second["cache_ready"])
            output = Path(temp_name) / "readiness.json"
            benchmark.write_readiness_report(output, [first, second])
            report = benchmark.load_json(output)
            self.assertTrue(report["readiness_passed"])
            self.assertTrue(report["cache_reuse_passed"])
            self.assertFalse(report["all_scene_gates_passed"])
            self.assertEqual(["focus"], report["attempts"][1]["failed_gates"])

    def test_rejected_first_prime_prepares_cache_reuse(self) -> None:
        manifest = benchmark.load_json(PERF_DIR / "scenarios" / "steady-warm-v1.json")
        results = [copy.deepcopy(self.fixture), copy.deepcopy(self.fixture)]
        self.match_result_to_manifest(results[0], manifest)
        self.match_result_to_manifest(results[1], manifest)
        results[0]["validity"]["observed"]["texture_fetch_requests_max"] = 1
        results[0]["validity"]["observed"]["self_avatar_loaded"] = False
        results[0]["validity"]["gates"] = benchmark.validity_gate_results(
            results[0]["validity"]["workload_id"],
            results[0]["validity"]["operator"],
            results[0]["validity"]["observed"],
            results[0]["validity"]["policy"],
        )
        results[0]["status"] = "invalid"
        results[0]["failure_reason"] = "scene validity gates failed: avatar, assets"
        results[0].pop("summary")

        with tempfile.TemporaryDirectory() as temp_name:
            readiness = Path(temp_name) / "readiness.json"
            parser = benchmark.build_parser()
            args = parser.parse_args([
                "run",
                "--viewer", "/viewer/SecondLife",
                "--manifest", str(PERF_DIR / "scenarios" / "steady-warm-v1.json"),
                "--credential-file", str(Path(temp_name) / "account.txt"),
                "--slurl", "secondlife://example/128/128/25",
                "--hardware-label", "fixture-hardware",
                *self.operator_args,
                "--output-dir", str(Path(temp_name) / "results"),
                "--warm-prime-attempts", "2",
                "--prime-only",
                "--readiness-output", str(readiness),
            ])
            launch = 0

            def fake_viewer_run(command: list[str], **kwargs: object) -> object:
                nonlocal launch
                launch += 1
                leap = json.loads(command[command.index("--leap") + 1])
                leap_args = benchmark.shlex.split(leap)
                config = benchmark.load_json(Path(leap_args[leap_args.index("--config") + 1]))
                self.assertTrue(config["appearance_diagnostics"])
                result = results[launch - 1]
                result["context"]["effective_settings"] = copy.deepcopy(
                    config["requested_settings"]
                )
                user_dir = Path(str(kwargs["env"]["SECONDLIFE_USER_DIR"]))
                asset_root = user_dir.parent / "cache" / "cache"
                asset_root.mkdir(parents=True, exist_ok=True)
                (asset_root / "asset").write_bytes(b"cached")
                Path(config["output"]).write_text(json.dumps(result), encoding="utf-8")
                return benchmark.subprocess.CompletedProcess(command, 0)

            with (
                patch.object(benchmark, "_read_credentials", return_value=("first", "last", "pw")),
                patch.object(benchmark, "_git_source", return_value={}),
                patch.object(benchmark.subprocess, "run", side_effect=fake_viewer_run),
                redirect_stdout(StringIO()),
            ):
                self.assertEqual(0, args.handler(args))

            report = benchmark.load_json(readiness)
            self.assertEqual(2, launch)
            self.assertEqual(["rejected", "readiness-passed"], [
                attempt["outcome"] for attempt in report["attempts"]
            ])
            self.assertEqual("ready", report["attempts"][0]["sentinel_install"])
            self.assertEqual("ready", report["attempts"][1]["cache_before_launch"]["requested_sentinel"])
            self.assertTrue(report["readiness_passed"])
            self.assertTrue(report["cache_reuse_passed"])
            self.assertEqual(0, report["valid_measured_repeats"])
            self.assertFalse(report["retained_timing"])

    def test_prime_only_without_report_rejects_failed_cache_lifecycle(self) -> None:
        manifest_path = PERF_DIR / "scenarios" / "steady-warm-v1.json"
        manifest = benchmark.load_json(manifest_path)
        result = copy.deepcopy(self.fixture)
        self.match_result_to_manifest(result, manifest)
        result["context"]["effective_settings"] = copy.deepcopy(manifest["settings"])

        with tempfile.TemporaryDirectory() as temp_name:
            parser = benchmark.build_parser()
            args = parser.parse_args([
                "run",
                "--viewer", "/viewer/SecondLife",
                "--manifest", str(manifest_path),
                "--credential-file", str(Path(temp_name) / "account.txt"),
                "--slurl", "secondlife://example/128/128/25",
                "--hardware-label", "fixture-hardware",
                *self.operator_args,
                "--output-dir", str(Path(temp_name) / "results"),
                "--warm-prime-attempts", "2",
                "--prime-only",
            ])
            original_load_json = benchmark.load_json

            def load_fixture(path: Path) -> object:
                return copy.deepcopy(result) if path.name == "warm-prime.json" else original_load_json(path)

            def fake_viewer_run(command: list[str], **_kwargs: object) -> object:
                leap = json.loads(command[command.index("--leap") + 1])
                leap_args = benchmark.shlex.split(leap)
                config = original_load_json(Path(leap_args[leap_args.index("--config") + 1]))
                Path(config["output"]).write_text("{}", encoding="utf-8")
                return benchmark.subprocess.CompletedProcess(command, 0)

            def fake_readiness_attempt(attempt: int, *_args: object, **_kwargs: object) -> dict[str, object]:
                return {"cache_ready": attempt == 1, "outcome": "readiness-passed"}

            with (
                patch.object(benchmark, "_read_credentials", return_value=("first", "last", "pw")),
                patch.object(benchmark, "_git_source", return_value={}),
                patch.object(benchmark, "load_json", side_effect=load_fixture),
                patch.object(benchmark, "cache_lifecycle_facts", return_value={}),
                patch.object(benchmark, "readiness_attempt", side_effect=fake_readiness_attempt),
                patch.object(benchmark.subprocess, "run", side_effect=fake_viewer_run),
                redirect_stdout(StringIO()),
            ):
                with self.assertRaisesRegex(benchmark.BenchmarkError, "cache lifecycle check"):
                    args.handler(args)

    def test_readiness_rejects_fallback_cache_use(self) -> None:
        with tempfile.TemporaryDirectory() as temp_name:
            state = Path(temp_name) / "state"
            benchmark.benchmark_state_settings({}, state, "warm")
            (state / "cache" / "cache").mkdir()
            self.assertEqual("ready", benchmark._install_cache_sentinel(state))
            (state / "user" / "cache" / "cache").mkdir(parents=True)
            before = benchmark.cache_lifecycle_facts(state)
            attempt = benchmark.readiness_attempt(2, "readiness-passed", self.fixture, before, state)
            self.assertFalse(attempt["cache_ready"])
            self.assertIn(
                "fallback-asset-root-present-before-reuse", attempt["cache_failures"]
            )
            self.assertIn("fallback-asset-root-present-after", attempt["cache_failures"])

    def test_readiness_explains_target_gate_failures_without_timing(self) -> None:
        result = copy.deepcopy(self.fixture)
        result["validity"]["observed"]["agent_travel_m"] = 1.0
        result["validity"]["observed"]["settle_seconds_observed"] = 0.0
        result["validity"]["gates"] = benchmark.validity_gate_results(
            result["validity"]["workload_id"],
            result["validity"]["operator"],
            result["validity"]["observed"],
            result["validity"]["policy"],
        )
        with tempfile.TemporaryDirectory() as temp_name:
            state = Path(temp_name) / "state"
            benchmark.benchmark_state_settings({}, state, "warm")
            (state / "cache" / "cache").mkdir()
            attempt = benchmark.readiness_attempt(1, "rejected", result, {}, state)
            self.assertFalse(attempt["target_gates"]["assets"])
            self.assertFalse(attempt["assets"]["settlement_complete"])
            self.assertTrue(attempt["assets"]["queues_settled"])
            self.assertFalse(attempt["target_gates"]["avatar"])
            self.assertFalse(attempt["avatar"]["stationary"])
            self.assertEqual("avatar-moved", attempt["avatar"]["blocker"])

    def test_scene_summary_contains_only_relative_view_fingerprint(self) -> None:
        state = {
            "destination_matches": True,
            "teleport_in_progress": False,
            "progress_visible": False,
            "app_focused": True,
            "frame_count_total": 100,
            "foreground_frame_count_total": 100,
            "camera_animating": False,
            "agent_distance_traveled_total": 5.0,
            "agent_speed_mps": 0.0,
            "view": {"camera_offset": [1.0, 2.0, 3.0], "mode": 1},
            "modal_dialog_count": 0,
            "alert_toast_visible": False,
            "welcome_pack_visible": False,
            "hint_visible": False,
            "closeable_floaters_closed": True,
            "texture_fetch_requests": 0,
            "texture_http_requests": 0,
            "texture_create_queue": 0,
            "texture_fast_cache": 0,
            "texture_upload_count_total": 10,
            "mesh_lod_unresolved": 0,
            "mesh_skin_unresolved": 0,
            "self_avatar_loaded": True,
            "visible_avatars": 1,
            "active_objects": 100,
            "circuit_present": True,
            "circuit_alive": True,
            "circuit_blocked": False,
            "pings_in_transit": 0,
            "packets_in_total": 100,
            "packets_lost_total": 0,
        }
        final = copy.deepcopy(state)
        final["frame_count_total"] = 110
        final["foreground_frame_count_total"] = 110
        final["packets_in_total"] = 120
        observed = collector.summarize_scene_validity(
            self.fixture["validity"]["policy"],
            [state, final],
            [state, final],
            [{"sim_ping_ms": 40, "new_objects": 0}],
            15.0,
        )
        self.assertRegex(observed["view_hash"], r"^[a-f0-9]{64}$")
        self.assertNotIn("view", observed)
        self.assertNotIn("destination", observed)

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
                *self.operator_args,
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
            self.assertNotIn("secondlife://example/128/128/25", rendered)

    def test_prime_only_dry_run_has_no_measured_repeat(self) -> None:
        parser = benchmark.build_parser()
        with tempfile.TemporaryDirectory() as temp_name:
            readiness = Path(temp_name) / "readiness.json"
            args = parser.parse_args([
                "run",
                "--viewer", "/viewer/SecondLife",
                "--manifest", str(PERF_DIR / "scenarios" / "steady-warm-v1.json"),
                "--credential-file", str(Path(temp_name) / "does-not-exist"),
                "--slurl", "secondlife://example/128/128/25",
                "--hardware-label", "fixture-hardware",
                *self.operator_args,
                "--output-dir", str(Path(temp_name) / "results"),
                "--warm-prime-attempts", "2",
                "--prime-only",
                "--readiness-output", str(readiness),
                "--dry-run",
            ])
            output = StringIO()
            with redirect_stdout(output):
                self.assertEqual(0, args.handler(args))
            rendered = output.getvalue()
            self.assertEqual(2, rendered.count("--login"))
            self.assertNotIn("run-01", rendered)
            self.assertFalse(readiness.exists())

    def test_normal_benchmark_path_does_not_probe_the_cache(self) -> None:
        parser = benchmark.build_parser()
        with tempfile.TemporaryDirectory() as temp_name:
            args = parser.parse_args([
                "run",
                "--viewer", "/viewer/SecondLife",
                "--manifest", str(PERF_DIR / "scenarios" / "steady-warm-v1.json"),
                "--credential-file", str(Path(temp_name) / "does-not-exist"),
                "--slurl", "secondlife://example/128/128/25",
                "--hardware-label", "fixture-hardware",
                *self.operator_args,
                "--output-dir", str(Path(temp_name) / "results"),
                "--repeats", "1",
                "--dry-run",
            ])
            with patch.object(
                benchmark,
                "cache_lifecycle_facts",
                side_effect=AssertionError("measurement path probed the cache"),
            ):
                with redirect_stdout(StringIO()):
                    self.assertEqual(0, args.handler(args))

    def test_normal_warm_launches_disable_appearance_diagnostics(self) -> None:
        manifest_path = PERF_DIR / "scenarios" / "steady-warm-v1.json"
        manifest = benchmark.load_json(manifest_path)
        result = copy.deepcopy(self.fixture)
        self.match_result_to_manifest(result, manifest)
        flags: list[bool] = []

        with tempfile.TemporaryDirectory() as temp_name:
            parser = benchmark.build_parser()
            args = parser.parse_args([
                "run",
                "--viewer", "/viewer/SecondLife",
                "--manifest", str(manifest_path),
                "--credential-file", str(Path(temp_name) / "account.txt"),
                "--slurl", "secondlife://example/128/128/25",
                "--hardware-label", "fixture-hardware",
                *self.operator_args,
                "--output-dir", str(Path(temp_name) / "results"),
                "--repeats", "1",
            ])

            def fake_viewer_run(command: list[str], **_kwargs: object) -> object:
                leap = json.loads(command[command.index("--leap") + 1])
                leap_args = benchmark.shlex.split(leap)
                config = benchmark.load_json(Path(leap_args[leap_args.index("--config") + 1]))
                flags.append(config["appearance_diagnostics"])
                launch_result = copy.deepcopy(result)
                launch_result["context"]["effective_settings"] = copy.deepcopy(
                    config["requested_settings"]
                )
                Path(config["output"]).write_text(json.dumps(launch_result), encoding="utf-8")
                return benchmark.subprocess.CompletedProcess(command, 0)

            with (
                patch.object(benchmark, "_read_credentials", return_value=("first", "last", "pw")),
                patch.object(benchmark, "_git_source", return_value={}),
                patch.object(benchmark.subprocess, "run", side_effect=fake_viewer_run),
                redirect_stdout(StringIO()),
            ):
                self.assertEqual(0, args.handler(args))

        self.assertEqual([False, False], flags)

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
