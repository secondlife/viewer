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
import re
import shlex
import stat
import statistics
import subprocess
import sys
import tempfile
from typing import Any, Iterable, Mapping, Sequence
import xml.etree.ElementTree as ET


SCHEMA_VERSION = 3
DEFAULT_STARTUP_TIMEOUT_SECONDS = 180
MIN_COMPARISON_REPEATS = 5
BENCHMARK_BACKING_WIDTH = 1280
BENCHMARK_BACKING_HEIGHT = 720
BENCHMARK_EFFECTIVE_UI_SCALE = 1.0
DISPLAY_SCALE_TOLERANCE = 1e-4
OPERATIONAL_SETTINGS = {
    "AllowMultipleViewers": True,
    "FirstLoginThisInstall": False,
    "MigrateCacheDirectory": False,
    "SLURLPassToOtherInstance": False,
}
OPERATIONAL_SWITCHES = (
    "--multiple",
    "--skipupdatecheck",
    "--noaudio",
    "--nonotifications",
    "--novoice",
)
CACHE_FAILURE_SIGNATURES = (
    ("Failure in vf.write()", "asset-cache-write"),
    ("Unable to set cache location", "cache-location"),
    ("Unable to write header entry!", "texture-cache-header-write"),
    ("writeToFastCache failed", "texture-fast-cache-write"),
    ("Failed to write cache to disk", "object-cache-write"),
    ("Failed to write cache entry to disk", "object-cache-entry-write"),
    ("Failed to write cache. Unable to save inventory", "inventory-cache-write"),
    ("Couldn't mkdir", "cache-directory-create"),
)
AVATAR_BLOCKER_SIGNATURES = (
    ("COF info is not complete", "cof-incomplete"),
    ("Self is clouded due to missing one or more required body parts", "required-bodyparts-missing"),
    ("Self is clouded because of no hair texture", "hair-texture-missing"),
    ("Self is clouded because lower textures not baked", "lower-bake-missing"),
    ("Self is clouded because upper textures not baked", "upper-bake-missing"),
    ("Self is clouded because texture at index", "baked-texture-not-renderable"),
)
REQUIRED_APPEARANCE_PARTS = ("shape", "skin", "hair", "eyes")
APPEARANCE_CLASSIFICATIONS = frozenset({
    "avatar-unavailable",
    "cof-incomplete",
    "required-link-missing-or-unresolved",
    "wearable-delivery-pending-or-failed",
    "avatar-later-blocker",
    "ready",
    "unknown",
})
READINESS_ASSET_FIELDS = (
    "mesh_lod_unresolved_max",
    "mesh_skin_unresolved_max",
    "texture_create_queue_max",
    "texture_fast_cache_max",
    "texture_fetch_requests_max",
    "texture_http_requests_max",
    "texture_upload_count_delta",
)
REQUIRED_CONTEXT_FIELDS = {
    "backend_label",
    "backing_height",
    "backing_scale_x",
    "backing_scale_y",
    "backing_width",
    "build_type",
    "configured_ui_scale",
    "cpu",
    "detected_backend",
    "driver",
    "effective_display_scale_x",
    "effective_display_scale_y",
    "effective_settings",
    "git_commit",
    "git_diff_hash",
    "git_dirty",
    "gpu",
    "gpu_vendor",
    "hardware_label",
    "height",
    "logical_core_count",
    "logical_height",
    "logical_width",
    "opengl_profile",
    "opengl_version",
    "os",
    "viewer_version",
    "width",
}
COMPARISON_FIELDS = (
    ("run", "scenario"),
    ("run", "cache_mode"),
    ("run", "manifest_hash"),
    ("run", "settings_hash"),
    ("context", "effective_settings_hash"),
    ("context", "feature_flags_hash"),
    ("context", "git_commit"),
    ("context", "git_diff_hash"),
    ("context", "git_dirty"),
    ("context", "build_type"),
    ("context", "width"),
    ("context", "height"),
    ("context", "backing_width"),
    ("context", "backing_height"),
    ("context", "logical_width"),
    ("context", "logical_height"),
    ("context", "backing_scale_x"),
    ("context", "backing_scale_y"),
    ("context", "configured_ui_scale"),
    ("context", "effective_display_scale_x"),
    ("context", "effective_display_scale_y"),
    ("instrumentation", "mode"),
    ("validity", "workload_id"),
    ("validity", "policy_hash"),
    ("validity", "operator", "power_source"),
    ("validity", "operator", "low_power_mode"),
    ("validity", "operator", "thermal_state"),
    ("validity", "operator", "scene_events"),
    ("validity", "operator", "ui_state"),
    ("validity", "operator", "camera_state"),
    ("validity", "observed", "view_hash"),
    ("validity", "observed", "visible_avatars_min"),
    ("validity", "observed", "active_objects_min"),
)
PRIVATE_KEYS = {
    "account",
    "account_id",
    "agent_id",
    "credential",
    "credentials",
    "destination",
    "hostname",
    "login",
    "machine",
    "machine_id",
    "parcel",
    "parcel_id",
    "password",
    "position",
    "orientation",
    "origin",
    "region",
    "region_id",
    "serial_number",
    "slurl",
    "username",
    "view",
}
WORKLOAD_ID_PATTERN = re.compile(r"^[a-z0-9][a-z0-9-]{2,63}$")
VALIDITY_FIELDS = {
    "asset_mode",
    "max_active_object_delta",
    "max_agent_travel_m",
    "max_camera_rotation_rad",
    "max_camera_translation_m",
    "max_new_objects",
    "max_sim_ping_ms",
    "max_visible_avatar_delta",
    "population_mode",
    "settle_seconds",
    "ui_mode",
}
OPERATOR_STATE_FIELDS = {
    "power_source",
    "low_power_mode",
    "thermal_state",
    "scene_events",
    "ui_state",
    "camera_state",
}
VALIDITY_GATE_NAMES = (
    "workload",
    "placement",
    "focus",
    "camera",
    "avatar",
    "ui",
    "assets",
    "population",
    "network",
    "scene_events",
    "power",
    "thermal",
)
READINESS_TARGET_GATES = ("assets", "avatar")
CACHE_SENTINEL_NAME = "sl_cache_renderer_benchmark_sentinel.asset"
CACHE_SENTINEL_CONTENT = b"renderer-benchmark-cache-sentinel"


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


def safe_appearance_attribution(value: Any) -> dict[str, Any]:
    """Project live appearance data into the fixed privacy-safe contract."""
    source = value if isinstance(value, Mapping) else {}
    scalar_fields = (
        "avatar_valid",
        "cof_present",
        "cof_complete",
        "cof_change_in_progress",
        "avatar_loaded",
    )

    def required_parts(field: str) -> dict[str, bool]:
        parts = source.get(field, {})
        if not isinstance(parts, Mapping):
            parts = {}
        return {name: parts.get(name) is True for name in REQUIRED_APPEARANCE_PARTS}

    projected = {
        "avatar_valid": source.get("avatar_valid") is True,
        "cof_present": source.get("cof_present") is True,
        "cof_complete": source.get("cof_complete") is True,
        "cof_change_in_progress": source.get("cof_change_in_progress") is True,
        "required_links_resolved": required_parts("required_links_resolved"),
        "required_wearables_delivered": required_parts("required_wearables_delivered"),
        "avatar_loaded": source.get("avatar_loaded") is True,
    }
    parts_are_typed = all(
        isinstance(source.get(field), Mapping)
        and all(type(source[field].get(name)) is bool for name in REQUIRED_APPEARANCE_PARTS)
        for field in ("required_links_resolved", "required_wearables_delivered")
    )
    facts_are_typed = all(type(source.get(field)) is bool for field in scalar_fields)
    expected_classification = (
        "avatar-unavailable"
        if not projected["avatar_valid"]
        else "cof-incomplete"
        if not projected["cof_present"] or not projected["cof_complete"]
        else "required-link-missing-or-unresolved"
        if not all(projected["required_links_resolved"].values())
        else "wearable-delivery-pending-or-failed"
        if not all(projected["required_wearables_delivered"].values())
        else "avatar-later-blocker"
        if not projected["avatar_loaded"]
        else "ready"
    )
    classification = source.get("classification")
    classification_is_known = (
        isinstance(classification, str)
        and classification in APPEARANCE_CLASSIFICATIONS
    )
    if (
        not facts_are_typed
        or not parts_are_typed
        or not classification_is_known
        or classification not in {expected_classification, "unknown"}
    ):
        classification = "unknown"
    return {"classification": classification, **projected}


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


def validate_validity_policy(validity: Any, warmup_seconds: Any | None = None) -> None:
    if not isinstance(validity, Mapping):
        raise BenchmarkError("validity must be an object")
    validity_missing = sorted(VALIDITY_FIELDS - validity.keys())
    validity_unknown = sorted(validity.keys() - VALIDITY_FIELDS)
    if validity_missing:
        raise BenchmarkError("validity is missing: " + ", ".join(validity_missing))
    if validity_unknown:
        raise BenchmarkError("validity has unknown fields: " + ", ".join(validity_unknown))
    if validity["asset_mode"] not in {"settled", "streaming"}:
        raise BenchmarkError("validity.asset_mode must be 'settled' or 'streaming'")
    if validity["population_mode"] not in {"stable", "observed"}:
        raise BenchmarkError("validity.population_mode must be 'stable' or 'observed'")
    if validity["ui_mode"] not in {"clear", "controlled"}:
        raise BenchmarkError("validity.ui_mode must be 'clear' or 'controlled'")
    for field in (
        "settle_seconds",
        "max_camera_rotation_rad",
        "max_camera_translation_m",
        "max_sim_ping_ms",
    ):
        if not _is_number(validity[field]) or validity[field] <= 0:
            raise BenchmarkError(f"validity.{field} must be positive")
    if not _is_number(validity["max_agent_travel_m"]) or validity["max_agent_travel_m"] < 0:
        raise BenchmarkError("validity.max_agent_travel_m must be non-negative")
    if warmup_seconds is not None and validity["settle_seconds"] > warmup_seconds:
        raise BenchmarkError("validity.settle_seconds must fit inside capture.warmup_seconds")
    for field in ("max_active_object_delta", "max_new_objects", "max_visible_avatar_delta"):
        if not isinstance(validity[field], int) or isinstance(validity[field], bool) or validity[field] < 0:
            raise BenchmarkError(f"validity.{field} must be a non-negative integer")


def validate_manifest(manifest: Mapping[str, Any]) -> None:
    required = {
        "schema_version", "id", "description", "cache_mode", "capture", "settings", "workload", "validity"
    }
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
    settings = manifest["settings"]
    expected_settings = {
        "WindowWidth": BENCHMARK_BACKING_WIDTH,
        "WindowHeight": BENCHMARK_BACKING_HEIGHT,
        "WindowMaximized": False,
        "RenderBenchmarkUIScale": BENCHMARK_EFFECTIVE_UI_SCALE,
        "RenderHiDPI": True,
    }
    display_mismatches = [
        f"{name}={settings.get(name)!r}, required {expected!r}"
        for name, expected in expected_settings.items()
        if settings.get(name) != expected
    ]
    if "UIScaleFactor" in settings:
        display_mismatches.append("UIScaleFactor must be derived from the detected backing scale")
    if display_mismatches:
        raise BenchmarkError("manifest display contract mismatch: " + "; ".join(display_mismatches))
    validate_validity_policy(manifest["validity"], capture["warmup_seconds"])
    if find_private_paths(manifest):
        raise BenchmarkError("manifest contains a private field; locations and credentials are operator input")


def validate_operator_state(workload_id: Any, operator: Any) -> None:
    if not isinstance(workload_id, str) or not WORKLOAD_ID_PATTERN.fullmatch(workload_id):
        raise BenchmarkError("workload ID must be a 3-64 character lowercase slug")
    if not isinstance(operator, Mapping):
        raise BenchmarkError("operator state must be an object")
    missing = sorted(OPERATOR_STATE_FIELDS - operator.keys())
    unknown = sorted(operator.keys() - OPERATOR_STATE_FIELDS)
    if missing:
        raise BenchmarkError("operator state is missing: " + ", ".join(missing))
    if unknown:
        raise BenchmarkError("operator state has unknown fields: " + ", ".join(unknown))
    allowed = {
        "power_source": {"ac", "battery", "unknown"},
        "low_power_mode": {"off", "on", "unknown"},
        "thermal_state": {"nominal", "elevated", "throttled", "unknown"},
        "scene_events": {"none", "observed", "unknown"},
        "ui_state": {"approved", "unapproved", "unknown"},
        "camera_state": {"approved", "unapproved", "unknown"},
    }
    for field, choices in allowed.items():
        if operator[field] not in choices:
            raise BenchmarkError(f"operator state {field}={operator[field]!r} is not supported")


def validity_gate_results(
    workload_id: Any,
    operator: Mapping[str, Any],
    observed: Mapping[str, Any],
    policy: Mapping[str, Any],
) -> dict[str, bool]:
    """Derive fail-closed scene gates from a privacy-safe observation summary."""
    validate_operator_state(workload_id, operator)
    validate_validity_policy(policy)
    required_observed = {
        "active_objects_max",
        "active_objects_min",
        "agent_travel_m",
        "alert_toast_seen",
        "background_frame_count",
        "camera_animating_seen",
        "camera_rotation_rad",
        "camera_translation_m",
        "circuit_healthy",
        "closeable_floaters_closed",
        "destination_ready",
        "hint_seen",
        "mesh_lod_unresolved_max",
        "mesh_skin_unresolved_max",
        "modal_dialog_max",
        "new_objects_total",
        "progress_seen",
        "self_avatar_loaded",
        "settle_seconds_observed",
        "sim_ping_ms_max",
        "teleport_seen",
        "texture_create_queue_max",
        "texture_fast_cache_max",
        "texture_fetch_requests_max",
        "texture_http_requests_max",
        "texture_upload_count_delta",
        "visible_avatars_max",
        "visible_avatars_min",
        "view_hash",
        "welcome_pack_seen",
    }
    if not isinstance(observed, Mapping) or required_observed - observed.keys():
        return {name: False for name in VALIDITY_GATE_NAMES}

    numeric = lambda name: observed[name] if _is_number(observed[name]) else math.inf
    integer = lambda name: observed[name] if isinstance(observed[name], int) and not isinstance(observed[name], bool) else math.inf
    valid_view_hash = (
        isinstance(observed["view_hash"], str)
        and re.fullmatch(r"[a-f0-9]{64}", observed["view_hash"]) is not None
    )
    assets_settled = all(
        integer(field) == 0
        for field in (
            "texture_fetch_requests_max",
            "texture_http_requests_max",
            "texture_create_queue_max",
            "texture_fast_cache_max",
            "mesh_lod_unresolved_max",
            "mesh_skin_unresolved_max",
        )
    ) and numeric("texture_upload_count_delta") == 0
    population_stable = (
        integer("visible_avatars_max") - integer("visible_avatars_min")
        <= policy["max_visible_avatar_delta"]
        and integer("active_objects_max") - integer("active_objects_min")
        <= policy["max_active_object_delta"]
        and numeric("new_objects_total") <= policy["max_new_objects"]
    )
    ui_clear = (
        integer("modal_dialog_max") == 0
        and observed["alert_toast_seen"] is False
        and observed["welcome_pack_seen"] is False
        and observed["hint_seen"] is False
        and observed["progress_seen"] is False
        and (policy["ui_mode"] == "controlled" or observed["closeable_floaters_closed"] is True)
    )
    return {
        "workload": True,
        "placement": observed["destination_ready"] is True and observed["teleport_seen"] is False,
        "focus": integer("background_frame_count") == 0,
        "camera": (
            observed["camera_animating_seen"] is False
            and valid_view_hash
            and numeric("camera_translation_m") <= policy["max_camera_translation_m"]
            and numeric("camera_rotation_rad") <= policy["max_camera_rotation_rad"]
            and operator["camera_state"] == "approved"
        ),
        "avatar": (
            observed["self_avatar_loaded"] is True
            and numeric("agent_travel_m") <= policy["max_agent_travel_m"]
        ),
        "ui": ui_clear and operator["ui_state"] == "approved",
        "assets": policy["asset_mode"] == "streaming" or (
            numeric("settle_seconds_observed") >= policy["settle_seconds"] and assets_settled
        ),
        "population": policy["population_mode"] == "observed" or population_stable,
        "network": (
            observed["circuit_healthy"] is True
            and 0 <= numeric("sim_ping_ms_max") <= policy["max_sim_ping_ms"]
        ),
        "scene_events": operator["scene_events"] == "none",
        "power": operator["power_source"] in {"ac", "battery"} and operator["low_power_mode"] == "off",
        "thermal": operator["thermal_state"] in {"nominal", "elevated"},
    }


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


def _is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def display_contract_mismatches(context: Mapping[str, Any], target_scale: Any) -> list[str]:
    mismatches: list[str] = []
    dimension_fields = (
        "width",
        "height",
        "backing_width",
        "backing_height",
        "logical_width",
        "logical_height",
    )
    for field in dimension_fields:
        value = context.get(field)
        if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
            mismatches.append(f"actual {field}={value!r}, required a positive integer")

    for explicit, legacy in (("backing_width", "width"), ("backing_height", "height")):
        if context.get(explicit) != context.get(legacy):
            mismatches.append(f"{explicit}={context.get(explicit)!r}, {legacy}={context.get(legacy)!r}")
    for field, expected in (
        ("backing_width", BENCHMARK_BACKING_WIDTH),
        ("backing_height", BENCHMARK_BACKING_HEIGHT),
    ):
        if context.get(field) != expected:
            mismatches.append(f"actual {field}={context.get(field)!r}, required {expected!r}")

    numeric_fields = (
        "backing_scale_x",
        "backing_scale_y",
        "configured_ui_scale",
        "effective_display_scale_x",
        "effective_display_scale_y",
    )
    for field in numeric_fields:
        value = context.get(field)
        if not _is_number(value) or value <= 0:
            mismatches.append(f"actual {field}={value!r}, required a positive number")

    if not _is_number(target_scale) or target_scale <= 0:
        mismatches.append(f"effective RenderBenchmarkUIScale={target_scale!r}, required a positive number")
    else:
        for axis in ("x", "y"):
            backing_scale = context.get(f"backing_scale_{axis}")
            configured_scale = context.get("configured_ui_scale")
            effective_scale = context.get(f"effective_display_scale_{axis}")
            if _is_number(backing_scale) and _is_number(configured_scale):
                derived_scale = float(backing_scale) * float(configured_scale)
                if not math.isclose(derived_scale, float(target_scale), abs_tol=DISPLAY_SCALE_TOLERANCE):
                    mismatches.append(
                        f"configured UI scale produces {derived_scale!r} on {axis}, requested {target_scale!r}"
                    )
            if _is_number(effective_scale) and not math.isclose(
                float(effective_scale), float(target_scale), abs_tol=DISPLAY_SCALE_TOLERANCE
            ):
                mismatches.append(
                    f"actual effective_display_scale_{axis}={effective_scale!r}, requested {target_scale!r}"
                )

    for axis, backing_field, logical_field in (
        ("x", "backing_width", "logical_width"),
        ("y", "backing_height", "logical_height"),
    ):
        backing_size = context.get(backing_field)
        logical_size = context.get(logical_field)
        backing_scale = context.get(f"backing_scale_{axis}")
        if all(_is_number(value) for value in (backing_size, logical_size, backing_scale)):
            reconstructed = float(logical_size) * float(backing_scale)
            if not math.isclose(reconstructed, float(backing_size), abs_tol=1.0):
                mismatches.append(
                    f"logical {axis} geometry reconstructs {reconstructed!r} backing pixels, actual {backing_size!r}"
                )
    return mismatches


def validate_result(result: Mapping[str, Any], require_valid: bool = True) -> None:
    if result.get("schema_version") != SCHEMA_VERSION:
        raise BenchmarkError(f"unsupported result schema {result.get('schema_version')!r}")
    for field in ("status", "run", "context", "instrumentation", "frames", "validity"):
        if field not in result:
            raise BenchmarkError(f"result is missing {field}")
    if result["status"] not in {"valid", "invalid"}:
        raise BenchmarkError("status must be 'valid' or 'invalid'")
    if require_valid and result["status"] != "valid":
        raise BenchmarkError(f"invalid run: {result.get('failure_reason') or 'no reason supplied'}")
    if not isinstance(result["frames"], list):
        raise BenchmarkError("frames must be an array")
    validity = result["validity"]
    if not isinstance(validity, Mapping):
        raise BenchmarkError("result validity must be an object")
    for field in ("workload_id", "policy", "policy_hash", "operator", "observed", "gates"):
        if field not in validity:
            raise BenchmarkError(f"result validity is missing {field}")
    policy = validity["policy"]
    if not isinstance(policy, Mapping) or canonical_hash(policy) != validity["policy_hash"]:
        raise BenchmarkError("result validity policy hash does not match its policy")
    observed = validity["observed"]
    if not isinstance(observed, Mapping):
        raise BenchmarkError("result validity observations must be an object")
    if "appearance" in observed:
        safe_appearance = safe_appearance_attribution(observed["appearance"])
        if canonical_hash(observed["appearance"]) != canonical_hash(safe_appearance):
            raise BenchmarkError("result appearance attribution is outside the privacy-safe contract")
    validate_operator_state(validity["workload_id"], validity["operator"])
    expected_gates = validity_gate_results(
        validity["workload_id"], validity["operator"], validity["observed"], policy
    )
    if validity["gates"] != expected_gates:
        raise BenchmarkError("result validity gates do not match the observed facts")
    if result["status"] == "valid" and not all(expected_gates.values()):
        failed = ", ".join(name for name, passed in expected_gates.items() if not passed)
        raise BenchmarkError(f"valid result has failed scene gates: {failed}")
    missing_context = sorted(REQUIRED_CONTEXT_FIELDS - result["context"].keys())
    if missing_context:
        raise BenchmarkError(f"result context is missing: {', '.join(missing_context)}")
    effective_settings = result["context"].get("effective_settings")
    target_scale = effective_settings.get("RenderBenchmarkUIScale") if isinstance(effective_settings, Mapping) else None
    display_mismatches = display_contract_mismatches(result["context"], target_scale)
    if not isinstance(effective_settings, Mapping) or effective_settings.get("RenderHiDPI") is not True:
        display_mismatches.append("effective RenderHiDPI must be true")
    if display_mismatches:
        raise BenchmarkError("result display contract mismatch: " + "; ".join(display_mismatches))
    private_paths = find_private_paths(result)
    if private_paths:
        raise BenchmarkError(f"result contains private fields: {', '.join(private_paths)}")
    summarize_result(result)


def validate_result_against_manifest(
    result: Mapping[str, Any],
    manifest: Mapping[str, Any],
    require_all_gates: bool = True,
) -> None:
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
    run = result.get("run", {})
    if not isinstance(run, Mapping):
        mismatches.append("result run is not an object")
    else:
        expected_manifest_hash = canonical_hash(manifest)
        expected_settings_hash = canonical_hash(requested_settings)
        if run.get("manifest_hash") != expected_manifest_hash:
            mismatches.append(
                f"manifest_hash={run.get('manifest_hash')!r}, requested {expected_manifest_hash!r}"
            )
        if run.get("settings_hash") != expected_settings_hash:
            mismatches.append(
                f"settings_hash={run.get('settings_hash')!r}, requested {expected_settings_hash!r}"
            )
    validity = result.get("validity")
    if not isinstance(validity, Mapping):
        mismatches.append("result validity is not an object")
    else:
        requested_policy = manifest.get("validity")
        if validity.get("policy") != requested_policy:
            mismatches.append("scene validity policy differs from the manifest")
        if validity.get("policy_hash") != canonical_hash(requested_policy):
            mismatches.append("scene validity policy hash differs from the manifest")
        gates = validity_gate_results(
            validity.get("workload_id"),
            validity.get("operator", {}),
            validity.get("observed", {}),
            requested_policy,
        )
        if validity.get("gates") != gates:
            mismatches.append("scene validity gates do not match the manifest policy")
        elif require_all_gates and not all(gates.values()):
            mismatches.append("scene validity gates do not pass the manifest policy")
    if requested_settings.get("WindowMaximized") is False:
        for context_name, setting_name in (
            ("width", "WindowWidth"),
            ("height", "WindowHeight"),
            ("backing_width", "WindowWidth"),
            ("backing_height", "WindowHeight"),
        ):
            requested = requested_settings.get(setting_name)
            if requested is not None and context.get(context_name) != requested:
                mismatches.append(
                    f"actual {context_name}={context.get(context_name)!r}, requested {requested!r}"
                )

    mismatches.extend(display_contract_mismatches(context, requested_settings.get("RenderBenchmarkUIScale")))
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


def _redacted_command(command: Sequence[str], private_indices: Iterable[int]) -> str:
    redacted = list(command)
    for index in private_indices:
        redacted[index] = "<redacted>"
    return shlex.join(redacted)


def benchmark_run_numbers(
    cache_mode: str,
    repeats: int,
    warm_prime_attempts: int = 1,
    prime_only: bool = False,
) -> list[int]:
    """Return measured repeats, optionally preceded by bounded warm-cache primes."""
    measured = list(range(1, repeats + 1))
    if cache_mode != "warm":
        return measured
    return [0] * warm_prime_attempts + ([] if prime_only else measured)


def failed_validity_gate_names(result: Any) -> list[str]:
    """Extract only known failed gate names for a privacy-safe rejection message."""
    if not isinstance(result, Mapping):
        return []
    validity = result.get("validity")
    gates = validity.get("gates") if isinstance(validity, Mapping) else None
    if not isinstance(gates, Mapping):
        return []
    return [name for name in VALIDITY_GATE_NAMES if gates.get(name) is False]


def benchmark_state_settings(
    manifest_settings: Mapping[str, Any],
    state: Path,
    cache_mode: str,
    initialize: bool = True,
) -> dict[str, Any]:
    """Create the disposable state roots and return a complete cache selection."""
    user_dir = state / "user"
    cache_dir = state / "cache"
    if initialize:
        user_dir.mkdir(parents=True, exist_ok=True)
        cache_dir.mkdir(parents=True, exist_ok=True)

    settings = dict(manifest_settings)
    settings.update(OPERATIONAL_SETTINGS)
    settings["PurgeCacheOnStartup"] = cache_mode == "cold"
    settings["CacheLocation"] = str(cache_dir)
    settings["NewCacheLocation"] = str(cache_dir)
    return settings


def _cache_probe(directory: Path) -> str:
    """Exercise create, write, rename, read, and cleanup without exposing a path."""
    source = directory / ".renderer-benchmark-cache-probe"
    renamed = directory / ".renderer-benchmark-cache-probe-renamed"
    if not directory.is_dir():
        return "root-missing"
    cleanup_failed = False
    for entry in (source, renamed):
        try:
            entry.unlink(missing_ok=True)
        except OSError:
            cleanup_failed = True
    if cleanup_failed:
        return "cleanup-failed"
    try:
        source.write_bytes(b"renderer-cache-probe")
    except OSError:
        return "write-failed"

    status = "ready"
    try:
        source.replace(renamed)
    except OSError:
        status = "rename-failed"
    else:
        try:
            if renamed.read_bytes() != b"renderer-cache-probe":
                status = "readback-failed"
        except OSError:
            status = "readback-failed"

    cleanup_failed = False
    for entry in (source, renamed):
        try:
            entry.unlink(missing_ok=True)
        except OSError:
            cleanup_failed = True
    return "cleanup-failed" if status == "ready" and cleanup_failed else status


def _file_count(directory: Path) -> int:
    try:
        return sum(1 for entry in directory.rglob("*") if entry.is_file())
    except OSError:
        return -1


def _safe_finite_number(value: Any) -> int | float | None:
    if not _is_number(value):
        return None
    try:
        return value if math.isfinite(value) else None
    except OverflowError:
        return None


def _cache_sentinel_status(state: Path) -> str:
    sentinel = state / "cache" / "cache" / CACHE_SENTINEL_NAME
    try:
        return "ready" if sentinel.read_bytes() == CACHE_SENTINEL_CONTENT else "content-mismatch"
    except FileNotFoundError:
        return "missing"
    except OSError:
        return "read-failed"


def _install_cache_sentinel(state: Path) -> str:
    sentinel = state / "cache" / "cache" / CACHE_SENTINEL_NAME
    if not sentinel.parent.is_dir():
        return "asset-root-missing"
    try:
        sentinel.write_bytes(CACHE_SENTINEL_CONTENT)
    except OSError:
        return "write-failed"
    return _cache_sentinel_status(state)


def cache_lifecycle_facts(state: Path) -> dict[str, Any]:
    """Return aggregate facts for the requested and fallback cache roots."""
    requested = state / "cache"
    fallback = state / "user" / "cache"
    asset_dir_name = "cache"
    return {
        "requested_root_present": requested.is_dir(),
        "requested_write_probe": _cache_probe(requested),
        "requested_asset_root_present": (requested / asset_dir_name).is_dir(),
        "requested_asset_write_probe": _cache_probe(requested / asset_dir_name),
        "requested_texture_root_present": (requested / "texturecache").is_dir(),
        "requested_object_root_present": (requested / "objectcache").is_dir(),
        "requested_sentinel": _cache_sentinel_status(state),
        "requested_file_count": _file_count(requested),
        "fallback_root_present": fallback.is_dir(),
        "fallback_asset_root_present": (fallback / asset_dir_name).is_dir(),
        "fallback_file_count": _file_count(fallback),
    }


def _viewer_log_cursor(log_root: Path) -> tuple[int, int, int] | None:
    """Capture an internal identity and byte offset for the active viewer log."""
    try:
        status = (log_root / "SecondLife.log").stat()
    except OSError:
        return None
    return status.st_dev, status.st_ino, status.st_size


def _first_log_category(
    log_root: Path,
    signatures: Sequence[tuple[str, str]],
    cursor: tuple[int, int, int] | None = None,
) -> str | None:
    """Reduce new bytes from the active viewer log to one known category."""
    viewer_log = log_root / "SecondLife.log"
    try:
        status = viewer_log.stat()
    except OSError:
        return None
    offset = 0
    if cursor is not None:
        device, inode, previous_size = cursor
        if (status.st_dev, status.st_ino) == (device, inode) and status.st_size >= previous_size:
            offset = previous_size
    try:
        with viewer_log.open("rb") as stream:
            stream.seek(offset)
            for raw_line in stream:
                line = raw_line.decode("utf-8", errors="replace")
                for signature, category in signatures:
                    if signature in line:
                        return category
    except OSError:
        return None
    return None


def readiness_attempt(
    attempt: int,
    outcome: str,
    result: Any,
    cache_before: Mapping[str, Any],
    state: Path,
    log_cursor: tuple[int, int, int] | None = None,
    prepare_reuse: bool = False,
) -> dict[str, Any]:
    """Reduce one prime to privacy-safe readiness facts with no frame timing."""
    validity = result.get("validity", {}) if isinstance(result, Mapping) else {}
    observed = validity.get("observed", {}) if isinstance(validity, Mapping) else {}
    if not isinstance(observed, Mapping):
        observed = {}
    gates = validity.get("gates", {}) if isinstance(validity, Mapping) else {}
    if not isinstance(gates, Mapping):
        gates = {}
    policy = validity.get("policy", {}) if isinstance(validity, Mapping) else {}
    if not isinstance(policy, Mapping):
        policy = {}
    self_avatar_loaded = observed.get("self_avatar_loaded") is True
    agent_stationary = (
        _is_number(observed.get("agent_travel_m"))
        and _is_number(policy.get("max_agent_travel_m"))
        and observed["agent_travel_m"] <= policy["max_agent_travel_m"]
    )
    settlement_complete = (
        _is_number(observed.get("settle_seconds_observed"))
        and _is_number(policy.get("settle_seconds"))
        and observed["settle_seconds_observed"] >= policy["settle_seconds"]
    )
    asset_queues_settled = all(
        _is_number(observed.get(field)) and observed[field] == 0
        for field in READINESS_ASSET_FIELDS
    )
    log_avatar_blocker = _first_log_category(
        state / "user" / "logs", AVATAR_BLOCKER_SIGNATURES, log_cursor
    )
    appearance = (
        safe_appearance_attribution(observed["appearance"])
        if "appearance" in observed
        else None
    )
    appearance_classification = appearance["classification"] if appearance else "unknown"
    avatar_blocker = (
        "none"
        if self_avatar_loaded and agent_stationary
        else "avatar-moved"
        if self_avatar_loaded
        else appearance_classification
        if appearance_classification not in {"ready", "unknown"}
        else log_avatar_blocker or "unknown"
    )
    attempt_record = sanitize({
        "attempt": attempt,
        "outcome": outcome,
        "failed_gates": failed_validity_gate_names(result),
        "target_gates": {
            name: gates.get(name) if isinstance(gates.get(name), bool) else None
            for name in READINESS_TARGET_GATES
        },
        "all_scene_gates_passed": all(gates.get(name) is True for name in VALIDITY_GATE_NAMES),
        "cache_before_launch": dict(cache_before),
        "cache_after_launch": cache_lifecycle_facts(state),
        "first_cache_failure": _first_log_category(
            state / "user" / "logs", CACHE_FAILURE_SIGNATURES, log_cursor
        ) or "none",
        "assets": {
            "settlement_complete": settlement_complete,
            "queues_settled": asset_queues_settled,
            "observed": {
                field: _safe_finite_number(observed.get(field))
                for field in READINESS_ASSET_FIELDS
                if field in observed
            },
        },
        "avatar": {
            "self_avatar_loaded": self_avatar_loaded,
            "stationary": agent_stationary,
            "blocker": avatar_blocker,
            **({"appearance": appearance} if appearance else {}),
        },
    })
    before = attempt_record["cache_before_launch"]
    after = attempt_record["cache_after_launch"]
    cache_failures: list[str] = []
    if before.get("requested_root_present") is not True:
        cache_failures.append("requested-root-missing-before")
    if before.get("requested_write_probe") != "ready":
        cache_failures.append("write-probe-failed-before")
    if attempt > 1 and before.get("requested_asset_root_present") is not True:
        cache_failures.append("asset-root-missing-before-reuse")
    if attempt > 1 and before.get("requested_asset_write_probe") != "ready":
        cache_failures.append("asset-write-probe-failed-before-reuse")
    if attempt > 1 and before.get("requested_sentinel") != "ready":
        cache_failures.append("sentinel-missing-before-reuse")
    if attempt > 1 and before.get("fallback_asset_root_present") is True:
        cache_failures.append("fallback-asset-root-present-before-reuse")
    if after.get("requested_root_present") is not True:
        cache_failures.append("requested-root-missing-after")
    if after.get("requested_write_probe") != "ready":
        cache_failures.append("write-probe-failed-after")
    if after.get("requested_asset_root_present") is not True:
        cache_failures.append("asset-root-missing-after")
    if after.get("requested_asset_write_probe") != "ready":
        cache_failures.append("asset-write-probe-failed-after")
    if attempt > 1 and after.get("requested_sentinel") != "ready":
        cache_failures.append("sentinel-missing-after-reuse")
    if after.get("fallback_asset_root_present") is True:
        cache_failures.append("fallback-asset-root-present-after")
    if attempt_record["first_cache_failure"] != "none":
        cache_failures.append("cache-log-failure")
    if prepare_reuse:
        sentinel_install = _install_cache_sentinel(state)
        attempt_record["sentinel_install"] = sentinel_install
        if sentinel_install != "ready":
            cache_failures.append("sentinel-install-failed")
    attempt_record["cache_failures"] = cache_failures
    attempt_record["cache_ready"] = not cache_failures
    return attempt_record


def write_readiness_report(path: Path, attempts: Sequence[Mapping[str, Any]]) -> None:
    """Write a no-timing diagnostic artifact after a bounded prime-only run."""
    last_attempt = attempts[-1] if attempts else {}
    last_targets = last_attempt.get("target_gates", {})
    readiness_passed = isinstance(last_targets, Mapping) and all(
        last_targets.get(name) is True for name in READINESS_TARGET_GATES
    )
    cache_reuse_passed = len(attempts) >= 2 and all(
        attempt.get("cache_ready") is True for attempt in attempts
    )
    report = sanitize({
        "schema_version": 1,
        "kind": "renderer-readiness",
        "scene_contract": "schema-3-gates-unchanged",
        "target_gates": list(READINESS_TARGET_GATES),
        "readiness_passed": bool(attempts)
        and last_attempt.get("outcome") == "readiness-passed"
        and cache_reuse_passed
        and readiness_passed,
        "cache_reuse_passed": cache_reuse_passed,
        "all_scene_gates_passed": bool(attempts)
        and last_attempt.get("all_scene_gates_passed") is True,
        "valid_measured_repeats": 0,
        "retained_timing": False,
        "attempts": list(attempts),
    })
    private = find_private_paths(report)
    if private:
        raise BenchmarkError("readiness report contains a private field")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _run_command(args: argparse.Namespace) -> int:
    manifest_path = Path(args.manifest).resolve()
    manifest = load_json(manifest_path)
    if not isinstance(manifest, Mapping):
        raise BenchmarkError("manifest root must be an object")
    validate_manifest(manifest)

    operator_state = {
        "power_source": args.power_source,
        "low_power_mode": args.low_power_mode,
        "thermal_state": args.thermal_state,
        "scene_events": args.scene_events,
        "ui_state": args.ui_state,
        "camera_state": args.camera_state,
    }
    validate_operator_state(args.workload_id, operator_state)

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
    if args.warm_prime_attempts < 1:
        raise BenchmarkError("warm prime attempts must be positive")
    if manifest["cache_mode"] != "warm" and (args.prime_only or args.warm_prime_attempts != 1):
        raise BenchmarkError("prime-only controls require a warm-cache manifest")
    if args.warm_prime_attempts != 1 and not args.prime_only:
        raise BenchmarkError("multiple warm primes require --prime-only")
    if args.readiness_output and not args.prime_only:
        raise BenchmarkError("readiness output requires --prime-only")
    if args.readiness_output and args.warm_prime_attempts < 2:
        raise BenchmarkError("readiness output requires at least two warm primes")
    output_dir.mkdir(parents=True, exist_ok=True)
    readiness_output = Path(args.readiness_output).resolve() if args.readiness_output else None
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
        initialized_states: set[Path] = set()
        readiness_attempts: list[dict[str, Any]] = []
        prime_attempt = 0
        run_numbers = benchmark_run_numbers(
            str(manifest["cache_mode"]),
            repeats,
            args.warm_prime_attempts,
            args.prime_only,
        )
        for run_number in run_numbers:
            is_prime = run_number == 0
            if is_prime:
                prime_attempt += 1
            run_label = (
                f"warm-cache prime {prime_attempt}/{args.warm_prime_attempts}"
                if is_prime and args.warm_prime_attempts > 1
                else "warm-cache prime"
                if is_prime
                else f"run {run_number}"
            )
            temp = batch / (
                f"prime-{prime_attempt:02d}" if is_prime else f"run-{run_number:02d}"
            )
            temp.mkdir()
            output = (
                temp / "warm-prime.json"
                if is_prime
                else output_dir / f"{manifest['id']}-{backend}-run-{run_number:02d}.json"
            )
            state = warm_state if manifest["cache_mode"] == "warm" else temp
            settings = benchmark_state_settings(
                manifest["settings"],
                state,
                manifest["cache_mode"],
                initialize=state not in initialized_states,
            )
            initialized_states.add(state)
            cache_before = cache_lifecycle_facts(state) if args.prime_only else {}
            log_cursor = (
                _viewer_log_cursor(state / "user" / "logs") if args.prime_only else None
            )
            settings_file = temp / "session.xml"
            _settings_xml(settings, settings_file)

            plugin_config = {
                "schema_version": SCHEMA_VERSION,
                "output": str(output),
                "appearance_diagnostics": bool(is_prime and args.prime_only),
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
                "validity": {
                    "workload_id": args.workload_id,
                    "operator": operator_state,
                    "policy": manifest["validity"],
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
            login_index = command.index("--login")
            slurl_index = command.index("--slurl")
            print(_redacted_command(command, (*range(login_index + 1, login_index + 4), slurl_index + 1)))
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
                if is_prime and args.prime_only:
                    readiness_attempts.append(
                        readiness_attempt(
                            prime_attempt, "timeout", {}, cache_before, state, log_cursor
                        )
                    )
                if readiness_output:
                    write_readiness_report(readiness_output, readiness_attempts)
                raise BenchmarkError(f"viewer timed out during {run_label}") from error
            if not output.exists():
                if is_prime and args.prime_only:
                    readiness_attempts.append(
                        readiness_attempt(
                            prime_attempt, "no-artifact", {}, cache_before, state, log_cursor
                        )
                    )
                if readiness_output:
                    write_readiness_report(readiness_output, readiness_attempts)
                raise BenchmarkError(f"viewer produced no artifact for {run_label}")
            try:
                result = load_json(output)
            except BenchmarkError:
                if is_prime and args.prime_only:
                    readiness_attempts.append(
                        readiness_attempt(
                            prime_attempt,
                            "invalid-artifact",
                            {},
                            cache_before,
                            state,
                            log_cursor,
                        )
                    )
                if readiness_output:
                    write_readiness_report(readiness_output, readiness_attempts)
                raise
            failed_gates = failed_validity_gate_names(result)
            if completed.returncode != 0:
                if is_prime and args.prime_only:
                    readiness_attempts.append(
                        readiness_attempt(
                            prime_attempt,
                            "viewer-error",
                            result,
                            cache_before,
                            state,
                            log_cursor,
                        )
                    )
                if readiness_output:
                    write_readiness_report(readiness_output, readiness_attempts)
                raise BenchmarkError(f"viewer exited with {completed.returncode} during {run_label}")
            try:
                validate_result(result, require_valid=False)
                validate_result_against_manifest(result, manifest, require_all_gates=False)
            except BenchmarkError:
                if is_prime and args.prime_only:
                    readiness_attempts.append(
                        readiness_attempt(
                            prime_attempt,
                            "invalid-artifact",
                            result,
                            cache_before,
                            state,
                            log_cursor,
                        )
                    )
                if readiness_output:
                    write_readiness_report(readiness_output, readiness_attempts)
                raise
            if result.get("status") == "invalid":
                expected_reason = f"scene validity gates failed: {', '.join(failed_gates)}"
                if result.get("failure_reason") != expected_reason:
                    if is_prime and args.prime_only:
                        readiness_attempts.append(
                            readiness_attempt(
                                prime_attempt,
                                "invalid-run",
                                result,
                                cache_before,
                                state,
                                log_cursor,
                            )
                        )
                    if readiness_output:
                        write_readiness_report(readiness_output, readiness_attempts)
                    raise BenchmarkError(f"{run_label} failed outside the scene validity gates")
            gates = result["validity"]["gates"]
            gates_to_retry = (
                [name for name in READINESS_TARGET_GATES if gates.get(name) is not True]
                if args.prime_only
                else failed_gates
            )
            if gates_to_retry:
                gate_family = "readiness gates" if args.prime_only else "scene validity gates"
                if is_prime and args.prime_only:
                    readiness_attempts.append(
                        readiness_attempt(
                            prime_attempt,
                            "rejected",
                            result,
                            cache_before,
                            state,
                            log_cursor,
                            prepare_reuse=prime_attempt == 1
                            and prime_attempt < args.warm_prime_attempts,
                        )
                    )
                if is_prime and prime_attempt < args.warm_prime_attempts:
                    print(
                        f"{run_label} rejected by {gate_family}: {', '.join(gates_to_retry)}; "
                        "retrying with the same disposable warm cache"
                    )
                    continue
                if readiness_output:
                    write_readiness_report(readiness_output, readiness_attempts)
                raise BenchmarkError(
                    f"{run_label} rejected by {gate_family}: {', '.join(gates_to_retry)}"
                )
            if not args.prime_only:
                validate_result(result)
                validate_result_against_manifest(result, manifest)
            if is_prime and args.prime_only:
                attempt_record = readiness_attempt(
                    prime_attempt,
                    "readiness-passed",
                    result,
                    cache_before,
                    state,
                    log_cursor,
                    prepare_reuse=prime_attempt == 1
                    and prime_attempt < args.warm_prime_attempts,
                )
                if not attempt_record["cache_ready"]:
                    attempt_record["outcome"] = "cache-rejected"
                readiness_attempts.append(attempt_record)
                if prime_attempt < args.warm_prime_attempts:
                    status = (
                        "passed the readiness gates"
                        if attempt_record["cache_ready"]
                        else "failed the cache lifecycle check"
                    )
                    print(
                        f"{run_label} {status}; "
                        "validating the same disposable warm cache again"
                    )
                    continue
                cache_lifecycle_passed = bool(readiness_attempts) and all(
                    attempt.get("cache_ready") is True for attempt in readiness_attempts
                )
                if not cache_lifecycle_passed:
                    attempt_record["outcome"] = "cache-rejected"
                    if readiness_output:
                        write_readiness_report(readiness_output, readiness_attempts)
                    raise BenchmarkError(f"{run_label} rejected by the cache lifecycle check")
                if readiness_output:
                    write_readiness_report(readiness_output, readiness_attempts)
                return 0
            if not is_prime:
                output.write_text(
                    json.dumps(sanitize(result), indent=2, sort_keys=True) + "\n",
                    encoding="utf-8",
                )
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
    run.add_argument("--workload-id", required=True, help="privacy-safe controlled-workload slug")
    run.add_argument("--power-source", required=True, choices=("ac", "battery", "unknown"))
    run.add_argument("--low-power-mode", required=True, choices=("off", "on", "unknown"))
    run.add_argument(
        "--thermal-state", required=True, choices=("nominal", "elevated", "throttled", "unknown")
    )
    run.add_argument("--scene-events", required=True, choices=("none", "observed", "unknown"))
    run.add_argument("--ui-state", required=True, choices=("approved", "unapproved", "unknown"))
    run.add_argument("--camera-state", required=True, choices=("approved", "unapproved", "unknown"))
    run.add_argument("--expect-gpu-substring", help="invalidate a run if a different GPU is selected")
    run.add_argument("--backend", choices=("native-gl", "zink"), default="native-gl")
    run.add_argument("--repeats", type=int)
    run.add_argument(
        "--warm-prime-attempts",
        type=int,
        default=1,
        help="number of unmeasured prime-only launches sharing one warm cache",
    )
    run.add_argument(
        "--prime-only",
        action="store_true",
        help="stop after one schema-valid warm prime; never run a measured repeat",
    )
    run.add_argument(
        "--readiness-output",
        help="write privacy-safe no-timing facts for a prime-only investigation",
    )
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
