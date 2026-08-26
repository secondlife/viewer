# Renderer benchmark protocol

This benchmark classifies frame-time regressions before renderer fixes or a Vulkan port are chosen. It records low-overhead CPU phases and resource counters during normal rendering. GPU pass timings come from a separate diagnostic capture so the steady benchmark never waits for an OpenGL timer query.

## Contract

- Use a ReleaseOS build without validation or per-shader profiling for performance runs.
- Use the checked-in scenario without editing it. Operator-supplied locations and credentials are deliberately outside manifests and results.
- Run 30 seconds of warm-up, 120 seconds of capture, and five repeats unless the selected scenario documents a different warm-up.
- Keep cold-cache and warm-cache results separate.
- Disable VSync, automatic FPS tuning, and intentional yield time for throughput runs. Measure normal present behavior in a separate pacing run.
- Treat a change as meaningful only when its p95 delta exceeds both 1 ms and three times the run-to-run p95 range for that scenario.
- Randomize A/B order where practical. Repeat a threshold-crossing result on a second GPU or driver of the same class before generalizing it.

The six versioned manifests under `scripts/perf/scenarios/` cover steady warm cache, cold streaming, avatars, draw and alpha pressure, GPU-heavy passes, and UI/HUD composition. A location is supplied with `--slurl`; it is never copied into the output.

## Running

Configure the viewer with `-DLL_RENDER_BENCHMARK:BOOL=ON`. The option defaults to `OFF`; ordinary builds therefore compile out the added phase timers, resource-counter updates, and benchmark LLLeap payload. The collector is an LLLeap child process. Do not use `--noninteractive`: that viewer mode intentionally skips world rendering.

Create a credential file outside the repository. On POSIX it must have mode `0600`; the runner rejects group- or world-accessible files. On Windows, restrict its ACL to the benchmark operator. The file must contain exactly one non-comment line in one of these forms:

```text
username password
first last password
```

A one-word username is sent with the legacy last name `Resident`. The runner never prints the credential path or password. A dry run does not open the credential file.

```bash
python3 scripts/perf/render_benchmark.py run \
  --viewer /path/to/secondlife-bin \
  --viewer-cwd /path/to/runtime/tree \
  --manifest scripts/perf/scenarios/steady-warm-v1.json \
  --credential-file /secure/path/demo-account.txt \
  --slurl secondlife://operator-supplied-location \
  --hardware-label linux-mesa-current \
  --expect-gpu-substring 'expected renderer text' \
  --backend native-gl \
  --output-dir /path/to/results
```

Use `--backend zink` for the matched Linux GL-on-Vulkan run. The runner sets `MESA_LOADER_DRIVER_OVERRIDE=zink`; the collector independently rejects the result unless the OpenGL renderer string contains Zink. Use the same commit, manifest, window size, scene, and GPU for both backends.

For a no-secret launch check:

```bash
python3 scripts/perf/render_benchmark.py run \
  --viewer /path/to/secondlife-bin \
  --manifest scripts/perf/scenarios/steady-warm-v1.json \
  --credential-file /path/that/need/not/exist \
  --slurl secondlife://operator-supplied-location \
  --hardware-label dry-run \
  --output-dir /tmp/renderer-results \
  --repeats 1 \
  --dry-run
```

The runner sets `SECONDLIFE_USER_DIR` and creates session settings, cache, logs, and account data inside a private per-invocation temporary directory. It does not read or alter the normal viewer profile, and the isolated data is removed after the sequence exits. Cold-cache repeats receive separate state and purge before startup. Warm-cache sequences first run one unmeasured full-duration prime, then reuse that isolated profile and cache for all measured repeats. The prime is validated but its artifact is discarded. First-use notifications, audio, and voice are disabled for benchmark sessions so they cannot cover the workload or crash a headless test host.

## Result and comparison rules

Raw artifacts use `renderer-benchmark-result.schema.json`, schema version 1. Each valid result contains:

- scenario, repeat, cache mode, requested settings hash, and manifest hash;
- viewer version, source commit, tracked-diff hash/dirty state, build type, OS, CPU and logical-core count;
- operator hardware label, GPU, driver, reported VRAM, requested and detected backend;
- OpenGL version/profile, limits, extension set and hash, shader level, feature flags, actual settings and hash, and resolution;
- per-frame median inputs plus p95, p99, worst frame and 1 percent low FPS;
- geometry creation, partition, geometry update, culling, shadows/impostors, texture work, state sort, rebuild, GL submission, deferred lighting, UI/HUD, swap, idle, and unclassified CPU time;
- draw count, batch size, triangles, shader program changes, texture uploads, texture readbacks, explicit texture synchronization, shader compilation, and tracked texture memory.

`rebuild_ms` is nested inside `state_sort_ms` and is diagnostic detail; do not add both to a total. The other named phases are non-overlapping at their instrumentation sites. `unclassified_ms` is frame time minus the non-nested named phases, clamped to zero. Texture readbacks currently count `LLImageGL::readBackRaw`; framebuffer screenshot and picker readbacks need an external trace. Texture bytes are source upload estimates, not bus-transaction measurements. Shader compile time is CPU time inside `glCompileShader` and its status query; later driver pipeline work can still appear in submission or an external trace.

Generate a report with:

```bash
python3 scripts/perf/render_benchmark.py report /path/to/results/*.json \
  --format markdown \
  --output summary.md
```

The reporter rejects different scenarios, cache modes, requested or actual settings, feature sets, source commits, build types, resolutions, or instrumentation modes. Extension sets must match between repeats of one backend; native OpenGL and Zink extension differences are expected and their hashes remain visible in the cross-backend report. `--allow-mismatch` is an audit escape hatch; the mismatches are printed into the report.

## Invalid runs

Discard a run and preserve its reason when any of the following applies:

- login or startup did not finish;
- too few frames were captured;
- requested and detected backends differ;
- the selected GPU does not match `--expect-gpu-substring`;
- asset loading is incomplete for a steady run;
- scene population, camera, window focus, feature flags, or resolution changed;
- simulator/network events dominate the sample;
- the device power or thermal state throttled;
- visual comparison differs outside the project's accepted tolerance.

The collector can detect the first four cases. The operator must record the remaining observations in the decision record. Never silently average invalid runs into a summary.

## GPU and system diagnostics

Run one separate diagnostic capture for each important steady configuration using Tracy plus RenderDoc, Nsight Graphics, Radeon GPU Profiler, or the platform's equivalent. Record GPU pass timestamps, queue idle, driver waits, readbacks, pipeline/shader creation, per-thread CPU traces, and per-core utilization. Keep large captures as CI or investigation artifacts, not in Git.

Do not enable the viewer's per-shader Frame Profile for steady runs: it reads query results and can change the workload. Preserve the feature manager's existing AMD RDNA 3.5 query safeguards. A Zink result is Linux evidence only; it is not proof that Windows or macOS will behave the same way.

## Privacy

The C++ event API selects safe fields from viewer information rather than exporting the full About/profile context. The runner removes account, credential, machine ID, serial, hostname, position, parcel, region, and location keys recursively before writing or reporting an artifact. Checked-in fixtures use synthetic hardware strings. Review `find_private_paths()` results before publishing a new fixture.

## Validation

```bash
python3 -m unittest discover -s scripts/perf/tests -v
python3 scripts/perf/render_benchmark.py validate manifest scripts/perf/scenarios/steady-warm-v1.json
python3 scripts/perf/render_benchmark.py validate result scripts/perf/fixtures/renderer-result-v1.json
```

The tests cover all manifests, missing contract fields, percentile calculations, resource deltas, invalid data, comparison mismatches, recursive privacy filtering, the checked-in fixture, and the no-secret dry-run path.
