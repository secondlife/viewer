# Stage 13 offscreen Vulkan material replay decision

## Decision

Keep the standalone Vulkan material replay as a migration probe, but mark the
Stage 13 parity gate blocked. Both native Vulkan implementations execute the
unchanged Stage 12 packet correctly, pass every internal coverage and
fail-closed gate, and finish with no validation messages. Neither produces an
artifact that is bit-exact with Apple OpenGL under the frozen zero-tolerance
policy.

The result does not justify changing the packet, adding a driver-specific LOD
bias, or widening the tolerances after observing output. Vulkan remains
test-only and the production viewer remains OpenGL. Master Stage 1 continues
with its independent streaming-upload slice; the material mismatch stays a
known input to later parity-policy design.

## Frozen mapping

The Vulkan diagnostic consumes the unchanged Stage 12 material
`FrameSnapshot`, fixture fingerprint, and schema-1 artifact. Backend-specific
mapping is limited to API conventions and live Vulkan objects:

| Contract field | Vulkan mapping |
| --- | --- |
| Seven planar vertex streams | Bind the same 304-byte allocation seven times at packet offsets, with explicit locations 0 through 6 and the packet formats and strides. |
| Six 16-bit indices | Validate the canonical `{0, 1, 2, 0, 2, 3}` bytes, then bind `{2, 0, 1, 3, 0, 2}`. Each cyclic permutation preserves geometry, winding, and interpolation while making Vulkan's first provoking vertex equal OpenGL's last provoking vertex for the flat tangent sign. |
| Vertex clip position | Run the existing `materialV.glsl` body, then map OpenGL clip depth with `z = (z + w) * 0.5`. A negative-height viewport preserves the packet's bottom-left convention. |
| Cull and front face | Back-face culling remains enabled. With the negative-height viewport, Vulkan's signed-area definition still classifies the contract triangles as counter-clockwise, so the Vulkan pipeline keeps `VK_FRONT_FACE_COUNTER_CLOCKWISE`. The cyclic index rotation preserves that winding. |
| 272-byte parameter packet | Bind the unchanged 68 words as one `std140` array of 17 `vec4` values at descriptor set 0, binding 0. Declaration-wrapper accessors reconstruct the existing shader uniforms without repacking or copying material equations. |
| Three sampled mip chains | `VK_FORMAT_R8G8B8A8_UNORM`, three levels, combined image samplers at set 1 bindings 0 through 2, linear minification/magnification/mip filtering, repeat addressing, and 8x anisotropy. |
| Three color targets | Two `VK_FORMAT_R8G8B8A8_UNORM` attachments and one `VK_FORMAT_R16G16B16A16_UNORM` attachment, cleared to zero, stored, and left shader-readable. |
| Depth24 target | The sole allowed Vulkan storage substitution is `VK_FORMAT_D32_SFLOAT`. It must support depth attachment and transfer source/destination usage. Loaded and read values are canonicalized to the nearest depth24 storage code. No other depth format fallback is permitted. |
| Indexed draw | One triangle-list `vkCmdDrawIndexed` with six indices, one instance, `LEQUAL` depth testing and writes, fill mode, replacement color writes, and one sample. |
| Lifetime and synchronization | The standalone process owns every object. Sampled images enter shader-read layout before execution and depth enters and remains depth-attachment layout. Diagnostic sentinel snapshots leave color images in a truthful, tracked transfer-source layout; the packet's undefined initial state maps to a discard transition from that known layout into color-attachment layout, followed by the declared shader-read final layout. |

The material sources remain the existing class 1 vertex and class 3 fragment
files selected by OpenGL shader 12 with `HAS_NORMAL_MAP=1`,
`HAS_SPECULAR_MAP=1`, and `DIFFUSE_ALPHA_MODE=0`. Vulkan-only files may add
declarations and coordinate mapping, but may not copy or replace the material
equations.

## Frozen comparison policy

All four artifact planes retain the Stage 12 zero tolerances. The comparator
first requires the exact schema, fixture fingerprint, case, dimensions,
formats, bottom-left row order, and component counts. RGBA8, RGBA16 UNORM, and
canonical depth24 values then compare as exact storage codes. A backend that
cannot satisfy this policy blocks the stage; observed output may not be used to
relax it.

The valid Vulkan run must also retain nontrivial writes in every color plane,
both depth-pass and depth-fail samples, and mirror-clipped samples whose loaded
depth remains unchanged. Every rejected registry must produce no executor
command recording or queue submission, no target mutation, and no validation
message.

## Deliberate exclusions

This stage adds no surface, swapchain, presentation, viewer runtime backend,
production draw-pool route, general shader manifest, streaming upload path,
timing, account input, or live-world input. Vulkan remains behind the existing
test-only build option.

## Native results

A fresh Stage 13 Apple OpenGL run passed all 832 direct-versus-contract
components, five depth passes, four depth failures, four mirror-clipped passing
samples, and 28 rejection cases. Its artifact was byte-identical to the
independent Stage 12 reference. This proves that the Vulkan declaration guards
did not change the OpenGL oracle.

The native Linux RADV replay passed the Vulkan-side gates:

```text
VULKAN_MATERIAL result=pass case=nonrigged_normspec_indexed components=832 rejection_cases=39 recordings=1 submissions=1 depth_passes=5 depth_failures=4 mirror_clipped_passes=4 validation_messages=0 portability_enumeration=enabled portability_subset=not_advertised artifact=written
```

Its exact comparison failed with 92 of 832 components different and maximum
absolute error `0.120729387`. Eighty-seven color differences occur only in the
same nine written pixels: 27 components in G-buffer plane 0, 33 in plane 1,
and 27 in plane 2. Constant alpha channels, every sentinel pixel, fragment
coverage, attachment placement, and row order remain exact. Five depth values
differ by one depth24 storage code; the other 59 depth values are exact.

The native Apple Silicon MoltenVK replay also passed the Vulkan-side gates:

```text
VULKAN_MATERIAL result=pass case=nonrigged_normspec_indexed components=832 rejection_cases=39 recordings=1 submissions=1 depth_passes=5 depth_failures=4 mirror_clipped_passes=4 validation_messages=0 portability_enumeration=enabled portability_subset=enabled artifact=written
```

All 768 MoltenVK color components match Apple OpenGL. Its exact comparison
fails only on five depth values, each separated by one depth24 storage code;
the maximum decoded difference is `5.96046448e-08`.

## Mismatch analysis

No packet, descriptor, shader-interface, attachment, row-origin, winding, or
coverage error was found. The Vulkan and OpenGL paths use the same texture
bytes, mip extents, normalized coordinates, repeat addressing, linear filters,
trilinear mip selection, and 8x anisotropy. Reconstructing the written RADV
pixels shows Apple OpenGL selecting a smaller effective anisotropic footprint
than RADV. Vulkan deliberately leaves the particular anisotropic filtering
scheme implementation-dependent, so a device-specific correction would not
be a truthful API mapping. See the
[Vulkan texture operations specification](https://docs.vulkan.org/spec/latest/chapters/textures.html).

The five depth differences are non-systematic one-code rounding differences
after D32 interpolation and depth24 canonicalization. Four written pixels are
depth-exact, all untouched depth values are exact, and MoltenVK reproduces the
same class of difference without the RADV color differences. This supports a
rasterization/interpolation precision explanation rather than a range or
orientation defect. It remains an evidence-backed inference, not a claim about
either driver's internal algorithm.

The mapping review also corrected a pre-run assumption: a negative viewport
height does not require clockwise fronts for this fixture. Under Vulkan's
framebuffer signed-area definition the intended triangles are counter-clockwise,
and native back-face-culling coverage confirms `VK_FRONT_FACE_COUNTER_CLOCKWISE`.

## Fail-closed boundary

The registry validates the decoded packet and all registered Vulkan metadata
before the executor copies the parameter block or records a command. Thirty-nine
rejection cases cover stale generations, shader identities, buffer and image
metadata, view ranges, usages and layouts, descriptors, sampler policy,
attachment aliasing, render-pass and pipeline compatibility, vertex and index
state, parameter layout, and depth storage.

Each rejected case proves zero executor recordings, zero queue submissions,
unchanged parameter bytes, unchanged four-plane sentinels, and no added
validation message. The only valid execution records once and submits once.
The registry describes resources created and owned by this synchronous
diagnostic; it is not general live Vulkan introspection.

## Code size

| Code | Size |
| --- | ---: |
| Vulkan registry and executor | 964 lines |
| Standalone runner and comparator | 2,824 lines |
| Declaration wrappers around shared shader math | 120 lines |
| Focused Vulkan registry tests | 219 lines |

The 964-line registry and executor are the backend-adapter migration-cost
signal for this slice. The larger runner owns instance/device selection,
fixture upload, validation, rejection evidence, readback, and bounded reporting
that production infrastructure would eventually centralize.

## Verification

- The final Linux opt-in targets built with project warnings treated as errors.
  The 26 render-contract, four material-artifact, two OpenGL registry, five
  startup-argument, and four Vulkan registry cases all passed.
- Native RADV passed 39 rejection cases, one recording, one submission,
  nontrivial color/depth/clipping gates, and zero validation messages. Exact
  artifact comparison then failed under the frozen policy as reported above.
- The macOS runner and comparator built as universal `arm64`/`x86_64`
  executables and passed strict code-signature checks. The native arm64
  registry passed all four cases. MoltenVK advertised portability subset,
  passed all internal gates with zero validation messages, and produced the
  five-code depth-only comparison failure.
- The fresh Stage 13 Apple OpenGL artifact matched the Stage 12 artifact byte
  for byte. No account, live scene, or timing was used.
- With the Vulkan option disabled, the universal macOS and Linux Release
  viewers built successfully. The Mac executable had no Vulkan loader linkage,
  and the default Linux graph contained no Vulkan diagnostic targets.
- The benchmark harness's 57 Python tests passed, including settings XML,
  schema-3 manifests, and the checked-in fixture. Shader compilation and
  `spirv-val`, strict no-replace artifact publication, whitespace validation,
  and adversarial source review passed.

The ordinary Linux build initially exposed an incomplete ambient development
closure: GLX and X11 headers, the X11 link library, and `zlib.h` were absent.
Supplying libglvnd, libX11, xorgproto, and zlib through Nix completed the same
default graph and final viewer link without changing repository files.

## Reanalysis

Tonemap is dual-API. The indexed material slice now has an exact OpenGL oracle,
a native Vulkan executor, and a documented cross-implementation parity block.
The remaining master Stage 1 slice is streaming texture upload, which stresses
declared writes, staging, synchronization, ownership, and lifetime without
depending on material sampling or presentation.

The next committable stage should define one immutable streaming-upload request
and replay it through the existing OpenGL upload mechanism in an account-free
diagnostic. It should freeze source bytes, mip/update regions, destination
format, generation, ordering, completion, and readback artifact; reject stale
or incompatible resources before mutation; and measure the OpenGL adapter
size. Vulkan upload, surfaces, timing, live asset fetching, and production
routing remain outside that stage. Reanalyze again only after that OpenGL
oracle is committed.
