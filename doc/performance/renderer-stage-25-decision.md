# Stage 25 Vulkan material attachment-profile decision

## Decision

Accept one opt-in, backend-private resolver that turns the canonical legacy
normal/specular pipeline key into an immutable, physical-device-specific
attachment profile. The profile fixes the native formats, image roles,
required capabilities, load and clear contract, color write masks, and alpha
semantics needed by a later render-pass and graphics-pipeline owner. Stage 27
later strengthened this profile with the physical-device feature check and
logical-device requirement needed to make the unequal write masks valid.

This is the tenth committable slice of master Stage 2. The library and its
integration test remain behind `LL_VULKAN_TONEMAP_TEST`. They use injected
physical-device query functions and `VK_NO_PROTOTYPES`; they allocate no native
objects, link no Vulkan loader, and expose nothing through the neutral renderer
contract.

## Portable format decision

The resolver accepts only the two canonical production target profiles. It
uses four color attachments and one depth attachment at one sample per pixel.

| Profile | Slot | Logical format | Native Vulkan format |
| --- | ---: | --- | --- |
| Modern HDR | 0 | RGBA8 UNORM | `VK_FORMAT_R8G8B8A8_UNORM` |
| Modern HDR | 1 | RGBA8 UNORM | `VK_FORMAT_R8G8B8A8_UNORM` |
| Modern HDR | 2 | RGBA16 UNORM | `VK_FORMAT_R16G16B16A16_UNORM` |
| Modern HDR | 3 | RGB16 float | `VK_FORMAT_R16G16B16A16_SFLOAT` |
| Compatibility | 0 | RGBA8 UNORM | `VK_FORMAT_R8G8B8A8_UNORM` |
| Compatibility | 1 | RGBA8 UNORM | `VK_FORMAT_R8G8B8A8_UNORM` |
| Compatibility | 2 | RGB10A2 UNORM | `VK_FORMAT_A2B10G10R10_UNORM_PACK32` |
| Compatibility | 3 | RGB8 UNORM | `VK_FORMAT_R8G8B8A8_UNORM` |
| Both | depth | Depth24 UNORM | `VK_FORMAT_D32_SFLOAT` |

Logical three-channel outputs widen to four-channel native images. The
resolver never requests a three-channel Vulkan image. This avoids depending on
optional three-channel attachment support while preserving the shader-visible
RGB contract.

Every color role is both a color attachment and a sampled image. Depth is both
a depth/stencil attachment and a sampled image. Transfer usage is not claimed.
All roles use two-dimensional, optimal-tiling images with zero creation flags,
at least one mip level and array layer, nonzero width, height, and depth
capability, and sample-count-one support.

## Alpha, load, and clear invariant

Slots 0 through 2 write RGBA, load with `VK_ATTACHMENT_LOAD_OP_CLEAR`, and
clear to `(0, 0, 0, 0)`. Slot 3 represents a logical RGB output using an RGBA
image. It writes RGB only, loads with `VK_ATTACHMENT_LOAD_OP_CLEAR`, and clears
to `(0, 0, 0, 1)`. Its alpha is therefore explicitly one after the clear and
cannot be altered by the material fragment shader.

The widened slot's `ImplicitOneAfterClear` semantic, RGB write mask, required
load operation, and clear value form one invariant. A later render-pass and
pipeline implementation must consume them together. Depth likewise requires a
clear load with depth 1 and stencil 0.

The fourth color attachment state differs from the first three, even though
blending is disabled for every slot. Vulkan therefore requires the optional
`independentBlend` feature to be enabled on the logical device. Using an RGBA
write mask for slot 3 is not an alternative because the production fragment
shader writes zero to that alpha component.

## Capability and provenance boundary

Before resolving a profile, the resolver queries physical-device features once
and requires `independentBlend`. It then requires at least four color
attachments and four fragment outputs. Each ordered attachment slot is queried
for the exact optimal-tiling format features and image role that will be
published. Repeated native formats are intentionally queried per slot so a
failure retains exact slot and logical-format context; deduplicating these few
queries is deferred until measurement shows a need.

The result copies the returned `VkImageFormatProperties` envelope and records
the physical device used for selection. Only the resolver can construct or
mutate the profile; it is neither an aggregate nor default-constructible.
`selectedFor()` provides the narrow provenance check needed by later owners.
The profile also retains an immutable typed device requirement whose
`independentBlendRequired()` accessor returns true. The successful feature
query proves support on the selected physical device. It does not prove that
an existing `VkDevice` enabled the feature. A future logical-device owner must
enable the retained requirement and provide its own authenticated capability
view to native pipeline creation.

These capability records are maxima, not permission for an arbitrary
allocation. The future image and framebuffer owner must validate its concrete
extent, layers, mip count, sample count, and resource size before publishing
resources. The caller owns the physical device and guarantees that its four
query callbacks address the same Vulkan implementation for the duration of
the call.

## Failure contract

Invalid device, missing dispatch, malformed production key, and unsupported
diagnostic-profile inputs fail before a callback. Missing
`independentBlend` support fails after the sole feature query with exact query
and feature context, before properties or attachment queries. Limit failures
identify the exact physical-device limit and required versus available value.
Format and image-role failures identify the query, attachment kind, color slot
when applicable, logical and native formats, required and available feature
bits, native `VkResult`, or the exact missing capability and values.

Resolution stops at the first failed ordered role. A Modern HDR failure does
not silently fall back to compatibility. The function is `noexcept`, owns no
native resource, and returns either one complete immutable profile or one
typed error.

## Focused tests and review

The original Stage 25 commit had eight fake-dispatch cases covering:

- result type, `noexcept`, fixed attachment count, and the profile's
  construction and immutability boundary;
- every invalid input before native callbacks;
- exact Modern HDR and compatibility mappings, ordered queried-to-published
  formats, usage, tiling, flags, capability copies, provenance, clears, alpha
  semantics, and write masks;
- both four-attachment physical limits with early termination;
- each required optimal-tiling feature missing from every ordered role,
  including proof that linear or buffer support does not substitute;
- every ordered image-format query returning a native failure without
  fallback; and
- each separate image capability dimension plus a depth failure with exact
  typed context.

Stage 27 adds a ninth case, supplies the feature callback to every existing
fixture, and extends the two success cases. The suite now checks missing
feature dispatch before every callback, unsupported physical support with exact
typed context, one early feature query, full callback order, retained logical
enablement requirements for both profiles, and zero later queries after
feature rejection. The Stage 26 render-pass fixture also checks that its owner
retains the strengthened profile unchanged.

The Stage 25 reviews made the result non-aggregate and
non-default-constructible, clarified that its capabilities are not a concrete
allocation guarantee, coupled the widened RGB alpha semantic to its clear and
write mask, split width, height, and depth failures, and added direct proof that
every queried format is the one published. Stage 27 reanalysis found and closed
the missing optional-feature prerequisite. Repeated per-slot queries remain a
deliberate diagnostic tradeoff.

## Linux evidence

The Release viewer, production shader chain, attachment archive, integration
test, and affected targets built with project warnings treated as errors. All
eight new cases and 15 focused CTest routes passed. The related Python suites
passed all 12 reflection, eight artifact-delivery, and 57 benchmark-harness
tests. No benchmark scenario or timing ran.

The attachment archive has no direct physical-device query symbol, and its
integration executable has no Vulkan, MoltenVK, OpenGL, SDL, window-system, or
viewer dynamic dependency. With the option disabled, the attachment and other
opt-in material targets were absent, the Release viewer built, and no Vulkan
or MoltenVK dependency or packaged shader artifact escaped. Restoring the
option rebuilt the attachment test and production shader chain. Formatting,
whitespace, source-boundary, symbol, dynamic-linkage, and option-isolation
checks passed.

## macOS evidence

A user-owned disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5
contained the Stage 24 commit plus the four reviewed Stage 25 source, test, and
build files. Their SHA-256 values matched the final local working tree. The
ReleaseOS gate used `arm64;x86_64`, deployment target 11, tests, packaging,
warnings as errors, and the opt-in shader path, with benchmarks, signing, and
crash reporting disabled.

The attachment archive contains both architecture slices; the integration
test is native arm64 under existing project policy. All eight new cases, all
15 focused CTest routes, 12 reflection cases, eight artifact-delivery cases,
and 57 benchmark-harness tests passed. The test has no graphics-runtime
dependency and the archive has no direct physical-device query symbol. The
configured loader and MoltenVK paths only satisfied inherited opt-in
configuration; fake dispatch called neither.

The option-on app contained exactly the two canonical production modules:

| Module | SHA-256 |
| --- | --- |
| Vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` |
| Fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` |

All 367 packaged Mach-O files were readable and had no Vulkan or MoltenVK
linkage. After an option-off reconfigure, the attachment test, library, and
production-shader targets were absent. A fresh universal viewer build and
package passed, the app contained zero SPIR-V files, and the same 367 Mach-O
files remained free of Vulkan and MoltenVK linkage.

The first direct option-off rebuild lost the disposable venv's explicit
`autobuild` path during reconfiguration and stopped in package metadata
generation. Restoring that tool path and the build-variable environment made
the complete gate pass without a source change or weakened check. The gate did
not launch the viewer, create a graphics context, call a driver, use an
account, enter a region, compare pixels, or collect timing. Its exact
disposable source, build, dependency, package, and metadata root was validated
as user-owned, deleted, and verified absent.

## Code size

| Stage 25 code, excluding this decision record | Lines added | Lines removed |
| --- | ---: | ---: |
| Attachment backend header and implementation | 417 | 0 |
| Fake-dispatch attachment tests | 487 | 0 |
| Opt-in build wiring | 24 | 0 |
| Total | 928 | 0 |

## Explicit runtime gaps

Stage 25 does not create a Vulkan instance, physical or logical device, query
dispatch through `vkGetInstanceProcAddr`, call a real driver, allocate memory
or images, validate a concrete extent or memory size, create an image view,
framebuffer, render pass, or graphics pipeline, establish a layout transition,
record or submit commands, synchronize completion, retain shader modules,
connect a viewer call site, select a backend, present or compare pixels, or
measure performance.

Stage 27's injected feature callback does not change those runtime limits. It
proves physical support only in the supplied query transaction and publishes a
future enablement requirement. It neither creates nor authenticates a logical
device.

The profile is descriptive and non-executable until later owners consume its
load, clear, format, usage, and write-mask contract. The next-stage choice
remains open until this stage is committed and the dependency graph is
reanalyzed.
