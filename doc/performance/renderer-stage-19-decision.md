# Stage 19 production material parameter decision

## Decision

Accept a pure backend-neutral materializer for the existing non-rigged opaque
World normal-plus-specular draw. A valid production draw packet and an explicit
matching frame/pass context now produce the complete owned 272-byte parameter
block required by the Stage 18 production Vulkan shader.

This is the fourth committable slice of master Stage 2. It closes the parameter
ownership gap without installing or loading SPIR-V, publishing a shader,
creating a native object, changing a viewer call site, or executing a draw.

## Public boundary

`LegacyNormSpecWorldParameterContext` owns:

- frame and pass identities;
- the base modelview matrix;
- the projection matrix; and
- the eye-space clip plane.

`materializeLegacyNormSpecWorldParameters()` accepts only a validator-clean
Modern HDR or Compatibility production packet whose nonzero frame and pass
exactly match the context. Diagnostic packets, malformed packets, mismatched
identities, and nonfinite inputs fail closed. The function owns its result and
reads no renderer, matrix-stack, environment, OpenGL, or Vulkan global.

The function maps the selected legacy path as follows:

| Parameter | Source or derivation |
| --- | --- |
| Modelview | `base_modelview * draw_model` |
| Modelview-projection | `projection * modelview` |
| Normal | inverse-transpose of the modelview linear 3 by 3 block |
| Texture matrix 0 | draw diffuse texture matrix |
| Specular color | draw specular RGBA |
| Clip plane | context eye-space clip plane |
| Environment intensity | draw environment intensity |
| Emissive brightness | draw emissive value |
| Mirror flag | fixed zero for the admitted World domain |

Alpha cutoff is deliberately absent. The admitted opaque shader variant does
not consume it. Modern HDR and Compatibility therefore return equal parameter
blocks for otherwise equal draws.

Matrices use the viewer's contiguous column-major upload convention. The
composed modelview must be affine; projection remains free to be projective.
This matches the current World draw path and makes its boundary explicit.
Reflection, mirrors, rigging, alpha masking, alpha blending, and other material
domains remain rejected before this function.

## Numerical fail-closed policy

The normal matrix is calculated without GLM so `llrendercontract` remains an
independently linkable standard-library target. The implementation performs a
scaled double-precision inversion of the affine linear block, converts only
finite in-range values to the float parameter packet, and verifies both matrix
products after conversion.

A normalized-row determinant gate rejects a numerically rank-deficient block.
The returned float inverse may have at most `1/1024` absolute two-sided
residual. Any singular 3 by 3 product differs from identity by at least `1/3`
in one entry, so this bound is separated from the singular boundary while
allowing normal float inversion error.

Adversarial review found that the first float Gauss-Jordan version accepted an
exactly dependent matrix when cancellation left a small nonzero pivot. A
simple scaled-pivot threshold closed that bug but incorrectly coupled a row's
translation magnitude to its linear rank. A full-matrix residual likewise
rejected valid translated objects because inverse translation is rounded even
though the shader consumes only the normal block. The final affine linear-block
policy removes that unrelated state.

The frozen implementation rejected both original counterexamples and 98,766
additional exact row-dependent integer affine matrices with no false accept.
It accepted all 100,000 deterministic valid rotation, nonuniform-scale, and
translation fixtures with scales from 0.01 through 64 and translations through
plus or minus one million. Their maximum observed inverse residual was
`1.64552e-4`. A `1e-20` well-scaled axis and a one-ULP near-dependent but
invertible fixture also remained accepted.

## Focused contract tests

Eight deterministic cases cover:

- identity derivation, every copied field, final validation, and all 68 word
  offsets;
- noncommuting modelview and projection multiplication order;
- rotated nonuniform scale and inverse-transpose orientation;
- independent alpha-cutoff omission and production-profile equality;
- owned return values after every source object is mutated;
- diagnostic and malformed packet rejection;
- zero and mismatched frame/pass identities;
- nonfinite context, exact singularity, non-affine modelview, finite product
  overflow, small scale, and large rotated translation.

The approximate comparison helper rejects nonfinite operands. This prevents a
NaN from satisfying a tolerance assertion.

## Build and backend isolation

The new source is part of `llrendercontract`, and the focused integration test
links only that neutral library and `llcommon`. Source and linkage scans found
no OpenGL, Vulkan, MoltenVK, GLM, window, or viewer dependency in the contract.

On Linux, the Release viewer and all six affected targets rebuilt with project
warnings treated as errors. The material-parameter, draw-packet,
material-diagnostic, render-contract, shader-manifest, and viewer-adapter
suites passed 8, 7, 4, 28, 7, and 7 cases. The focused neutral executables had
no graphics API linkage. With `LL_VULKAN_TONEMAP_TEST=OFF`, neither material
shader target nor the manifest dumper existed in the default graph, and the
viewer had no Vulkan or MoltenVK dependency.

A disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5 contained
the Stage 18 commit plus exactly the four reviewed Stage 19 source files. Its
fresh ReleaseOS build used `arm64;x86_64`, tests enabled, packaging enabled,
renderer benchmark instrumentation disabled, Vulkan diagnostics disabled, and
signing and crash reporting disabled. All 167 build targets and the explicit
`llpackage` target passed. The package route assembled the universal app and
base package and wrote the standard universal-DMG marker; it did not leave a
standalone DMG file.

The app executable contained both architecture slices. The six focused test
executables were native arm64, and all six runners passed. A post-package scan
read all 367 Mach-O files in the bundle and found no Vulkan or MoltenVK
dependency or named path. Source hashes on the Mac matched the frozen local
files. The ordinary Mac checkout and installed viewer were not used or
modified. The 32 GB disposable source, dependency, build, package, and Python
tree was removed after verification.

All 12 reflection-checker cases and all 57 benchmark-harness Python tests also
passed. Settings and command-line XML parsed successfully. Clang-format,
whitespace, local-path, credential, backend-dependency, and default-graph
checks passed.

No viewer, graphics context, GPU, account, region, world asset, pixel,
benchmark, or timing path ran on either platform.

## Code size

| Stage 19 code, excluding this decision record | Lines added |
| --- | ---: |
| Public contract and implementation | 288 |
| Focused integration tests | 356 |
| CMake wiring | 4 |
| Total | 648 |

The inversion code is longer than a GLM call because this library deliberately
has no renderer math dependency and because singularity must fail closed rather
than propagate invalid shader values.

## Explicit runtime gaps

Stage 19 does not deliver or open a shader artifact, create a Vulkan shader
module or pipeline, choose attachment formats from a device, allocate parameter
or descriptor storage, capture live viewer state, resolve resources, publish a
generation, record or submit commands, present pixels, compare parity, or
measure performance. The inactive World clip plane must also be canonicalized
to a finite value at a later viewer capture boundary because the current
OpenGL global can contain an inert nonfinite mirror plane when mirror mode is
off.

## Reanalysis

The production shader and its complete parameter block now exist, but the
verified SPIR-V remains in an opt-in build directory. A registry still has no
truthful producer: no packaged bytes reach the viewer, and the neutral contract
has no shader publication generation or retirement lifecycle.

The next smallest non-speculative stage is verified production artifact
delivery plus an owned-byte loader. The verified production vertex and
fragment modules should be copied explicitly to
`app_settings/shaders/vulkan/legacy_normspec`, with packaging depending on the
complete Stage 18 validation stamp. A Vulkan-free bounded loader should accept
an explicit app-settings root, own the SPIR-V words, validate the header and
instruction stream, require the expected `main` entry point and stage, and tie
the result to the canonical production manifest.

That stage should remain opt-in and should deliver only the two production
modules. Diagnostic modules, reflection JSON, disassembly, hash evidence,
validation tools, Vulkan loaders, native objects, registries, viewer directory
globals, startup integration, backend selection, and GPU work remain later
stages. Once verified owned bytes exist at the runtime boundary, the following
stage can define generation-aware publication and retirement without inventing
test-only shader metadata.
