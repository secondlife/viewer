# Stage 15 Vulkan streaming texture upload decision

## Decision

Accept the standalone Linux Vulkan replay as an exact implementation of the
Stage 14 streaming-upload contract. Native Vulkan copied the frozen pitched
source, generated both lower mips with linear image blits, sampled the result
through the viewer copy shader, preserved the retired image, and produced the
same 480-byte schema-2 artifact as OpenGL.

The macOS evidence lane was blocked by SSH connectivity before a fresh source
sync, build, or run could start. ICMP reached the host, but repeated TCP port
22 connections timed out before authentication. No remote state was changed.
This is an infrastructure block, not a Vulkan capability or byte-comparison
failure, and it does not support a cross-platform Vulkan conclusion.

The production viewer remains OpenGL-only. The three master Stage 1 slices now
provide enough migration-cost evidence to stop extending standalone pass
runners and move to a production-facing, backend-neutral draw boundary.

## Frozen request and artifact

The packet, fixture, schema-2 artifact, and exact comparison policy are
unchanged from Stage 14.

| Contract field | Frozen value |
| --- | --- |
| Frame and revisions | Frame 1, prior revision 22, replacement revision 23 |
| Source | 8x4 RGBA8, top-left rows, 36-byte pitch, four poison bytes per row |
| Logical image | 32x16 with resident discard 2 |
| Destination | New persistent generation `{11, 2}` replacing `{11, 1}` |
| Mips | Three levels: 8x4, 4x2, and 2x1 |
| States | Undefined to transfer destination to shader read |
| Sampling | Fixed 4x2 RGBA8 output through the viewer copy program |
| Lifetime | One revision-23 completion and one retirement of `{11, 1}` at frame 1 |

The artifact remains 480 bytes: 168 bytes of bottom-left mip data, 32 bytes of
bottom-left sampled output, canonical metadata and lifecycle evidence, then a
big-endian 64-bit FNV-1a checksum. All 200 image bytes and all metadata must be
identical. No tolerance, bias, alternate source, or driver-specific rule was
introduced.

## Vulkan resource mapping

The executor requires Vulkan 1.1 and one queue family with graphics and
transfer support. It owns one queue, a reset-capable command pool, one primary
command buffer, and all resources used by the synchronous run.

| Resource | Frozen Vulkan mapping |
| --- | --- |
| Screen triangle | Distinct 48-byte vertex buffer |
| Upload staging | Distinct 144-byte mapped transfer-source buffer |
| Replacement | 8x4 `VK_FORMAT_R8G8B8A8_UNORM`, three mips, transfer source, transfer destination, and sampled usage |
| Output | 4x2 `VK_FORMAT_R8G8B8A8_UNORM`, color attachment, transfer source, transfer destination, and sampled usage |
| Readback | Distinct 200-byte mapped transfer-destination buffer |
| Sampler | Linear minification, magnification, and mip filtering; clamp; LOD 0 through 2; anisotropy disabled |
| Raster state | Viewport `{0, 2, 4, -2, 0, 1}`, 4x2 scissor, no blend or depth |
| Final layouts | Replacement and output both shader read-only |

The four upload regions copy one 8x1 row each from staging offsets 108, 72,
36, and 0. This converts the owned top-left source to Vulkan's image row order
without copying any poison byte. Two `vkCmdBlitImage` calls generate 4x2 and
2x1 mips. The sampled draw writes the 4x2 output. Readback starts at offsets 0,
128, 160, and 168 for the three replacement mips and the output. Only the
two-row output plane is flipped during artifact assembly.

Format preflight requires sampled-image, color-attachment, transfer, and
linear-filter blit support before recording. All barriers use ignored queue
family indices because the run has one queue family. The replacement and
output begin with explicit undefined discard transitions, and the one valid
command buffer performs the copies, blits, render pass, readbacks, and final
shader-read transitions before one submission and one queue wait.

## Pre-artifact mapping correction

The first frozen mapping required the output image to end in shader-read layout
but omitted sampled usage. Khronos validation correctly rejected that
contradiction with `VUID-VkImageMemoryBarrier-newLayout-01213`. The run
published no artifact and retained no candidate bytes.

Before collecting evidence, the mapping was corrected by adding
`VK_IMAGE_USAGE_SAMPLED_BIT` to the output image. This is the minimum truthful
capability for the packet's declared `After=ShaderRead` state. It did not
change the packet, source, shader, LOD policy, readback, artifact, or exact
comparison rule.

## Shader reuse and identity

Small stage wrappers compile the existing viewer `copyV.glsl` and
`copyF.glsl` bodies for Vulkan. Source guards provide the wrapper-controlled
version and interface declarations while leaving the viewer's ordinary
OpenGL compilation unchanged. The descriptor is one combined image sampler,
and the fragment body still performs the implicit-LOD copy used by Stage 14.

The final Vulkan 1.1 SPIR-V modules passed `spirv-val`. Their SHA-256 identities
were:

```text
vertex   139f3d06e998cdd95ad6ae751dd97cf7ecaeb9c210efca379a7b1ee73270789c
fragment 2d07ec80932a25934493be1d4f8bdfb3ca3d2bac0cf3ffa9cbb7d7520bdaafb1
```

The registry matches these full identities rather than trusting filenames or
nonzero shader modules.

## Registry, ownership, and lifetime

`LLRenderVulkanTextureUpload` resolves the packet through a pass-specific
registry. It validates exact generations and every relevant native property:
sizes, formats, mip counts, usage, memory properties and ranges, image views,
layouts, descriptors, render-pass compatibility, shaders, vertex input,
viewport and scissor policy, raster state, command objects, queue family, and
lifecycle ledger.

Each run receives a unique nonzero ownership token from a monotonic atomic
allocator. Every buffer, image, sampler, pipeline, and execution context must
carry that same token. Native handles, image views, memory allocations, and
the three buffer allocations are also checked for the required disjointness.
This prevents a registry assembled from valid objects owned by different
runners from passing preflight.

The old image remains a fixed 168-byte sentinel. A queue-idle readback before
execution must match it. Preflight then proves that old, replacement, and
output images, views, and memory are pairwise distinct. The sole executor
command buffer never names the old image, so that structural proof is complete
before the local lifecycle ledger is published. A second queue-idle snapshot
after execution must also match before the runner may publish the artifact.

There is a deliberate evidence-order distinction here. The fixed 200-byte
executor readback cannot include another old-image snapshot without changing
the frozen artifact path, so the local logical ledger changes after structural
proof and before the runner's external post-execution snapshot. Durable
artifact publication still waits for that snapshot. A failed post-snapshot
therefore cannot publish durable evidence, although the already completed
local run is not transactionally rolled back.

## Fail-closed boundary

The runner exercises all 32 Stage 14 packet mutations, six focused packet and
draw-shape supplements, and the Vulkan registry, native-object, execution
context, and lifecycle mutations. The final matrix contains 111 rejection
cases. Preflight rejections preserve staging and readback sentinels, image
bytes and layouts, result state, lifecycle state, registry resolution,
recording and submission counters, and validation count.

The valid case records one executor recording attempt, one submission, one
completion, and one retirement. Registry resolution and physical-object
checks occur before mapped staging memory changes or either execution counter
increments.

The guarantee narrows after successful preflight. A device failure during
recording, submission, waiting, or readback prevents logical result and
artifact publication, but the diagnostic does not promise transactional
rollback of scratch buffers or GPU image state. This distinction avoids
claiming stronger recovery than a synchronous Vulkan command stream can
provide.

## Native result

The final native Linux RADV run with Khronos validation reported:

```text
VULKAN_TEXTURE_UPLOAD result=pass rejection_cases=111 recording_attempts=1 submissions=1 mip_bytes=168 sample_bytes=32 mismatches=0 completions=1 retirements=1 validation_messages=0 portability_enumeration=enabled portability_subset=not_advertised artifact=written
```

A fresh Stage 14 OpenGL oracle ran on Linux OpenGL 4.5 compatibility and
reported all 59 rejection cases, one completion, one retirement, zero
mismatches, and a written 480-byte artifact. The standalone comparator then
reported:

```text
TEXTURE_UPLOAD_COMPARE result=pass mip_bytes=168 sample_bytes=32 mismatches=0
```

Both artifacts had the Stage 14 digest:

```text
50b10b206d54e48fdc9fef7cb4554169e15b0cfb1dc8a8773d271e6bf7ccc0ea
```

No timing was captured or retained. The stage did not log in, fetch a live
asset, enter a region, or use account credentials.

## macOS infrastructure block

Repeated fresh SSH probes reached the Mac by ICMP but timed out on TCP port 22
before authentication. The host was not modified, and no Stage 15 source,
generated shader, build, profile, log, or artifact was copied there. The last
known Stage 14 daily checkout state is therefore not fresh Stage 15 evidence.

Source review confirmed that the opt-in CMake path still selects the existing
universal Vulkan loader arrangement, shader compiler, SPIR-V validator,
validation layers, and MoltenVK ICD handling. That review supports build-graph
plausibility only. It is not a substitute for an Apple Silicon compile,
MoltenVK run, old-image preservation check, or artifact comparison.

## Production isolation

The Vulkan runner, comparator, registry test, and shader targets remain behind
the existing opt-in diagnostic switch. With that switch off, the default
CMake target graph exposes none of the new targets. The Linux Release viewer
target built successfully, and its executable has no Vulkan or MoltenVK
dynamic dependency.

No surface, swapchain, presentation path, backend selection, live viewer draw,
or production texture manager changed. The shared neutral lifecycle type is a
name cleanup; the Stage 14 OpenGL name remains an alias and its behavior is
unchanged.

## Code size

| Code | Size |
| --- | ---: |
| Vulkan registry and executor | 1,486 lines |
| Standalone Vulkan runner | 2,252 lines |
| Comparator and shader wrappers | 98 lines |
| Focused Vulkan registry tests | 519 lines |

The 1,486-line adapter is the useful migration-cost signal. The 2,252-line
runner also repeats device selection, validation, allocation, shader loading,
pipeline creation, command setup, rejection poisoning, readback, and artifact
publication. Those services should not be copied into another pass runner.

## Verification

- The opt-in Linux targets built with project warnings treated as errors.
- Vulkan shader compilation and Vulkan 1.1 SPIR-V validation passed.
- Native RADV and Khronos validation passed 111 rejection cases with one
  recording, one submission, zero validation messages, and a 480-byte
  artifact.
- A fresh OpenGL oracle and the independent comparator proved exact equality
  with the frozen Stage 14 artifact.
- Focused C++ suites passed 27 render-contract, three OpenGL upload registry,
  four upload packet, four upload artifact, six Vulkan upload registry, five
  startup-argument, and seven directory-isolation cases.
- The benchmark harness's 57 Python tests passed. Settings and command-line XML
  parsed successfully, both shaders passed `spirv-val`, and `git diff --check`
  was clean.
- Missing arguments, an occupied destination, and a dangling destination
  symlink all failed without replacing or changing the destination.
- Two independent adversarial reviews ended with no remaining concrete Vulkan
  correctness, lifetime, artifact, shader, CMake isolation, or MoltenVK
  findings.

## Reanalysis

Master Stage 1 has now measured three different pass boundaries. Tonemapping
can match OpenGL and Vulkan exactly. The material packet is structurally valid
but exposes a frozen cross-driver precision block. Streaming upload matches
exactly on Linux and exercises explicit transfer, layout, sampling, and
lifetime behavior; its fresh macOS lane remains blocked by connectivity.

The common result is architectural rather than a performance number. Narrow
immutable packets work, exact or explicitly blocked artifacts keep evidence
honest, and pass-local native registries can reject stale resources. The cost
comes from rebuilding ownership, context validation, shader identity, command
services, and publication machinery around each standalone fixture. A fourth
diagnostic pass would mostly measure the same duplication.

The next commit should begin master Stage 2 at the production draw seam. It
should translate one real `LLDrawInfo` shape into an immutable,
backend-neutral packet with copied draw ranges, constants, texture and
material references, and a stable pipeline key. It should prove that the
packet survives source mutation and retirement without carrying OpenGL or
Vulkan handles. Existing OpenGL submission should remain unchanged until that
boundary is canonical and tested.

Shader manifests, shared Vulkan allocation and command services, descriptors,
pipeline caches, deferred destruction, surfaces, swapchains, presentation,
and backend selection should follow in later commits. Combining them with the
first production packet would hide which ownership boundary is responsible
for a failure.
