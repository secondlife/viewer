# Stage 28 Vulkan material pipeline-capability decision

## Decision

Accept one opt-in, backend-private resolver that combines an immutable Stage 27
attachment profile with the remaining physical-device requirements for the
canonical production material pipeline. The resolver returns either one typed
error or one immutable `LegacyNormSpecPipelineCapabilityProfile` selected for
the same `VkPhysicalDevice` as its retained attachment profile.

This is the thirteenth committable slice of master Stage 2. It closes the
physical shader and pipeline-capability contract before native graphics-pipeline
creation. The library and its focused integration test remain behind
`LL_VULKAN_TONEMAP_TEST`, use injected dispatch with `VK_NO_PROTOTYPES`, and do
not link a Vulkan loader or expose Vulkan through the neutral renderer contract.

The original renderer-modernization plan places a portability-aware Vulkan
prototype in master Stage 2. This slice stays within that boundary. It does not
pull master Stage 3 instance, surface, WSI-aware device, swapchain, or whole-frame
ownership forward. The invalid benchmark baseline remains paused without a
performance conclusion.

## Separate composed profile

Stage 25 owns attachment formats, image roles, capabilities, clears, color write
masks, and alpha semantics. Stage 27 added the `independentBlend` requirement
caused by those unequal write masks. Stage 28 does not add unrelated queue,
extension, or vertex policy to that attachment resolver.

Instead, the new resolver accepts:

- a non-null borrowed `VkPhysicalDevice`;
- one already resolved `LegacyNormSpecAttachmentProfile`; and
- injected physical properties, queue-family, device-extension, and properties2
  callbacks.

It rejects an attachment profile selected for another physical device before
calling any callback. A successful profile retains a copy of the exact
attachment profile, the accepted Vulkan API version, seven canonical vertex
records, the portability stride alignment, and read-only logical-device
requirements. Callers cannot combine attachment facts from one physical device
with pipeline facts from another.

The profile is private to the resolver's construction path. It is neither an
aggregate nor publicly default-constructible. Its accessors return read-only
state or immutable copies, so callers cannot turn a successful result into an
unsupported configuration.

## Vulkan 1.1 core contract

The resolver queries `VkPhysicalDeviceProperties::apiVersion`, rejects a
nonzero API variant, and then rejects a standard Vulkan device below 1.1. A raw
packed-version comparison is not sufficient because the variant occupies the
highest bits. This proves the physical device advertises the required floor; it
does not prove that a parent instance requested Vulkan 1.1. The future instance
owner must authenticate that separate creation input. The floor matches the
production modules, which target SPIR-V 1.3. Vulkan 1.1 requires support for
SPIR-V 1.3 and earlier, as described in the official
[Vulkan versions guide](https://docs.vulkan.org/guide/latest/versions.html#_spir_v).

The accepted API floor also supplies fixed minimum limits. Stage 28 does not
turn those guarantees into redundant driver queries. Focused agreement tests
bind the retained records to the production shader manifest and the existing
pipeline-layout contract. The relevant Vulkan 1.1 guarantees are documented in
the [physical-device limits table](https://docs.vulkan.org/spec/latest/chapters/limits.html#limits-minmax).

The canonical vertex state is:

| Semantic | Location | Binding | Offset | Stride | Native Vulkan format |
| --- | ---: | ---: | ---: | ---: | --- |
| Position | 0 | 0 | 0 | 16 | `VK_FORMAT_R32G32B32_SFLOAT` |
| Normal | 1 | 1 | 0 | 16 | `VK_FORMAT_R32G32B32_SFLOAT` |
| Texture coordinate 0 | 2 | 2 | 0 | 8 | `VK_FORMAT_R32G32_SFLOAT` |
| Color | 3 | 3 | 0 | 4 | `VK_FORMAT_R8G8B8A8_UNORM` |
| Tangent | 4 | 4 | 0 | 16 | `VK_FORMAT_R32G32B32A32_SFLOAT` |
| Texture coordinate 1 | 5 | 5 | 0 | 8 | `VK_FORMAT_R32G32_SFLOAT` |
| Texture coordinate 2 | 6 | 6 | 0 | 8 | `VK_FORMAT_R32G32_SFLOAT` |

These seven attributes and bindings fit the Vulkan 1.1 core minima. Their
largest offset is zero and their largest stride is 16. The four formats have
mandatory `VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT` support in the
[required format-support table](https://docs.vulkan.org/spec/latest/chapters/formats.html#formats-required-features).
The resolver therefore does not query vertex-format properties.

The production contract also stays below the Vulkan 1.1 minimums for:

- eight interstage locations carrying 20 components;
- two descriptor sets;
- one 272-byte uniform block visible to vertex and fragment stages;
- three fragment-stage combined image samplers;
- four fragment outputs and four combined fragment-output resources; and
- one viewport, one sample, and no push constants.

The accepted SPIR-V modules declare only the `Shader` capability. They do not
require optional geometry, tessellation, numeric-width, subgroup, storage,
atomic, sample-rate, clip-distance, or cull-distance features. Triangle-list
topology, fill mode, back-face culling, counter-clockwise front faces, depth
test and write with `LESS_OR_EQUAL`, disabled stencil and logic operations,
line width one, and dynamic viewport and scissor also need no optional feature.

`independentBlend` remains the one optional core feature required by the fixed
pipeline. Slots 0 through 2 use RGBA write masks while slot 3 uses RGB. Vulkan
requires identical color-blend attachment states unless the logical device
enables [`independentBlend`](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFeatures.html).
Stage 28 copies that established Stage 27 obligation. It does not query the
feature again.

## Variable physical-device queries

After input and dispatch preflight, the resolver performs a fixed transaction:

1. query physical-device properties, reject nonstandard variants, and reject a
   standard API version below 1.1;
2. enumerate queue-family properties and require one non-empty family with
   `VK_QUEUE_GRAPHICS_BIT`;
3. enumerate device extensions with a null layer name and detect only the full
   `VK_KHR_portability_subset` name; and
4. if that extension is advertised, query its stride-alignment property through
   `vkGetPhysicalDeviceProperties2`.

[`vkCreateGraphicsPipelines`](https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateGraphicsPipelines.html)
requires its device to support at least one graphics-capable queue family. The
profile records that requirement and the resolver proves that such a family
exists. It does not retain a queue-family index. Presentation support depends on
a future surface, so a later WSI-aware device owner must choose and authenticate
the actual family used to create the logical device.

Queue-family and extension enumeration use checked, nonthrowing scratch
allocation. A returned count that cannot fit the allocation size fails before
allocation, allocation failure has a typed result, and a second queue-family
query cannot publish more records than the first query provided room for.

Device-extension enumeration retries the complete count-and-list transaction at
most three times when either call returns `VK_INCOMPLETE`. It preserves the
failing `VkResult` and attempt number, rejects a returned list count above its
allocated capacity, and fails closed after the retry bound. The public resolver
remains `noexcept`; no partial profile escapes any query, enumeration, or
allocation failure. The Vulkan enumeration rules are documented with
[`vkEnumerateDeviceExtensionProperties`](https://docs.vulkan.org/refpages/latest/refpages/source/vkEnumerateDeviceExtensionProperties.html).

## Portability-subset policy

The [`VK_KHR_portability_subset`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_portability_subset.html)
device extension changes which otherwise-core behavior a portability
implementation must provide. If a physical device advertises the extension, a
future `VkDeviceCreateInfo` must enable it. This is a Vulkan valid-usage rule,
not an optional viewer preference. The rule appears in the
[`VkDeviceCreateInfo` valid usage](https://docs.vulkan.org/refpages/latest/refpages/source/VkDeviceCreateInfo.html).

When the extension is absent, the profile records no extension obligation and
uses the neutral stride alignment value one. It does not interpret
portability-only properties. When the extension is present, the resolver chains
`VkPhysicalDevicePortabilitySubsetPropertiesKHR` into a Vulkan 1.1
`VkPhysicalDeviceProperties2` query and retains
`minVertexInputBindingStrideAlignment`.

The reported alignment must be a nonzero power of two. Every canonical stride,
`16, 16, 8, 4, 16, 8, 8`, must be at least the alignment and evenly divisible by
it. A mismatch reports the first offending binding and its stride. This enforces
the portability valid-usage rule for
[`VkVertexInputBindingDescription`](https://docs.vulkan.org/refpages/latest/refpages/source/VkVertexInputBindingDescription.html)
before a native pipeline can consume the profile. The four-byte color stride is
the tightest accepted record.

Stage 28 requires no member of
[`VkPhysicalDevicePortabilitySubsetFeaturesKHR`](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDevicePortabilitySubsetFeaturesKHR.html)
to be true. In particular, it does not require or enable
`vertexAttributeAccessBeyondStride`. Every attribute starts at offset zero, and
the format sizes fit their strides: `12/16, 12/16, 8/8, 4/4, 16/16, 8/8, 8/8`.
The fixed pipeline also avoids constant-alpha blend factors, triangle fans,
point polygon mode, stencil state that needs separate masks or references,
sample-rate interpolation functions, and tessellation modes.

Portability features concerning events, image-view reinterpretation or swizzle,
2D views of 3D images, multisampled array images, comparison samplers, and
sampler LOD bias belong to later resource, sampler, and command owners. Those
owners must choose states valid when the corresponding portability feature is
false. Stage 28 neither queries nor enables unused feature bits.

`VK_KHR_portability_enumeration` and
`VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` may be needed for a future
instance owner to expose a MoltenVK physical device. They precede this resolver
and remain outside this stage. Stage 28 can assess only a physical device that a
caller has already enumerated.

## Physical support and future device authentication

A successful Stage 28 profile proves physical support at query time. It also
publishes the requirements a future logical-device transaction must satisfy:

- enable the retained `independentBlend` core feature;
- create at least one queue from a graphics-capable family; and
- enable `VK_KHR_portability_subset` if the profile records that it was
  advertised.

This is not proof that an existing `VkDevice` enabled any of them. Vulkan does
not provide a query that reconstructs an arbitrary device's creation state.
Only the future owner of the exact `VkDeviceCreateInfo` can publish an
authenticated enabled-feature, enabled-extension, and created-queue view. A
later graphics-pipeline owner must consume that view, the retained capability
profile, shader modules, pipeline layout, and render pass from the same device
generation.

The future instance owner must likewise prove that its exact
`VkApplicationInfo::apiVersion` selected standard Vulkan 1.1 or newer. A
physical device advertising 1.1 cannot raise an instance created against a
lower API ceiling.

## Focused tests and review

Nine new fake-dispatch cases cover result traits; every input and dispatch
preflight; physical provenance; nonzero API variants and Vulkan 1.0 rejection;
both production attachment-profile successes; the retained manifest, core
floor, vertex records, and logical-device requirements; graphics-queue absence,
growth, and success; bounded extension enumeration and every failure class;
exact and decoy portability-extension names; the properties2 chain; and valid,
invalid, and incompatible portability alignments. The nine attachment and six
render-pass cases also pass, for 24 directly dependent C++ cases. The exact
focused set contains 17 CTest routes.

Scratch-allocation failure is structurally reviewed through checked size
arithmetic and `new (std::nothrow)`; the stage does not add a test-only allocator
or attempt a destructive real out-of-memory test.

Independent implementation, fake-dispatch test, gate, and final Vulkan
portability/specification reviews corrected five material issues: raw packed API
comparison now rejects nonzero variants; failed extension enumeration no longer
reads undefined output counts; successful and incomplete list results both
validate capacity before retry or publication; the decision distinguishes a
physical API floor from future instance authentication; and the allocation
claim no longer implies an unperformed out-of-memory test. Final review found no
unresolved medium- or high-severity issue. The low residual is the structurally
reviewed allocation-failure branch on a 64-bit host.

Fake dispatch can prove host-side query formation, ordering, bounded failure,
typed errors, immutable publication, and provenance. It cannot prove that a real
driver reports the faked values, that a logical device enabled the recorded
requirements, or that a native graphics pipeline accepts the later create info.

## Linux evidence

The Release cache enabled tests and `LL_VULKAN_TONEMAP_TEST` and treated project
warnings as errors. The viewer, production shader chain, attachment, capability,
and render-pass archives, and all three focused integration executables built.
All nine new cases and the exact 17 focused CTest routes passed. The 12
reflection, eight artifact-delivery, and 57 benchmark-harness unit tests also
passed. The last suite exercised only Python harness logic; it did not launch a
benchmark.

Formatting, diff, five-file source-boundary, path/privacy, and backend-include
scans passed. Neither the capability archive nor its executable contains a
direct physical-properties, queue-family, device-extension, or properties2
query symbol. The executable has no Vulkan, MoltenVK, OpenGL, SDL,
window-system, or viewer dynamic dependency, and the viewer has no Vulkan or
MoltenVK dependency.

With the option disabled, all 23 enumerated material targets were absent while
the neutral stale-artifact cleanup target remained. The Release viewer rebuilt
without Vulkan or MoltenVK linkage. The cache was restored to option-on and the
17 focused routes passed again. This Linux tree has packaging disabled, so it
is not used as package evidence.

No test loaded Vulkan, queried a driver, launched the viewer, used an account or
region, rendered pixels, ran a benchmark scenario, or retained timing.

## macOS evidence

A user-owned disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5
contained the Stage 27 commit plus exactly the five planned Stage 28 files. It
had exactly two synthetic commits and the intended five-file delta. The four
compiled source and build-input hashes matched before configuration, after both
build modes, and against the local candidate before deletion. This decision
record was then completed locally from the observed gate evidence; it is not a
compiled input.

The option-on ReleaseOS gate used deployment target 11, `arm64;x86_64`, tests,
packaging, and warnings as errors, with renderer benchmarks, signing, crash
reporting, and proprietary components disabled. The full 202-target universal
build and package passed, as did the explicit viewer, package, production shader,
attachment, capability, and render-pass targets. The exact 17 focused CTest
routes, all nine capability cases, 12 reflection tests, eight artifact-delivery
tests, and 57 benchmark-harness unit tests passed.

The attachment, capability, and render-pass archives contain both architecture
slices. Their three focused executables are native arm64 under existing project
policy, have valid signatures, and have no Vulkan, MoltenVK, OpenGL, SDL, X11,
Wayland, or viewer dependency. The capability archive has no direct physical
query symbol. The universal app contained exactly two production SPIR-V files,
byte-identical to the build outputs and carrying the canonical hashes below.
All 367 packaged Mach-O files were readable and free of Vulkan or MoltenVK
linkage.

After option-off reconfiguration, the full 168-target universal build and
package passed. All 23 enumerated opt-in targets were absent, the neutral cleanup
target remained, the app contained zero SPIR-V files, and all 367 Mach-O files
remained readable and dependency-clean.

Two harness-only corrections were needed. The synthetic repository inherited a
global SSH commit-signing preference, so only its stalled disposable commit was
terminated and signing was disabled locally there. Xcode 26 also requires the
input file before `-verify_arch`, so that read-only `lipo` check was rerun with
the accepted argument order. Neither correction changed source or weakened a
gate.

The gate did not launch the viewer or a sample, call a GPU or driver, set a
Vulkan runtime environment, use an account or region, compare pixels, run a
benchmark, or retain timing. The exact disposable root was revalidated as
user-owned and non-symlinked, deleted, and independently verified absent.

## Production artifact hashes

| Module | SHA-256 |
| --- | --- |
| Vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` |
| Fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` |

## Code size

| Stage 28 code, excluding this decision record | Lines added | Lines removed |
| --- | ---: | ---: |
| Capability resolver header and implementation | 534 | 0 |
| Fake-dispatch capability tests | 849 | 0 |
| Opt-in build wiring | 24 | 0 |
| Total | 1,407 | 0 |

## Commit boundary

This decision, the resolver header and implementation, its focused test, and the
adjacent build wiring form one Stage 28 commit whose parent is
`731d46e1ff09380355be299fa4c8be02bdc88f27`. The unrelated user documents are
not part of the boundary.

## Explicit runtime gaps

Stage 28 does not obtain callbacks from `vkGetInstanceProcAddr`, create or own a
Vulkan loader, instance, debug messenger, surface, physical device, logical
device, queue, shader module, descriptor layout, pipeline layout, render pass,
pipeline cache, or graphics pipeline. It does not choose a presentation-capable
queue family or authenticate an existing logical device.

It also does not allocate memory, buffers, images, views, samplers, descriptors,
or framebuffers; validate a concrete render extent or memory size; record render
pass begin or draw commands; transition layouts; establish barriers, semaphores,
or queue-family transfers; submit work; synchronize completion; handle device
loss; present; connect a viewer route; select a runtime backend; compare pixels;
or collect timing.

The profile closes one physical capability boundary. It is not a working Vulkan
renderer. Native graphics-pipeline ownership remains dependent on a future
authenticated logical-device view and on same-device shader-module, layout, and
render-pass lifetimes.
