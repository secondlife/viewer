# Stage 17 material shader profile and manifest decision

## Decision

Accept the corrected material profile taxonomy and the backend-neutral manifest
for the existing three-output diagnostic shader. The production draw adapter
now accepts either of the viewer's four-target production profiles. The
diagnostic OpenGL and Vulkan shader recipes are exact and reflectable, but both
production profiles deliberately resolve no shader artifact in this stage.

This is the second committable slice of master Stage 2. It corrects a label
before it can become runtime behavior and makes the proven shader interface
machine-checkable. It does not create a production shader, registry, parameter
materializer, Vulkan pipeline, command path, backend selector, or performance
result.

## Corrected profile boundary

Stage 16 used variant 0 and three color targets as its canonical key. Source
inventory showed that this is the Stage 12 parity diagnostic, where emissive
output and shadow assembly were intentionally disabled. It is not the normal
viewer configuration. The Stage 16 adapter has no call site, so the error had
no runtime effect. Its decision record now carries an explicit erratum rather
than rewriting the committed history.

The typed shader variant uses one emissive bit and a two-bit shadow-assembly
field. Decoding rejects unknown bits and enum values. Pipeline validation then
accepts only these exact combinations:

| Profile | Encoded variant | Emissive | Shadow assembly | Ordered color targets | Depth |
| --- | ---: | --- | --- | --- | --- |
| Diagnostic | 0 | Disabled | Disabled | RGBA8, RGBA8, RGBA16 UNORM | Depth24 UNORM |
| Modern HDR | 5 | Enabled | Sun and spot | RGBA8, RGBA8, RGBA16 UNORM, RGB16 float | Depth24 UNORM |
| Compatibility | 5 | Enabled | Sun and spot | RGBA8, RGBA8, RGB10A2 UNORM, RGB8 UNORM | Depth24 UNORM |

Production defaults come from `RenderEnableEmissiveBuffer=1` and
`RenderShadowDetail=2`. The modern or compatibility target family is selected
by HDR enablement and the OpenGL capability check. `RGB16Float` was appended to
the neutral pixel-format enum so existing serialized enum values did not move.

The normal viewer adapter requires a production key from its injected pipeline
resolver. It accepts the two target families above and rejects the diagnostic
key even if a resolver offers it. The neutral packet validator still recognizes
the diagnostic key because the standalone parity evidence remains valid.

## Diagnostic OpenGL manifest

The OpenGL manifest records the class 1 material vertex source, class 3
material fragment source, and the 18 feature objects in their exact attachment
order and resolved shader classes. The proven diagnostic uses class 1 feature
objects except class 3 `reflectionProbeF.glsl`; its disabled SSR setting
resolves `screenSpaceReflUtil.glsl` at class 1. Its permutations are
`DIFFUSE_ALPHA_MODE=0`, `HAS_NORMAL_MAP=1`, and `HAS_SPECULAR_MAP=1`. It
contains no emissive or shadow permutation.

The manifest records the viewer's sparse legacy vertex locations for position,
normal, three texture coordinates, color, and tangent; the three material
sampler channels; and three logical G-buffer outputs. The linked fragment
declaration remains `frag_data[4]`, with its fourth element explicitly marked
inert for this diagnostic. It also records the linked `ReflectionProbes`
uniform-block baggage at binding 0, 49,248 bytes, and its 12 active members.
That block is not a packet input, but a different linked interface cannot be
silently treated as the proven shader.

## Diagnostic Vulkan manifest

The Vulkan recipe records the two Stage 13 wrappers and their three shared
shader includes in compile order. Vertex locations are dense from 0 through 6.
Eight interstage values occupy locations 0 through 7, with `vary_sign` flat at
location 2. The same 272-byte parameter block is visible to both stages at set
0, binding 0. Diffuse, normal, and specular combined-image samplers occupy set
1, bindings 0 through 2. The fragment module declares exactly three outputs.
There are no push constants, linked OpenGL blocks, shadow descriptors, or
unlisted resource categories.

The generic validator checks owned strings and arrays, known enums, unique
names, semantics, roles, locations, bindings, stage visibility, contiguous
parameter words, bounded output declarations, collision-free descriptor
coordinates, and non-overlapping push-constant ranges. Exact diagnostic
validation additionally compares all ordered values with the canonical recipe.
Reordering a source or define can therefore remain structurally valid while
failing canonical identity.

Production Modern HDR and Compatibility keys return no manifest for either
backend. This prevents the three-output artifact from acquiring a production
label through fallback behavior.

## Logical parameter provenance

The manifest describes all 68 words consumed by the existing standalone
Vulkan material block. It does not claim that the dormant production adapter
already materializes them.

| Words | Shader name | Provenance |
| --- | --- | --- |
| 0 to 15 | `modelview_matrix` | Derived from draw and frame state |
| 16 to 31 | `modelview_projection_matrix` | Derived from draw and frame state |
| 32 to 40 | `normal_matrix` | Derived from draw and frame state |
| 41 to 56 | `texture_matrix0` | Copied draw state |
| 57 to 60 | `specular_color` | Copied draw state |
| 61 to 64 | `clipPlane` | Frame or pass state |
| 65 | `env_intensity` | Copied draw state |
| 66 | `emissive_brightness` | Copied draw state |
| 67 | `mirror_flag` | Fixed diagnostic default 0 |

Alpha cutoff is absent because opaque alpha mode does not consume it. Future
production materialization must derive the three matrix families and clip
state before submission rather than hiding them behind mutable globals.

## Generated reflection evidence

The existing opt-in Vulkan build now compiles and validates both Stage 13
material modules before reflecting them with `spirv-cross` and disassembling
them with `spirv-dis`. A small manifest-linked executable emits the expected
interface as deterministic JSON. A standard-library-only checker then verifies
entry points, vertex and interstage I/O, descriptor names and cardinality,
set/binding coordinates, block size and stage union, fragment outputs,
resource-category absence, push-constant absence, and the exact flat
decorations. Hashes are written only after this gate passes, and the stamp is
written last.

The focused checker test proves one valid fixture and rejects swapped stages,
descriptor arrays, unexpected push constants, missing flat decorations,
unexpected smooth-interface qualifiers, and interpolation decorations on
unlocated data. Swapping the real vertex and fragment reflection also failed
on their entry point modes.

The fresh Linux build produced these build-local module identities with
glslang 16.4:

```text
vertex   b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095
fragment e653afd5a5d541888cba40806c6b6f922db20ec2eb9414661893d8e29521f6ba
```

Compiler versions are not pinned, so these hashes are evidence for this build,
not source-controlled canonical constants. Generated modules, reflection,
disassembly, manifests, hashes, and stamps are removed after verification.

## Production isolation

Shader reflection, disassembly, and dumping exist only below the default-off
`LL_VULKAN_TONEMAP_TEST` switch. With that switch off, the default graph has no
material shader or manifest-dump target. The Linux Release viewer relinked and
has no Vulkan or MoltenVK dynamic dependency.

A disposable macOS 26.6 Apple Silicon snapshot then completed the universal
`arm64`/`x86_64` ReleaseOS build with tests enabled and both Vulkan and renderer
benchmark options disabled. The final changed source bytes were overlaid and
the default build repeated before the four focused runners passed. The viewer,
all focused tests, and all 367 Mach-O files in the bundle had no Vulkan or
MoltenVK linkage; the bundle also contained no file named for either loader.
The disposable source, build, package, and Python environment were removed.

Read-only preflight and final checks observed that another operation
fast-forwarded the ordinary Mac checkout from its previous Stage 10-era commit
to the shared Stage 16 commit during this lane. The disposable build did not use
that checkout as a configure or build directory and did not alter or revert the
external fast-forward.

There is still no adapter call site. The material draw pools, OpenGL shader
manager, bindings, and submission code are unchanged. The neutral manifest has
no newview, OpenGL, or Vulkan type dependency, and the adapter adds no direct
OpenGL call. No account, region, asset, benchmark, GPU submission, or timing was
used.

## Code size

| Stage 17 code | Size |
| --- | ---: |
| Profile correction and existing focused-test updates | 353 additions, 54 removals |
| Neutral manifest schema and canonical recipes | 834 lines |
| Manifest and reflection-checker tests | 488 lines |
| Manifest dump, reflection verifier, and hash writer | 899 lines |
| Opt-in CMake wiring | 106 additions, 1 removal |

The schema is intentionally value-owned and backend-neutral. The diagnostic
tooling is larger because it compares named reflection rather than trusting a
module hash, but it remains outside the default build graph and is reused by
the next production artifact.

## Verification

- The default Linux Release viewer relinked with the Vulkan option off and
  project warnings treated as errors.
- The default universal macOS ReleaseOS viewer built and packaged after the
  final source overlay. Its four focused CTest runners passed, both architecture
  slices were present, and bundle-wide linkage isolation passed.
- The draw-packet, render-contract, shader-manifest, and viewer-adapter suites
  passed 7, 28, 5, and 7 cases respectively.
- A fresh opt-in build recompiled and SPIR-V-validated both diagnostic modules,
  ran all seven reflection-checker cases, validated the real reflected interface,
  and wrote hashes only after success.
- All 57 benchmark-harness Python tests passed. Settings and command-line XML
  parsed successfully.
- Clang-format dry-run, `git diff --check`, neutral dependency scans, direct-GL
  scans, default-target scans, and executable linkage checks passed.

## Reanalysis

The diagnostic shader identity is now explicit and the production target
families can no longer alias it. Production lookup remains intentionally empty,
so a registry would still have nothing truthful to resolve. Source inspection
shows a smaller dependent boundary first: compile and reflect a distinct
four-output Vulkan artifact for opaque non-rigged normal-plus-specular draws.

Stage 18 should preserve the three-output diagnostic bytes, make the wrapper's
fragment declaration four elements only for the production permutation, and
compile production variant 5 with emissive plus sun-and-spot identity. The
opaque path does not sample shadows, so it should expose no invented shadow
descriptors. Both Modern HDR and Compatibility pipeline keys may resolve the
same Vulkan shader interface because target formats remain pipeline state.

Stage 18 must extend exact manifest and reflection coverage for the fourth
output and retain build-local hashes. It must not create a Vulkan registry,
render pass, pipeline, command submission, target-format capability policy,
runtime backend route, pixel-parity claim, or benchmark. Those remain later
stages after the production artifact itself is proven.
