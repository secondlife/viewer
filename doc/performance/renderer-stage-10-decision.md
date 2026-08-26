# Stage 10 OpenGL tonemap replay decision

## Decision

Accept the tonemap packet and narrow OpenGL executor as master Stage 1B. The
same immutable packet now validates independently and drives one real viewer
pass through the existing OpenGL objects. The legacy implementation remains
the production default, and a rejected packet falls back before the destination
is bound or modified.

This does not complete master Stage 1. There is no Vulkan executor, dual-API
comparison, material replay, streaming-upload replay, or performance result.
The live-world baseline remains paused after zero valid measured repeats.

## Frozen trace

Tracing corrected and completed the earlier representative fixture:

- the four-word parameter block is `{ exposure, tonemap mix, tonemap type,
  gamma }`; gamma was previously a hidden environment-fed shader input;
- all six compiled variants are represented by stable bits for `NO_POST`,
  `GAMMA_CORRECT`, and `LEGACY_GAMMA`, including the currently unreachable
  legacy-gamma/non-no-post combination;
- the destination may be RGBA8 or RGBA16F and its extent is dynamic;
- the scene and exposure render-target textures retain mirrored addressing,
  with point and linear filtering respectively;
- disabled depth retains the legacy `LEQUAL` comparison; and
- the real screen triangle remains three `Float3` positions in a 48-byte
  buffer with a 16-byte stride.

The pure builder owns a copy of the parameter bytes and rejects invalid
variants, formats, extents, handles, exposure ranges, and non-finite values.
The decoder accepts only the canonical packet shape rather than treating any
otherwise valid frame as a tonemap request.

## Boundary and fallback

`lltonemapcontract` remains part of the independently linked, GL-free contract
target. It contains no viewer settings, sky state, render-target pointers,
shader objects, GL types, or API calls.

The OpenGL registry borrows existing `LLRenderTarget`, `LLVertexBuffer`, and
`LLGLSLShader` objects for one synchronous replay. Resolution requires the
exact typed handle index and generation, and pipeline resolution also requires
the exact program name and variant. These handles are lexical replay handles;
they are not yet persistent ownership records across reallocations or shader
reloads.

The executor decodes the complete packet and resolves every object, format,
extent, sampler, shader feature, permutation, texture channel, and required
uniform before binding the destination. It then applies the packet's color,
blend, cull, depth, viewport, scissor, sampler, parameter, geometry, and draw
state through the existing wrappers. The developer setting
`RenderUseTonemapContract` is non-persistent and defaults to false. Failure in
the builder, registry, or executor takes the independent legacy path once.

## Fixed-input parity

The account-free `--tonemapparity` startup mode runs after GL and shader
initialization and exits before login. It creates an 8 by 8 HDR scene, a 1 by 1
exposure image, and separate legacy and contract destinations. Before each
path, it poisons shader uniforms, texture bindings, vertex-buffer binding, and
relevant GL state so equality cannot come from cached state inherited from the
other path. Before contract execution it also poisons the actual sampler
objects and blend, cull, scissor, and color-mask state.

The reference side calls the same direct submission helper used by the
production legacy path. Before every case, the two destinations receive
different case-specific sentinels, and both draws are bracketed by explicit GL
error checks. A missing draw therefore cannot pass by reusing an old output or
by making both paths fail in the same way.

The matrix covers both output formats, all six compiled variants, and both
supported tonemap types: 24 cases. Every RGBA component is compared after
readback with a declared tolerance of zero. A separate stale-generation case
requires preflight rejection and proves that an RGBA8 destination sentinel is
unchanged.

On the Apple OpenGL 4.1 context, all 24 cases matched exactly:

```text
TONEMAP_CONTRACT_PARITY result=pass cases=24 tolerance=0 max_abs_error=0 mismatches=0 execution_failures=0 rejection_failures=0
```

This is correctness evidence for one OpenGL pass. It is not a performance
measurement and says nothing about Vulkan speed.

## Measured surface

| Surface | Size |
| --- | ---: |
| Pure tonemap contract header and implementation | 386 lines |
| OpenGL registry and executor | 395 lines |
| Focused registry tests | 80 lines |
| Existing contract fixture changes | 87 additions, 41 removals |
| Render-target and build wiring | 16 additions, 1 removal |
| Viewer integration, parity harness, and Mac isolation | 536 additions, 45 removals |

The 395-line API adapter is deliberately pass-specific. Material, upload,
presentation, window, and general-purpose command-encoder behavior did not
enter it.

## Verification

- Linux passed 18 contract cases and 2 registry cases.
- macOS passed the same 18 and 2 focused cases on the host architecture.
- The account-free native OpenGL parity mode passed all 24 exact comparisons
  and the stale-generation non-mutation check.
- macOS rejected parity startup without `SECONDLIFE_USER_DIR` before Cocoa or
  viewer construction. The XIB now starts the main window hidden; ordinary
  launches order it forward after context creation, while parity keeps it
  hidden and still creates the required GL context.
- The benchmark-enabled universal Release viewer built after the final changes
  and contains both `x86_64` and `arm64` executable slices.
- All 57 Python benchmark tests passed. Draft 2020-12 validation accepted both
  schemas, all six manifests, and the schema-3 fixture.
- GCC 15 accepted the pure builder and decoder as C++20 with `-Wall -Wextra
  -Wpedantic -Werror`.
- Whitespace validation passed, and adversarial review found no remaining
  correctness blocker in the executor or parity boundary.

The parity launch used a disposable profile and no credentials. Its temporary
profile, cache, logs, build environment, and incidental startup output were
removed. No benchmark result or timing was retained.

## Reanalysis

The next dependency is no longer another OpenGL abstraction. The tonemap
packet has a reference output and a proven executor boundary, so master Stage
1 now needs the same packet consumed by an isolated Vulkan process.

Repository inspection found no Vulkan headers, loader target, MoltenVK package,
SPIR-V compiler, shader manifest, or Vulkan implementation. Existing GLSL is
compiled and reflected at runtime through OpenGL-specific shader management.
The next commit therefore has to establish a portable Vulkan toolchain and an
offscreen executor together, while keeping platform surfaces and GL/Vulkan
interop out of scope. A dependency-only commit would not test the contract; a
windowed or whole-frame backend would jump ahead of the master plan.

Stage 11 should end with the canonical tonemap packet replayed into offscreen
Vulkan images from the same fixed CPU fixture, validation enabled, deterministic
readback compared with the Stage 10 reference, and an explicit portability
result on Linux and MoltenVK-capable macOS. If shader translation or platform
availability prevents that bounded replay, the stage must commit only an
evidence-backed stop decision and remove unused scaffolding rather than widen
into presentation or production renderer work.
