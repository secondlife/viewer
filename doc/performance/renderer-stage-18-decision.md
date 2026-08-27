# Stage 18 production material shader artifact decision

## Decision

Accept one separate production Vulkan material permutation for the existing
non-rigged opaque normal-plus-specular draw. The production vertex and fragment
modules compile and validate for Vulkan 1.1, their exact interfaces are derived
from the production manifest, and both production target profiles resolve the
same four-output shader contract.

This is the third committable slice of master Stage 2. It proves a production
shader artifact without creating a shader registry, native shader module,
pipeline, descriptor set, parameter buffer, render pass, command path, backend
selector, or performance result.

## Production permutation boundary

The two Vulkan wrappers retain their diagnostic defaults. A separate compiler
selector, `LL_VULKAN_MATERIAL_PRODUCTION=1`, activates these effective defines
in both stages:

- `HAS_EMISSIVE=1`
- `HAS_SUN_SHADOW=1`
- `SUN_SHADOW=1`
- `SPOT_SHADOW=1`

The selector also changes the fragment declaration from `frag_data[3]` to
`frag_data[4]`. Without the selector, the preprocessed diagnostic interface and
generated modules remain byte-identical to Stage 17 with the same compiler.
The production vertex module has its own artifact path even though its unused
additional defines produce the same bytes as the diagnostic vertex module.

The shared material equations were not changed. In opaque alpha mode,
`getShadow()` returns `1.0` and never calls the directional-shadow sampler.
Consequently the production wrapper records the viewer's sun-and-spot variant
identity but does not include `shadowUtil.glsl` and reflects no shadow uniforms
or descriptors. Shadow resources remain a later requirement for a blended
material variant that actually samples them.

## Fourth output semantics

The production fragment module declares four `vec4` outputs. The selected
legacy material path continues to place emissive brightness in output 0 alpha
and explicitly writes zero to output 3. Location 3 is nevertheless part of the
production pipeline interface, so the manifest names it `EmissiveBuffer` rather
than treating it as diagnostic baggage.

The shader interface is independent of attachment formats. Modern HDR and
Compatibility retain their distinct ordered target formats in their pipeline
keys while resolving equal owned Vulkan shader manifests:

| Target profile | Shader variant | Outputs | Location 3 target |
| --- | ---: | ---: | --- |
| Modern HDR | 5 | 4 | RGB16 float |
| Compatibility | 5 | 4 | RGB8 UNORM |

Both production OpenGL lookups remain unresolved. The complete linked OpenGL
recipe is setting-dependent and was not inferred from the standalone Vulkan
artifact.

## Exact production manifest

The production manifest retains the diagnostic Vulkan wrapper and shared-source
order, seven dense vertex inputs, eight interstage variables, flat `vary_sign`,
272-byte parameter block at set 0 binding 0, and three fragment samplers at set
1 bindings 0 through 2. It changes only the program to encoded variant 5, adds
the production compiler selector and expanded effective defines, and adds the
fourth logical and declared output.

Exact validation rejects reordered or missing defines, a diagnostic program
identity, source-order changes, an OpenGL backend label, changed descriptor
coordinates, three-output production descriptions, duplicate or unknown output
roles, output declaration changes, and mixed target-profile dimensions. The
generic validator still distinguishes structurally sound mutations from the
one canonical production recipe.

The logical parameter list remains 68 words. For the currently admitted World
domain, `mirror_flag` remains the fixed value zero. Reflection, cube, HUD,
rigged, alpha-blended, and mirror-capable material paths are still rejected by
the dormant adapter.

## Generated artifact and reflection evidence

The existing opt-in build now exposes two independent targets:

- `llvulkanmaterial_shaders` for the three-output diagnostic profile
- `llvulkanmaterialproduction_shaders` for the four-output production profile

A small CMake profile helper gives each pair separate SPIR-V, reflection,
disassembly, manifest, hash, and completion-stamp paths. Each target compiles
and runs `spirv-val` before reflection, runs the checker suite and strict real
interface verification, writes its own hash file only after those gates pass,
and touches its own stamp last.

With glslang 16.4, the clean build produced these build-local identities and
module sizes:

| Profile and stage | SHA-256 | Bytes |
| --- | --- | ---: |
| Diagnostic vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` | 8,068 |
| Diagnostic fragment | `e653afd5a5d541888cba40806c6b6f922db20ec2eb9414661893d8e29521f6ba` | 7,776 |
| Production vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` | 8,068 |
| Production fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` | 7,824 |

Compiler versions are not pinned, so these hashes are evidence for this build
and are not source-controlled constants. Real `spirv-cross` reflection reported
a literal array size of 3 for diagnostic output and 4 for production output.
Applying the production expectation to the diagnostic reflection failed on the
missing location 3.

## Hardened reflection gate

The manifest dumper now requires an explicit `diagnostic` or `production`
profile and validates the selected canonical manifest before opening its output.
The same strict checker serves both profiles.

Adversarial review found that reflected I/O array expansion initially ignored
`array_size_is_literal`. The checker now requires literal metadata to be a
same-length boolean array whose entries are all true. Missing, malformed,
length-mismatched, and non-literal metadata fail closed. The focused checker
suite now has 12 passing cases, including the diagnostic-to-production output
mismatch. A second full review found no remaining issue.

## Default-build isolation

All shader compilation and reflection wiring remains below the existing
default-off Vulkan diagnostic option. With the option restored to off, the
Linux build graph contains neither material Vulkan target nor the manifest
dumper. The Release viewer relinked without a Vulkan or MoltenVK dynamic
dependency. The normal material draw pools, OpenGL shader manager, bindings,
and submission path are unchanged, and the adapter still has no call site.

A disposable macOS snapshot contained the Stage 17 commit plus exactly the nine
reviewed Stage 18 source changes. On macOS 26.6.2 with Xcode 26.6 and SDK 26.5,
the fresh ReleaseOS build used `arm64;x86_64`, tests enabled, renderer benchmark
instrumentation disabled, Vulkan diagnostics disabled, packaging enabled, and
signing and crash reporting disabled. All 166 build targets and the `llpackage`
target passed. The local package route assembled the universal app and base
package and wrote the standard universal-DMG package marker; it did not produce
a standalone DMG file.

The viewer contained both architecture slices. The focused test executables
were native arm64. Direct viewer and focused-test linkage scans had no Vulkan or
MoltenVK hit. A bundle-wide scan read all 367 Mach-O files successfully and
found no Vulkan or MoltenVK dependency or named path. The ordinary Mac checkout
and installed viewer were not used or modified. The 32 GB disposable source,
dependency, build, package, and Python tree was removed after verification.

No viewer, graphics context, device, account, region, asset, pixel comparison,
benchmark, or timing path ran on either platform.

## Code size

| Stage 18 code, excluding this decision record | Additions | Removals |
| --- | ---: | ---: |
| Profiled shader wrappers and CMake artifact helper | 149 | 126 |
| Production manifest schema and tests | 176 | 6 |
| Profile dumper, reflection checker, and checker tests | 152 | 28 |
| Total | 477 | 160 |

The CMake removal count is principally the prior single-profile commands moved
into one profile helper. Production does not duplicate the verifier or shader
source.

## Verification

- Clean diagnostic and production shader targets compiled, passed Vulkan 1.1
  validation, reflected, disassembled, verified, hashed, and stamped
  independently.
- Diagnostic module hashes match Stage 17 exactly. Production reflection has
  four outputs and no additional descriptors.
- The draw-packet, render-contract, shader-manifest, and viewer-adapter suites
  passed 7, 28, 7, and 7 cases respectively on Linux and macOS.
- All 12 reflection-checker cases and all 57 benchmark-harness Python tests
  passed. Settings and command-line XML parsed successfully.
- Clang-format dry-run, Python parsing, `git diff --check`, credential and local
  path scans, neutral dependency scans, direct graphics-API scans, default
  target scans, and Linux and macOS linkage scans passed.
- Generated shader artifacts and review probes were removed. The Vulkan option
  is off again, and the worktree contains only the Stage 18 change plus the two
  pre-existing unrelated documents.

## Explicit runtime gaps

Stage 18 does not install or load SPIR-V, create a native shader module or
pipeline, choose target formats from device capabilities, allocate descriptors
or parameter storage, capture frame/pass state, resolve live viewer resources,
record commands, submit work, present pixels, compare parity, or measure
performance. A reflected shader is still not an executable production draw.

## Reanalysis

The production shader now has a truthful interface, but the owned Stage 16 draw
packet still cannot populate its 272-byte parameter block. It owns the draw
model and texture matrices plus specular, environment, alpha, and emissive
values. Modelview, modelview-projection, inverse-transpose normal, and clip-plane
values must be derived from explicit frame/pass state. The existing OpenGL path
currently obtains those values through mutable matrix and environment globals.

The next smallest non-speculative boundary is therefore a pure production
parameter materializer. It should accept a valid production draw packet and a
value-owned World pass context with matching frame/pass identities, base
modelview, projection, and eye-space clip plane. It should return owned
`MaterialParameters`, derive matrices in legacy order, copy draw values, force
mirror to zero for the current World-only scope, and fail closed for diagnostic,
mismatched, nonfinite, singular, or overflow-derived inputs.

A generation-aware shader registry should not precede this materializer. Stage
18 artifacts exist only in the opt-in build tree and are neither installed nor
loaded, so a registry today would publish injected test metadata without a real
producer or consumer. Native objects, artifact installation, registries,
resource pinning, pipeline creation, adapter call sites, and submission remain
later stages.
