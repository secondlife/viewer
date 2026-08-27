# Stage 22 Vulkan material shader-module transaction decision

## Decision

Accept one opt-in, backend-private transaction that turns a canonical Stage 21
shader-generation lease into an owned Vulkan vertex and fragment shader-module
pair. The factory receives a borrowed `VkDevice` and exactly
`PFN_vkCreateShaderModule` plus `PFN_vkDestroyShaderModule`. It creates no
loader, device service, pipeline, cache, descriptor, frame resource, viewer
call site, command, context, or GPU work.

This is the seventh committable slice of master Stage 2. The library and its
integration test remain behind `LL_VULKAN_TONEMAP_TEST`. They use Vulkan types
from the headers but neither link the loader nor expose Vulkan handles through
the neutral renderer contract.

## Borrowed native boundary

`ShaderModuleDevice` copies one logical-device handle and the two exact
function pointers. The logical device must support Vulkan 1.1 or newer. The
device and implementation addressed by the pointers must remain valid until
every returned generation is destroyed, and callers externally synchronize
host access.

`ShaderModuleGeneration` owns the logical `ShaderHandle`, the exact immutable
`LoadedShaderProgram` shared storage, and both native module handles. It is
noncopyable and nonmovable and transfers only through `unique_ptr`. It does not
retain the lease or its frame. Shader modules are transient pipeline-creation
inputs, so their lifetime is not a frame-completion or shader-publication
retirement policy.

The error-first result distinguishes invalid device, incomplete dispatch,
invalid lease shape, owner-allocation failure, native create failure, and a
successful native result with a null handle. Native failures retain the exact
stage and `VkResult`.

## Admission and provenance limits

The factory rejects a null device, either missing function pointer, a null
program, zero frame, invalid handle, a handle outside the canonical logical
index, and any program rejected by
`validLegacyNormSpecProductionShaderProgram()`. All bounded validation and the
nothrow owner allocation finish before the first native callback. Once native
creation starts, result construction, rollback, and ownership transfer do not
allocate.

The public lease is an aggregate and handles are relative to one publication
owner. The factory cannot prove that a numerically valid lease came from a
particular `LegacyNormSpecShaderPublication`, that its frame has not completed,
or that its bytes came through the trusted packaged-artifact route. Callers
must supply a lease from the matching live publication before that frame
completes, and its program must originate from the Stage 20 build-validated
artifacts. The runtime validator checks bounded structure and labels; it is not
`spirv-val`, hash-backed provenance, or semantic validation.

## Transaction and destruction

The owner and retained source bytes exist before the first callback. The
factory then creates vertex followed by fragment with an exact zero-initialized
`VkShaderModuleCreateInfo`, exact word pointer and byte count, null extension
chain, zero flags, and null allocation callbacks.

Callback output remains local until the callback returns `VK_SUCCESS` with a
non-null module. A failed `VkResult` does not establish ownership of whatever
bits a callback may have written to the output. A failed vertex therefore
destroys nothing; a failed or null fragment destroys the already owned vertex.
Success owns both modules. Final destruction runs fragment then vertex through
the captured device and destroy pointer, again with null allocation callbacks.

Review corrected two unsafe assumptions in the original Stage 22 draft:

- a non-null value written alongside a failed `VkResult` is not a created
  object and must not be destroyed; and
- non-dispatchable Vulkan handles are not guaranteed to have unique numeric
  values. Two successful creates with equal `VkShaderModule` values still
  create two independent destruction obligations, so the destructor calls
  destroy twice in reverse creation order.

## Focused tests and review

Nine fake-dispatch cases cover:

- result ordering, `noexcept`, and the owner's copy and move traits;
- null device and missing create or destroy dispatch with zero native calls;
- null program, invalid and noncanonical handles, zero frame, wrong program,
  swapped stage, wrong entry point, and malformed words with zero native calls;
- exact device, create-info fields, retained source pointers, byte counts,
  call order, logical handle, native handles, `unique_ptr` transfer, and
  reverse destruction;
- source ownership after supported Stage 21 replacement and logical retirement;
- vertex and fragment native failures, including poisoned output values;
- success with a null vertex or fragment and exact rollback;
- equal numeric module handles with two successful creates and two destroys;
  and
- two independent transactions for one valid lease, proving there is no
  hidden cache or completion policy.

The positive fixtures are minimal SPIR-V 1.3 vertex and fragment modules
compiled for Vulkan 1.1 and checked with `spirv-val`. They make fake call
formation internally coherent but do not establish production artifact
provenance or real-driver acceptance.

Adversarial review found and closed an invalid duplicate-handle rule, unsafe
ownership of output written on native failure, an unsupported publication
owner-lifetime test, unchecked variant extraction, missing owner-relative and
completion preconditions, a vague allocation error, a missing Vulkan 1.1
precondition, and an invalid positive SPIR-V fixture. No high- or
medium-severity issue remains in the reviewed transaction.

Deterministic C++ owner-allocation failure is not fault-injected. The branch is
implemented with `new (std::nothrow)`, precedes every native callback, and has a
specific `OwnerAllocationFailure` result. Allocation failure inside the
bounded public validator remains fail-closed as `InvalidLease`, as established
by the validator contract.

## Linux evidence

The Release viewer, production shader validation target, new library and test,
and all affected integration targets rebuilt with project warnings treated as
errors. The module, publication, artifact-loader, shader-manifest,
render-contract, material-parameter, draw-packet, material-diagnostic,
viewer-adapter, and Vulkan material-registry suites passed. The 12 reflection
cases, eight delivery cases, manifest argument route, and all 57 benchmark
harness tests also passed. No benchmark scenario or timing ran.

The new archive and integration executable have no direct
`vkCreateShaderModule` or `vkDestroyShaderModule` symbol. The executable has no
Vulkan, MoltenVK, OpenGL, X11, Wayland, window, or viewer dynamic dependency.
`VK_NO_PROTOTYPES` is private to the implementation and test so an accidental
direct loader call fails at compile time.

With the option disabled, the module, publication, artifact-loader, and
production-shader targets are absent. The Release viewer remains free of
Vulkan and MoltenVK linkage. Restoring the option restores the exact targets.
Settings and command-line XML parsing, clang-format, whitespace, privacy,
local-path, source-boundary, target-graph, direct-symbol, and dynamic-linkage
checks passed.

## macOS evidence

A disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5 contained
the Stage 21 commit plus exactly the four reviewed Stage 22 implementation
files. Their SHA-256 values matched before and after the gate. The ReleaseOS
build used `arm64;x86_64`, deployment target 11, tests, packaging, and the
opt-in shader path, with renderer benchmarks, signing, and crash reporting
disabled.

The module archive contains both architecture slices; the integration test is
native arm64 by existing project policy. It passed all nine fake-dispatch cases
with warnings treated as errors and valid ad-hoc signing. Its source, archive,
link line, and executable have no direct Vulkan create or destroy symbol and
no Vulkan, MoltenVK, OpenGL, or SDL dependency. The configured loader and
MoltenVK paths only satisfied the inherited opt-in configuration; the fake
test did not invoke either.

The universal viewer, production validation target, exact artifact-copy route,
and package passed. The same 12 focused routes as Linux and all 12 reflection
cases passed. The benchmark harness passed 57 unit tests after the archive
snapshot received disposable empty Git metadata required by three dry-run
metadata cases. No benchmark was executed.

The option-on app contained exactly the two canonical production modules,
byte-identical to the validated build outputs, with these SHA-256 values:

| Module | SHA-256 |
| --- | --- |
| Vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` |
| Fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` |

All 367 packaged Mach-O files were readable and had no Vulkan or MoltenVK
linkage. The scan used file descriptors for helper executables whose final
parenthesized names are otherwise interpreted by `otool` as archive-member
syntax.

After a full option-off reconfigure, all opt-in targets were absent. The
universal viewer, cleanup-copy route, and package passed. The dedicated shader
directory and canonical production modules were absent, and all 367 packaged
Mach-O files remained free of Vulkan and MoltenVK linkage. Prior option-on
build products were unreachable in the build directory and were removed with
the exact disposable root.

The Mac gate did not launch the viewer, set an ICD environment variable, touch
a graphics context, invoke a GPU, use an account or world asset, compare a
pixel, run a benchmark, or retain timing. The exact disposable source, build,
dependency, package, log, evidence, and Git-metadata root was deleted and
verified absent.

## Code size

| Stage 22 code, excluding this decision record | Lines added |
| --- | ---: |
| Backend header and implementation | 247 |
| Fake-dispatch integration tests | 453 |
| Opt-in build wiring | 24 |
| Total | 724 |

No existing source line was removed.

## Explicit runtime gaps

Stage 22 does not create a real Vulkan logical device, obtain dispatch through
`vkGetDeviceProcAddr`, run `vkCreateShaderModule` against a driver, authenticate
artifact hashes at runtime, define a pipeline layout, create or cache a
pipeline, destroy pipeline generations after native completion, allocate
descriptors or staging memory, bind a frame resource, connect a viewer call
site, select a backend, record or submit commands, present pixels, compare
parity, or measure performance.

The next-stage choice remains open until this stage is committed. Post-commit
reanalysis must decide whether the smallest dependent slice is a canonical
pipeline-layout transaction, a real device/dispatch owner, or another missing
pipeline prerequisite.
