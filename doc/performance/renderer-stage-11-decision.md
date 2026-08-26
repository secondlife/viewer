# Stage 11 offscreen Vulkan tonemap replay decision

## Decision

Accept the standalone Vulkan tonemap replay as the completion of the first
dual-API renderer slice. The same immutable packet and synthetic resources now
drive the existing OpenGL pass and a native Vulkan command buffer in separate
processes. Both backends produce finite output within the tolerance fixed
before execution, and Vulkan validation stays clean on Linux RADV and macOS
MoltenVK.

The viewer still starts through OpenGL. Vulkan remains an opt-in test-only
dependency and owns no window, surface, swapchain, viewer frame, or production
resource. This result proves a narrow correctness seam. It does not establish a
performance benefit and does not complete master Stage 1: the indexed material
draw and streaming texture upload still lack real dual-API execution.

## Shared diagnostic boundary

The Stage 10 fixture is now API-free and independently linked. It owns the
asymmetric 8 by 8 half-float scene, 1 by 1 exposure image, screen triangle,
six shader variants, two tonemap types, two destination formats, and all 24
canonical packets. Both executors consume those packets rather than rebuilding
equivalent private descriptions.

The versioned artifact records case identity, extent, format, bottom-left row
order, and finite normalized components. Publication creates a unique sibling
temporary file exclusively, writes and closes it completely, and installs it
with an atomic no-replace hard link. Existing destinations are rejected. The
OpenGL exporter canonicalizes readback to the actual RGBA8 or RGBA16F storage
codes before publication.

## Shader and Vulkan boundary

The Vulkan shaders are declaration wrappers around the existing tonemap
sources. They add explicit vertex, descriptor, fragment-output, and push-
constant interfaces without copying the sRGB or tonemap equations. The build
produces one vertex module and six fragment variants in the build tree and
validates every module with `spirv-val`.

The pass-specific registry borrows one run's Vulkan buffers, image views,
samplers, pipelines, descriptor sets, render passes, and framebuffers. Before
beginning the command buffer or changing an image, the executor checks exact
handle generations, program and variant, formats, extents, required usage bits,
sampler behavior, descriptor bindings, vertex layout, push-constant size, and
the canonical packet shape. It then applies the declared image transitions,
translated bottom-left viewport and scissor, descriptors, parameters, vertex
buffer, and three-vertex draw, and leaves the destination shader-readable.

Eight incompatible-resource mutations are rejected in preflight with no
submission. The native runner executes 24 valid packets and records exactly 24
submissions.

## Fixed-input comparison

On Apple Silicon, the OpenGL exporter and native arm64 MoltenVK process matched
all 6,144 components exactly:

```text
TONEMAP_COMPARE result=pass cases=24 components=6144 mismatches=0 max_abs_error=0 rgba8_tolerance=0.00392156886 rgba16f_tolerance=0.001953125
```

Linux RADV output compared with the same Mac OpenGL reference without a failed
case or component. Its maximum absolute delta was `0.00392156839`, just under
the frozen `1/255` RGBA8 allowance. The RGBA16F allowance remained `2/1024`.
No output contained a non-finite component.

These are correctness comparisons only. No frame, draw, build, startup, or
test timing was collected as migration evidence.

## Code size

| Code | Size |
| --- | ---: |
| Shared fixture, artifact, and comparison implementation | 960 lines |
| Vulkan registry and executor | 465 lines |
| Standalone Vulkan runner and artifact comparator | 1,841 lines |
| Vulkan shader wrappers | 48 lines |
| Focused diagnostic and registry tests | 406 lines |
| Opt-in CMake and option wiring | 240 additions |

The 465-line API adapter is the migration-cost signal for this pass. Most of
the remaining code handles capability checks, resource creation, validation,
readback, artifacts, and portability for a process that
does not borrow production state.

## Verification

- All seven generated SPIR-V modules passed Vulkan 1.1 validation, with no
  partially generated module left behind.
- Linux passed the 2-case Vulkan registry suite and the native RADV runner:
  24 valid cases, 8 preflight rejections, 24 submissions, and zero validation
  messages. Portability enumeration was available; portability subset was not
  advertised.
- macOS passed the same 2-case registry suite and the native arm64 MoltenVK
  runner with 24 valid cases, 8 preflight rejections, 24 submissions, zero
  validation messages, portability enumeration, and portability subset.
- The macOS Vulkan runner, comparator, and build-tree loader contain both
  `x86_64` and `arm64` slices. Their ad hoc signatures verify, and the runner
  resolves the loader through `@rpath`.
- The account-free Mac OpenGL exporter retained its 24 exact legacy-versus-
  contract comparisons and wrote the reference only after passing them.
- The default universal Mac viewer built and packaged with the option off. Its
  executable contains both architectures and no Vulkan or MoltenVK linkage.
- The default Linux graph contains no Vulkan target or cached Vulkan discovery.
  Its viewer build required only a build-local suppression for an existing GCC
  15 `-Warray-bounds` false positive outside the Stage 11 files.
- Linux and macOS each passed the 18 contract, 4 diagnostic, and 2 OpenGL
  registry cases. Both also passed the 2 Vulkan registry cases in their opt-in
  builds.
- All 57 Python benchmark tests passed. The checked-in benchmark manifest and
  schema-3 fixture also validated.
- GCC accepted the diagnostic, executor, runner, and comparator as C++20 with
  warnings treated as errors; the Vulkan runner additionally suppresses the
  aggregate `missing-field-initializers` warning for canonical `{sType}` Vulkan
  structure initialization. Whitespace validation passed, and an adversarial
  review found no remaining material issue.

## Dependency friction

No Vulkan Autobuild package or production package dependency was added. The
opt-in build takes explicit Vulkan header, loader, validation-layer,
`glslangValidator`, and `spirv-val` paths supplied by a Nix shell.
Rebuilding that target outside the declared shell cannot rely on the compiler's
cached implicit include environment; rebuilding inside it succeeds.

The universal Mac test target required combining separate arm64 and x86_64
Nix loader slices, correcting the dylib install name, adding a build-tree
runtime path, and signing the result. The available loaders target macOS 14
while the viewer still declares macOS 11, so the opt-in link emits a deployment
warning. This test-only loader is neither installed nor packaged with the
viewer. A maintained dependency package would be required before Vulkan could
enter ordinary builds.

The default Linux build also exposed X11's `None` and `Always` macros colliding
with contract enum members when the viewer's precompiled GL headers came first.
The contract now uses `Disabled` and `AlwaysPass` names. This avoids local
preprocessor exceptions and keeps the header safe in both API implementations.

Windows remains unclaimed. Stage 11 established no presentation, interop,
runtime selection, recovery, or performance result.

## Reanalysis

The tonemap slice now satisfies the master Stage 1 requirement for one
full-screen pass through both APIs. The next unmet slice is the indexed
material draw, which is materially different: it needs an index buffer, seven
vertex streams, three sampled textures with mip and anisotropy behavior, a
160-byte parameter block, three G-buffer color attachments, depth load and
write, culling, and indexed ranges.

The current material description is only a validation-test fixture with zeroed
bytes. Upstream's class 1 deferred material fragment source is also an explicit
constant-output debug stub, so it cannot serve as a reference. The class 3
source still implements the candidate non-rigged normal-plus-specular path and
samples the inputs represented by the packet. Stage 12 must first prove that
the target context selects that implementation, then freeze one asymmetric
fixture and replay it through a narrow OpenGL executor with account-free
readback. If class 3 is not reachable, the stage must stop rather than accept
the stub. Vulkan material execution, streaming uploads, presentation,
packaging, runtime selection, and performance work remain later stages.
