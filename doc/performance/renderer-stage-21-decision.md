# Stage 21 shader publication lifecycle decision

## Decision

Accept one API-neutral publication owner for the canonical Stage 20 production
material program. A successful publication deep-copies the bounded loaded
program into immutable shared ownership and returns a typed `ShaderHandle`.
Replacement advances the generation exactly once, makes the old handle stale
for new resolution, and retains the old bytes until every frame recorded
against that generation has completed.

This is the sixth committable slice of master Stage 2. The publication library,
test, and artifact admission path remain behind `LL_VULKAN_TONEMAP_TEST`. The
neutral handle declarations and manifest-validator safety repair remain in the
always-built contract. The stage creates no Vulkan object, graphics pipeline,
descriptor, frame resource, viewer call site, graphics context, or GPU work.

## Handle and owner boundary

`ShaderHandle` uses the neutral typed-handle representation but is deliberately
not a `ResourceHandle`. A loaded shader program is not yet a native frame
resource.

The one `LegacyNormSpecShaderPublication` owner uses logical index 1. Its first
publication is `{1,1}`; every replacement preserves the index and advances by
exactly one nonzero generation. The generic generation helper rejects invalid
handles and refuses to wrap the maximum generation. The maximum value itself
is reachable from its predecessor and is terminal.

Handles are owner-relative, not process-global identifiers. Two independent
owners can issue the same numeric handle, so a handle must never cross owner
instances. Copy and move construction and assignment are disabled to prevent
cloning or moving an existing owner identity. The owner is externally
sequenced and makes no thread-safety claim. It must outlive its handles and
in-flight frame tracking; shutdown must otherwise quiesce them externally
before destroying the owner. Its default destructor emits no retirement
records.

## Admission and immutable ownership

The artifact loader and publisher share
`validLegacyNormSpecProductionShaderProgram()`. It requires the exact canonical
production program identity and variant, vertex and fragment stage labels,
exact entry point labels, the expected SPIR-V execution models, a bounded
instruction walk, and the same 16 MiB per-module ceiling applied by the file
loader. Public aggregate construction therefore cannot bypass the loader's
memory bound.

These are bounded runtime structural checks. They do not prove packaged-byte
provenance, hashes, full SPIR-V validity, reflected interfaces, or shader
semantics, and they are not substitutes for the build-time `spirv-val`,
reflection, and production-verifier chain.

Each accepted value is copied into a new
`shared_ptr<const LoadedShaderProgram>`. This is a deep copy even when the
caller passes an rvalue whose vectors still have outside aliases. Later caller
mutation or destruction cannot change the published words.

Publication is transactional. An invalid initial program leaves the owner
empty. An invalid replacement, terminal generation, or allocation failure
leaves the current generation and pending generations unchanged. The
allocation-using manifest helpers no longer carry an unsound internal
`noexcept`; the public fail-closed validators catch allocation and canonical
construction failures and return `false`.

## Frame leases and completion

`resolveForFrame()` accepts only the exact current handle, a nonzero frame
strictly newer than the completion watermark, and globally nondecreasing record
order. Same-frame reacquisition is allowed. Every recorded frame must acquire
its own lease because reusing an older lease for a later frame would bypass
last-use tracking.

Replacement makes the old handle logically stale immediately. The owner keeps
the superseded program and its maximum recorded frame in publication order.
The old and replacement generations may both acquire leases in the same frame;
both remain owned through completion of that frame.

`completeThrough()` rejects zero and watermark regression, accepts equal
watermarks, and reports superseded generations in publication order only when
their last recorded frame is complete. It never retires the current generation.
Logical retirement removes owner reachability but does not revoke an outside
lease; the immutable bytes remain alive until the final shared owner releases
them. Missing completion intentionally grows pending ownership. A later native
completion service must advance the watermark.

## Focused tests and review

Nine publication cases cover:

- typed-handle separation, exact generation arithmetic, terminal maximum, and
  noncopyable and nonmovable owner identity;
- empty and canonical lookup, first publication, deep ownership from an
  aliased rvalue, and wrong-key rejection;
- canonical structural acceptance plus program, variant, stage, label,
  execution-model, word, and in-memory-size mutations;
- zero, unknown, old, future, completed, and regressing frame resolution;
- replacement, exact next generation, cross-generation record order,
  same-frame mixed generations, immediate staleness, and caller mutation;
- partial and exact completion, ordered multigeneration retirement, current
  retention, equal completion, and regression atomicity; and
- an outside lease surviving logical retirement and releasing physical bytes
  only after its final reset.

Adversarial review found and closed duplicate and moved-from owner identities,
an rvalue vector-alias hole, allocation-triggered termination in public
fail-closed manifest validation, unsafe test setup dereferences, missing
symmetric validator mutations, cross-generation frame regression coverage,
zero-completion coverage, masked transitive CMake dependencies, and misleading
fixture wording. No high- or medium-severity issue remains.

Generation exhaustion through the real owner would require billions of
replacements, so only the reusable arithmetic helper is exercised at the
boundary. Allocation rollback is established by operation ordering and
nothrow commit steps but is not directly fault-injected. Same-frame mixed
generations and owner-relative handles are deliberate documented limits.

## Linux evidence

The Release viewer, publication library and integration test, and all affected
targets rebuilt with project warnings treated as errors. The publication,
artifact-loader, shader-manifest, render-contract, material-parameter,
draw-packet, material-diagnostic, and viewer-adapter suites passed 9, 11, 7,
28, 8, 7, 4, and 7 cases. All 12 reflection cases, all eight delivery cases,
the manifest argument test, and all 57 benchmark-harness tests also passed. No
benchmark scenario or timing ran.

The current Nix command environment initially lacked the existing viewer's GLX
development headers. Supplying the immutable `libglvnd`, `libx11`, and
`xorgproto` include closures allowed the unchanged full viewer build to pass;
no source workaround was added.

The publication integration executable has no Vulkan, MoltenVK, OpenGL, X11,
Wayland, SDL, window, or viewer dependency, and the publication source and
archive contain no native Vulkan API reference. With the option disabled, the
publication, artifact-loader, and production-shader targets are absent. The
option-off Release viewer remains free of Vulkan and MoltenVK linkage. Turning
the option back on restores the exact publication targets and test.

Settings and command-line XML parsing, clang-format, whitespace, privacy,
local-path, backend-source, target-graph, and dynamic-linkage scans passed.

## macOS evidence

A disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5 contained
the Stage 20 commit plus exactly the eight frozen Stage 21 implementation
files. Their SHA-256 values matched before and after the gate. The ReleaseOS
build used `arm64;x86_64`, deployment target 11, tests, packaging, and the
opt-in shader path, with renderer benchmarks, signing, and crash reporting
disabled. All 13 requested sequential build invocations, including the viewer
and `llpackage`, passed with warnings treated as errors.

The publication, artifact-loader, shader-manifest, render-contract,
material-parameter, draw-packet, material-diagnostic, and viewer-adapter suites
passed 9, 11, 7, 28, 8, 7, 4, and 7 cases. The 12 reflection cases, eight
delivery cases, and manifest argument parse also passed. The publication and
artifact archives contained both architecture slices; runnable integration
tests were native arm64 by existing project policy.

The packaged app contained exactly the two canonical production modules, with
no intermediate evidence, and both were byte-identical to the validated build
outputs. The app and all 367 bundle Mach-O files had no Vulkan or MoltenVK
linkage. The publication executable, link line, source, and archives had no
native graphics API or higher-layer dependency.

A full option-off reconfigure removed the publication, artifact, production,
and opt-in test targets. The universal viewer, cleanup-copy route, and package
passed. The final app had no dedicated material directory, bundled SPIR-V, or
intermediate evidence, and all 367 Mach-O files remained free of Vulkan and
MoltenVK linkage. Nine prior ON outputs remained unreachable in the build-only
shader directory after the in-place reconfigure; no OFF graph or package
referenced them, and deletion of the exact disposable root removed them.

The Mac gate did not launch the viewer or touch a graphics context, GPU,
account, world, benchmark, or timing path. The exact disposable source, build,
dependency, package, log, and evidence root was deleted and verified absent.

## Code size

| Stage 21 code, excluding this decision record | Lines added |
| --- | ---: |
| Typed handle, shared admission, and fail-closed validation | 79 |
| Publication owner and opt-in build wiring | 229 |
| Publication integration tests | 312 |
| Total | 620 |

Nine obsolete or unsound declaration and implementation lines were removed.

## Explicit runtime gaps

Stage 21 does not load the program from a viewer call site, create or destroy a
native shader module, own a Vulkan device or dispatch table, handle device loss,
define a pipeline layout or cache, allocate descriptors or parameter buffers,
bind frame resources, connect completion to a fence or timeline, record or
submit commands, present pixels, compare parity, or measure performance.

The next-stage choice remains intentionally open until this stage is committed.
Post-commit reanalysis must decide whether native shader-module creation is now
the smallest complete dependent slice or whether another prerequisite is still
missing.
