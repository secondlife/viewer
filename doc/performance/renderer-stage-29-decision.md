# Stage 29 Vulkan material pipeline-cache decision

## Decision

Accept one opt-in, backend-private owner for an empty in-memory
`VkPipelineCache`. The factory accepts one exact borrowed logical device and
injected create and destroy callbacks. It returns either one typed error or one
unique owner for a non-null cache handle.

This is the fourteenth committable slice of master Stage 2. The library and its
focused integration test remain behind `LL_VULKAN_TONEMAP_TEST`, use injected
dispatch with `VK_NO_PROTOTYPES`, and do not link a Vulkan loader or expose
Vulkan through the neutral renderer contract.

The original renderer-modernization plan places pipeline caches in master
Stage 2 and instance, surface, and logical-device ownership in master Stage 3.
Stage 29 follows that boundary. It defines ownership for one native Vulkan
object type without creating it on a real driver or claiming that the supplied
device satisfies the Stage 28 material-pipeline requirements.

## Why the graphics pipeline waits

Stage 28 proved physical support and retained the obligations for a future
logical device:

- enable `independentBlend`;
- create at least one graphics-capable queue; and
- enable `VK_KHR_portability_subset` when the physical device advertises it.

A raw `VkDevice` does not reveal the `VkDeviceCreateInfo` that created it.
Wrapping the handle with caller-provided booleans would rename the assumption,
not authenticate it. A native graphics-pipeline owner must instead consume a
view minted by the owner of the exact device-create transaction.

That transaction remains in master Stage 3. Its instance owner must retain the
exact standard Vulkan 1.1-or-newer `VkApplicationInfo::apiVersion`. Its
WSI-aware device owner must choose queues against a real surface, enable the
Stage 28 feature and extension obligations, and retain the exact device
generation. Presentation support cannot be selected before the surface exists.

An empty pipeline cache has none of those requirements. Core
[`vkCreatePipelineCache`](https://docs.vulkan.org/refpages/latest/refpages/source/vkCreatePipelineCache.html)
accepts a cache with no initial data. Flags zero require no optional feature,
and cache creation itself needs no graphics queue. The cache can therefore be
owned now without weakening the later pipeline boundary or pulling Stage 3
runtime ownership forward.

## Borrowed logical-device boundary

`MaterialPipelineCacheDevice` contains one `VkDevice` and a
`MaterialPipelineCacheDispatch` with exactly:

- `PFN_vkCreatePipelineCache`; and
- `PFN_vkDestroyPipelineCache`.

The factory rejects a null device or either missing callback before allocation
or a native call. It does not accept a physical device, capability profile,
queue, loader, instance, or global dispatch table.

The caller owns the logical device and the implementation addressed by the two
callbacks. Both remain valid until the returned owner is destroyed. The caller
also prevents destruction while another host operation uses the cache. This is
particularly important once a later stage passes the cache to pipeline
creation. Vulkan's
[`vkDestroyPipelineCache`](https://docs.vulkan.org/refpages/latest/refpages/source/vkDestroyPipelineCache.html)
contract requires host access to the cache to be externally synchronized.

`MaterialPipelineCache` is noncopyable and nonmovable. It exposes the
borrowed `VkPipelineCache` handle by value and answers whether it was
`createdOn(VkDevice)`. It retains only the creating device, destroy callback,
and cache handle. It stores no pointer into temporary create information.

The owner is intentionally device-wide rather than bound to one Stage 28
target profile. A future graphics-pipeline owner will compose the cache with an
authenticated device generation, the exact Stage 28 capability profile, shader
modules, pipeline layout, and render pass. The cache does not claim those
objects or facts itself.

## Exact cold creation

The factory submits one fixed
[`VkPipelineCacheCreateInfo`](https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCacheCreateInfo.html):

- `sType` is `VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO`;
- `pNext` is null;
- `flags` is zero;
- `initialDataSize` is zero; and
- `pInitialData` is null.

The call also passes null allocation callbacks. No extension structure,
initial cache bytes, application allocator, or optional synchronization flag is
accepted through the public API.

Flags zero leave pipeline-cache access under Vulkan's default synchronization
behavior for later pipeline-creation calls. Stage 29 does not choose an
external-synchronization mode or define concurrent pipeline compilation. The
owner's lifetime rule still requires the caller to prevent cache destruction
during any use.

## Transaction and destruction

The factory allocates the C++ owner with `new (std::nothrow)` before calling
Vulkan. This order prevents a successful native cache from being stranded by a
later owner-allocation failure. Allocation failure returns its own typed error
and performs no native call.

After allocation, the factory calls the injected create callback once with the
exact borrowed device, fixed create info, null allocation callbacks, and a local
output handle.

Only `VK_SUCCESS` plus a non-null output establishes ownership. A failed
`VkResult` is preserved in the typed error. Any output bits written by a failed
callback are ignored and never destroyed because failure does not publish
ownership. `VK_SUCCESS` with a null output returns a separate typed error.

Once success is validated, the factory stores the handle in the preallocated
owner and returns it. Destruction calls the captured destroy callback exactly
once with the creating device, owned cache, and null allocation callbacks.
Independent successful owners retain independent destruction obligations even
when their opaque cache-handle values compare equal.

The result distinguishes:

- invalid logical device;
- incomplete dispatch;
- owner-allocation failure;
- native creation failure with the exact `VkResult`; and
- native success with a null cache handle.

No partial owner or native handle escapes a failed transaction.

## Cold-cache limit

Stage 29 proves only cold creation and ownership. It does not call
[`vkGetPipelineCacheData`](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPipelineCacheData.html),
accept initial data, export or import bytes, validate cache headers or UUIDs,
merge caches, write files, or define invalidation policy.

It also creates no pipeline that could populate the cache. Fake dispatch can
prove create-info shape, failure handling, and lifetime, but it cannot prove a
driver cache hit, a warm reopen, reduced compilation work, or a performance
benefit. The master Stage 2 exit gate for tested cold and warm behavior remains
open until a real pipeline consumer and cache-data policy exist.

## Focused tests and review

Six focused fake-dispatch cases cover:

- owner and result copy, move, and destruction traits;
- the public factory's `noexcept` boundary;
- a null logical device and each missing callback, all with zero native calls;
- every fixed create-info field, the exact logical device, and null allocation
  callbacks;
- a stable borrowed handle and exact `createdOn` answers after success;
- native failure with the exact result and a poisoned non-null output that is
  not adopted or destroyed;
- native success with a null output;
- exact destruction of a successful owner; and
- independent owners that deliberately reuse one opaque handle value and each
  destroy exactly once.

The test also verifies that no hidden singleton, generation registry,
pipeline creation, export callback, or global dispatch path exists.
Owner-allocation failure was structurally reviewed through the nothrow
allocation and pre-call ordering. The stage did not add an allocator seam or
force a destructive real out-of-memory test.

Independent implementation, test, gate, and adversarial reviews covered API
shape, failure-output ownership, exact create info, destruction, host
synchronization, CMake isolation, and test completeness. Review found no high-
or medium-severity issue. It found two low precision gaps, both resolved before
the platform gate: the result's copy, move, and destruction traits now have
explicit assertions, and the header limits its default internal-synchronization
claim to later pipeline-creation calls while retaining external requirements
for destruction and other host operations.

**Gate result:** Accepted. No unresolved review finding remains.

## Linux evidence

The Release cache enabled tests and `LL_VULKAN_TONEMAP_TEST`, disabled both
compiler warning opt-outs, and built the viewer, production shader chain,
pipeline-cache archive, focused executable, and adjacent material targets. All
six cache cases and the exact 18 focused CTest routes passed. The 12 reflection,
eight artifact-delivery, and 57 benchmark-harness unit tests also passed. The
last suite exercised only Python harness logic and launched no benchmark.

Formatting, diff, five-file source-boundary, path/privacy, backend-include, and
final source inspections passed. Neither the cache archive nor its executable
contains a direct `vkCreatePipelineCache` or `vkDestroyPipelineCache` symbol.
The focused executable has no Vulkan, MoltenVK, OpenGL, SDL, window-system, or
viewer dynamic dependency, and the viewer has no Vulkan or MoltenVK dependency.

With the option disabled, all 25 enumerated opt-in targets were absent while
the neutral stale-artifact cleanup target remained. The Release viewer rebuilt
without Vulkan or MoltenVK linkage. The cache was restored to option-on, the
new targets returned, and the exact 18 focused routes passed again. This Linux
tree has packaging disabled, so it is not package evidence.

**Linux result:** Passed.

No Linux evidence may be described as a driver or cache-behavior test. The
focused path uses injected callbacks only.

## macOS evidence

A fresh user-owned disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK
26.5 contained the Stage 28 commit plus exactly the five planned Stage 29 files.
It had exactly two synthetic commits and the intended five-file delta. The four
compiled source and build-input hashes matched the local candidate before
configuration, after both modes, and immediately before deletion. This decision
record was completed locally afterward and is not included in that frozen-input
claim.

The option-on ReleaseOS gate used deployment target 11, `arm64;x86_64`, tests,
packaging, and warnings as errors, with renderer benchmarks, signing, crash
reporting, and proprietary components disabled. The full 204-target universal
build and package passed, as did the explicit viewer, package, production
shader, cache archive, and cache-test targets. All six cache cases, the exact 18
focused CTest routes, 12 reflection tests, eight artifact-delivery tests, and 57
benchmark-harness unit tests passed.

The cache archive contains both architecture slices. Its focused executable is
native arm64 under existing project policy, has a valid signature, contains no
direct cache create or destroy symbol, and has no Vulkan, MoltenVK, OpenGL, SDL,
X11, Wayland, or viewer dependency. The universal app contained exactly two
production SPIR-V files, byte-identical to the build outputs and carrying the
canonical hashes below. All 367 packaged Mach-O files were readable and free of
Vulkan or MoltenVK linkage.

After option-off reconfiguration, the full 168-target universal build and
package passed. All 25 opt-in targets were absent, the neutral cleanup target
remained, the app contained zero SPIR-V files, and all 367 Mach-O files remained
readable and dependency-clean. The exact disposable root was revalidated as
user-owned and non-symlinked, deleted, and independently verified absent.

**macOS result:** Passed.

The macOS gate did not launch the viewer or a sample, call a GPU or Vulkan
driver, create a real cache, use an account or region, compare pixels, run a
benchmark, or retain timing.

## Production artifact hashes

The gate confirmed that Stage 29 does not change the two accepted production
modules.

| Module | Expected SHA-256 | Gate observation |
| --- | --- | --- |
| Vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` | Exact match |
| Fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` | Exact match |

## Candidate source hashes

These are the final hashes used by the disposable platform gate after the
source and build wiring were frozen.

| File | SHA-256 |
| --- | --- |
| `indra/llrender/CMakeLists.txt` | `e5733266e7774094bd37a63b2c986ac236df28224e3eb6f7f50b4391bb508b54` |
| Pipeline-cache header | `4b0535752befb5af5006c46d1c97d0b7cb27a23191443ef6926d70b6001efa62` |
| Pipeline-cache implementation | `33a8513b7e72e82d2c7f998d72de34acbbc8ee2eb4c126fa01d7821aa0b28190` |
| Fake-dispatch test | `c5c2d4d778e9c86e78d8800fc64896e43251ee808670f1e314ac2726c626ee3e` |

## Code size

This table records the final Stage 29 diff, excluding this decision record.

| Stage 29 change | Lines added | Lines removed |
| --- | ---: | ---: |
| Pipeline-cache backend header and implementation | 187 | 0 |
| Fake-dispatch pipeline-cache tests | 278 | 0 |
| Opt-in build wiring | 22 | 0 |
| Total | 487 | 0 |

## Commit boundary

The pipeline-cache header and implementation, its focused test, adjacent build
wiring, and this decision record form one Stage 29 commit whose parent is
`8ffed4368e0623dbc96cc18967885ab85074ed4f`. Unrelated user files are not part
of the boundary.

## Explicit runtime gaps

Stage 29 does not load Vulkan, obtain global or instance dispatch, create or own
an instance, debug messenger, surface, physical device, logical device, queue,
shader module, descriptor layout, pipeline layout, descriptor pool, render
pass, graphics pipeline, buffer, image, memory, view, sampler, framebuffer,
command pool, command buffer, semaphore, fence, or swapchain.

It does not authenticate the instance API floor, the physical-to-logical-device
relationship, enabled features or extensions, or created queue families. It
does not select presentation support, enable the portability subset, or make
the Stage 28 material profile executable.

It does not export, import, persist, invalidate, merge, populate, or measure a
pipeline cache. It defines no cache-key, variant-generation, cache-hit,
concurrent-creation, completion, or device-loss policy. It records no commands,
submits no work, presents no image, connects no viewer call site, selects no
runtime backend, compares no pixels, and collects no timing.

The cache is one cold device-owned object, not a working Vulkan renderer. A
native material graphics pipeline remains dependent on the future authenticated
Stage 3 instance and logical-device generation, the Stage 28 capability
profile, same-device shader modules, pipeline layout and render pass, and this
cache.
