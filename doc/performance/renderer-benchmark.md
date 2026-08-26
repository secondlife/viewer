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

### Display contract

Schema version 2 fixes the benchmark render surface at 1280×720 backing pixels and the effective viewer UI scale at 1.0. Every checked-in scenario declares `RenderHiDPI: true`, `RenderBenchmarkUIScale: 1.0`, `WindowWidth: 1280`, `WindowHeight: 720`, and `WindowMaximized: false`. A scenario must not declare `UIScaleFactor`; the viewer derives that configured factor from the backing scale detected after window creation. Making HiDPI explicit ensures a fresh isolated profile exercises Retina backing on supported hardware instead of inheriting a local default.

On a 1× display, the configured UI factor is 1.0. On a 2× Retina display, it is 0.5. Both produce an effective display scale of 1.0 without changing the existing meaning of `context.width` and `context.height`, which remain backing pixels. The result also records explicit backing and logical dimensions, backing scale on both axes, the configured UI factor, and the final effective display scale on both axes.

The runner rejects a result when any explicit backing dimension differs from the legacy backing dimension or manifest, when logical size multiplied by backing scale does not reconstruct the backing size, when the configured factor does not produce the requested scale, or when the final display scale differs from the request. These fields must also match across repeated results. Schema version 1 results are not accepted by the version 2 runner or reporter.

`RenderBenchmarkUIScale` defaults to zero, is not persisted, and has an effect only in a build compiled with `LL_RENDER_BENCHMARK`. Ordinary builds and non-benchmark launches retain the normal platform UI-scale behavior.

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

For an authenticated launch and export smoke, create a short manifest outside the repository from `steady-warm-v1.json`. Change only the capture duration and repeat count. Keep the version 2 display settings unchanged. Use a short warm-up, a short capture, and one measured repeat. The warm scenario runs one unmeasured prime followed by the measured capture. Validate the temporary manifest before launching. Supply the credential file and controlled destination separately.

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
  --expect-gpu-substring Apple \
  --backend native-gl \
  --output-dir /private/tmp/renderer-smoke

python3 scripts/perf/render_benchmark.py validate \
  result /private/tmp/renderer-smoke/RESULT.json
```

The smoke validates app launch, native OpenGL selection, version 2 geometry export, and ordinary-looking UI coverage only. It is not performance evidence and its timing fields must be discarded. First-use UI, an unresolved destination, asset streaming, focus changes, or an unsettled scene invalidate any timing interpretation.

The runner sets `SECONDLIFE_USER_DIR` and creates session settings, cache, logs, and account data inside a private per-invocation temporary directory. It does not read or alter the normal viewer profile, and the isolated data is removed after the sequence exits. Cold-cache repeats receive separate state and purge before startup. Warm-cache sequences first run one unmeasured full-duration prime, then reuse that isolated profile and cache for all measured repeats. The prime is validated but its artifact is discarded. First-install UI, notifications, audio, and voice are disabled for benchmark sessions so they cannot cover the workload or crash a headless test host.

After the viewer reaches its started state, the LLLeap collector reapplies the requested scenario settings. This ordering prevents hardware feature-table initialization from replacing explicit benchmark controls. Every prime and measured result is rejected if an effective setting differs from the manifest. A non-maximized result is also rejected unless its actual backing-pixel width and height exactly match the requested resolution. On macOS, the XIB-owned app window converts requested backing pixels to Cocoa content dimensions so Retina scaling does not change the rendered resolution. The benchmark-only display target then normalizes the viewer UI scale from the detected backing scale.

## Result and comparison rules

Raw artifacts use `renderer-benchmark-result.schema.json`, schema version 2. Each valid result contains:

- scenario, repeat, cache mode, requested settings hash, and manifest hash;
- viewer version, source commit, tracked-diff hash/dirty state, build type, OS, CPU and logical-core count;
- operator hardware label, GPU, driver, reported VRAM, requested and detected backend;
- OpenGL version/profile, limits, extension set and hash, shader level, feature flags, actual settings and hash, backing resolution, logical content size, backing scale, configured UI factor, and effective display scale;
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
python3 scripts/perf/render_benchmark.py validate result scripts/perf/fixtures/renderer-result-v2.json
```

The tests cover all manifests, the 1× and 2× scale contract, missing geometry, schema version rejection, scale and resolution mismatches, comparison drift, percentile calculations, resource deltas, invalid data, recursive privacy filtering, the checked-in fixture, and the no-secret dry-run path. A focused C++ integration test proves the scale derivation and the disabled normal-launch path.
