# Stage 1 renderer benchmark decision

## Decision

Do not begin a Vulkan implementation or select an OpenGL optimization from the measurements collected in Stage 1. The benchmark suite is ready, but the available live-world workload was not controlled well enough to diagnose the reported newer-hardware regression.

Stage 2 will establish a deterministic workload on affected newer hardware, verify the context the driver actually creates, and capture external CPU/GPU traces. It will not add Vulkan. A current-OpenGL fix or a renderer-interface spike becomes eligible only after that evidence identifies the limiting path.

## What Stage 1 delivered

- Compile-gated CPU phase timers and renderer resource counters. Ordinary builds define `LL_RENDER_BENCHMARK=0` and compile the instrumentation out.
- A benchmark-safe LLLeap event that exports selected renderer, build, settings, and hardware context without exporting the normal profile or location context.
- Versioned manifest and result schemas, six workload manifests, a synthetic fixture, an isolated-profile runner, and a guarded reporter.
- Native OpenGL and Mesa Zink backend detection, including a check against the renderer string rather than trusting the requested backend.
- A 30-second warm-up, 120-second capture, five-repeat protocol. Warm-cache sequences use an unmeasured prime and one shared isolated cache; cold-cache repeats receive separate state.
- Comparison and privacy tests that reject incomplete results, unlike configurations, too few repeats, and identifying keys.

The steady loop adds no GPU query reads. GPU pass timing remains an explicitly separate RenderDoc, Radeon GPU Profiler, Nsight Graphics, Tracy, or equivalent capture.

## Reference host and coverage

The suite was built and launched on Linux with an AMD Ryzen 7 5800X, an AMD Radeon RX 480, Mesa 26.2.1, and the RADV Vulkan driver. Native OpenGL and Zink both selected the physical RX 480. This is an older reference machine, not the reported newer-hardware class.

The following required matrix cells were not available and no conclusion is drawn for them:

- affected newer GPU and a matched older control;
- Windows on current NVIDIA, AMD, and Intel drivers;
- Linux on current Intel and NVIDIA drivers;
- macOS on Apple Silicon and a still-supported Intel baseline;
- a second machine or driver from any class showing a threshold-crossing result.

The local build disabled media plugins because their development dependencies were not present. A GCC 15 false positive in an untouched source file required a generated-build-only warning suppression. Neither deviation is part of the commit.

## Diagnostic matrix

Five native OpenGL and five Zink captures completed end to end. They are invalid as steady warm-cache performance evidence: the viewer did not honor the requested destination, first-use UI opened over the scene, assets were still arriving, and each initial repeat had a fresh profile and cache. The runner now performs an unmeasured prime and reuses warm state, but the live location and first-use UI still cannot be gated automatically. Raw captures and screenshots are intentionally not checked in because they include live-world details.

The invalid captures remain useful as a harness and noise diagnostic:

| Backend | Median run p50 | Median run p95 | Median run p99 | Run-p95 range |
| --- | ---: | ---: | ---: | ---: |
| Native OpenGL | 5.155 ms | 8.634 ms | 13.266 ms | 4.21 ms |
| Zink | 5.399 ms | 7.364 ms | 11.071 ms | 2.70 ms |

Zink's median p95 was 1.27 ms lower. The predeclared threshold for this matrix is the larger of 1 ms and three times the largest run-p95 range: 12.63 ms. The observed delta is below that threshold and cannot support a backend claim even if the scene had been valid.

The largest median-of-run p95 CPU phases were:

| Phase | Native OpenGL | Zink |
| --- | ---: | ---: |
| Unclassified | 3.180 ms | 2.738 ms |
| Swap | 0.319 ms | 1.602 ms |
| Idle | 1.355 ms | 1.256 ms |
| GL submission | 1.275 ms | 0.827 ms |
| State sort | 0.870 ms | 0.541 ms |
| Texture work | 0.553 ms | 0.445 ms |

Texture uploads varied too widely to treat the runs as the same workload. Native upload bytes ranged from roughly 1.0 MB to 521.7 MB; Zink ranged from roughly 1.0 MB to 360.7 MB. No explicit texture readbacks, synchronization events, or shader compilations occurred during the capture windows. External GPU pass timing and per-core traces were not collected because the workload had already failed its validity checks.

The initial artifacts labeled the profile from the requested core-profile setting. The advertised OpenGL version was actually a Mesa 4.6 compatibility profile. The exporter now derives `opengl_profile` from the advertised version and records the requested setting separately; a short post-fix launch verified that distinction. Results collected before that fix are not comparison inputs.

## What the evidence supports

- The build option, viewer export, isolated runner, schemas, backend verification, reporter, and native/Zink launch path work end to end on the Linux reference host.
- Live-world asset and UI variability is larger than the backend delta seen here.
- Zink is viable as a Linux diagnostic backend on this host, but this run says nothing about Windows, macOS, or affected newer hardware.
- The actual versus requested OpenGL profile must be treated as separate data. Context negotiation is now a specific investigation target.

## What the evidence does not support

- that OpenGL compatibility behavior causes the reported regression;
- that Zink or native Vulkan would improve it;
- that scene preparation, GL submission, present, or a GPU pass is the primary bottleneck;
- a visual-equivalence claim between native OpenGL and Zink;
- any cross-platform or newer-hardware performance conclusion.

No current-OpenGL fix is justified yet. Candidate fixes must come from a trace and may include eliminating an identified driver wait, reducing state or draw submission, correcting context selection, or reducing resource churn. They are hypotheses, not Stage 1 findings.

## Stage 2 selection

Stage 2 is **capture the affected-hardware regression**. Its commit will add the smallest deterministic workload and validity gates needed to obtain five comparable runs on an affected current system and an older control. It will also capture actual context negotiation, per-core CPU behavior, and one external GPU trace per important configuration.

The decision gate at the end of Stage 2 is:

- fix the current OpenGL path first when the controlled trace identifies a bounded scene-preparation, submission, synchronization, present, or GPU-pass bottleneck;
- proceed to an explicit dual-backend contract only when the measured problem is inseparable from the current API boundary or when a separately funded platform-longevity requirement justifies it;
- do not use GL-on-Vulkan as a production compatibility layer unless matched visual, feature, stability, and frame-time evidence shows that it is a supportable deployment path.

## Verification

- Benchmark-enabled Linux Release build linked and launched.
- Benchmark-disabled touched objects compiled, confirming the default-off path.
- Six manifests and the synthetic result validate against their JSON schemas.
- Eighteen runner and reporter unit tests pass.
- Native OpenGL and Zink selected the expected physical GPU and completed capture/export.
- Dry-run coverage verifies that credentials are neither opened nor printed.
- A native visual launch rendered the world without gross corruption; deterministic screenshot parity remains a Stage 2 gate.

Stage 1 changes no production service, live database, normal viewer profile, shader, renderer output, or Vulkan dependency.
