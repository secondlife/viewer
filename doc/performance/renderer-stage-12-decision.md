# Stage 12 indexed material OpenGL replay decision

## Decision

Accept the account-free OpenGL material replay as the reference half of the
second master Stage 1 slice. One immutable backend-neutral packet now drives a
direct legacy submission and a narrow contract executor against the existing
class 3 non-rigged normal-plus-specular shader. Their three G-buffer
attachments and depth target match exactly on Apple Silicon.

This is a correctness result, not a performance result. The production viewer
still renders through OpenGL. This stage adds no Vulkan material code,
streaming upload executor, presentation path, runtime backend selection, live
world input, account input, or timing.

## Frozen material trace

The target is `gDeferredMaterialProgram[12]`, reported by the viewer as
`Material Shader 12` at shader class 3. The executor rejects any other program,
variant, shader class, source pair, define set, feature set, attribute
interface, fragment-output mapping, sampler channel mapping, uniform mapping,
or reflected uniform-block layout. The selected program uses the existing
`deferred/materialV.glsl` and `deferred/materialF.glsl` sources with normal and
specular maps enabled. No material equation was copied and the class 1 debug
stub is not accepted.

Reflection also freezes the shader's single `ReflectionProbes` uniform block:
binding 0, 49,248 bytes, and exactly 12 active members. This block is not a
material packet input for the selected fixture, but rejecting a different
layout prevents a silently changed shader interface from becoming the oracle.

The frozen draw contains four vertices and six 16-bit indices. Its 304-byte
planar vertex allocation contains position, normal, three texture-coordinate
streams, color, and tangent with exact locations, component types, strides,
offsets, buffer bindings, and zero divisors. The index allocation is 12 bytes
and must contain `{0, 1, 2, 0, 2, 3}`.

Three distinct RGBA8 textures contain asymmetric 4 by 4, 2 by 2, and 1 by 1
mip levels. Sampling is linear with linear mip selection, repeat addressing,
and 8x anisotropy. The draw uses an 8 by 8 viewport and scissor, back-face
culling with counter-clockwise fronts, depth test and write with `LEQUAL`, fill
mode, and replacement blending. Dithering, multisampling, framebuffer sRGB,
stencil, logic operations, primitive restart, and unrelated raster features
are disabled for deterministic storage.

The three color targets are RGBA8, RGBA8, and RGBA16 UNORM. The contract calls
the depth target depth24 UNORM. Apple's OpenGL implementation reports the
requested depth storage as `GL_DEPTH_COMPONENT32`; the executor permits only
that known substitution or native depth24 and canonicalizes readback to
24-bit codes.

## Corrected parameter boundary

The planned 160-byte parameter placeholder was incomplete. The reachable
shader consumes four transforms, specular color, clip plane, environment
intensity, emissive state, and mirror state. The frozen backend-neutral block
is therefore 272 bytes, or 68 32-bit words, with compile-time size and offset
checks. Both builder and decoder require the complete block and reject
non-finite or out-of-policy values.

This is a correction to the Stage 12 estimate, not scope growth. Keeping the
160-byte placeholder would have hidden shader-visible mutable state outside
the packet and invalidated the migration seam.

## Fixture, artifact, and ownership

The shared diagnostic owns all vertex, index, texture, parameter, depth-load,
and color-sentinel bytes. The canonical builder copies those bytes into a
`FrameSnapshot`; the strict decoder returns owned values and accepts only this
draw shape. Tests mutate every represented family of state and verify that the
packet remains valid after caller storage is destroyed.

The version-1 artifact is 3,436 bytes. It records fixture identity, case
metadata, bottom-left row order, all three storage-canonical color planes, and
canonical depth24. All three tolerances are zero. Publication writes a unique
sibling temporary file and installs it atomically without replacing an
existing destination.

## OpenGL boundary and fail-closed behavior

The pass-specific registry borrows one synchronous run's viewer buffers,
textures, target, sampler description, and shader. It owns no GL object. The
executor resolves exact handle generations and validates the complete packet,
live buffer metadata and bytes, live texture formats/extents/mips, distinct GL
names, target attachments, sampler policy, shader identity and reflection, and
vertex-array state before binding or clearing the destination framebuffer.

The account-free harness allocates independent direct and contract resource
sets. Both paths strict-decode the same immutable `FrameSnapshot`. Before each
submission it poisons the other framebuffer, shader, texture and sampler
bindings, vertex and index bindings, attribute divisors, viewport, scissor,
color and depth masks, depth and cull state, polygon state, blend state, and
all four output sentinels. GL errors bracket both submissions.

Twenty-eight rejection cases cover stale generations for every resource
family, wrong program and variant, descriptor-format and extent claims, packet
layout and ranges, parameter size, sampler policy, and genuinely incompatible
live buffers, index topology, texture storage, mip chains, and color/depth
targets. Every rejected execution leaves the selected target's four sentinels
unchanged.

The diagnostic startup selector is parsed before viewer construction. It
matches the viewer's direct-flag and composing `--set` precedence, accepts
`Global.`-qualified controls, honors false and invalid boolean values, and
stops at `--`. A selected diagnostic without `SECONDLIFE_USER_DIR` fails before
profile initialization. Linux also creates and write-probes that isolated root
before construction, and the SDL cleanup callback tolerates this early exit.

## Fixed-input result

Native Apple OpenGL produced this result:

```text
MATERIAL_CONTRACT_PARITY result=pass case=nonrigged_normspec_indexed shader_index=12 shader_class=3 components=832 mismatches=0 max_abs_delta=0 depth_passes=5 depth_failures=4 mirror_clipped_passes=4 rejection_cases=28 rejection_failures=0 artifact=written
```

The 832 compared values are 256 components from each of three color planes and
64 depth values. Every color plane contains multiple written storage values.
The fixture also proves five depth passes, four depth failures, and four
mirror-clipped passing samples, preventing a clear-only, missing-draw, disabled
depth, or disabled clipping result from passing.

## Code size

| Code | Size |
| --- | ---: |
| Backend-neutral builder and decoder | 555 lines |
| Shared fixture, artifact, and comparison | 879 lines |
| OpenGL registry and executor | 1,034 lines |
| Account-free GL harness | 1,332 lines |
| Early diagnostic argument parser | 165 lines |
| New focused diagnostic, registry, and argument tests | 473 lines |

The 1,034-line OpenGL adapter is the migration-cost signal for this slice.
Much of it is deliberate live-object and shader reflection validation needed
to prove that the packet describes what the existing viewer actually submits.
It is not a general RHI and it is not routed into production draw pools.

## Verification

- Linux and macOS each passed 26 render-contract cases, four material artifact
  cases, two GL registry cases, and five startup argument cases.
- GCC accepted the final OpenGL executor and Linux startup translation unit as
  C++20 with the project's warnings treated as errors.
- The benchmark's 57 Python tests passed. Both edited settings XML files parsed
  successfully; the tests also validated all schema-3 manifests and the
  checked-in fixture.
- The default universal Release viewer built on macOS. Its executable contains
  both `x86_64` and `arm64` slices.
- Five native missing-isolation invocations, including qualified and attached
  `--set` forms, returned the intended material or tonemap failure marker before
  viewer construction.
- The native material replay passed with 832 exact values, nontrivial depth and
  clipping coverage, and 28 clean fail-closed cases. Its disposable profile,
  artifact, and log were removed immediately afterward.
- Whitespace validation passed. An adversarial source review was run throughout
  the stage.

A final whole-viewer Linux link is not claimed. The existing GCC 15 build tree
repeatedly terminated with compiler bus errors in unrelated common/filesystem
translation units during earlier clean attempts. The final Stage 12 Linux
files have warning-as-error syntax coverage and their 37 focused cases pass;
the default Mac build supplies the complete ordinary-build check. This
toolchain limitation should be retired with a stable Linux build environment,
but it does not change the packet or the native GL result.

The executor also trusts the protected FBO wiring of the internally-created
`LLRenderTarget` after validating its metadata and all seven attached or
sampled texture objects. The registry must gain direct attachment introspection
before it can safely borrow externally mutable targets. That is a reuse
constraint, not a gap in this lexical harness, whose targets are private to one
synchronous run.

## Reanalysis

The indexed material draw now has a deterministic OpenGL oracle. Master Stage
1 still requires its Vulkan executor, and the streaming upload slice remains
validation-only. The next committable stage is therefore a standalone Vulkan
replay of this exact packet and artifact.

Stage 13 should keep Vulkan opt-in and process-isolated. It should compile
wrappers around the existing class 3 material shader math, build Vulkan
buffers, images, sampler, three color attachments, and depth from the shared
fixture, and consume the same `FrameSnapshot`. A strict Vulkan registry must
reject packet and live-resource mismatches before command-buffer recording or
image mutation. Native Linux RADV and macOS MoltenVK runs must produce the
version-1 material artifact, compare it against the OpenGL reference, and
finish with clean validation. Ordinary viewer builds must remain Vulkan-free.

Streaming uploads, presentation, packaging, runtime selection, a whole-frame
Vulkan path, and performance measurement remain later stages.
