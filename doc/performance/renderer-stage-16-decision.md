# Stage 16 production material draw snapshot decision

## Decision

Accept the dormant production translation boundary for one ordinary-world,
non-rigged, opaque `PASS_NORMSPEC` draw. The adapter snapshots a real
`LLDrawInfo` into an owned, backend-neutral packet with generation-tagged
resource handles and a complete logical pipeline key. It is compiled into the
normal viewer but has no call site, so the existing OpenGL shader selection,
binding order, and indexed draw remain unchanged.

This is the first committable slice of master Stage 2. It proves that prepared
renderer work can outlive the mutable viewer objects from which it was built.
It does not provide a production resource registry, pin resource lifetime,
compile a shader variant, submit a draw, select Vulkan, or make a performance
claim.

## Stage 17 profile erratum

Post-commit shader inventory found that variant 0 and its three color targets
describe the Stage 12 parity diagnostic, where emissive output and shadow
assembly are disabled. They are not the viewer's normal production defaults.
Production enables a fourth emissive target and uses a distinct shader variant
and one of two platform target profiles. Stage 16 has no adapter call site, so
the mistaken label had no runtime effect. Stage 17 corrects the key taxonomy
and retains this section as a historical erratum rather than rewriting the
Stage 16 implementation result.

## Supported production source

Capture receives the external render-map type, one `LLDrawInfo`, explicit
frame and pass identities, an ordinary-world render domain, a deferred-material
submission kind, a canonical logical pipeline key, and an injected resolver.
Only `PASS_NORMSPEC` is accepted.

| Source or context | Packet mapping or policy |
| --- | --- |
| `mVertexBuffer` | Resolve distinct vertex and index handles, exact live sizes and counts, and UInt16 or UInt32 index type |
| `mStart`, `mEnd` | Copy as inclusive minimum and maximum vertex indices |
| `mOffset`, `mCount` | Copy in index-element units as first index and index count |
| Diffuse, normal, and specular textures | Resolve three image, sampler, and subresource records by role |
| `mModelMatrix` | Copy all 16 values; null becomes identity |
| `mTextureMatrix` | Copy all 16 diffuse texture-transform values; null becomes identity |
| `mSpecColor` | Copy RGBA/specular-exponent values |
| `mEnvIntensity`, `mAlphaMaskCutoff` | Copy validated unit-range constants |
| `mFullbright` | Normalize to emissive brightness 0 or 1 |
| `mTextureList` | Allow only null entries or the same diffuse source; distinct texture-array inputs fail |
| Render context | Require ordinary world rendering; HUD, impostor, reflection, cube, and invalid domains fail |

The source must carry the exact position, normal, diffuse UV, normal UV,
specular UV, color, and tangent vertex attributes. The adapter rejects a
missing legacy material, GLTF material state, avatar or skin state, glow,
distinct texture-array batching, and non-null normal-map or specular-map
matrices. Those two auxiliary matrices have no assignment or consumption site
in the selected production path, so the adapter does not invent semantics for
them.

External `PASS_NORMSPEC` identity is the opaque-mode authority. The adapter
does not copy `mShaderMask`, pointer-derived identity, an OpenGL cache hash, or
a shader object into the packet.

## Owned packet and logical key

The neutral packet lives under `llrender` and contains only values. It owns
frame and pass identities, distinct buffer and pipeline handles, three sampled
image records, exact indexed ranges, copied matrices and constants, index type,
and the complete pipeline key. It retains no viewer pointer, `LLPointer`,
reference, callback, span, GL name, Vulkan handle, or mutable shared storage.

The canonical key names existing program semantics as
`deferred.material.normspec`, variant 0. It fixes the legacy material vertex
layout, triangle-list topology, back-face culling, counter-clockwise front
faces, depth test and write with less-or-equal comparison, one sample, three
unblended full-write targets in RGBA8, RGBA8, and RGBA16 order, and Depth24.
Structural equality and a pure validator cover every represented field.

Vertex and index handles must be distinct. The packet has no binding-base
offsets with which one logical buffer could truthfully describe two different
stream layouts. Images may intentionally be reused across descriptor roles;
when one source texture fills more than one role, every resolution must return
the same image identity while role-specific samplers and ranges may differ.

## Resolver and lifetime boundary

The newview resolver interface is deliberately injected and side-effect-free
from the adapter's perspective. A successful lookup promises the source's
current live generation and validated immutable metadata. Missing, retired,
stale, undersized, malformed, or policy-incompatible records return no value.
All source and resource resolution completes before a packet can be returned.

The deterministic fake resolver distinguishes current records from candidate
records and models explicit retirement. Focused cases reject nonzero stale
buffer, image, sampler, and pipeline generations as well as zero generations,
missing resolutions, retired records, undersized storage, invalid subresource
ranges, and inconsistent same-source image identities.

The packet copies handle values but does not pin the resources they name.
Keeping a generation alive through submission, advancing generations only
after completion, and deferring destruction are responsibilities of the later
production registry and fence service. Stage 16 proves snapshot ownership, not
registry lifetime.

## `LLDrawInfo` extraction

The constructor, range validation, debug color, and skin-hash methods moved
byte-equivalently from `llspatialpartition.cpp` into a focused
`lldrawinfo.cpp` translation unit so the adapter test does not link the whole
spatial pipeline. The production destructor remains in
`llspatialpartition.cpp` with its original `gDebugGL` reference check. Only
the focused test target supplies a default destructor shim, so normal viewer
behavior is preserved even if that debug callback becomes active later.

## Ownership and rejection evidence

The canonical case checks exact frame, pass, ranges, typed handles, descriptor
roles, matrix storage order, scalar values, full pipeline key, and deterministic
repeat translation. Identity matrices are covered separately.

After capture, the ownership case advances every fake buffer, image, sampler,
and pipeline generation; mutates ranges, constants, brightness, and both
pointer-backed matrices; then releases the draw, material, textures, and
vertex buffer. The saved packet remains equal to its original value and still
passes neutral validation.

The compact rejection matrix also covers wrong pass, wrong submission kind,
special render domains, missing resources and material, GLTF, avatar and skin
state, glow, unsupported auxiliary matrices, distinct texture batching,
missing vertex attributes, inverted and out-of-bounds draw ranges, per-stream
aliasing, 16-byte attribute padding, UInt16 and UInt32 index storage,
non-finite or out-of-range constants, invalid image ranges, and shared-source
identity conflicts.

## Production isolation

There is no adapter call site. The material draw pool, global pipeline,
OpenGL bindings, and submission code were not edited. The default Linux
Release viewer relinked with project warnings treated as errors, and its
executable has no Vulkan or MoltenVK dynamic dependency. The default build
graph still has the existing opt-in Vulkan diagnostic switch disabled.

The focused adapter test creates CPU-side viewer objects and opens no GL
context. A target-private texture implementation and destructor shim avoid
GPU allocation and spatial-pipeline linkage. The neutral packet files have no
newview, OpenGL, or Vulkan dependency, and the production adapter adds no
direct GL call.

A bounded macOS SSH attempt timed out on port 22 before authentication. No
source sync, build, test, or machine change occurred. This is an infrastructure
block, not a C++ portability failure; no universal macOS build is claimed for
Stage 16.

No account, live region, asset, benchmark, GPU artifact, or timing was used.

## Code size

| Code | Size |
| --- | ---: |
| Neutral packet, key, builder, and validator | 318 lines |
| Production `LLDrawInfo` implementation and adapter | 350 lines |
| Focused neutral and adapter tests plus test shim | 883 lines |

The adapter remains narrow. The resolver is an interface rather than a hidden
partial registry, and the packet does not absorb shader assembly, allocation,
descriptor, command, or synchronization services.

## Verification

- The default Linux Release viewer relinked successfully with warnings treated
  as errors.
- The draw-packet contract passed 4 of 4 cases, the adapter passed 7 of 7, and
  the existing render contract passed 27 of 27.
- All 57 Python benchmark-harness tests passed. Settings and command-line XML
  parsed successfully.
- Clang-format dry-run, `git diff --check`, dependency scans, direct-GL scans,
  and the executable linkage check passed.
- An independent adversarial review ended with no remaining correctness,
  ownership, range, layering, CMake, or production-behavior findings.

## Reanalysis

The source-to-packet ownership boundary is now stable, but the logical
`deferred.material.normspec` key still has no production description of how
source modules, feature defines, descriptor bindings, vertex locations, and
parameter layout form that program. Implementing a production registry first
would force it to resolve a pipeline whose shader identity and binding
contract remain implicit.

Stage 17 should therefore add one backend-neutral shader manifest for the
canonical non-rigged opaque normal-and-specular variant. It should map the
logical key to the existing class-3 material shader assembly, ordered feature
fragments and defines, explicit descriptor and vertex locations, the consumed
parameter layout, and the already validated standalone Vulkan module
identities. A strict validator should reject reordered, missing, duplicate,
or backend-incompatible declarations.

Stage 17 should remain dormant and production-compiled. It must not create the
resource registry, compile shaders at viewer runtime, add a Vulkan dependency
to the default executable, route a draw, or change the existing OpenGL shader
manager. Once the manifest is canonical and tested, the following stage can
implement the first production registry against an explicit pipeline contract
instead of reverse-engineering implicit shader state during lookup.
