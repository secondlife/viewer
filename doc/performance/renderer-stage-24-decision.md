# Stage 24 Vulkan material descriptor-generation decision

## Decision

Accept one opt-in, backend-private transaction that allocates and populates a
bounded immutable batch of descriptor-set pairs for the canonical legacy
normal/specular material interface. For N resource tuples, the factory creates
one exact descriptor pool, allocates 2N sets in canonical set-0/set-1 order,
writes all 4N bindings once, and publishes the generation only after the update
returns.

This is the ninth committable slice of master Stage 2. The library and its
integration test remain behind `LL_VULKAN_TONEMAP_TEST`. They use injected
Vulkan functions and `VK_NO_PROTOTYPES`; they do not link the loader, expose
Vulkan handles through the neutral renderer contract, or define a frame,
pipeline, submission, or viewer policy.

## Immutable resource contract

Each tuple supplies one non-null uniform buffer, its declared creation size,
an explicit offset, and three non-null sampler/image-view pairs. The canonical
`MaterialParameters` block is 272 bytes. The factory verifies that the full
block fits at the offset and always writes an exact 272-byte uniform range.
Each sampled binding is written as one combined image sampler in
`VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`.

Raw handles cannot prove resource provenance or state. The caller must ensure
that every resource belongs to the supplied Vulkan 1.1-or-newer device, was
created for the declared use, and remains valid through the last submitted
command that consumes the generation. Uniform offsets must satisfy the
device's `minUniformBufferOffsetAlignment`. Images must be in shader-read-only
layout when consumed, and sampled resources must not use YCbCr conversion.

The Stage 23 layout owner must have been created on the same device and must
conservatively outlive the descriptor generation. `createdOn()` supplies this
narrow provenance check without exposing mutable layout state.

## Count and allocation boundary

Input must contain at least one tuple and at most `UINT32_MAX / 4` tuples. The
upper bound makes the 2N set, 3N sampled-image, and 4N write calculations safe
in Vulkan's 32-bit count fields. Every input handle and uniform range is
validated before a native callback.

The implementation allocates all host-side storage before pool creation:
copied binding metadata, alternating layout handles, set outputs, buffer infos,
image infos, write records, and the nothrow generation owner. Host allocation
failure therefore cannot strand a native pool. Deterministic allocation
failure is not fault-injected; the evidence is the explicit exception and
`new (std::nothrow)` handling plus source ordering before the first callback.

The pool has zero flags, `maxSets = 2N`, exactly N uniform-buffer descriptors,
and exactly 3N combined-image-sampler descriptors. One allocation requests the
alternating canonical layout sequence `[set0, set1] x N`. There is no
individual-free or reset capability.

## Native transaction and ownership

Only `VK_SUCCESS` plus a non-null pool establishes pool ownership. Output bits
written alongside a failed result are ignored. Set allocation is one Vulkan
transaction; a failed result preserves its exact `VkResult`, ignores every
output slot, and rolls back the accepted pool. A successful allocation must
produce 2N non-null sets. Any null slot fails closed with its tuple and set
kind identified.

After allocation, one void update call receives exactly four writes per tuple:
set 0 binding 0 receives the uniform buffer, offset, and fixed range; set 1
bindings 0 through 2 receive the corresponding sampler and image view. No
binding pair can escape before every write has been issued.

`LegacyNormSpecDescriptorGeneration` is noncopyable and nonmovable and
transfers only through `unique_ptr`. It owns the pool and copied borrowed
metadata, not the buffers, views, samplers, layouts, device, or dispatch.
Destroying the pool implicitly frees every set. The owner exposes no pool,
free, reset, or update operation; callers with the borrowed raw set handles
must preserve that immutability externally. A generation can be reused across
draws and frames while its resources remain valid; recreating it every frame
is not a policy of this stage. Callers must wait for all submitted users to
complete before destroying it, as required by the
[Vulkan descriptor-set lifetime rules](https://docs.vulkan.org/spec/latest/chapters/descriptorsets.html).

Opaque non-dispatchable handle equality is never used as object identity.
Equal non-null parameter and sampled-set values are valid, and the existing
material registry now applies that rule at both exact consumers while still
rejecting either null set. This follows Vulkan's
[non-dispatchable handle model](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-objectmodel).

## Focused tests and review

Nine fake-dispatch cases cover:

- result ordering, `noexcept`, owner traits, bounds checks, and out-of-range
  access;
- null device, each missing dispatch entry, a layout/device mismatch, empty
  and oversized counts, every invalid resource field, and later-tuple error
  context, all before native callbacks;
- exact N=2 pool sizes, flags, alternating layouts, allocation call, 272-byte
  buffer ranges including a nonzero offset, 4N writes, image layout, ordered
  publication, and copied resource metadata;
- failed pool creation with poisoned output, successful creation with a null
  pool, failed set allocation, and null parameter or sampled sets at multiple
  positions, with exact rollback;
- equal non-null set and resource values, owner transfer, one final pool
  destruction, and independent generations that deliberately reuse opaque
  values; and
- direct registry proof that an equal non-null pair is accepted while either
  null slot is rejected through the same rule used by both consumers.

Two independent adversarial reviews found no remaining high-severity product
defect. Review replaced a false-positive registry assertion with the pure rule
used by the production paths, documented and exercised nonzero uniform
offsets, added precise context for a later invalid tuple, and removed a no-op
address assertion. Owner-allocation failure remains structurally reviewed, not
deterministically injected.

## Linux evidence

The Release viewer, production shader validation chain, descriptor archive and
test, and all affected targets rebuilt with project warnings treated as
errors. All nine new fake-dispatch cases passed. Fourteen focused CTest routes
covering draw packets, material diagnostics and parameters, renderer contract,
shader manifest, artifact loading and delivery, publication, shader modules,
layouts, descriptors, registry, manifest arguments, and the viewer adapter
passed. All 12 reflection cases and 65 Python unit tests passed: 57 benchmark
harness cases and eight artifact-delivery cases. No benchmark scenario or
timing ran.

The descriptor archive and integration executable have no direct descriptor
pool, allocation, update, or destroy symbol. The executable has no Vulkan,
MoltenVK, OpenGL, window-system, SDL, or viewer dynamic dependency. With the
option disabled, every opt-in material implementation and production-shader
target is absent, the Release viewer still builds, and Vulkan or MoltenVK is
not linked. Restoring the option restores the descriptor test. Formatting,
whitespace, privacy, source-boundary, symbol, dynamic-linkage, and option
isolation checks passed.

## macOS evidence

A disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5 contained
the Stage 23 commit plus the nine reviewed Stage 24 source, test, and build
files. Their SHA-256 values matched the local working tree at the end of the
gate. The ReleaseOS build used `arm64;x86_64`, deployment target 11, tests,
packaging, and the opt-in shader path, with renderer benchmarks, signing, and
crash reporting disabled.

The descriptor archive contains both architecture slices; the integration
test is native arm64 by existing project policy. It passed all nine
fake-dispatch cases with warnings treated as errors and valid ad-hoc signing.
Neither the archive nor test directly references descriptor-pool creation,
allocation, update, or destruction, and the test has no Vulkan, MoltenVK,
OpenGL, or SDL dependency. The configured loader and MoltenVK paths only
satisfied inherited opt-in configuration; fake dispatch invoked neither.

The universal viewer, production validation chain, exact artifact-copy route,
and local package passed. All 14 focused routes, 12 reflection cases, 57
benchmark-harness tests, and eight artifact-delivery tests passed. No benchmark
scenario ran. The option-on app contained exactly the two canonical production
modules, byte-identical to the validated build outputs:

| Module | SHA-256 |
| --- | --- |
| Vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` |
| Fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` |

All 367 packaged Mach-O files were readable and had no Vulkan or MoltenVK
linkage. The scan used file descriptors for helper executables whose
parenthesized names would otherwise be interpreted by `otool` as archive
members.

After a full option-off reconfigure, every opt-in material implementation,
test, and production-shader target was absent. The option-neutral artifact-copy
target remained to remove stale outputs. A full universal warnings-as-errors
viewer rebuild and package passed, the app contained no SPIR-V files, and all
367 Mach-O files remained readable and free of Vulkan and MoltenVK linkage.

The disposable gate needed several environment and orchestration corrections
before evidence was accepted: explicit `spirv-cross` and `spirv-dis` paths,
the expected `LL_BUILD` and deployment inputs, an empty local Git commit for
three metadata-dependent harness cases, explicit builds for focused test
executables omitted by the package target, and a linkage scan limited to
dependency lines rather than a test filename containing “vulkan.” A final
comment-only source sync rebuilt its affected target before tests. None of
these corrections changed runtime source, suppressed a test, or weakened an
acceptance condition.

The Mac gate did not launch the viewer, set an ICD environment variable, touch
a graphics context, call the transaction through a driver, invoke a GPU, use
an account or world asset, compare a pixel, run a benchmark, or retain timing.
The exact disposable source, build, dependency, package, log, evidence, and
Git-metadata root was validated as user-owned, deleted, and verified absent.

## Code size

| Stage 24 code, excluding this decision record | Lines added | Lines removed |
| --- | ---: | ---: |
| Descriptor backend header and implementation | 441 | 0 |
| Fake-dispatch descriptor tests | 694 | 0 |
| Consumer and layout corrections | 28 | 3 |
| Opt-in build wiring | 24 | 0 |
| Total | 1,187 | 3 |

## Explicit runtime gaps

Stage 24 does not create a Vulkan instance, physical or logical device, obtain
dispatch through `vkGetDeviceProcAddr`, call a real driver, allocate buffers or
images, establish image transitions, enforce native completion, define a
portable attachment profile, choose render-pass compatibility or dynamic
rendering, create or cache a graphics pipeline, retain shader modules, connect
a viewer call site, select a backend, record or submit commands, present or
compare pixels, or measure performance.

The next-stage choice remains open until this stage is committed. Post-commit
reanalysis must choose the smallest dependency among portable attachment
formats and alpha semantics, render-pass or dynamic-rendering policy, pipeline
ownership and caching, real device dispatch, and completion-backed descriptor
reuse.
