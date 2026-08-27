# Stage 23 Vulkan material layout transaction decision

## Decision

Accept one opt-in, backend-private transaction that creates and owns the fixed
descriptor-set layouts and pipeline layout required by the canonical legacy
normal/specular material shaders. The factory receives a borrowed `VkDevice`
and exactly the create and destroy entry points for descriptor-set layouts and
pipeline layouts. It creates no loader, device service, descriptor pool or set,
shader module, graphics pipeline, cache, frame resource, viewer call site,
command, context, or GPU work.

This is the eighth committable slice of master Stage 2. The library and its
integration test remain behind `LL_VULKAN_TONEMAP_TEST`. They use Vulkan types
from the headers but do not link the loader or expose Vulkan handles through
the neutral renderer contract.

## Canonical interface

The implementation has one compile-time layout recipe:

- set 0, binding 0 is one uniform buffer visible to vertex and fragment
  stages;
- set 1, bindings 0 through 2 are one combined image sampler each, visible
  only to the fragment stage;
- the pipeline layout contains set 0 followed by set 1; and
- there are no immutable samplers, push constants, extension chains, or
  create flags.

This matches the production material manifest established by the earlier
neutral contract. The manifest is an independent test oracle, not a runtime
input to the Vulkan implementation. Stage 23 therefore cannot admit a
different interface through mutable data and does not add a second schema or
translation path.

## Borrowed native boundary

`MaterialLayoutDevice` copies one logical-device handle and four function
pointers: create and destroy descriptor-set layout, then create and destroy
pipeline layout. The logical device must support Vulkan 1.1 or newer. The
device and implementation addressed by the pointers must outlive every
returned owner, and callers externally synchronize host access.

`LegacyNormSpecPipelineLayout` owns two descriptor-set-layout obligations and
one pipeline-layout obligation. It is noncopyable and nonmovable and transfers
only through `unique_ptr`. Its accessors return borrowed handles that expire
with the owner; the ordered descriptor layout pair is returned by value so a
caller cannot retain a reference into a destroyed owner.

The layout interface is invariant across valid shader byte generations, so it
does not retain a Stage 21 publication lease or a Stage 22 shader-module
generation. Pipeline generations that eventually consume these layouts will
need a separate native-completion retirement policy.

## Transaction and destruction

The factory rejects a null device or any missing function pointer before a
native callback. The nothrow owner allocation also completes before the first
callback. The transaction then creates the parameter descriptor-set layout,
the sampled-image descriptor-set layout, and the pipeline layout in that
order, always with null allocation callbacks.

Callback output remains local until the callback returns `VK_SUCCESS` with a
non-null handle. A failed `VkResult` does not establish ownership of output
bits written by the callback. A successful native result with a null handle
also fails closed. Once accepted, each successful create establishes one
destruction obligation even if two opaque non-dispatchable handles happen to
have equal numeric values.

Failure rolls back only accepted objects in reverse creation order. Final
destruction is pipeline layout, sampled-image descriptor-set layout, then
parameter descriptor-set layout. Result errors distinguish invalid device,
incomplete dispatch, owner-allocation failure, native create failure, and
successful creation with a null handle. Native failures retain the exact
object and `VkResult`.

## Focused tests and review

Nine fake-dispatch cases cover:

- result ordering, `noexcept`, and the owner's copy and move traits;
- null device and each missing entry point with zero native calls;
- exact device, create-info fields, binding types, counts, stages, layout
  order, null allocation callbacks, and agreement with the production
  manifest;
- failed and null parameter, sampled-image, and pipeline layout creation with
  exact rollback;
- poisoned failed-call output, including a sampled-image output that aliases
  the already owned parameter handle;
- equal values from two successful descriptor-layout creates, retaining two
  independent destruction obligations;
- `unique_ptr` transfer and exact reverse final destruction; and
- two independent transactions whose opaque values deliberately repeat,
  proving there is no hidden cache or owner coalescing.

Adversarial review corrected a test that initially inferred owner independence
from unequal opaque handle values. Vulkan does not promise such uniqueness, so
the final test proves independence through distinct owners and exact callback
obligations while deliberately reusing all native values. Review also added
explicit borrowed-handle documentation and the poisoned-output alias case. No
high- or medium-severity issue remains in the reviewed transaction.

Deterministic owner-allocation failure is not fault-injected. The branch is
implemented with `new (std::nothrow)`, precedes every native callback, and has
a specific `OwnerAllocationFailure` result.

## Linux evidence

The Release viewer, production shader validation chain, new library and test,
and all affected targets rebuilt with project warnings treated as errors. All
nine new fake-dispatch cases passed. Thirteen focused CTest routes covering the
draw-packet contract, material diagnostics and parameters, renderer contract,
shader manifest, artifact loader and delivery, publication, shader modules,
layouts, registry, manifest arguments, and viewer adapter passed. All 12
reflection cases and all 57 benchmark-harness unit tests passed. No benchmark
scenario or timing ran.

The new archive and integration executable have no direct
`vkCreateDescriptorSetLayout`, `vkDestroyDescriptorSetLayout`,
`vkCreatePipelineLayout`, or `vkDestroyPipelineLayout` symbol. The executable
has no Vulkan, MoltenVK, OpenGL, X11, Wayland, SDL, window, or viewer dynamic
dependency. `VK_NO_PROTOTYPES` is private to the implementation and test, so an
accidental direct loader call fails at compile time.

With the option disabled, the new layout, module, publication, artifact, and
production-shader targets are absent. The Release viewer remains free of
Vulkan and MoltenVK linkage. Restoring the option restores the exact targets.
Settings and command-line XML parsing, clang-format, whitespace, privacy,
local-path, source-boundary, direct-symbol, and dynamic-linkage checks passed.

## macOS evidence

A disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5 contained
the Stage 22 commit plus exactly the four reviewed Stage 23 implementation
files. Their SHA-256 values matched before and after the gate. The ReleaseOS
build used `arm64;x86_64`, deployment target 11, tests, packaging, and the
opt-in shader path, with renderer benchmarks, signing, and crash reporting
disabled.

The new archive contains both architecture slices; the integration test is
native arm64 by existing project policy. It passed all nine fake-dispatch cases
with warnings treated as errors and valid ad-hoc signing. Neither the archive
nor test contains a direct reference to the four Vulkan entry points, and the
test has no Vulkan, MoltenVK, OpenGL, or SDL dependency. The configured loader
and MoltenVK paths only satisfied the inherited opt-in configuration; fake
dispatch did not invoke either.

The universal viewer, production validation chain, exact artifact-copy route,
and local package actions passed. The same 13 focused routes as Linux, all 12
reflection cases, and all 57 benchmark-harness unit tests passed. The option-on
app contained exactly the two canonical production modules, byte-identical to
the validated build outputs, with these SHA-256 values:

| Module | SHA-256 |
| --- | --- |
| Vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` |
| Fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` |

All 367 packaged Mach-O files were readable and had no Vulkan or MoltenVK
linkage. The scan used file descriptors for helper executables whose final
parenthesized names are otherwise interpreted by `otool` as archive-member
syntax.

After a full option-off reconfigure, every Vulkan implementation and shader
target was absent. The option-neutral artifact-copy target remained so the
manifest could remove stale outputs. The universal viewer and local
copy/package actions passed with an empty Vulkan artifact argument. The app
contained no SPIR-V files, and all 367 packaged Mach-O files remained readable
and free of Vulkan and MoltenVK linkage.

The disposable gate required environment corrections before evidence was
accepted: project build flags had to be present, the manifest needed the
disposable Python environment containing `llsd`, the noninteractive path had
to expose CMake, and the regenerated Xcode option-off build needed its intended
macOS 11 target pinned explicitly. The final source builds passed; none of
these setup failures changed repository source or weakened a check.

The Mac gate did not launch the viewer, set an ICD environment variable, touch
a graphics context, call the new transaction through a real driver, invoke a
GPU, use an account or world asset, compare a pixel, run a benchmark, or retain
timing. The exact disposable source, build, dependency, package, log, evidence,
and Git-metadata root was deleted and verified absent.

## Code size

| Stage 23 code, excluding this decision record | Lines added |
| --- | ---: |
| Backend header and implementation | 288 |
| Fake-dispatch integration tests | 529 |
| Opt-in build wiring | 22 |
| Total | 839 |

No existing source line was removed.

## Explicit runtime gaps

Stage 23 does not create a real Vulkan logical device, obtain dispatch through
`vkGetDeviceProcAddr`, run any layout create through a driver, allocate a
descriptor pool or descriptor set, write descriptors, retain buffer, image,
or sampler resources, consume the Stage 22 shader modules, create or cache a
graphics pipeline, define render-pass compatibility or dynamic rendering,
retire native generations after GPU completion, connect a viewer call site,
select a backend, record or submit commands, present pixels, compare parity, or
measure performance.

The next-stage choice remains open until this stage is committed. Post-commit
reanalysis must decide whether the smallest dependent slice is descriptor
allocation, graphics-pipeline prerequisites, real device dispatch, or another
missing resource-lifetime boundary.
