# Stage 27 Vulkan independent-blend capability decision

## Decision

Strengthen the existing material attachment-profile resolver so every
successful production profile proves physical support for Vulkan's optional
`independentBlend` feature. Retain the matching logical-device enablement
requirement as typed profile data.

This is the twelfth committable slice of master Stage 2. It changes the Stage
25 resolver and its existing opt-in test target. It creates no Vulkan object,
adds no build target, links no loader, and does not start graphics-pipeline
work.

## Why the feature is required

The material attachment profile retains four color write masks. Slots 0
through 2 write RGBA. Slot 3 writes RGB so its alpha remains one after a clear
to `(0, 0, 0, 1)`, even though the production fragment shader writes zero to
that alpha component.

Vulkan requires every `VkPipelineColorBlendAttachmentState` to be identical
unless the logical device enables
[`independentBlend`](https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineColorBlendStateCreateInfo.html).
The fourth write mask makes the states unequal even though blending itself is
disabled. `independentBlend` is an optional member of
[`VkPhysicalDeviceFeatures`](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFeatures.html),
so the Stage 25 format and limit queries were not enough to establish that a
future pipeline can consume the retained masks.

Changing slot 3 to an RGBA write mask would break the established alpha-one
invariant. Changing the shared shader output would also change the accepted
production artifact. Stage 27 keeps both target mappings, all four masks, and
the shader artifacts unchanged.

## Query and failure contract

`MaterialAttachmentDispatch` now includes injected
`PFN_vkGetPhysicalDeviceFeatures`. Missing feature dispatch returns
`InvalidDispatch` before any callback, like every other incomplete dispatch.
Null device, malformed key, and unsupported target-profile checks also remain
ahead of native queries.

After preflight, the resolver performs this fixed transaction:

1. query `VkPhysicalDeviceFeatures` once;
2. reject a false `independentBlend` value;
3. query physical-device properties once; and
4. query format properties followed by image-format properties for each of
   the four ordered color roles and the depth role.

Feature rejection returns `MissingDeviceFeature` with
`PhysicalDeviceFeatures` query context and the `IndependentBlend` feature
discriminator. It performs no property, format, or image-format query and
publishes no partial profile.

## Support, requirement, and provenance

A successful profile now establishes two related facts:

- the injected query reported `independentBlend` support for the exact
  physical device recorded by `selectedFor()`; and
- `deviceRequirements()` returns an immutable copy whose
  `independentBlendRequired()` accessor returns true. Its state is private and
  the value is neither an aggregate nor publicly default-constructible.

These facts are deliberately separate. A physical feature query cannot prove
which features an existing `VkDevice` enabled. The profile does not
authenticate a physical-to-logical-device relationship, mint a logical-device
capability token, or make the write masks executable. A later device owner
must enable the retained requirement, and a later pipeline owner must consume
an authenticated view of those enabled features.

The existing format, role, capability, clear, alpha, target-profile, and
physical-device provenance data remain unchanged. `selectedFor()` still
answers only whether the profile came from a particular physical device.

## Focused tests

The attachment fake defaults `independentBlend` to true and records the full
callback sequence. Nine attachment cases now cover:

- result traits and the typed requirement value;
- every invalid input and missing callback before all native queries;
- Modern HDR and Compatibility success with one early feature query, exact
  callback order, retained requirement, unchanged formats, roles, masks,
  clears, capabilities, and physical provenance;
- an unsupported feature with exact error context and zero later callbacks;
- both physical limits;
- every required optimal-tiling feature for every attachment role;
- every ordered native image-format failure; and
- every separate image-capability failure.

The Stage 26 render-pass fixture supplies the new callback. Its six existing
cases also check that both target-profile owners retain the strengthened
requirements value after copying the source profile.

The fake proves query ordering, typed failure, retained metadata, and absence
of later callbacks after feature rejection. It does not prove that a driver
supports the feature or that a logical device enabled it.

## Focused Linux evidence

The Release configuration had `LL_VULKAN_TONEMAP_TEST=ON` and treated project
warnings as errors. The viewer, production shader chain, attachment and
render-pass libraries, and their integration executables built successfully.
All nine attachment cases and all six dependent render-pass cases passed.

The established 16-route focused CTest set passed, including the draw-packet,
diagnostic, contract, manifest, artifact, publication, module, layout,
descriptor, attachment, render-pass, registry, delivery, manifest-argument,
and parity-argument routes. The 12 reflection tests, eight artifact-delivery
tests, and 57 benchmark-harness unit tests also passed. The harness tests did
not run a benchmark.

The production modules retained their canonical hashes:

| Module | SHA-256 |
| --- | --- |
| Vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` |
| Fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` |

The focused archives and executables have no direct physical-device query or
render-pass symbols. Their dynamic dependencies contain no Vulkan, MoltenVK,
OpenGL, SDL, X11, Wayland, or viewer dependency. A full option-off reconfigure
removed all enumerated opt-in material targets from the generated graph while
leaving the neutral stale-artifact cleanup route. The Release viewer rebuilt
without Vulkan or MoltenVK linkage. The option was restored before the final
focused build and test pass.

The tests use only injected callbacks. They did not load Vulkan, query a real
physical device, create a logical device, launch the viewer, enter a region,
render pixels, or collect timing.

## macOS evidence

A user-owned disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5
contained the Stage 26 commit plus exactly the six reviewed Stage 27 files.
The final resolver, dependent-test, and Stage 25 correction hashes matched the
local working tree before the gate was removed.

The option-on ReleaseOS gate used `arm64;x86_64`, deployment target 11, tests,
packaging, and warnings as errors, with renderer benchmarks, signing, and
crash reporting disabled. The full universal viewer and package passed. The
exact 16 focused CTest routes, all nine attachment cases, all six render-pass
cases, 12 reflection tests, eight artifact-delivery tests, and 57
benchmark-harness unit tests passed.

Both focused archives contain x86_64 and arm64 slices. The two focused test
executables are native arm64 under existing project policy and have valid
ad-hoc signatures. Neither the archives nor tests have direct physical-device
query or render-pass symbols, and the tests have no Vulkan, MoltenVK, OpenGL,
SDL, X11, or Wayland dependency. The option-on app contained exactly the two
canonical production modules listed above. All 367 packaged Mach-O files were
readable and had no Vulkan or MoltenVK linkage.

After a full option-off reconfigure, all 21 enumerated opt-in material targets
were absent while the neutral stale-artifact cleanup target remained. A fresh
universal viewer and package passed, the app contained zero SPIR-V files, and
all 367 Mach-O files remained readable and free of Vulkan and MoltenVK
linkage.

The first disposable configure paired the Xcode generator with a Ninja
make-program override. Its fresh build directory was discarded and recreated
without that mismatch. A direct configure was then rerun from the snapshot
root so its synthetic Git revision was available. These harness corrections
did not change source or weaken a gate.

The macOS gate did not launch the viewer, create a graphics context, call a
driver, use an account, enter a region, compare pixels, run a benchmark, or
retain timing. The exact disposable root was revalidated as user-owned and
non-symlinked, deleted, and verified absent.

An independent review found that the first requirements value was publicly
mutable. Its state and construction were made private, a read-only accessor
and type-boundary tests were added, and the final review reported no medium or
high issue.

## Code size

| Stage 27 change, excluding this decision record | Lines added | Lines removed |
| --- | ---: | ---: |
| Attachment resolver API and implementation | 54 | 4 |
| Attachment fake-dispatch tests | 112 | 23 |
| Dependent render-pass fixture | 22 | 4 |
| Stage 25 decision correction | 49 | 18 |
| Total | 237 | 49 |

## Explicit runtime gaps

Stage 27 does not obtain dispatch from `vkGetInstanceProcAddr`, create a Vulkan
instance or logical device, select queues, enable a device feature, create a
pipeline layout or graphics pipeline, allocate images or memory, create views
or a framebuffer, record commands, establish transitions or synchronization,
submit work, present, handle device loss, connect a viewer route, compare
pixels, or measure performance.

Native graphics-pipeline ownership remains a dependent stage. Its device input
must carry authenticated logical-feature enablement, preserve the Vulkan 1.1
pipeline-layout lifetime, consume the resolved slot masks, and handle
partially successful pipeline creation correctly.
