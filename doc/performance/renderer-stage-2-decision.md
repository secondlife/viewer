# Mac Stage 2 renderer benchmark decision

## Decision

The macOS benchmark display contract is ready for a controlled Apple Silicon baseline. Stage 2 fixes the rendered surface at 1280×720 backing pixels and the effective viewer UI scale at 1.0. It does not produce or preserve performance evidence.

Stage 3 may collect the native OpenGL baseline only after the remaining scene-state gates are in place. Hardware comparison remains locked until Stage 4.

## Display model

Version 2 results keep `context.width` and `context.height` as backing pixels and add explicit fields for every coordinate space:

| Field | Meaning on the verified 2× path |
| --- | ---: |
| Backing width and height | 1280×720 pixels |
| Cocoa logical width and height | 640×360 points |
| Backing scale X and Y | 2.0 |
| Configured `UIScaleFactor` | 0.5 |
| Effective display scale X and Y | 1.0 |

On a 1× path, logical and backing dimensions are both 1280×720 and the configured UI factor is 1.0. The benchmark derives both cases from the factual native backing scale. It does not hardcode the Retina factor.

Every version 2 scenario explicitly enables `RenderHiDPI`, requests a 1280×720 non-maximized window, and requests an effective scale of 1.0. The viewer applies the contract only in a benchmark-enabled build after the native window is attached. It reapplies the requested backing dimensions before reflowing the UI, which prevents Cocoa point dimensions from being saved as backing pixels. Ordinary builds compile this path out.

## Validation behavior

The runner rejects a result when:

- legacy and explicit backing dimensions differ;
- actual backing dimensions differ from the manifest;
- logical dimensions, backing scale, and backing dimensions do not reconstruct the same surface;
- the configured UI factor does not produce the target scale;
- the final display scale differs from the target;
- any display field changes between comparable repeats;
- required display fields are missing; or
- a version 1 manifest or result is supplied.

The LLLeap collector reapplies scenario settings after startup, requests the benchmark-only display normalization, and waits for the exported contract to settle before warmup. This keeps invalid geometry out of the capture window and still leaves final validation as a fail-closed boundary.

## Stage 2 evidence

- All 26 Python benchmark tests passed, including 1× and 2× result validation, geometry and scale mismatches, missing fields, old-schema rejection, and runtime normalization. The focused C++ helper test covers preserving the configured scale when benchmark normalization is inactive.
- The focused display and macOS directory integration tests passed.
- Benchmark-enabled universal ReleaseOS compilation and app packaging passed for Apple Silicon and Intel architectures.
- Benchmark-disabled universal compilation passed, confirming that normal viewer behavior remains on the existing path.
- A private native OpenGL smoke completed one unmeasured warm prime and one measured repeat on a Retina display. Both validated schema 2, 1280×720 backing pixels, 640×360 Cocoa points, 2.0 backing scale, 0.5 configured UI factor, and 1.0 effective display scale.
- Private visual inspection showed normalized UI coverage rather than the oversized coverage in the original diagnostic evidence. No image was retained.
- The smoke timings were discarded. Credentials, isolated viewer state, and live result artifacts were kept outside Git and removed after validation.

Stage 2 changes no renderer backend, shader, normal profile, production service, signing identity, Vulkan dependency, Zink path, or performance conclusion.

## Reanalysis

`WindowWidth` and `WindowHeight` remain the requested backing workload in benchmark manifests. Cocoa content size is exported separately and no longer inferred from those settings. The runner can distinguish backing pixels, Cocoa points, native backing scale, configured UI scale, and final viewer scale without exporting a screen identifier or window position.

The scale factor is derived only in a benchmark-enabled viewer with a positive benchmark target. A benchmark-disabled build and a zero target retain the current configured value. Version 1 data cannot satisfy the new contract because both schemas now require version 2 and the new fields.

The remaining uncertainty is scene state, not display state. Asset completion, fixed camera, stable visible population, simulator events, focus, power, and thermal state still need an auditable gate before frame times become evidence.

## Superseded Mac Stage 3 boundary

### Objective

This decision originally combined scene-validity implementation and a controlled Apple Silicon baseline in Mac Stage 3. The later Stage 4 reanalysis splits those tasks so every result is collected from a clean build of an already committed validity definition.

### Scene-validity contract

The first new commit adds only the scene-state facts needed to reject an unstable steady capture:

- a stable workload identifier supplied without a private location;
- fixed camera and avatar movement state;
- visible avatar and active-object counts sampled before and after capture;
- asset-loading or pending-fetch state at the capture boundary;
- window focus state;
- simulator ping and material scene-event notes; and
- an operator record for power mode and thermal throttling.

Prefer existing viewer facts and runner-side equality checks. Do not add a general telemetry system, screenshot pipeline, renderer abstraction, or platform comparison framework.

### Deferred capture protocol

The following work moves to Renderer Stage 5 and remains dependent on the committed schema-3 contract:

1. Use the checked-in steady warm scenario without editing its settings or validity policy.
2. Use a controlled destination and fixed camera supplied outside Git.
3. Run native Apple OpenGL only with the expected Apple GPU.
4. Prime the isolated warm state once without retaining its timing.
5. For each measured repeat, warm up for 30 seconds and capture for 120 seconds.
6. Collect five valid repeats with identical source, settings, display fields, scene-state fields, build type, and instrumentation mode.
7. Discard and repeat any run that fails a display, scene, focus, asset, simulator-event, power, or thermal gate.
8. Keep raw live results and any visual reference private. Publish only a sanitized aggregate and decision record.

### Exit evidence

The baseline stage is complete only when five repeats pass every gate, run-to-run p95 range is reported, major CPU phase and resource-counter behavior is summarized, and no timing claim exceeds the existing greater-than-1-ms and greater-than-three-times-noise threshold without matching evidence.

The baseline decision must choose one of these outcomes:

- identify a bounded current-OpenGL investigation target supported by the controlled data;
- record that the baseline is stable but does not isolate a renderer bottleneck, then specify the next diagnostic trace; or
- keep the baseline blocked because the scene cannot yet be controlled.

The matched hardware-class comparison remains locked until the baseline is complete. The detailed current boundary is in `renderer-stage-4-decision.md`.
