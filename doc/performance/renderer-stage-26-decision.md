# Stage 26 Vulkan material render-pass decision

## Decision

Accept one opt-in, backend-private owner for the classic Vulkan 1.1 render
pass compatible with the canonical legacy normal/specular attachment profile.
The factory consumes one immutable Stage 25 profile and returns either one
complete uniquely owned `VkRenderPass` generation or one typed error.

This is the eleventh committable slice of master Stage 2. The library and its
integration test remain behind `LL_VULKAN_TONEMAP_TEST`, use injected dispatch
with `VK_NO_PROTOTYPES`, and expose nothing through the neutral renderer
contract. The object describes one shared deferred pass generation. It does
not begin a render pass and must not be created or cleared per material draw.

## Render-pass compatibility policy

The create info contains exactly five ordered attachment descriptions:

1. the four color formats from the retained Stage 25 profile; then
2. the profile's depth format.

Every attachment uses flags zero and one sample. Color and depth consume the
profile's required `CLEAR` load operation and use `STORE`; stencil load and
store are `DONT_CARE`. One graphics subpass references colors 0 through 3 and
depth 4. It has no input, resolve, preserve, multiview, or extension state.
The create info has zero explicit dependencies.

Color initial, subpass, and final layouts are all
`VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`. Depth initial, subpass, and final
layouts are all `VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL`. Keeping
each attachment in one pass-local layout avoids an incomplete hybrid of
automatic transitions and external synchronization. The owner exposes both
layout values so later code does not duplicate private policy.

Attachment flags zero mean this pass does not permit overlapping attachment
memory. A future concrete resource owner must preserve that non-aliasing
policy unless the render-pass compatibility contract is deliberately changed.

## Synchronization and clear boundary

The render pass performs no automatic layout transition. A future image-aware
command encoder must transition actual images into the exposed attachment
layouts before begin and out to their real consumer layouts afterward. That
encoder also owns the exact pipeline barriers, semaphore dependencies, queue
family transfers, and reuse rules required by its schedule. Vulkan's implicit
external dependencies are not treated as a complete frame-scheduling policy.

`vkCreateRenderPass` does not consume clear values. The owner publishes the
ordered five-value clear array as metadata for a future pass-begin owner:

- color slots 0 through 2 clear to transparent black;
- widened logical-RGB slot 3 clears to `(0, 0, 0, 1)`; and
- depth clears to 1 with stencil 0.

The owner retains the complete immutable attachment profile, including slot
3's RGB-only write mask and `ImplicitOneAfterClear` alpha semantic. Alpha-one
correctness remains non-executable until a later graphics pipeline consumes
that write mask.

## Device, ownership, and failure contract

The caller supplies borrowed physical and logical devices plus injected
`vkCreateRenderPass` and `vkDestroyRenderPass` callbacks. It guarantees a
Vulkan 1.1 logical device created from the supplied physical device, at least
one graphics-capable queue family, callbacks valid for that device, handle and
callback lifetime through owner destruction, externally synchronized host
access, and completion of every submitted native user before destruction.
Raw handles cannot authenticate those relationships.

Null physical or logical devices, incomplete dispatch, and a profile selected
for another physical device fail before a native callback. The noncopyable,
nonmovable C++ owner and retained profile are allocated before native create.
A failed `VkResult` ignores any poisoned output. Success with a null handle is
rejected. Only one non-null success is published.

Destruction uses the creating device, retained destroy callback, and null
allocator exactly once. Independent successful calls retain independent
destruction obligations even when their opaque handle values compare equal.
Owner allocation failure has a typed result and is structurally ordered before
native creation, but deterministic allocation failure is not fault-injected.

## Focused tests and review

Six fake-dispatch cases cover:

- result types, fixed attachment count, copy/move/destruction traits, profile
  nothrow-copy, and public `noexcept` boundaries;
- every preflight rejection with zero native callbacks;
- exact Modern HDR and Compatibility attachment order and every create-info,
  attachment, reference, subpass, optional-pointer, and dependency field;
- retained provenance, profile, write mask, alpha semantic, layouts, and clear
  values after the source result is replaced;
- failed creation with poisoned output and success with a null handle; and
- exact destruction, including independent owners that deliberately reuse an
  opaque handle value.

The fake proves host-side call formation, rollback, metadata, and ownership.
It cannot establish the physical/logical device relationship, graphics queue
capability, real-driver acceptance, concrete image validity, synchronization,
or submitted lifetime. Those remain explicit caller preconditions or later
runtime gates.

Four independent adversarial review passes corrected the original mixed
automatic-transition policy, made the deferred synchronization and layout
contract public, added the graphics-queue precondition, completed the owner
trait and Compatibility alpha assertions, and clarified explicit versus
implicit dependencies. The final reviewed source has no remaining medium- or
high-severity issue.

## Linux evidence

The Release viewer, production shader chain, render-pass archive, integration
test, and affected targets built with project warnings treated as errors. All
six new cases and the exact 16 focused CTest routes passed. The related Python
suites passed all 12 reflection, eight artifact-delivery, and 57
benchmark-harness tests. No benchmark scenario or timing ran.

The archive and integration executable have no direct
`vkCreateRenderPass` or `vkDestroyRenderPass` symbol. The focused executable
has no Vulkan, MoltenVK, OpenGL, window-system, SDL, or viewer dynamic
dependency. Both production SPIR-V modules retained their established hashes.

With the option disabled, the new render-pass target and all other opt-in
material targets were absent, the Release viewer built, and it retained no
Vulkan or MoltenVK linkage. Restoring the option rebuilt the focused target
and production shader chain. This Linux tree has `PACKAGE=OFF`; its broader
viewer-manifest copy graph also references inherited media-plugin outputs not
present in this build graph, so it is not used as package evidence. The eight
delivery tests and the fresh macOS ON/OFF package gate cover the artifact
delivery boundary.

## macOS evidence

A user-owned disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5
contained the Stage 25 commit plus exactly the four reviewed Stage 26 source,
test, and build files. Final snapshot hashes matched the local working tree:

| File | SHA-256 |
| --- | --- |
| `indra/llrender/CMakeLists.txt` | `f005839dfc6e705bd9c91b3b6a93e23dcfb9a5ebbe58a269e58950e13e8b764f` |
| Render-pass header | `283b7811b8a8b855f4ecfdd14bb88869159fc9583f9325a396028b0226f1d720` |
| Render-pass implementation | `3cca4050d7ead6ad06cd618eedf967c0d3a25b183ca6844fa3367c552334f1f8` |
| Fake-dispatch test | `de7ff7eba84f0c41eef1b070986a85fdf889c7c0dba439d94059098dde76c6cb` |

The option-on ReleaseOS gate used `arm64;x86_64`, deployment target 11,
tests, packaging, and warnings as errors, with renderer benchmarks, signing,
and crash reporting disabled. The full universal viewer and package passed.
The exact 16 focused CTest routes, all six new cases, 12 reflection cases,
eight artifact-delivery cases, and 57 benchmark-harness tests passed.

The render-pass archive contains both architecture slices. The focused test is
native arm64 under existing project policy and has a valid code signature. The
archive and test have no direct render-pass symbol, and the test has no Vulkan,
MoltenVK, OpenGL, SDL, or viewer dependency. The option-on app contained
exactly the two canonical production modules:

| Module | SHA-256 |
| --- | --- |
| Vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` |
| Fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` |

All 367 packaged Mach-O files were readable and had no Vulkan or MoltenVK
linkage. After an option-off reconfigure, all 21 enumerated opt-in material
targets were absent while the neutral stale-artifact cleanup route remained.
A fresh universal viewer and package passed, the app contained zero SPIR-V
files, and all 367 Mach-O files remained readable and free of Vulkan and
MoltenVK linkage.

The first option-on build and package succeeded, but Autobuild metadata then
failed because the synthetic disposable Git snapshot lacked an origin URL.
Adding the repository's public origin as harness-only metadata and rerunning
incrementally passed without a source change or weakened check.

The gate did not launch the viewer, create a graphics context, call a driver,
use an account, enter a region, compare pixels, or collect timing. The exact
disposable root was revalidated as user-owned and non-symlinked, deleted, and
verified absent.

## Code size

| Stage 26 code, excluding this decision record | Lines added | Lines removed |
| --- | ---: | ---: |
| Render-pass backend header and implementation | 294 | 0 |
| Fake-dispatch render-pass tests | 514 | 0 |
| Opt-in build wiring | 24 | 0 |
| Total | 832 | 0 |

## Explicit runtime gaps

Stage 26 does not acquire Vulkan loader dispatch, create an instance, select a
physical device, create a logical device or queues, call a real driver,
allocate memory or images, validate a concrete extent or resource size, create
an image view or framebuffer, permit attachment aliasing, create a graphics
pipeline or cache, record begin/end commands, transition layouts, establish
barriers or semaphore dependencies, submit work, synchronize completion,
handle device loss, connect a viewer call site, select a backend, present or
compare pixels, or measure performance.

The render pass is a compatibility object and ownership transaction, not a
working rendering path. The smallest next dependency remains open until this
stage is committed and the dependency graph is reanalysed.
