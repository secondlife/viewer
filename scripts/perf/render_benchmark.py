#!/usr/bin/env python3
"""Run, validate, and summarize reproducible renderer benchmarks."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import shlex
import stat
import statistics
import subprocess
import sys
import tempfile
from typing import Any, Iterable, Mapping, Sequence
import xml.etree.ElementTree as ET


SCHEMA_VERSION = 1
DEFAULT_STARTUP_TIMEOUT_SECONDS = 180
MIN_COMPARISON_REPEATS = 5
OPERATIONAL_SETTINGS = {
    "AllowMultipleViewers": True,
    "FirstLoginThisInstall": False,
    "SLURLPassToOtherInstance": False,
}
OPERATIONAL_SWITCHES = (
    "--multiple",
    "--skipupdatecheck",
    "--noaudio",
    "--nonotifications",
    "--novoice",
)
REQUIRED_CONTEXT_FIELDS = {
    "backend_label",
    "build_type",
    "cpu",
    "detected_backend",
    "driver",
    "effective_settings",
    "git_commit",
    "git_diff_hash",
    "git_dirty",
    "gpu",
    "gpu_vendor",
    "hardware_label",
    "height",
    "logical_core_count",
    "opengl_profile",
    "opengl_version",
    "os",
    "viewer_version",
    "width",
}
COMPARISON_FIELDS = (
    ("run", "scenario"),
    ("run", "cache_mode"),
    ("run", "settings_hash"),
    ("context", "effective_settings_hash"),
    ("context", "feature_flags_hash"),
    ("context", "git_commit"),
    ("context", "git_diff_hash"),
    ("context", "git_dirty"),
    ("context", "build_type"),
    ("context", "width"),
    ("context", "height"),
    ("instrumentation", "mode"),
)
PRIVATE_KEYS = {
    "account",
    "account_id",
    "agent_id",
    "credential",
    "credentials",
    "hostname",
    "login",
    "machine",
    "machine_id",
    "parcel",
    "parcel_id",
    "password",
    "position",
    "region",
    "region_id",
    "serial_number",
    "slurl",
    "username",
}


class BenchmarkError(ValueError):
    """A benchmark artifact or invocation violates the benchmark contract."""


def canonical_hash(value: Any) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def llsd_string_notation(value: str) -> str:
    """Encode a string for viewer command-line settings that use LLSD notation."""
    return json.dumps(value, ensure_ascii=False)


def percentile(values: Sequence[float], quantile: float) -> float:
    if not values:
        raise BenchmarkError("cannot calculate a percentile without samples")
    if not 0.0 <= quantile <= 1.0:
        raise BenchmarkError("quantile must be between zero and one")
    ordered = sorted(float(value) for value in values)
    position = (len(ordered) - 1) * quantile
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    weight = position - lower
    return ordered[lower] * (1.0 - weight) + ordered[upper] * weight


def sanitize(value: Any) -> Any:
    """Remove identifying fields before an artifact can be written or reported."""
    if isinstance(value, Mapping):
        return {
            str(key): sanitize(item)
            for key, item in value.items()
            if str(key).strip().lower() not in PRIVATE_KEYS
        }
    if isinstance(value, list):
        return [sanitize(item) for item in value]
    return value


def find_private_paths(value: Any, prefix: str = "$") -> list[str]:
    found: list[str] = []
    if isinstance(value, Mapping):
        for key, item in value.items():
            path = f"{prefix}.{key}"
            if str(key).strip().lower() in PRIVATE_KEYS:
                found.append(path)
            found.extend(find_private_paths(item, path))
    elif isinstance(value, list):
        for index, item in enumerate(value):
            found.extend(find_private_paths(item, f"{prefix}[{index}]"))
    return found


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise BenchmarkError(f"cannot read JSON from {path}: {error}") from error


def validate_manifest(manifest: Mapping[str, Any]) -> None:
    required = {"schema_version", "id", "description", "cache_mode", "capture", "settings", "workload"}
    missing = sorted(required - manifest.keys())
    if missing:
        raise BenchmarkError(f"manifest is missing: {', '.join(missing)}")
    if manifest["schema_version"] != SCHEMA_VERSION:
        raise BenchmarkError(f"unsupported manifest schema {manifest['schema_version']!r}")
    if manifest["cache_mode"] not in {"warm", "cold"}:
        raise BenchmarkError("cache_mode must be 'warm' or 'cold'")
    capture = manifest["capture"]
    if not isinstance(capture, Mapping):
        raise BenchmarkError("capture must be an object")
    for field in ("warmup_seconds", "duration_seconds", "repeats", "poll_interval_seconds"):
        if not isinstance(capture.get(field), (int, float)) or capture[field] <= 0:
            raise BenchmarkError(f"capture.{field} must be positive")
    if not isinstance(manifest["settings"], Mapping):
        raise BenchmarkError("settings must be an object")
    if find_private_paths(manifest):
        raise BenchmarkError("manifest contains a private field; locations and credentials are operator input")


def _frame_times(result: Mapping[str, Any]) -> list[float]:
    times = [
        float(frame["frame_time_ms"])
        for frame in result.get("frames", [])
        if isinstance(frame, Mapping) and isinstance(frame.get("frame_time_ms"), (int, float))
    ]
    if not times:
        raise BenchmarkError("result has no numeric frame_time_ms samples")
    return times


def _counter_delta(frames: Sequence[Mapping[str, Any]], field: str) -> float:
    samples = [float(frame[field]) for frame in frames if isinstance(frame.get(field), (int, float))]
    return max(0.0, samples[-1] - samples[0]) if len(samples) >= 2 else 0.0


def summarize_result(result: Mapping[str, Any]) -> dict[str, Any]:
    times = _frame_times(result)
    slow_count = max(1, math.ceil(len(times) * 0.01))
    slow_mean = statistics.fmean(sorted(times, reverse=True)[:slow_count])
    frames = [frame for frame in result["frames"] if isinstance(frame, Mapping)]
    phase_fields = (
        "geometry_create_ms",
        "partition_ms",
        "geometry_update_ms",
        "cull_ms",
        "shadows_ms",
        "texture_work_ms",
        "state_sort_ms",
        "rebuild_ms",
        "submission_ms",
        "lighting_ms",
        "ui_ms",
        "swap_ms",
        "idle_ms",
    )
    phase_p95_ms: dict[str, float] = {}
    for field in phase_fields:
        values = [float(frame[field]) for frame in frames if isinstance(frame.get(field), (int, float))]
        if values:
            phase_p95_ms[field] = percentile(values, 0.95)

    return {
        "sample_count": len(times),
        "frame_time_ms": {
            "median": percentile(times, 0.5),
            "p95": percentile(times, 0.95),
            "p99": percentile(times, 0.99),
            "worst": max(times),
        },
        "one_percent_low_fps": 1000.0 / slow_mean if slow_mean > 0 else 0.0,
        "phase_p95_ms": phase_p95_ms,
        "counter_deltas": {
            "texture_upload_count": _counter_delta(frames, "texture_upload_count_total"),
            "texture_upload_bytes": _counter_delta(frames, "texture_upload_bytes_total"),
            "texture_readback_count": _counter_delta(frames, "texture_readback_count_total"),
            "texture_readback_time_us": _counter_delta(frames, "texture_readback_time_us_total"),
            "texture_wait_count": _counter_delta(frames, "texture_wait_count_total"),
            "texture_wait_time_us": _counter_delta(frames, "texture_wait_time_us_total"),
            "shader_compile_count": _counter_delta(frames, "shader_compile_count_total"),
            "shader_compile_time_us": _counter_delta(frames, "shader_compile_time_us_total"),
            "shader_bind_count": _counter_delta(frames, "shader_bind_count_total"),
        },
    }


def validate_result(result: Mapping[str, Any], require_valid: bool = True) -> None:
    if result.get("schema_version") != SCHEMA_VERSION:
        raise BenchmarkError(f"unsupported result schema {result.get('schema_version')!r}")
    for field in ("status", "run", "context", "instrumentation", "frames"):
        if field not in result:
            raise BenchmarkError(f"result is missing {field}")
    if result["status"] not in {"valid", "invalid"}:
        raise BenchmarkError("status must be 'valid' or 'invalid'")
    if require_valid and result["status"] != "valid":
        raise BenchmarkError(f"invalid run: {result.get('failure_reason') or 'no reason supplied'}")
    if not isinstance(result["frames"], list):
        raise BenchmarkError("frames must be an array")
    missing_context = sorted(REQUIRED_CONTEXT_FIELDS - result["context"].keys())
    if missing_context:
        raise BenchmarkError(f"result context is missing: {', '.join(missing_context)}")
    private_paths = find_private_paths(result)
    if private_paths:
        raise BenchmarkError(f"result contains private fields: {', '.join(private_paths)}")
    if require_valid:
        summarize_result(result)


def validate_result_against_manifest(result: Mapping[str, Any], manifest: Mapping[str, Any]) -> None:
    context = result.get("context")
    requested_settings = manifest.get("settings")
    if not isinstance(context, Mapping) or not isinstance(requested_settings, Mapping):
        raise BenchmarkError("result or manifest has no settings context")
    effective_settings = context.get("effective_settings")
    if not isinstance(effective_settings, Mapping):
        raise BenchmarkError("result context has no effective settings")

    mismatches = [
        f"{name}={effective_settings.get(name)!r}, requested {requested!r}"
        for name, requested in requested_settings.items()
        if effective_settings.get(name) != requested
    ]
    if requested_settings.get("WindowMaximized") is False:
        for context_name, setting_name in (("width", "WindowWidth"), ("height", "WindowHeight")):
            requested = requested_settings.get(setting_name)
            if requested is not None and context.get(context_name) != requested:
                mismatches.append(
                    f"actual {context_name}={context.get(context_name)!r}, requested {requested!r}"
                )
    if mismatches:
        raise BenchmarkError("result does not match manifest: " + "; ".join(mismatches))


def _nested(result: Mapping[str, Any], path: Sequence[str]) -> Any:
    value: Any = result
    for key in path:
        if not isinstance(value, Mapping) or key not in value:
            return None
        value = value[key]
    return value


def comparison_mismatches(results: Sequence[Mapping[str, Any]]) -> dict[str, list[Any]]:
    if len(results) < 2:
        return {}
    mismatches: dict[str, list[Any]] = {}
    for path in COMPARISON_FIELDS:
        values = [_nested(result, path) for result in results]
        if any(value != values[0] for value in values[1:]):
            mismatches[".".join(path)] = values

    # Native OpenGL and Zink expose different extension sets by design. Require
    # extension stability between repeats of one backend, but retain the hashes
    # in reports so cross-backend capability differences remain auditable.
    backend_indices: dict[str, list[int]] = {}
    for index, result in enumerate(results):
        backend = str(_nested(result, ("context", "backend_label")))
        backend_indices.setdefault(backend, []).append(index)
    for backend, indices in backend_indices.items():
        values = [_nested(results[index], ("context", "gl_extensions_hash")) for index in indices]
        if any(value != values[0] for value in values[1:]):
            mismatches[f"context.gl_extensions_hash[{backend}]"] = values
    return mismatches


def _read_credentials(path: Path) -> tuple[str, str, str]:
    try:
        if os.name != "nt" and stat.S_IMODE(path.stat().st_mode) & 0o077:
            raise BenchmarkError("credential file must not be accessible by group or other users")
        lines = [line.strip() for line in path.read_text(encoding="utf-8").splitlines()]
    except OSError as error:
        raise BenchmarkError(f"cannot read credential file: {error}") from error
    lines = [line for line in lines if line and not line.startswith("#")]
    if len(lines) != 1:
        raise BenchmarkError("credential file must contain exactly one non-comment account")
    fields = lines[0].split()
    if len(fields) == 2:
        return fields[0], "Resident", fields[1]
    if len(fields) == 3:
        return fields[0], fields[1], fields[2]
    raise BenchmarkError("credential line must be: username password, or first last password")


def _settings_xml(settings: Mapping[str, Any], destination: Path) -> None:
    root = ET.Element("llsd")
    root_map = ET.SubElement(root, "map")
    for name, value in sorted(settings.items()):
        ET.SubElement(root_map, "key").text = str(name)
        setting_map = ET.SubElement(root_map, "map")
        ET.SubElement(setting_map, "key").text = "Value"
        if isinstance(value, bool):
            ET.SubElement(setting_map, "integer").text = "1" if value else "0"
        elif isinstance(value, int):
            ET.SubElement(setting_map, "integer").text = str(value)
        elif isinstance(value, float):
            ET.SubElement(setting_map, "real").text = repr(value)
        elif isinstance(value, str):
            ET.SubElement(setting_map, "string").text = value
        else:
            raise BenchmarkError(f"unsupported setting value for {name}: {value!r}")
    ET.ElementTree(root).write(destination, encoding="utf-8", xml_declaration=True)


def _git_source(repo: Path) -> dict[str, Any]:
    commit = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=repo,
        check=True,
        capture_output=True,
        text=True,
    )
    tracked_diff = subprocess.run(
        ["git", "diff", "--binary", "HEAD", "--", "."],
        cwd=repo,
        check=True,
        capture_output=True,
    ).stdout
    return {
        "git_commit": commit.stdout.strip(),
        "git_dirty": bool(tracked_diff),
        "git_diff_hash": hashlib.sha256(tracked_diff).hexdigest(),
    }


def _redacted_command(command: Sequence[str], password_index: int) -> str:
    redacted = list(command)
    redacted[password_index] = "<redacted>"
    return shlex.join(redacted)


def benchmark_run_numbers(cache_mode: str, repeats: int) -> list[int]:
    """Return measured repeat numbers, preceded by an unmeasured warm-cache prime."""
    measured = list(range(1, repeats + 1))
    return [0, *measured] if cache_mode == "warm" else measured


def _run_command(args: argparse.Namespace) -> int:
    manifest_path = Path(args.manifest).resolve()
    manifest = load_json(manifest_path)
    if not isinstance(manifest, Mapping):
        raise BenchmarkError("manifest root must be an object")
    validate_manifest(manifest)

    viewer = Path(args.viewer).resolve()
    plugin = Path(__file__).with_name("render_benchmark_leap.py").resolve()
    repo = Path(__file__).resolve().parents[2]
    output_dir = Path(args.output_dir).resolve()
    capture = manifest["capture"]
    repeats = args.repeats if args.repeats is not None else int(capture["repeats"])
    if repeats < 1:
        raise BenchmarkError("repeats must be positive")
    if args.startup_timeout < 1:
        raise BenchmarkError("startup timeout must be positive")
    output_dir.mkdir(parents=True, exist_ok=True)
    backend = args.backend
    settings_hash = canonical_hash(manifest["settings"])
    manifest_hash = canonical_hash(manifest)

    if args.dry_run:
        first, last, password = "<first>", "<last>", "<password>"
    else:
        first, last, password = _read_credentials(Path(args.credential_file))

    with tempfile.TemporaryDirectory(prefix="sl-render-benchmark-") as batch_name:
        batch = Path(batch_name)
        warm_state = batch / "warm-state"
        for run_number in benchmark_run_numbers(str(manifest["cache_mode"]), repeats):
            is_prime = run_number == 0
            run_label = "warm-cache prime" if is_prime else f"run {run_number}"
            temp = batch / ("prime" if is_prime else f"run-{run_number:02d}")
            temp.mkdir()
            output = (
                temp / "warm-prime.json"
                if is_prime
                else output_dir / f"{manifest['id']}-{backend}-run-{run_number:02d}.json"
            )
            state = warm_state if manifest["cache_mode"] == "warm" else temp
            settings = dict(manifest["settings"])
            settings.update(OPERATIONAL_SETTINGS)
            settings["PurgeCacheOnStartup"] = manifest["cache_mode"] == "cold"
            settings["CacheLocation"] = str(state / "cache")
            settings_file = temp / "session.xml"
            _settings_xml(settings, settings_file)

            plugin_config = {
                "schema_version": SCHEMA_VERSION,
                "output": str(output),
                "run": {
                    "scenario": manifest["id"],
                    # The prime is a complete, schema-valid capture whose
                    # artifact is discarded. It may share run 1's ordinal
                    # because it can never enter a report.
                    "run_number": 1 if is_prime else run_number,
                    "cache_mode": manifest["cache_mode"],
                    "manifest_hash": manifest_hash,
                    "settings_hash": settings_hash,
                    "warmup_seconds": capture["warmup_seconds"],
                    "duration_seconds": capture["duration_seconds"],
                    "poll_interval_seconds": capture["poll_interval_seconds"],
                },
                "context": {
                    "backend_label": backend,
                    "hardware_label": args.hardware_label,
                    **_git_source(repo),
                },
                "requested_settings": settings,
                "startup_timeout_seconds": args.startup_timeout,
                "expected_gpu_substring": args.expect_gpu_substring,
            }
            plugin_config_path = temp / "plugin-config.json"
            plugin_config_path.write_text(json.dumps(plugin_config), encoding="utf-8")
            leap_command = shlex.join([sys.executable, str(plugin), "--config", str(plugin_config_path)])
            command = [
                str(viewer),
                *OPERATIONAL_SWITCHES,
                "--usersessionsettings",
                str(settings_file),
                "--leap",
                llsd_string_notation(leap_command),
                "--login",
                first,
                last,
                password,
                "--slurl",
                args.slurl,
            ]
            password_index = command.index("--login") + 3
            print(_redacted_command(command, password_index))
            if args.dry_run:
                continue

            environment = os.environ.copy()
            environment["SECONDLIFE_USER_DIR"] = str(state / "user")
            if backend == "zink":
                environment["MESA_LOADER_DRIVER_OVERRIDE"] = "zink"
            timeout = args.startup_timeout + float(capture["warmup_seconds"]) + float(capture["duration_seconds"]) + 60
            try:
                completed = subprocess.run(
                    command,
                    cwd=args.viewer_cwd or viewer.parent,
                    env=environment,
                    timeout=timeout,
                    check=False,
                )
            except subprocess.TimeoutExpired as error:
                raise BenchmarkError(f"viewer timed out during {run_label}") from error
            if completed.returncode != 0:
                raise BenchmarkError(f"viewer exited with {completed.returncode} during {run_label}")
            if not output.exists():
                raise BenchmarkError(f"viewer produced no artifact for {run_label}")
            result = load_json(output)
            validate_result(result)
            validate_result_against_manifest(result, manifest)
            if not is_prime:
                output.write_text(json.dumps(sanitize(result), indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


def _format_markdown(results: Sequence[Mapping[str, Any]], override_note: str | None) -> str:
    rows = []
    backend_p95: dict[str, list[float]] = {}
    for result in results:
        summary = summarize_result(result)
        p95 = summary["frame_time_ms"]["p95"]
        backend = str(result["context"]["backend_label"])
        backend_p95.setdefault(backend, []).append(p95)
        rows.append(
            "| {run} | {backend} | {samples} | {median:.2f} | {p95:.2f} | {p99:.2f} | {worst:.2f} | {low:.1f} |".format(
                run=result["run"]["run_number"],
                backend=result["context"]["backend_label"],
                samples=summary["sample_count"],
                median=summary["frame_time_ms"]["median"],
                p95=p95,
                p99=summary["frame_time_ms"]["p99"],
                worst=summary["frame_time_ms"]["worst"],
                low=summary["one_percent_low_fps"],
            )
        )
    lines = [
        f"# Renderer benchmark: {results[0]['run']['scenario']}",
        "",
        "| Run | Backend | Frames | Median ms | p95 ms | p99 ms | Worst ms | 1% low FPS |",
        "|---:|---|---:|---:|---:|---:|---:|---:|",
        *rows,
        "",
        "| Backend | Repeats | Median run p95 ms | Run-to-run p95 range ms | Extension hash |",
        "|---|---:|---:|---:|---|",
    ]
    backend_medians: dict[str, float] = {}
    backend_ranges: dict[str, float] = {}
    for backend, values in sorted(backend_p95.items()):
        median_p95 = percentile(values, 0.5)
        p95_range = max(values) - min(values)
        backend_medians[backend] = median_p95
        backend_ranges[backend] = p95_range
        extension_hash = next(
            str(result["context"].get("gl_extensions_hash", "missing"))
            for result in results
            if result["context"]["backend_label"] == backend
        )
        lines.append(
            f"| {backend} | {len(values)} | {median_p95:.2f} | {p95_range:.2f} | `{extension_hash[:12]}` |"
        )

    if {"native-gl", "zink"}.issubset(backend_medians):
        delta = backend_medians["zink"] - backend_medians["native-gl"]
        noise = max(backend_ranges["native-gl"], backend_ranges["zink"])
        direction = "slower" if delta > 0 else "faster"
        repeat_count = min(len(backend_p95["native-gl"]), len(backend_p95["zink"]))
        if repeat_count < MIN_COMPARISON_REPEATS:
            verdict = (
                f"Decision is indeterminate: {repeat_count} matched repeat(s) were supplied, "
                f"but {MIN_COMPARISON_REPEATS} per backend are required."
            )
        else:
            meaningful = abs(delta) > 1.0 and abs(delta) > 3.0 * noise
            threshold = "meaningful" if meaningful else "below the decision threshold"
            verdict = f"This result is {threshold} (>1 ms and >3x noise required)."
        lines.extend([
            "",
            (
                f"Zink median run p95 is {abs(delta):.2f} ms {direction} than native GL; "
                f"the largest within-backend p95 range is {noise:.2f} ms. {verdict}"
            ),
        ])
    if override_note:
        lines.extend(["", f"Comparison guard overridden: {override_note}"])
    return "\n".join(lines) + "\n"


def _format_csv(results: Sequence[Mapping[str, Any]]) -> str:
    from io import StringIO

    stream = StringIO()
    writer = csv.writer(stream, lineterminator="\n")
    writer.writerow(("scenario", "run", "backend", "frames", "median_ms", "p95_ms", "p99_ms", "worst_ms", "one_percent_low_fps"))
    for result in results:
        summary = summarize_result(result)
        writer.writerow((
            result["run"]["scenario"],
            result["run"]["run_number"],
            result["context"]["backend_label"],
            summary["sample_count"],
            summary["frame_time_ms"]["median"],
            summary["frame_time_ms"]["p95"],
            summary["frame_time_ms"]["p99"],
            summary["frame_time_ms"]["worst"],
            summary["one_percent_low_fps"],
        ))
    return stream.getvalue()


def _report_command(args: argparse.Namespace) -> int:
    results = []
    for raw_path in args.results:
        result = load_json(Path(raw_path))
        if not isinstance(result, Mapping):
            raise BenchmarkError(f"result root must be an object: {raw_path}")
        validate_result(result)
        results.append(result)
    if not results:
        raise BenchmarkError("at least one result is required")
    mismatches = comparison_mismatches(results)
    override_note = None
    if mismatches:
        detail = "; ".join(f"{key}={values!r}" for key, values in mismatches.items())
        if not args.allow_mismatch:
            raise BenchmarkError(f"comparison context mismatch: {detail}")
        override_note = detail
    rendered = _format_markdown(results, override_note) if args.format == "markdown" else _format_csv(results)
    if args.output == "-":
        sys.stdout.write(rendered)
    else:
        Path(args.output).write_text(rendered, encoding="utf-8")
    return 0


def _validate_command(args: argparse.Namespace) -> int:
    value = load_json(Path(args.path))
    if not isinstance(value, Mapping):
        raise BenchmarkError("JSON root must be an object")
    if args.kind == "manifest":
        validate_manifest(value)
    else:
        validate_result(value, require_valid=not args.allow_invalid)
    print(f"valid {args.kind}: {args.path}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    subparsers = parser.add_subparsers(dest="command", required=True)

    run = subparsers.add_parser("run", help="run a viewer benchmark")
    run.add_argument("--viewer", required=True)
    run.add_argument("--viewer-cwd")
    run.add_argument("--manifest", required=True)
    run.add_argument("--credential-file", required=True)
    run.add_argument("--slurl", required=True, help="operator-supplied location; omitted from results")
    run.add_argument("--hardware-label", required=True, help="non-identifying operator label")
    run.add_argument("--expect-gpu-substring", help="invalidate a run if a different GPU is selected")
    run.add_argument("--backend", choices=("native-gl", "zink"), default="native-gl")
    run.add_argument("--repeats", type=int)
    run.add_argument("--startup-timeout", type=int, default=DEFAULT_STARTUP_TIMEOUT_SECONDS)
    run.add_argument("--output-dir", required=True)
    run.add_argument("--dry-run", action="store_true")
    run.set_defaults(handler=_run_command)

    report = subparsers.add_parser("report", help="validate and summarize raw results")
    report.add_argument("results", nargs="+")
    report.add_argument("--format", choices=("markdown", "csv"), default="markdown")
    report.add_argument("--output", default="-")
    report.add_argument("--allow-mismatch", action="store_true")
    report.set_defaults(handler=_report_command)

    validate = subparsers.add_parser("validate", help="validate a manifest or result")
    validate.add_argument("kind", choices=("manifest", "result"))
    validate.add_argument("path")
    validate.add_argument("--allow-invalid", action="store_true")
    validate.set_defaults(handler=_validate_command)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return args.handler(args)
    except (BenchmarkError, OSError, subprocess.CalledProcessError) as error:
        parser.error(str(error))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
