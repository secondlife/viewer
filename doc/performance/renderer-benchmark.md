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

The six versioned manifests under `scripts/perf/scenarios/` cover steady warm cache, cold streaming, avatars, draw and alpha pressure, GPU-heavy passes, and UI/HUD composition. A location is supplied with `--slurl`; it is never copied into the output or printed in a launch command.

### Display contract

The display contract introduced in schema version 2 fixes the benchmark render surface at 1280×720 backing pixels and the effective viewer UI scale at 1.0. Every checked-in scenario declares `RenderHiDPI: true`, `RenderBenchmarkUIScale: 1.0`, `WindowWidth: 1280`, `WindowHeight: 720`, and `WindowMaximized: false`. A scenario must not declare `UIScaleFactor`; the viewer derives that configured factor from the backing scale detected after window creation. Making HiDPI explicit ensures a fresh isolated profile exercises Retina backing on supported hardware instead of inheriting a local default.

On a 1× display, the configured UI factor is 1.0. On a 2× Retina display, it is 0.5. Both produce an effective display scale of 1.0 without changing the existing meaning of `context.width` and `context.height`, which remain backing pixels. The result also records explicit backing and logical dimensions, backing scale on both axes, the configured UI factor, and the final effective display scale on both axes.

The runner rejects a result when any explicit backing dimension differs from the legacy backing dimension or manifest, when logical size multiplied by backing scale does not reconstruct the backing size, when the configured factor does not produce the requested scale, or when the final display scale differs from the request. These fields must also match across repeated results. Schema versions 1 and 2 are not accepted by the version 3 runner or reporter because they do not carry the complete scene-validity proof.

`RenderBenchmarkUIScale` defaults to zero, is not persisted, and has an effect only in a build compiled with `LL_RENDER_BENCHMARK`. Ordinary builds and non-benchmark launches retain the normal platform UI-scale behavior.

### Scene-validity contract

Schema version 3 turns the controlled-workload rules into a fail-closed contract. Each manifest declares an asset mode, population mode, UI mode, settlement window, motion limits, population limits, new-object limit, and simulator-ping limit. Each run also supplies a privacy-safe workload slug and typed operator assertions for power source, low-power mode, thermal state, material scene events, intended UI, and intended camera.

The viewer exports only the live facts needed to evaluate those rules. During the end of warm-up and throughout capture, the collector checks placement, teleport and progress state, camera and avatar motion, focus, blocking UI, texture and mesh work, self-avatar completion, visible-avatar and active-object counts, new objects, circuit health, and simulator ping. Absolute location and camera coordinates are not retained. A rounded, agent-relative camera view is reduced to a hash before the raw view is discarded.

The checked-in steady policy uses existing viewer semantics where they fit: the requested region with the existing 2 m X/Y placement tolerance, the scene-loading monitor's accumulated 0.1 m camera-translation and 0.05 rad camera-rotation limits, no population-count drift, and the mesh subsystem's 15-second no-progress horizon. Applying the 15-second window to the composite texture and mesh gate, requiring zero avatar travel and new objects, and rejecting any capture-wide ping above 600 ms are conservative benchmark policies. They are not universal definitions of viewer correctness. A strict gate may reject a usable-looking scene; it may not admit an ambiguous timing sample.

Operator values are assertions, not platform telemetry. A valid run requires a known, repeatable power source, low-power mode off, a non-throttled thermal state, no material scene events, and approved UI and camera state. Power source and all operator fields must match across comparable repeats. AC power is not universally required.

## Running

Configure the viewer with `-DLL_RENDER_BENCHMARK:BOOL=ON`. The option defaults to `OFF`; ordinary builds therefore compile out the added phase timers, resource-counter updates, and benchmark LLLeap payload. The collector is an LLLeap child process. Do not use `--noninteractive`: that viewer mode intentionally skips world rendering.

Create a credential file outside the repository. On POSIX it must have mode `0600`; the runner rejects group- or world-accessible files. On Windows, restrict its ACL to the benchmark operator. The file must contain exactly one non-comment line in one of these forms:

```text
username password
first last password
```

A one-word username is sent with the legacy last name `Resident`. The runner never prints the account name, credential path, password, or SLURL. A dry run does not open the credential file.

```bash
python3 scripts/perf/render_benchmark.py run \
  --viewer /path/to/secondlife-bin \
  --viewer-cwd /path/to/runtime/tree \
  --manifest scripts/perf/scenarios/steady-warm-v1.json \
  --credential-file /secure/path/demo-account.txt \
  --slurl secondlife://operator-supplied-location \
  --hardware-label linux-mesa-current \
  --workload-id controlled-steady-scene \
  --power-source ac \
  --low-power-mode off \
  --thermal-state nominal \
  --scene-events none \
  --ui-state approved \
  --camera-state approved \
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
  --workload-id controlled-steady-scene \
  --power-source ac \
  --low-power-mode off \
  --thermal-state nominal \
  --scene-events none \
  --ui-state approved \
  --camera-state approved \
  --output-dir /tmp/renderer-results \
  --repeats 1 \
  --dry-run
```

### macOS local build and smoke

The macOS benchmark uses the native OpenGL backend. Configure a local ReleaseOS app with benchmark instrumentation and tests enabled, but signing and crash reporting disabled. The following route was verified with a disposable Python virtual environment and a Nix-provided CMake executable. Run CMake directly from its Nix output instead of entering a Nix shell so the build continues to use Xcode's compiler and SDK environment.

```zsh
viewer_root=/path/to/viewer
build_variables=/path/to/secondlife-build-variables
build_venv=/private/tmp/renderer-mac-build/venv
cmake_root="$(nix build --no-link --print-out-paths nixpkgs#cmake)"

python3 -m venv "$build_venv"
"$build_venv/bin/python" -m pip install autobuild llsd

cd "$build_variables"
set -a
source ./convenience Release
set +a

cd "$viewer_root"
env \
  PATH="$cmake_root/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin" \
  PYTHON="$build_venv" \
  "$build_venv/bin/autobuild" configure -c ReleaseOS -- \
    -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang \
    -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ \
    -DCMAKE_OSX_SYSROOT:PATH=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk \
    -DPython3_EXECUTABLE:FILEPATH="$build_venv/bin/python" \
    -DPYTHON_EXECUTABLE:FILEPATH="$build_venv/bin/python" \
    -DLL_RENDER_BENCHMARK:BOOL=ON \
    -DLL_TESTS:BOOL=ON \
    -DPACKAGE:BOOL=ON \
    -DRELEASE_CRASH_REPORTING:BOOL=OFF \
    -DNON_RELEASE_CRASH_REPORTING:BOOL=OFF \
    -DENABLE_SIGNING:BOOL=OFF

env \
  PATH="$cmake_root/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin" \
  "$build_venv/bin/autobuild" build -c ReleaseOS --no-configure

"$cmake_root/bin/ctest" \
  --test-dir "$viewer_root/build-darwin-universal" \
  -C Release \
  -R INTEGRATION_TEST_RUNNER_lldir \
  --output-on-failure
```

The app executable is at `build-darwin-universal/newview/Release/Second Life Test.app/Contents/MacOS/Second Life Test`. A first local launch can exercise bundle resource lookup without credentials or login:

```zsh
isolated_root="$(mktemp -d /private/tmp/secondlife-help.XXXXXX)"
SECONDLIFE_USER_DIR="$isolated_root/profile" \
  "$viewer_root/build-darwin-universal/newview/Release/Second Life Test.app/Contents/MacOS/Second Life Test" \
  --help
rm -r -- "$isolated_root"
```

Snapshot the normal profile and cache metadata before and after this check. The isolated profile should contain `data`, `logs`, `user_settings`, `browser_profile`, and `cache`, and the temporary root should be removed afterward.

For an authenticated launch and export smoke, create a short manifest outside the repository from `steady-warm-v1.json`. Keep the display and validity policy unchanged, shorten capture duration, and use one measured repeat. Keep the 30-second warm-up so the declared 15-second settlement window remains meaningful. The warm scenario runs one unmeasured prime followed by the measured capture. Validate the temporary manifest before launching. Supply the credential file and controlled destination separately.

```zsh
chmod 600 /secure/path/benchmark-account.txt

python3 scripts/perf/render_benchmark.py validate \
  manifest /private/tmp/steady-warm-smoke.json

python3 scripts/perf/render_benchmark.py run \
  --viewer "$viewer_root/build-darwin-universal/newview/Release/Second Life Test.app/Contents/MacOS/Second Life Test" \
  --manifest /private/tmp/steady-warm-smoke.json \
  --credential-file /secure/path/benchmark-account.txt \
  --slurl secondlife://operator-supplied-location \
  --hardware-label mac-apple-silicon-smoke \
  --workload-id controlled-steady-scene \
  --power-source ac \
  --low-power-mode off \
  --thermal-state nominal \
  --scene-events none \
  --ui-state approved \
  --camera-state approved \
  --expect-gpu-substring Apple \
  --backend native-gl \
  --output-dir /private/tmp/renderer-smoke

python3 scripts/perf/render_benchmark.py validate \
  result /private/tmp/renderer-smoke/RESULT.json
```

The smoke validates app launch, native OpenGL selection, version 3 export, geometry, and scene-gate behavior only. It is not performance evidence and its timing fields must be discarded. A failed scene gate is useful smoke evidence but is not a benchmark result.

### Asset and appearance readiness prime

Use the prime-only mode to investigate a warm isolated profile before collecting a baseline. It runs the unchanged warm scenario twice against one disposable cache, emits no measured result, and reduces each launch to privacy-safe cache, asset, and appearance facts. The second launch exists to prove that the first launch's cache survives and remains writable. Other scene-gate failures, including placement and focus, remain explicit but do not prevent the asset and avatar readiness check from succeeding.

```zsh
readiness_root="$(mktemp -d /private/tmp/renderer-readiness.XXXXXX)"

python3 scripts/perf/render_benchmark.py run \
  --viewer "$viewer_root/build-darwin-universal/newview/Release/Second Life Test.app/Contents/MacOS/Second Life Test" \
  --manifest scripts/perf/scenarios/steady-warm-v1.json \
  --credential-file /secure/path/benchmark-account.txt \
  --slurl secondlife://operator-supplied-location \
  --hardware-label mac-apple-silicon-readiness \
  --workload-id controlled-steady-scene \
  --power-source ac \
  --low-power-mode off \
  --thermal-state nominal \
  --scene-events none \
  --ui-state approved \
  --camera-state approved \
  --expect-gpu-substring Apple \
  --backend native-gl \
  --output-dir "$readiness_root/results" \
  --repeats 1 \
  --warm-prime-attempts 2 \
  --prime-only \
  --readiness-output "$readiness_root/readiness.json"

python3 -m json.tool "$readiness_root/readiness.json"
```

A readiness pass has `readiness_passed: true`, `cache_reuse_passed: true`, two attempts, zero valid measured repeats, and `retained_timing: false`. On the second attempt, the requested cache root and nested asset root must remain writable, the fixed disposable asset sentinel must be `ready` before and after launch, the fallback asset root must remain absent, and `first_cache_failure` must be `none`. Both target gates must be true. Report any remaining names in `failed_gates` separately. Asset readiness includes separate settlement and queue booleans; avatar readiness separates appearance completion from unintended movement.

In prime-only mode, each guarded poll also requests a benchmark-only paired scene and appearance snapshot. Those two diagnostic facts come from the same main-thread response; the collector rejects a response if their avatar-ready booleans disagree. It retains the last snapshot paired with a failed `self_avatar_loaded` sample, or the final snapshot when every guarded sample passed. The viewer exposes only fixed booleans for self-avatar validity, COF presence and completeness, COF-change context, resolved required links, delivered required wearables, and final avatar readiness. The four fixed required parts are shape, skin, hair, and eyes.

The appearance category has this precedence:

- `avatar-unavailable`
- `cof-incomplete`
- `required-link-missing-or-unresolved`
- `wearable-delivery-pending-or-failed`
- `avatar-later-blocker`
- `ready`

`required-link-missing-or-unresolved` is deliberately combined because a link whose target is absent from the local inventory cannot reveal its intended wearable type. `wearable-delivery-pending-or-failed` is also combined because the stable public appearance APIs do not distinguish an outstanding asset request from a terminal failure. `cof_change_in_progress` is context, not proof of either condition. The runner recomputes the category from the projected booleans and rejects contradictory facts. If appearance remains incomplete after cache readiness passes, use only these facts to choose the next investigation; do not change the account outfit without authorization.

The readiness file contains aggregate counts, booleans, and allow-listed categories only. It contains no frames, timing summary, account, destination, inventory or asset identifiers, item names, raw log line, or filesystem path. Appearance facts are optional in schema-3 results and do not participate in a gate, policy hash, manifest hash, comparison field, or summary. Normal primes and measured runs never request the diagnostic operation. Report the safe readiness fields, then remove the readiness root, any temporary credential file, private logs, isolated state, and raw prime artifacts.

The runner sets `SECONDLIFE_USER_DIR` and creates session settings, cache, logs, and account data inside a private per-invocation temporary directory. It precreates the user and cache roots once, never repairs them between warm launches, selects the same explicit path in both cache-location settings, and disables legacy cache migration for the isolated session. The viewer also skips migration when its normalized source and destination are identical while preserving migration between different locations. The runner does not read or alter the normal viewer profile, and the isolated data is removed after the sequence exits. Cold-cache repeats receive separate state and purge before startup. Warm-cache sequences first run one unmeasured full-duration prime, then reuse that isolated profile and cache for all measured repeats. The prime is validated but its artifact is discarded. Cache probes and the sentinel are active only in prime-only diagnosis; measured benchmark launches never execute them. First-install UI, notifications, audio, and voice are disabled for benchmark sessions so they cannot cover the workload or crash a headless test host.

After the viewer reaches its started state, the LLLeap collector reapplies the requested scenario settings. This ordering prevents hardware feature-table initialization from replacing explicit benchmark controls. Every prime and measured result is rejected if an effective setting differs from the manifest. A non-maximized result is also rejected unless its actual backing-pixel width and height exactly match the requested resolution. On macOS, the XIB-owned app window converts requested backing pixels to Cocoa content dimensions so Retina scaling does not change the rendered resolution. The benchmark-only display target then normalizes the viewer UI scale from the detected backing scale.

## Result and comparison rules

Raw artifacts use `renderer-benchmark-result.schema.json`, schema version 3. Each valid result contains:

- scenario, repeat, cache mode, requested settings hash, and manifest hash;
- viewer version, source commit, tracked-diff hash/dirty state, build type, OS, CPU and logical-core count;
- operator hardware label, GPU, driver, reported VRAM, requested and detected backend;
- OpenGL version/profile, limits, extension set and hash, shader level, feature flags, actual settings and hash, backing resolution, logical content size, backing scale, configured UI factor, and effective display scale;
- the workload slug, policy and policy hash, typed operator assertions, privacy-safe scene observations, every derived gate, and a relative-view hash;
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

The reporter rejects different scenarios, manifest hashes, requested or actual settings, feature sets, source commits, build types, resolutions, instrumentation modes, workload slugs, validity policies, operator assertions, relative camera views, or stable population counts. Extension sets must match between repeats of one backend; native OpenGL and Zink extension differences are expected and their hashes remain visible in the cross-backend report. `--allow-mismatch` is an audit escape hatch; the mismatches are printed into the report.

## Invalid runs

Discard a run and preserve its reason when any of the following applies:

- login or startup did not finish;
- too few frames were captured;
- requested and detected backends differ;
- the selected GPU does not match `--expect-gpu-substring`;
- placement does not match the requested start location;
- asset loading is incomplete for a settled run;
- scene population, camera, avatar, window focus, feature flags, or resolution changed;
- blocking UI, progress UI, or unapproved controlled UI is present;
- the circuit is unhealthy or simulator ping exceeds the scenario policy;
- a material scene event was observed;
- power source is unknown, low-power mode is enabled, or thermal state is unknown or throttled;
- visual comparison differs outside the project's accepted tolerance.

The collector measures viewer facts and derives each gate. Power, thermal, scene-event, intended-UI, and intended-camera values remain typed operator assertions. The runner independently recalculates the gates, checks their policy and manifest hashes, and rejects tampering or drift. Never silently average invalid runs into a summary.

## GPU and system diagnostics

Run one separate diagnostic capture for each important steady configuration using Tracy plus RenderDoc, Nsight Graphics, Radeon GPU Profiler, or the platform's equivalent. Record GPU pass timestamps, queue idle, driver waits, readbacks, pipeline/shader creation, per-thread CPU traces, and per-core utilization. Keep large captures as CI or investigation artifacts, not in Git.

Do not enable the viewer's per-shader Frame Profile for steady runs: it reads query results and can change the workload. Preserve the feature manager's existing AMD RDNA 3.5 query safeguards. A Zink result is Linux evidence only; it is not proof that Windows or macOS will behave the same way.

## Privacy

The C++ event API selects safe fields from viewer information rather than exporting the full About/profile context. Absolute placement and an agent-relative camera view exist only in the private collector process long enough to derive a placement boolean and relative-view hash. The runner removes account, credential, machine ID, serial, hostname, position, camera origin/orientation, parcel, region, location, and raw-view keys recursively before writing or reporting an artifact. Checked-in fixtures use synthetic hardware strings. Review `find_private_paths()` results before publishing a new fixture.

## Validation

```bash
python3 -m unittest discover -s scripts/perf/tests -v
python3 scripts/perf/render_benchmark.py validate manifest scripts/perf/scenarios/steady-warm-v1.json
python3 scripts/perf/render_benchmark.py validate result scripts/perf/fixtures/renderer-result-v3.json
```

The tests cover all manifests, the 1× and 2× scale contract, scene policy, wrong placement, focus loss, unsettled assets, population drift, unexpected UI, gate tampering, cache survival, launch-bounded log categories, scale and resolution mismatches, comparison drift, percentile calculations, resource deltas, recursive privacy filtering, the checked-in fixture, and the no-secret dry-run path. Focused C++ integration tests cover scale derivation, the disabled normal-launch path, and the cache self-migration guard.
