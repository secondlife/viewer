#!/usr/bin/env python3
"""LLLeap collector for render_benchmark.py; launched by the viewer."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
import time
from typing import Any, Mapping, Sequence

import llsd

from render_benchmark import (
    SCHEMA_VERSION,
    canonical_hash,
    display_contract_mismatches,
    safe_appearance_attribution,
    sanitize,
    summarize_result,
    validity_gate_results,
)


class ProtocolError(RuntimeError):
    pass


def read_packet() -> Any:
    header = bytearray()
    while b":" not in header and len(header) < 20:
        chunk = sys.stdin.buffer.read(1)
        if not chunk:
            raise EOFError("viewer closed the LLLeap stream")
        header.extend(chunk)
    if not header.endswith(b":"):
        raise ProtocolError(f"invalid LLLeap length header: {bytes(header)!r}")
    try:
        length = int(header[:-1])
    except ValueError as error:
        raise ProtocolError(f"non-numeric LLLeap length: {bytes(header[:-1])!r}") from error
    payload = sys.stdin.buffer.read(length)
    if len(payload) != length:
        raise EOFError(f"short LLLeap packet: expected {length}, received {len(payload)}")
    return llsd.parse(payload)


def write_packet(pump: str, data: Mapping[str, Any]) -> None:
    payload = llsd.format_notation({"pump": pump, "data": dict(data)})
    sys.stdout.buffer.write(str(len(payload)).encode("ascii") + b":" + payload)
    sys.stdout.buffer.flush()


class ViewerAPI:
    def __init__(self) -> None:
        initial = read_packet()
        self.reply_pump = initial["pump"]
        self.request_id = 0

    def request(self, pump: str, data: Mapping[str, Any]) -> Any:
        self.request_id += 1
        request = dict(data)
        request["reply"] = self.reply_pump
        request["reqid"] = self.request_id
        write_packet(pump, request)
        response = read_packet()
        if isinstance(response, Mapping) and "data" in response:
            return response["data"]
        return response

    def perf_data(self) -> Mapping[str, Any]:
        response = self.request("LLStats", {"op": "getPerfData"})
        if not isinstance(response, Mapping) or not isinstance(response.get("stats"), Mapping):
            raise ProtocolError("LLStats.getPerfData returned no stats map")
        return response["stats"]

    def renderer_diagnostic_state(self) -> Mapping[str, Any]:
        response = self.request("LLStats", {"op": "getRendererDiagnosticState"})
        if not isinstance(response, Mapping):
            raise ProtocolError("LLStats.getRendererDiagnosticState returned no map")
        return response

    def request_quit(self) -> None:
        write_packet("LLAppViewer", {"op": "requestQuit"})


def apply_requested_settings(settings: Mapping[str, Any], api: ViewerAPI) -> None:
    for name, value in settings.items():
        response = api.request(
            "LLViewerControl",
            {"op": "set", "group": "Global", "key": name, "value": value},
        )
        if not isinstance(response, Mapping) or response.get("error"):
            detail = response.get("error") if isinstance(response, Mapping) else "no response map"
            raise ProtocolError(f"could not apply setting {name}: {detail}")


def normalize_renderer_display(api: ViewerAPI) -> None:
    response = api.request("LLStats", {"op": "normalizeRendererDisplay"})
    if not isinstance(response, Mapping) or response.get("error") or not response.get("accepted"):
        detail = response.get("error") if isinstance(response, Mapping) else "no response map"
        raise ProtocolError(f"could not normalize renderer display: {detail}")


def wait_for_display_contract(
    settings: Mapping[str, Any],
    api: ViewerAPI,
    timeout_seconds: float,
    poll_interval: float,
) -> Mapping[str, Any]:
    deadline = time.monotonic() + timeout_seconds
    mismatches: list[str] = []
    while time.monotonic() < deadline:
        stats = api.perf_data()
        context = stats.get("renderer_context", {})
        if isinstance(context, Mapping):
            mismatches = display_contract_mismatches(
                context, settings.get("RenderBenchmarkUIScale")
            )
            if not mismatches:
                return stats
        time.sleep(poll_interval)
    raise ProtocolError("renderer display contract did not settle: " + "; ".join(mismatches))


def _merge_frames(target: dict[int, dict[str, Any]], stats: Mapping[str, Any], after: int) -> None:
    for raw_frame in stats.get("renderer_frames", []):
        if not isinstance(raw_frame, Mapping) or not isinstance(raw_frame.get("frame_number"), (int, float)):
            continue
        frame_number = int(raw_frame["frame_number"])
        if frame_number > after:
            target[frame_number] = dict(raw_frame)


def _latest_frame_number(stats: Mapping[str, Any]) -> int:
    values = [
        int(frame["frame_number"])
        for frame in stats.get("renderer_frames", [])
        if isinstance(frame, Mapping) and isinstance(frame.get("frame_number"), (int, float))
    ]
    return max(values, default=0)


def _add_unclassified_time(frames: list[dict[str, Any]]) -> None:
    exclusive_phases = (
        "geometry_create_ms",
        "partition_ms",
        "geometry_update_ms",
        "cull_ms",
        "shadows_ms",
        "texture_work_ms",
        "state_sort_ms",
        "submission_ms",
        "lighting_ms",
        "ui_ms",
        "swap_ms",
        "idle_ms",
    )
    for frame in frames:
        frame_time = frame.get("frame_time_ms")
        if not isinstance(frame_time, (int, float)):
            continue
        classified = sum(float(frame.get(field, 0.0)) for field in exclusive_phases)
        frame["unclassified_ms"] = max(0.0, float(frame_time) - classified)


def _scene_state(stats: Mapping[str, Any]) -> dict[str, Any]:
    state = stats.get("renderer_scene_state", {})
    return dict(state) if isinstance(state, Mapping) else {}


def sample_appearance_attribution(
    api: ViewerAPI,
) -> tuple[dict[str, Any], dict[str, Any]]:
    diagnostic = api.renderer_diagnostic_state()
    scene_state = diagnostic.get("scene_state")
    if not isinstance(scene_state, Mapping) or "appearance" not in diagnostic:
        raise ProtocolError("diagnostic renderer response is incomplete")
    scene = dict(scene_state)
    appearance = safe_appearance_attribution(diagnostic["appearance"])
    if scene.get("self_avatar_loaded") is not appearance["avatar_loaded"]:
        raise ProtocolError("appearance attribution does not match its scene observation")
    return scene, appearance


def sample_scene_and_appearance(
    stats: Mapping[str, Any], api: ViewerAPI, enabled: bool
) -> tuple[dict[str, Any], dict[str, Any] | None]:
    if not enabled:
        return _scene_state(stats), None
    return sample_appearance_attribution(api)


def summarize_appearance_attribution(
    scene_states: Sequence[Mapping[str, Any]],
    observations: Sequence[Mapping[str, Any]],
) -> dict[str, Any] | None:
    """Retain attribution for the last avatar failure in the guarded window."""
    if len(scene_states) != len(observations):
        raise ProtocolError("appearance observations do not match scene observations")
    projected = [safe_appearance_attribution(value) for value in observations]
    for state, value in reversed(list(zip(scene_states, projected))):
        if state.get("self_avatar_loaded") is not True:
            return value
    return projected[-1] if projected else None


def _numeric_values(states: Sequence[Mapping[str, Any]], field: str) -> list[float]:
    return [
        float(state[field])
        for state in states
        if isinstance(state.get(field), (int, float)) and not isinstance(state.get(field), bool)
    ]


def _frame_total(frames: Sequence[Mapping[str, Any]], field: str) -> float:
    return sum(
        max(0.0, float(frame[field]))
        for frame in frames
        if isinstance(frame.get(field), (int, float)) and not isinstance(frame.get(field), bool)
    )


def _counter_delta(states: Sequence[Mapping[str, Any]], field: str) -> float:
    values = _numeric_values(states, field)
    return max(0.0, max(values) - min(values)) if values else -1.0


def _maximum(states: Sequence[Mapping[str, Any]], field: str, default: float = -1.0) -> float:
    values = _numeric_values(states, field)
    return max(values, default=default)


def _minimum(states: Sequence[Mapping[str, Any]], field: str, default: float = -1.0) -> float:
    values = _numeric_values(states, field)
    return min(values, default=default)


def _rounded_fingerprint(value: Any) -> Any:
    if isinstance(value, Mapping):
        return {str(key): _rounded_fingerprint(item) for key, item in value.items()}
    if isinstance(value, list):
        return [_rounded_fingerprint(item) for item in value]
    if isinstance(value, float):
        return round(value, 4)
    return value


def summarize_scene_validity(
    policy: Mapping[str, Any],
    settle_states: Sequence[Mapping[str, Any]],
    capture_states: Sequence[Mapping[str, Any]],
    frames: Sequence[Mapping[str, Any]],
    settle_seconds_observed: float,
) -> dict[str, Any]:
    """Reduce private live scene state to facts that are safe to retain."""
    guarded_states = [*settle_states, *capture_states]
    first_state = capture_states[0] if capture_states else {}
    view = first_state.get("view", {}) if isinstance(first_state, Mapping) else {}

    frame_counts = _numeric_values(capture_states, "frame_count_total")
    foreground_counts = _numeric_values(capture_states, "foreground_frame_count_total")
    background_frames = -1
    if frame_counts and foreground_counts:
        background_frames = max(
            0,
            round((max(frame_counts) - min(frame_counts)) - (max(foreground_counts) - min(foreground_counts))),
        )
    if any(state.get("app_focused") is not True for state in capture_states):
        background_frames = max(1, background_frames)

    agent_distances = _numeric_values(capture_states, "agent_distance_traveled_total")
    visible_min = _minimum(capture_states, "visible_avatars")
    visible_max = _maximum(capture_states, "visible_avatars")
    active_min = _minimum(capture_states, "active_objects")
    active_max = _maximum(capture_states, "active_objects")
    ping_values = [
        float(frame["sim_ping_ms"])
        for frame in frames
        if isinstance(frame.get("sim_ping_ms"), (int, float)) and not isinstance(frame.get("sim_ping_ms"), bool)
    ]

    return {
        "settle_seconds_observed": max(0.0, settle_seconds_observed),
        "destination_ready": bool(guarded_states)
        and all(state.get("destination_matches") is True for state in guarded_states),
        "teleport_seen": any(state.get("teleport_in_progress") is True for state in guarded_states),
        "progress_seen": any(state.get("progress_visible") is True for state in guarded_states),
        "background_frame_count": background_frames,
        "camera_animating_seen": any(state.get("camera_animating") is True for state in capture_states),
        "camera_translation_m": _frame_total(frames, "camera_translation_m"),
        "camera_rotation_rad": _frame_total(frames, "camera_rotation_rad"),
        "agent_travel_m": (
            max(0.0, max(agent_distances) - min(agent_distances)) if agent_distances else -1.0
        ),
        "agent_speed_mps_max": _maximum(capture_states, "agent_speed_mps"),
        "view_hash": canonical_hash(_rounded_fingerprint(view)) if isinstance(view, Mapping) and view else "",
        "modal_dialog_max": round(_maximum(guarded_states, "modal_dialog_count")),
        "alert_toast_seen": any(state.get("alert_toast_visible") is True for state in guarded_states),
        "welcome_pack_seen": any(state.get("welcome_pack_visible") is True for state in guarded_states),
        "hint_seen": any(state.get("hint_visible") is True for state in guarded_states),
        "closeable_floaters_closed": bool(guarded_states)
        and all(state.get("closeable_floaters_closed") is True for state in guarded_states),
        "texture_fetch_requests_max": round(_maximum(guarded_states, "texture_fetch_requests")),
        "texture_http_requests_max": round(_maximum(guarded_states, "texture_http_requests")),
        "texture_create_queue_max": round(_maximum(guarded_states, "texture_create_queue")),
        "texture_fast_cache_max": round(_maximum(guarded_states, "texture_fast_cache")),
        "texture_upload_count_delta": _counter_delta(guarded_states, "texture_upload_count_total"),
        "mesh_lod_unresolved_max": round(_maximum(guarded_states, "mesh_lod_unresolved")),
        "mesh_skin_unresolved_max": round(_maximum(guarded_states, "mesh_skin_unresolved")),
        "self_avatar_loaded": bool(guarded_states)
        and all(state.get("self_avatar_loaded") is True for state in guarded_states),
        "visible_avatars_min": round(visible_min),
        "visible_avatars_max": round(visible_max),
        "active_objects_min": round(active_min),
        "active_objects_max": round(active_max),
        "new_objects_total": _frame_total(frames, "new_objects"),
        "sim_ping_ms_max": max(ping_values, default=-1.0),
        "circuit_healthy": bool(guarded_states)
        and all(
            state.get("circuit_present") is True
            and state.get("circuit_alive") is True
            and state.get("circuit_blocked") is False
            for state in guarded_states
        ),
        "pings_in_transit_max": round(_maximum(guarded_states, "pings_in_transit")),
        "packets_in_delta": _counter_delta(capture_states, "packets_in_total"),
        "packets_lost_delta": _counter_delta(capture_states, "packets_lost_total"),
    }


def collect(config: Mapping[str, Any], api: ViewerAPI) -> dict[str, Any]:
    run = dict(config["run"])
    poll_interval = float(run["poll_interval_seconds"])
    validity_config = config.get("validity", {})
    if not isinstance(validity_config, Mapping):
        raise ProtocolError("validity config is not a map")
    policy = validity_config.get("policy", {})
    operator = validity_config.get("operator", {})
    workload_id = validity_config.get("workload_id")
    if not isinstance(policy, Mapping) or not isinstance(operator, Mapping):
        raise ProtocolError("validity policy or operator state is not a map")
    appearance_diagnostics = config.get("appearance_diagnostics", False)
    if not isinstance(appearance_diagnostics, bool):
        raise ProtocolError("appearance_diagnostics is not a boolean")
    startup_deadline = time.monotonic() + float(config["startup_timeout_seconds"])
    latest_stats: Mapping[str, Any] | None = None
    while time.monotonic() < startup_deadline:
        latest_stats = api.perf_data()
        if latest_stats.get("renderer_schema_version") == SCHEMA_VERSION and latest_stats.get("renderer_ready"):
            break
        time.sleep(poll_interval)
    else:
        raise ProtocolError("viewer did not reach the started state before the startup timeout")

    requested_settings = config.get("requested_settings", {})
    if not isinstance(requested_settings, Mapping):
        raise ProtocolError("requested_settings is not a map")
    apply_requested_settings(requested_settings, api)
    normalize_renderer_display(api)
    latest_stats = wait_for_display_contract(
        requested_settings,
        api,
        min(10.0, float(config["startup_timeout_seconds"])),
        poll_interval,
    )
    warmup_started = time.monotonic()
    warmup_deadline = warmup_started + float(run["warmup_seconds"])
    initial_scene, initial_appearance = sample_scene_and_appearance(
        latest_stats, api, appearance_diagnostics
    )
    warmup_observations: list[tuple[float, dict[str, Any]]] = [
        (0.0, initial_scene)
    ]
    warmup_appearance = (
        [initial_appearance]
        if initial_appearance is not None
        else []
    )
    while time.monotonic() < warmup_deadline:
        latest_stats = api.perf_data()
        scene_state, appearance = sample_scene_and_appearance(
            latest_stats, api, appearance_diagnostics
        )
        warmup_observations.append((time.monotonic() - warmup_started, scene_state))
        if appearance is not None:
            warmup_appearance.append(appearance)
        time.sleep(poll_interval)
    latest_stats = api.perf_data()
    scene_state, appearance = sample_scene_and_appearance(
        latest_stats, api, appearance_diagnostics
    )
    warmup_observations.append((time.monotonic() - warmup_started, scene_state))
    if appearance is not None:
        warmup_appearance.append(appearance)

    settle_cutoff = warmup_observations[-1][0] - float(policy["settle_seconds"])
    settle_start = 0
    for index, (elapsed, _) in enumerate(warmup_observations):
        if elapsed <= settle_cutoff:
            settle_start = index
        else:
            break
    settle_observations = warmup_observations[settle_start:]
    settle_states = [state for _, state in settle_observations]
    settle_appearance = warmup_appearance[settle_start:] if appearance_diagnostics else []
    settle_seconds_observed = settle_observations[-1][0] - settle_observations[0][0]

    assert latest_stats is not None
    first_frame = _latest_frame_number(latest_stats)
    captured: dict[int, dict[str, Any]] = {}
    capture_states = [_scene_state(latest_stats)]
    capture_appearance = [warmup_appearance[-1]] if appearance_diagnostics else []
    capture_deadline = time.monotonic() + float(run["duration_seconds"])
    while time.monotonic() < capture_deadline:
        latest_stats = api.perf_data()
        _merge_frames(captured, latest_stats, first_frame)
        scene_state, appearance = sample_scene_and_appearance(
            latest_stats, api, appearance_diagnostics
        )
        capture_states.append(scene_state)
        if appearance is not None:
            capture_appearance.append(appearance)
        time.sleep(poll_interval)
    latest_stats = api.perf_data()
    _merge_frames(captured, latest_stats, first_frame)
    scene_state, appearance = sample_scene_and_appearance(
        latest_stats, api, appearance_diagnostics
    )
    capture_states.append(scene_state)
    if appearance is not None:
        capture_appearance.append(appearance)

    context = sanitize({**latest_stats.get("renderer_context", {}), **config["context"]})
    context["effective_settings_hash"] = canonical_hash(context.get("effective_settings", {}))
    context["feature_flags_hash"] = canonical_hash(context.get("feature_flags", {}))
    context["gl_extensions_hash"] = canonical_hash(context.get("gl_extensions", []))
    frames = [captured[key] for key in sorted(captured)]
    _add_unclassified_time(frames)
    instrumentation = dict(latest_stats.get("renderer_instrumentation", {}))
    instrumentation["mode"] = "steady-low-overhead"
    observed = summarize_scene_validity(
        policy,
        settle_states,
        capture_states,
        frames,
        settle_seconds_observed,
    )
    gates = validity_gate_results(workload_id, operator, observed, policy)
    appearance = (
        summarize_appearance_attribution(
            [*settle_states, *capture_states],
            [*settle_appearance, *capture_appearance],
        )
        if appearance_diagnostics
        else None
    )
    if appearance is not None:
        observed["appearance"] = appearance
    validity = {
        "workload_id": workload_id,
        "policy": dict(policy),
        "policy_hash": canonical_hash(policy),
        "operator": dict(operator),
        "observed": observed,
        "gates": gates,
    }
    result: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "status": "valid",
        "failure_reason": None,
        "run": run,
        "context": context,
        "instrumentation": instrumentation,
        "validity": validity,
        "frames": frames,
    }
    expected_gpu = config.get("expected_gpu_substring")
    if context.get("backend_label") != context.get("detected_backend"):
        result["status"] = "invalid"
        result["failure_reason"] = "requested and detected renderer backends differ"
    elif expected_gpu and expected_gpu.lower() not in str(context.get("gpu", "")).lower():
        result["status"] = "invalid"
        result["failure_reason"] = "selected GPU does not match the expected GPU"
    elif len(frames) < max(10, int(float(run["duration_seconds"]))):
        result["status"] = "invalid"
        result["failure_reason"] = "too few rendered frames captured"
    elif not all(gates.values()):
        failed = ", ".join(name for name, passed in gates.items() if not passed)
        result["status"] = "invalid"
        result["failure_reason"] = f"scene validity gates failed: {failed}"
    else:
        result["summary"] = summarize_result(result)
    return sanitize(result)


def write_result(path: Path, result: Mapping[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(sanitize(result), indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--config", required=True)
    args = parser.parse_args()
    config = json.loads(Path(args.config).read_text(encoding="utf-8"))
    output = Path(config["output"])
    api: ViewerAPI | None = None
    try:
        api = ViewerAPI()
        result = collect(config, api)
        write_result(output, result)
        return 0 if result["status"] == "valid" else 1
    except Exception as error:
        failure = sanitize({
            "schema_version": SCHEMA_VERSION,
            "status": "invalid",
            "failure_reason": f"{type(error).__name__}: {error}",
            "run": config.get("run", {}),
            "context": config.get("context", {}),
            "instrumentation": {"mode": "steady-low-overhead"},
            "frames": [],
        })
        write_result(output, failure)
        return 1
    finally:
        if api is not None:
            try:
                api.request_quit()
            except Exception:
                pass


if __name__ == "__main__":
    raise SystemExit(main())
