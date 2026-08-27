# Stage 14 OpenGL streaming texture upload decision

## Decision

Keep the backend-neutral streaming-upload packet and the production-seam
OpenGL replay as the exact oracle for the next migration slice. The fixed
request now exercises decoded image admission, replacement allocation, base
upload, generated mips, sampling, completion publication, and retirement of
the prior generation without an account or live asset.

Native OpenGL 4.1 on Apple Silicon and software OpenGL 4.1 and 4.5 on Linux
produced byte-identical schema-2 artifacts. Direct fixture execution and
packet execution also match exactly. This clears the stage to add a standalone
Vulkan implementation of the same immutable request. The production viewer
remains OpenGL.

## Frozen request

The request represents replacement generation `{11, 2}` of logical 32x16
image `{11, 1}` at revision 23 after revision 22. Discard level 2 makes the
resident base 8x4. The source is RGBA8 in top-left row order with a 36-byte row
pitch: 32 pixel bytes followed by four poison bytes on every row. The executor
may normalize those bytes only after all packet and live-resource checks pass.

| Contract field | Frozen value |
| --- | --- |
| Frame and revisions | Frame 1, prior revision 22, replacement revision 23 |
| Source | 8x4 RGBA8, top-left rows, 36-byte pitch, four poison bytes per row |
| Logical image | 32x16 with resident discard 2 |
| Destination | New persistent image generation `{11, 2}` replacing `{11, 1}` |
| Mips | Three levels: 8x4, 4x2, and 2x1; generate levels 1 and 2 |
| States | Undefined to transfer destination to shader read |
| Sampling | Fixed 4x2 RGBA8 output through the viewer copy program |
| Lifetime | One completion for revision 23 and one retirement of `{11, 1}` at frame 1 |

The packet owns its source storage. Its image, sampler, pipeline, pass,
viewport, scissor, release, and handle generations are all canonical and
strictly decoded. The direct path receives the same decoded inputs rather than
a second hand-built interpretation.

## OpenGL mapping

The contract adapter resolves registered viewer objects, validates their live
metadata, and then calls the real `LLImageGL::createGLTexture()` upload seam.
It does not substitute a diagnostic-only `glTexImage2D` implementation. The
source is converted to tight bottom-left rows after preflight, uploaded as the
resident base, and passed through the viewer's normal mip generation. The
replacement is sampled with `gCopyProgram` into a 4x2 render target.

The executor reads all three replacement mips and the sampled output. It
requires the base to equal the normalized source exactly, the sampled output
to equal mip 1 exactly, and every written plane to contain at least two changed
texels with different values. Only then may it publish the completion and
retirement ledger and expose an artifact. The old handle becomes unresolved,
the replacement becomes current and resolved, and the replacement GL object
stays owned by its existing viewer wrapper.

The route borrows the active viewer context and therefore fails closed unless
it runs on the main thread, OpenGL is at least 4.1, shader profiling is off,
the registry is canonical, and all inspected GL objects match the packet. It
uses a dedicated scratch VAO and restores the caller's cached and raw GL state
before publishing lifecycle evidence. The captured surface includes program
and shader caches, uniform dirtiness, active and bound textures, samplers,
pixel buffers and stores, array and element buffers, VAO attributes,
framebuffers, draw/read buffers, viewport, scissor, color mask, polygon mode,
capabilities, clip distances, and Linux clip-control state when available.

## Artifact and comparison policy

The artifact is schema 2 and exactly 480 bytes. It records the fixed fixture
identity, request metadata, 168 bottom-left RGBA8 mip bytes, 32 bottom-left
RGBA8 sampled bytes, completion and retirement evidence, and before/after
resolvability. A big-endian 64-bit FNV-1a checksum covers the first 472 bytes.

Encoding and decoding reject every noncanonical field. Comparison requires
identical metadata, lifecycle evidence, and all 200 image bytes; no tolerance
or driver-specific adjustment exists. Publication uses an unpredictable
sibling file and a no-replace atomic link. An existing destination is never
overwritten. Once the destination link is installed, failure to remove the
temporary sibling is a nonfatal warning because publication has already
succeeded.

## Fail-closed boundary

Fifty-nine rejection cases cover malformed packet fields, stale and aliased
handles, revisions and lifecycle state, image dimensions, mip counts and
formats, sampler and shader state, upload layout, output compatibility,
screen-triangle metadata, active-texture cache divergence, profiling,
framebuffer and texture parameters, and off-main-thread execution.

Each rejected case retains the caller's result object, resource pixels,
lifecycle ledger, registry resolvability, and ambient GL state. The valid case
records exactly one completion and one retirement. Live format rejection uses
a complete RGB8 poison mip chain so the resource remains readable on Apple
OpenGL while still being incompatible with the RGBA8 contract.

## Process isolation

The hidden route requires `SECONDLIFE_USER_DIR` before ordinary startup can use
a profile. Linux prepares the isolated root and proves it writable with an
exclusive temporary file. Win32 uses the Unicode environment value, creates
the exact isolated root, and performs an exclusive write/delete-on-close
probe. Both reject a missing or invalid root before viewer initialization.

The macOS global directory object now computes paths without creating profile
or cache directories. Directory creation is deferred to `initAppDirs()`, and
the Objective-C system-folder lookup is read-only. The native isolation test
proved that constructing the global object does not create the base or profile
tree and that explicit initialization creates only the supplied temporary
tree. The daily checkout and its normal profile were not modified.

Win32 received source and adversarial review but no Windows runtime lane was
available. That remains a portability residual, not evidence of Windows
execution.

## Native results

The final Linux OpenGL 4.1 compatibility run reported:

```text
TEXTURE_UPLOAD_CONTRACT_PARITY result=pass resident=8x4 logical=32x16 discard=2 mips=3 sampled=4x2 mip_bytes=168 sample_bytes=32 mismatches=0 rejection_cases=59 rejection_failures=0 completions=1 retirements=1 artifact=written
```

The final Linux OpenGL 4.5 compatibility run reported the same marker and
exercised clip-origin and clip-depth capture and restoration. Both artifacts
were 480 bytes.

The final native Apple Silicon OpenGL 4.1 core run reported the same passing
marker and a 480-byte artifact. The first Apple run exposed the incomplete
mixed-format poison chain described above; it failed one rejection gate and
published no artifact. After preserving mip-chain completeness, all 59 gates
passed. The failed run was discarded rather than counted as evidence.

The three accepted artifacts had the same SHA-256 digest:

```text
50b10b206d54e48fdc9fef7cb4554169e15b0cfb1dc8a8773d271e6bf7ccc0ea
```

No timing was captured or retained. The route did not log in, fetch a live
asset, enter a region, or use the temporary demo account.

## Code size

| Code | Size |
| --- | ---: |
| OpenGL registry, executor, and public seam | 1,484 lines |
| Viewer parity route and fixture execution | 1,588 lines |
| Neutral packet and artifact implementation | 1,253 lines |
| Focused packet, artifact, and registry tests | 780 lines |

The 1,484-line OpenGL adapter is the migration-cost signal for this slice. A
large part is explicit borrowed-context validation and state restoration that
a future backend-owned command path would centralize. The standalone parity
route owns rejection poisoning, dual execution, readback, and process-level
evidence; it is not proposed as production architecture.

## Verification

- The final Linux Release viewer relinked with project warnings treated as
  errors. The 27 render-contract, three OpenGL upload registry, four upload
  packet, four upload artifact, five startup-argument, and seven Linux
  directory cases passed.
- The benchmark harness's 57 Python tests passed. Settings and command-line XML
  parsed successfully, and `git diff --check` was clean.
- Native Linux OpenGL 4.1 and 4.5 passed all 59 rejection cases, exact direct
  versus contract comparison, nontrivial-write gates, one completion, one
  retirement, and fixed-size artifact publication.
- The full macOS ReleaseOS build succeeded for the universal app. The app
  executable contains both `arm64` and `x86_64`; the focused tests ran natively
  as arm64 and passed 27, three, four, four, five, and two sets of eight
  directory-isolation cases. The locally built app had no Vulkan or MoltenVK
  linkage. This development package was unsigned, so no signature result is
  claimed.
- All 32 Stage 14 tracked paths used by the Mac build matched the Linux working
  tree byte for byte. The ordinary Mac checkout remained at its original clean
  commit.
- Independent portability and adversarial reviews ended with no concrete
  correctness, GL-state, isolation, build-graph, or exit-gate findings.

## Reanalysis

The upload slice now has a neutral request, an exact cross-platform OpenGL
oracle, and explicit lifetime evidence. Unlike the blocked material slice, its
observable result is storage-code deterministic across Apple and Mesa OpenGL.
This makes it a strong next candidate for Vulkan because staging, image
transitions, mip generation, sampling, and retirement can be tested without a
swapchain or live scene.

Stage 15 should implement the same packet in the existing test-only Vulkan
build. It should resolve registered Vulkan buffers, images, views, memory,
samplers, pipeline, and lifecycle state; reject incompatible live resources
before recording; upload the canonical source; generate the two remaining
mips; sample to the fixed 4x2 target; read back schema-2 bytes; and compare the
result exactly with the Stage 14 artifact. Rejected cases must record and
submit nothing and retain sentinels and lifecycle state.

Stage 15 must not add a surface, swapchain, presentation path, production
backend selection, account input, live asset input, timing, tolerance changes,
or a general texture manager. Reanalyze the next production migration boundary
only after the standalone Vulkan result and exact comparison are committed.
