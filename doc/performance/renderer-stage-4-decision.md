# Renderer Stage 4 decision

## Decision

The schema-3 scene-validity contract is ready for hardware execution. Stage 4 makes an ambiguous workload fail before it can enter a performance report. It produces no retained timing evidence and makes no claim about OpenGL, Vulkan, Zink, or Apple Silicon performance.

Reanalysis in this stage splits the former combined Mac baseline stage into two committable stages. Stage 4 lands and verifies the validity definition. Stage 5 will collect the five-run Apple Silicon baseline from a clean build of that committed definition. Earlier documents combined those tasks; this decision supersedes that commit boundary.

## Contract delivered

Schema version 3 adds a declarative validity policy to every scenario and a privacy-safe validity record to every result. The collector and runner fail closed on:

- requested placement, teleport, and progress state;
- accumulated camera translation and rotation, camera animation, and avatar travel;
- foreground focus for the capture window;
- modal or first-use UI, alerts, hints, progress UI, and scenario-specific floater state;
- texture-fetch, HTTP, texture-creation, fast-cache, upload, and unresolved mesh work;
- self-avatar completion, visible-avatar drift, active-object drift, and new objects;
- circuit health and simulator ping; and
- typed operator assertions for power source, low-power mode, thermal state, material scene events, intended UI, and intended camera.

Comparable results must also have identical manifest hashes, workload slugs, validity policies, operator assertions, relative camera-view hashes, and stable population counts. The runner recalculates every gate from the retained observations instead of trusting the collector's booleans.

## Privacy boundary

The viewer compares the requested destination against the live region and position but exports only a boolean. The collector hashes a rounded camera view relative to the avatar, then discards the raw view. Accounts, credentials, SLURLs, regions, parcels, absolute positions, camera origins and orientations, host identifiers, and machine identifiers are removed recursively. Printed launch commands redact all login fields and the SLURL.

## Provenance and policy

The placement rule reuses the viewer's exact-region and 2 m X/Y start-location tolerance. Camera motion reuses the scene-loading monitor's accumulated 0.1 m translation and 0.05 rad rotation limits. The 15-second window comes from the mesh subsystem's no-progress horizon. Focus and zero population drift follow existing performance and benchmark behavior.

Their use as benchmark acceptance gates is a project decision. The composite all-zero asset rule, zero avatar travel, zero new objects, capture-wide 600 ms ping limit, typed operator vocabulary, five repeats, and 30-second warm-up are conservative benchmark policy rather than an upstream performance standard. In particular, the 15-second precedent is mesh-specific and the existing 600 ms lag-meter value is based on period-mean ping. A strict false rejection is acceptable; an ambiguous timing sample is not.

Power and thermal fields are operator assertions, not automatic hardware telemetry. A valid run requires a known and repeatable power source, low-power mode off, and a non-throttled thermal state. AC power is not universally required. All assertions must match across repeats.

## Evidence

- All focused Python benchmark tests pass, including valid manifests and fixtures, placement, focus, asset, population, UI, tamper, comparison, and privacy cases.
- Both JSON schemas accept all checked-in manifests and the synthetic schema-3 result fixture.
- The changed benchmark-enabled Linux C++ units compile with the new viewer facts and mutex-protected mesh queue counts. The stats listener also compiles with `LL_RENDER_BENCHMARK` disabled.
- The existing benchmark display and directory integration tests pass.
- A full Linux link was not re-proven in this environment. The build progressed through the changed mesh unit, then GCC 15 stopped on an unrelated pre-existing `-Werror=array-bounds` diagnostic in `llpaneloutfitedit.cpp`. The changed stats unit was compiled separately after that failure.
- No live result, credential, SLURL, private scene identifier, or timing measurement is retained in this commit.

The public source trail is consistent with the validity-first boundary: [PR #6032](https://github.com/secondlife/viewer/pull/6032#issuecomment-5298969752) records an Apple Silicon FPS comparison invalidated by lost focus; [issue #4274](https://github.com/secondlife/viewer/issues/4274#issuecomment-2985035642) calls for scene texture-load metrics before A/B testing; [issues #5742](https://github.com/secondlife/viewer/issues/5742) and [#5663](https://github.com/secondlife/viewer/issues/5663) request live scene and camera facts for automation; and [issue #5368](https://github.com/secondlife/viewer/issues/5368#issuecomment-4361798311) supports standardized performance export and a 120-second observation window. None mandates this stage split, schema number, or the project-specific acceptance thresholds.

## Reanalysis

The earlier display split and this scene split follow the same evidence boundary: a result cannot honestly claim proof that was added after it was collected. Stage 1 already showed that plausible-looking runs with different destination, UI, cache, and asset work cannot be repaired by reporting logic. Public viewer history also records an Apple Silicon FPS comparison invalidated by a brief focus loss and requests scene-level asset and camera facts before automation.

This stage does not prove that every conservative gate will pass in the chosen live scene. That is now the first question Stage 5 must answer. If the controlled scene cannot satisfy the contract without weakening it after seeing timings, the baseline remains blocked and the rejected gate facts become the next diagnostic input.

## Stage 5: controlled Apple Silicon native OpenGL baseline

### Objective

Collect one privacy-safe, five-repeat native OpenGL baseline on Apple Silicon using a clean ReleaseOS build of the committed schema-3 contract. Characterize the current path only. Do not compare hardware classes, add renderer code, or retain raw private artifacts in Git.

### Inputs and preflight

1. Start from a clean checkout containing the Stage 4 commit. Record the commit and confirm the result reports `git_dirty: false`.
2. Build the universal macOS viewer with `LL_RENDER_BENCHMARK=ON`, `LL_TESTS=ON`, signing and crash reporting off, and the existing ReleaseOS settings.
3. Run the Python benchmark suite and focused display/macOS integration tests.
4. Use `steady-warm-v1.json` without editing its display, graphics, or validity fields.
5. Keep the account, credential file, controlled SLURL, output directory, and visual reference outside the repository. Use a stable, non-identifying workload slug.
6. Record truthful operator assertions immediately before the batch. Use the same power source for every repeat, keep low-power mode off, and stop if the machine is throttled or the assertions change.

### Capture

1. Launch native Apple OpenGL with `--expect-gpu-substring Apple` and the typed schema-3 operator arguments.
2. Let the runner perform one full unmeasured warm-cache prime.
3. Collect five measured repeats. Each repeat uses 30 seconds of warm-up and 120 seconds of capture.
4. Do not touch the camera, avatar, window focus, or controlled UI after the prime begins.
5. Treat every nonzero exit or failed gate as a rejected run. Preserve only a privacy-safe rejection count and failed-gate names; do not include rejected timing in a report.
6. If five valid repeats cannot be obtained without changing the contract, stop and report the blocking gates. Do not tune thresholds from observed frame times.

### Analysis and exit evidence

1. Validate all five raw results and generate the standard report without `--allow-mismatch`.
2. Confirm equal source, manifest, settings, display geometry, workload, operator assertions, relative camera view, population, and instrumentation across repeats.
3. Report median, p95, p99, worst frame, 1 percent low FPS, run-to-run p95 range, dominant CPU phases, resource-counter deltas, and rejected-run reasons.
4. Keep raw results and visual material private. Check in only a sanitized aggregate and the Stage 5 decision.
5. Make no performance conclusion beyond the existing threshold: a delta must exceed both 1 ms and three times run-to-run p95 range. A single baseline has no backend or hardware delta.
6. End Stage 5 by selecting exactly one next investigation from the observed dominant cost, or by recording that the controlled baseline is stable but not diagnostic.

Stage 6, the matched older-hardware comparison, remains locked until Stage 5 is complete and replanned.
