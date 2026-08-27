# Stage 30 Vulkan global-dispatch decision

## Decision

Begin master Stage 3 with one generic, backend-private global-dispatch
generation. The resolver accepts a caller-owned `vkGetInstanceProcAddr`,
resolves the Vulkan global command set with a null instance, queries the
loader's standard API version, and returns either a typed failure or an
immutable value containing the exact accepted function pointers and version.

The existing offscreen material diagnostic is the first real consumer. Its
layer and extension enumeration, API qualification, instance creation, and
debug-messenger lookup use the accepted generation. The diagnostic retains its
existing native device, render, validation, and cleanup path.

The generic archive and focused fake-dispatch test are available through a new
default-off `LL_VULKAN_RUNTIME_TEST` option. The existing
`LL_VULKAN_TONEMAP_TEST` option also builds them and connects the material
diagnostic to the archive. Neither option changes the default viewer backend.

Stage 30 does not declare master Stage 2 complete. Production packet routing,
the direct-OpenGL allowlist, stable-resource pinning, reusable staging and
descriptor allocation, completion-backed native destruction, real cold and
warm cache behavior, and default CI coverage for the shader chain remain open.
Several of those exits need a real device, queue, pipeline, and completion
source, so the plan now interleaves their Stage 3 prerequisites instead of
adding another disconnected native-object transaction.

## Why instance and surface ownership wait

The current window abstraction has no Vulkan WSI request. Linux creates an SDL
OpenGL window and context, macOS installs an OpenGL view and context, and
Windows creates a WGL path. None returns an owned list of the Vulkan instance
extensions required by the selected native window or creates a Vulkan surface
for that exact window.

An instance generation must enable that platform-derived extension set at
creation. Hardcoding X11 or Wayland extensions would bypass SDL's active video
driver; creating a macOS instance before a Metal-backed layer exists would not
produce a usable surface path; and the Windows route must bind the real
`HWND`. Creating an instance first and discovering its WSI requirements later
would make the supposedly immutable generation a dead end.

The global command set and loader API version precede those platform choices.
They are also already duplicated by the native diagnostics. Stage 30 can
therefore establish a shared runtime dependency and prove it through a real
consumer without guessing the next platform contract.

## Loader-lifetime boundary

`VulkanGlobalDispatchGeneration` owns no operating-system library. The caller
owns the loader implementation behind `PFN_vkGetInstanceProcAddr` and keeps it
loaded while the generation or any object created through its functions can be
used. The value does not unload a library and does not claim that a loader was
found through any particular platform mechanism.

Only its resolver can construct an accepted generation. The value exposes:

- the exact caller-provided `PFN_vkGetInstanceProcAddr`;
- the resolved `PFN_vkCreateInstance`;
- the resolved `PFN_vkEnumerateInstanceExtensionProperties`;
- the resolved `PFN_vkEnumerateInstanceLayerProperties`;
- the resolved `PFN_vkEnumerateInstanceVersion`; and
- the exact accepted packed loader API version.

The value has no public default constructor or mutator. Copy and move
construction preserve function identity; assignment is unavailable, so an
accepted generation cannot be rewritten after construction.

It owns no instance, debug messenger, extension or layer storage, surface,
physical device, logical device, queue, allocator, or native object. It also
does not accept a portability flag. Portability enumeration is an instance
extension decision derived from actual extension enumeration, while the
portability subset remains a conditional device-extension obligation recorded
by Stage 28.

## Exact global resolution

The resolver first rejects a null `PFN_vkGetInstanceProcAddr` without making a
call. It then invokes the supplied resolver with `VK_NULL_HANDLE` and these
exact names in order:

1. `vkCreateInstance`;
2. `vkEnumerateInstanceExtensionProperties`;
3. `vkEnumerateInstanceLayerProperties`; and
4. `vkEnumerateInstanceVersion`.

The first three functions are required Vulkan 1.0 global commands. Resolution
stops at the first missing required command and identifies it in the typed
error. The version query is optional in Vulkan 1.0. If it is absent, the
resolver applies Vulkan's defined 1.0 loader fallback and rejects that version
against this renderer's established Vulkan 1.1 floor.

When `vkEnumerateInstanceVersion` exists, the resolver calls it exactly once.
A failed `VkResult` is preserved and the output value is ignored, including if
a faulty or fake implementation wrote poison into it. A successful query with
a nonzero API variant is rejected before the standard version comparison. A
standard version below Vulkan 1.1 is also rejected. Vulkan 1.1 and newer
standard versions are retained exactly.

The service does not enumerate instance extensions or layers. Those APIs can
return variable data and are policy inputs to the later instance transaction.
Stage 30 authenticates their function identities, not any extension, layer,
driver, device, or WSI capability.

## Typed failures

`VulkanGlobalDispatchResolutionError` distinguishes:

- a null `vkGetInstanceProcAddr`;
- a missing required global command, with exact command identity;
- a failed version query, with its exact `VkResult`;
- a successful nonstandard API variant; and
- a standard loader API version below Vulkan 1.1.

The error carries the relevant command and a defined available version only
when those facts exist. It never promotes the output from a failed version
query into evidence.

## Material diagnostic integration

`VulkanMaterialRun::createInstance()` resolves one generation before it
enumerates layers or extensions. The accepted value is retained for the run.
The diagnostic then uses it for both enumeration functions and
`vkCreateInstance`, eliminating those direct global calls and the duplicate
loader-version query.

The same retained `vkGetInstanceProcAddr` resolves debug-messenger creation and
destruction with the valid instance. The diagnostic continues to call its
existing directly linked instance- and device-level Vulkan functions. Moving
those calls behind an owned instance/device dispatch generation belongs to
later stages and is not implied here.

The integration preserves the diagnostic's established validation layer,
debug-utils extension, conditional portability-enumeration extension and
flag, physical-device selection, conditional portability-subset device
extension, render fixture, rejection cases, submission, artifact format, and
cleanup ordering.

## Build boundary

`LL_VULKAN_RUNTIME_TEST` defaults off and requires tests. It locates Vulkan
headers, builds the generic archive and focused integration executable with
`VK_NO_PROTOTYPES`, and does not link either target to a Vulkan loader. It is
not restricted to Linux or macOS, so the fake suite has a Windows compilation
path independent of the native diagnostics.

`LL_VULKAN_TONEMAP_TEST` remains the stronger Linux/macOS-only diagnostic
boundary. It implies the generic targets, retains its existing shader tools,
validation-layer requirements, native loader linkage, and macOS MoltenVK
packaging, and links only `llvulkanmaterial` to the new archive.

The new archive does not enter `llrender`, `llwindow`, the viewer, or the
neutral renderer contract. Both options off must remove every Stage 30 target
and preserve the viewer's existing Vulkan-free dependency boundary.

## Focused tests and review

The fake-dispatch suite covers construction traits, a null resolver, each
missing required global, the Vulkan 1.0 fallback, a failed version query with
poisoned output, nonstandard variants, versions below the renderer floor,
exact Vulkan 1.1 and newer versions, pointer identity, and exact lookup order
and counts. All six focused cases pass on Linux and macOS.

Independent API and build-boundary reviews found and resolved two medium and
three low issues before the final gates. Failed instance and debug-messenger
creation now publish handles only after success and reject success with a null
handle, so poisoned failure outputs cannot enter cleanup. Unrelated formatting
churn was removed. Nonstandard API variants receive a distinct diagnosis, the
new option describes its runtime-dispatch scope, and rejection tests assert
exact lookup counts. Final review found no unresolved high, medium, or low
issue.

## Linux evidence

Warnings-as-errors Release builds pass for the viewer, production shader
chain, generic dispatch archive and fake test, and native material diagnostic.
The runtime-only option builds the generic archive, fake test, and viewer
without the native diagnostic. With both Vulkan options off, the viewer still
builds and every opt-in target is absent.

All 19 focused renderer CTest routes pass, as do the 12 reflection tests, eight
artifact-delivery tests, and 57 benchmark-harness unit tests. A native RADV
material run passes all 832 component checks and 39 rejection cases with one
recording, one submission, zero validation messages, conditional portability
enumeration enabled, and no retained artifact. No benchmark or timing was run
or retained.

The generic archive and fake executable have no direct Vulkan symbol or loader
dependency. Among the migrated globals, the native diagnostic imports only
`vkGetInstanceProcAddr`; the viewer imports no Vulkan symbol and has no Vulkan
or MoltenVK dependency. The remaining diagnostic instance- and device-level
imports are explicitly deferred.

## macOS evidence

A warnings-as-errors universal Release build and package pass with both Vulkan
options enabled. The generic archive contains arm64 and x86_64 slices; the
focused executable follows the project's arm64 test policy, verifies its code
signature, and has no Vulkan or MoltenVK dependency. The same 19 focused CTest
routes and the 12, eight, and 57 Python test groups pass.

An arm64 MoltenVK material run passes all 832 component checks and 39 rejection
cases with one recording, one submission, zero validation messages,
portability enumeration enabled, portability subset enabled, and no retained
artifact. The enabled package contains exactly the two canonical production
SPIR-V files. All 367 packaged Mach-O files are readable and none depends on
Vulkan or MoltenVK.

Reconfiguring the same disposable source with both Vulkan options disabled
reduces the build dependency graph from 206 to 168 and removes all 38 opt-in
Xcode targets. The universal build and package pass again, the package contains
zero SPIR-V files, and the same 367 Mach-O files remain readable and free of
Vulkan or MoltenVK dependencies. The six compiled Stage 30 inputs still match
the local candidate byte for byte. No viewer, account, region, benchmark, or
timing path was used, and the disposable test tree was removed.

## Windows evidence boundary

The generic option and target graph are designed to compile and run the fake
suite on Windows without importing `vulkan-1.dll`. No Windows host is available
in this stage. Stage 30 therefore makes no claim about an installed Windows
loader, driver, instance, window surface, presentation queue, or runtime
behavior.

## Production artifact hashes

The enabled Linux and macOS builds reproduce the canonical production shader
artifacts:

- vertex: 8,068 bytes,
  `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095`;
- fragment: 7,824 bytes,
  `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb`.

## Candidate source hashes

The final compiled and build-wiring inputs have these SHA-256 identities:

- `indra/cmake/Variables.cmake`:
  `d3fd8713e96badf5c049fdaa0ef1ba1f27c4d2e0d2f8bcf1281f409626fd0062`;
- `indra/llrender/CMakeLists.txt`:
  `097b8aa16590a036d9be7b42f722a0a83a64106a2b7ed9b6bd20f7d72177ab73`;
- `indra/llrender/vulkan/llvulkanmaterial_main.cpp`:
  `850917c6586ac63b5712ad1f6d5ce9cabd2447b810d2db104807e4b1d7ed9e54`;
- `indra/llrender/tests/llrendervulkanglobaldispatch_test.cpp`:
  `4998ddd01abe85c5601f3cfff5936c193e2f817212c30afd700230503f49f21f`;
- `indra/llrender/vulkan/llrendervulkanglobaldispatch.cpp`:
  `bc0a71877dae48f555735c1d44647fb3b6797f65e63fed2de5bcab6592383e05`;
- `indra/llrender/vulkan/llrendervulkanglobaldispatch.h`:
  `b6d6e80f65bccec743a6529906408e26fca8ee3ae7013a26b8bfa25990013b91`.

The decision record is excluded because including its own digest would be
self-referential.

## Code size

The final patch adds 945 lines and removes 27 lines across seven files: 659
additions and 27 deletions are compiled or build-wiring inputs, while 286
additions are this decision record.

## Commit boundary

The generic dispatch header and implementation, its focused test, the material
diagnostic integration, adjacent build option and wiring, and this decision
record form one Stage 30 commit whose parent is
`8dc04bb34d8a9ab934a3d7932afbacc82184fe5e`. The rolling PostPlan and unrelated
user files are outside the commit boundary.

## Explicit runtime gaps

Stage 30 does not dynamically load or unload Vulkan. It does not own an
instance or debug messenger, define typed OpenGL/Vulkan/headless window
selection, request platform WSI extensions, create a window or surface, select
graphics and presentation queues, or create a logical device.

It does not connect the Stage 28 capability profile to a device-create
transaction or create the Stage 29 cache on a real owned device. It creates no
new shader module, layout, descriptor resource, render pass, pipeline, buffer,
image, memory, sampler, framebuffer, command pool, command buffer, semaphore,
fence, or swapchain.

It adds no viewer backend route, pass allowlist, direct-OpenGL lint, resource
resolver, staging ring, descriptor arena, completion-backed native retirement,
cache persistence, warm-cache test, login, region, pixel comparison, timing,
benchmark, parity result, performance conclusion, or default-backend claim.
