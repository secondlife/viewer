#!/usr/bin/env python3
"""LLLeap collector for render_benchmark.py; launched by the viewer."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
import time
from typing import Any, Mapping

import llsd

from render_benchmark import (
    SCHEMA_VERSION,
    canonical_hash,
    display_contract_mismatches,
    sanitize,
    summarize_result,
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


def collect(config: Mapping[str, Any], api: ViewerAPI) -> dict[str, Any]:
    run = dict(config["run"])
    poll_interval = float(run["poll_interval_seconds"])
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

    warmup_deadline = time.monotonic() + float(run["warmup_seconds"])
    while time.monotonic() < warmup_deadline:
        latest_stats = api.perf_data()
        time.sleep(poll_interval)

    assert latest_stats is not None
    first_frame = _latest_frame_number(latest_stats)
    captured: dict[int, dict[str, Any]] = {}
    capture_deadline = time.monotonic() + float(run["duration_seconds"])
    while time.monotonic() < capture_deadline:
        latest_stats = api.perf_data()
        _merge_frames(captured, latest_stats, first_frame)
        time.sleep(poll_interval)
    latest_stats = api.perf_data()
    _merge_frames(captured, latest_stats, first_frame)

    context = sanitize({**latest_stats.get("renderer_context", {}), **config["context"]})
    context["effective_settings_hash"] = canonical_hash(context.get("effective_settings", {}))
    context["feature_flags_hash"] = canonical_hash(context.get("feature_flags", {}))
    context["gl_extensions_hash"] = canonical_hash(context.get("gl_extensions", []))
    frames = [captured[key] for key in sorted(captured)]
    _add_unclassified_time(frames)
    instrumentation = dict(latest_stats.get("renderer_instrumentation", {}))
    instrumentation["mode"] = "steady-low-overhead"
    result: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "status": "valid",
        "failure_reason": None,
        "run": run,
        "context": context,
        "instrumentation": instrumentation,
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
