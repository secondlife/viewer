# Stage 20 production material artifact decision

## Decision

Accept opt-in delivery and bounded, owned loading of the verified Stage 18
production material shader pair. Packaging and installation now publish exactly
two canonical files:

- `app_settings/shaders/vulkan/legacy_normspec/production.vert.spv`
- `app_settings/shaders/vulkan/legacy_normspec/production.frag.spv`

The files come directly from the production shader target and are downstream of
its complete compile, `spirv-val`, reflection, and contract-verifier chain.
Build-local hashes and the final stamp are generated only after those gates. A
backend-neutral loader can read the installed pair into owned 32-bit words and
associate it with the canonical Modern HDR production manifest.

This is the fifth committable slice of master Stage 2. It creates no Vulkan
object, pipeline, registry, viewer call site, graphics context, or GPU work.
The artifacts and loader/test targets remain behind
`LL_VULKAN_TONEMAP_TEST`; option-off manifest and install wiring only cleans
the dedicated destination.

## Delivery boundary

The manifest helper first removes only its dedicated destination. With the
option enabled it then copies exactly:

| Validated build output | Installed or packaged name |
| --- | --- |
| `material.production.vert.spv` | `production.vert.spv` |
| `material.production.frag.spv` | `production.frag.spv` |

It snapshots the source bytes, rejects symlinked source roots, source files,
destination roots, and destination parents, and safely unlinks a symlink at the
final destination. It checks that each manifest copy happens exactly once,
compares the delivered bytes, and rejects any extra destination entry. A
failure removes the partial destination. With the option disabled, the same
route removes stale files and delivers nothing.

Linux copy and tar routes and the macOS all/explicit refresh and package routes
depend on the exact two SPIR-V outputs, the final production verification
stamp, the full production validation target, and an explicit ON/OFF state
file. A shader change or option transition therefore invalidates those delivery
routes instead of leaving an old staging tree or package. A direct already
up-to-date macOS viewer-only target does not imply the separate refresh target.

Installation uses the same exact destination and removes it before installing
or when the option is disabled. `cmake --install` consumes existing outputs and
does not build them; the normal build `install` target reaches the production
validation target through the viewer dependency first.

The cleanup code refuses to recurse through a symlinked parent and unlinks a
symlink at the final destination as a file. Its recursive scope is limited to
the dedicated `legacy_normspec` directory.

## Loader contract

`loadLegacyNormSpecProductionArtifacts()` accepts an explicit app-settings
root and returns either one typed error or one complete owned program. The
error alternative comes first, so default construction fails closed. Partial
programs are never returned.

The loader derives program identity, stages, and entry-point names from the
canonical production manifest. For each module it requires a real nonsymlink
file, a word-aligned size from 5 words through 16 MiB, a complete binary read,
the little-endian SPIR-V magic value, a bounded instruction walk, exactly one
well-formed `OpEntryPoint`, the exact `main` name, and the expected vertex or
fragment execution model. It then moves the words into the result, so their
lifetime does not depend on the source files.

These runtime checks detect truncation, basic instruction-boundary errors, and
obvious stage or entry-point mislabeling. They do not prove module hashes,
content identity, a complete SPIR-V interface, or semantic validity, and are
deliberately not a replacement for build-time `spirv-val` and reflection. The
app-settings tree is trusted installation data and must remain stable while a
load is in progress. Separate filesystem status and stream operations cannot
make this portable `std::filesystem` loader a sandbox against concurrent
hostile replacement.

## Focused tests and review

Eleven C++ cases cover fail-closed construction, manifest identity, canonical
success, missing roots and modules, no partial result, root, intermediate, and
leaf symlinks, nonregular files, undersized, misaligned, and oversized modules,
invalid magic, malformed instruction lengths, missing, renamed, unterminated,
padded, duplicate, and extra entry points, swapped execution models, and
ownership after source mutation and deletion. A deterministic portable
read-failure fixture was not added because it would require an injected
filesystem or stream seam solely for the test.

Eight Python cases cover the exact pair and bytes, exclusion of intermediate
evidence, missing and symlinked sources, stale cleanup, option-off cleanup,
partial-copy rollback, and final and parent destination symlink sentinels. A
parse-level test also exercises the exact viewer-manifest command-line option.

Adversarial review found and closed four material delivery issues before the
stage was frozen: the original command-line spelling did not match the
underscore argument parser, target-only dependencies did not express artifact
freshness to all generators, a failed second copy could leave a partial tree,
and destination parents needed explicit symlink rejection. Loader review also
prompted the fail-closed result ordering, intermediate-symlink and entry-point
padding cases, and the explicit filesystem trust boundary above.

## Platform evidence

On Linux, the production validation target rebuilt from clean shader outputs.
All 12 reflection cases and the final verifier passed; the build-local module
hashes and final stamp were then generated. The material-parameter, draw-packet,
material-diagnostic,
render-contract, shader-manifest, viewer-adapter, and artifact-loader suites
passed 8, 7, 4, 28, 7, 7, and 11 cases. All eight artifact-delivery cases and
the manifest argument test passed. The 57 benchmark-harness tests also passed,
but no benchmark run or timing was performed.

A fresh `DESTDIR` install was seeded with stale nested files. Installation
replaced them with exactly the canonical pair. The installed and build-output
files compared byte for byte and had these SHA-256 values:

| Module | SHA-256 |
| --- | --- |
| Vertex | `b5750a179572b2fc3545c7472a28cac963351f7e7bc44df38190bb8961894095` |
| Fragment | `850d765cb1a6cd31dfbe16f94cedd89ab8306eca771cf833fe47c441b9c743fb` |

A disposable C++ probe loaded that installed root through the public loader
and reported program `deferred.material.normspec`, variant 5, 2,017 vertex
words, and 1,956 fragment words. The probe and install tree were then removed.
Reconfiguring the same build with the option disabled and reinstalling removed
the dedicated artifact directory.

The real `LLManifest` copy route also staged the exact pair into a disposable
tree. The same public C++ loader read that staged app-settings root and reported
the same program, variant, and 2,017/1,956 word counts. An option-off pass then
removed the directory. The stock Linux tar target was not a clean platform gate
in this Nix environment: its pre-existing media-plugin target assumptions,
GStreamer include setup, and CMake 4 shared-library deploy logic fail before
this artifact route completes. Those unrelated packaging problems were not
folded into this stage. The exact manifest helper, generated dependency graph,
and full install route were exercised instead.

The install gate initially exposed a stale reference to
`featuretable_solaris.txt`, which was removed from the repository long ago.
Removing that nonexistent file from `ViewerInstall.cmake` is the minimal
prerequisite repair included in this stage.

A disposable snapshot on macOS 26.6.2 with Xcode 26.6 and SDK 26.5 contained
the Stage 19 commit plus exactly the ten reviewed Stage 20 implementation
files. Its ReleaseOS build used `arm64;x86_64`, tests, the opt-in shader path,
and packaging, with renderer benchmarks, signing, and crash reporting disabled.
The production shader, loader integration, viewer, and `llpackage` targets all
passed with warnings treated as errors. Packaging wrote the standard universal
DMG marker but no standalone DMG file.

The app bundle contained exactly the canonical vertex and fragment paths. Each
file compared byte for byte with its validated build output and matched the
Linux SHA-256 values above. A disposable neutral probe loaded the actual bundle
app-settings root, confirmed the exact stages and `main` entry points, and owned
2,017 vertex and 1,956 fragment words. It linked only the C++ and system runtime.
The loader and app contained both architecture slices; runnable test binaries
were native arm64 by existing project policy.

The 11 loader, 8 delivery, manifest argument, 12 reflection, and six canonical
suites all passed on the Mac. All 367 Mach-O files in the bundle were readable
and had no Vulkan or MoltenVK linkage. After a full option-off reconfigure, the
loader and production targets and opt-in tests were absent, the viewer, refresh,
and package targets passed, and no dedicated SPIR-V or intermediate file
remained in the app.

On both platforms with `LL_VULKAN_TONEMAP_TEST=OFF`, neither the artifact library
and test nor the production shader target exists in the build graph. The
Release viewer has no Vulkan or MoltenVK dynamic dependency. The Linux loader
integration test itself has no Vulkan, MoltenVK, OpenGL, X11, Wayland, or viewer
linkage.

Clang-format, whitespace, settings and command-line XML parsing, credential,
local-path, backend-dependency, and private-artifact scans passed. Generated
modules and task-specific disposable probes, staging and install roots, and
private logs are removed before the commit. The disposable Mac source, build,
package, dependency, and probe root was also removed and verified absent.

No viewer, graphics context, GPU, account, region, world asset, pixel,
benchmark, or timing path ran as part of this stage.

## Code size

| Stage 20 code, excluding this decision record | Lines added |
| --- | ---: |
| Owned loader contract and implementation | 442 |
| C++ loader integration tests | 465 |
| Delivery, cleanup, and Python tests | 359 |
| Build, manifest, and install wiring | 149 |
| Total | 1,415 |

Five obsolete or replaced wiring lines were removed. The bounded parser and
defensive delivery code account for most of the size; no graphics backend code
was added.

## Explicit runtime gaps

Stage 20 does not publish a shader generation, retain an old generation for an
in-flight frame, retire a generation after completion, create a native shader
module or pipeline, select device formats or features, allocate parameters or
descriptors, capture viewer state, resolve resources, record or submit
commands, present pixels, compare parity, or measure performance. Nothing calls
the loader from the viewer yet.

## Reanalysis

Verified production bytes now cross the build-to-runtime boundary with an
owned representation, but there is still no lifecycle between those bytes and
future native resources. Creating a Vulkan shader module next would make
replacement and in-flight ownership implicit in backend code.

The next smallest dependent stage is therefore neutral generation-aware shader
publication and retirement. It should publish only a fully loaded immutable
program, issue a fresh nonzero generation for replacement, resolve only exact
generation handles, retain superseded data while a recorded frame can still
refer to it, and retire it only after an explicit completion boundary. Unknown,
stale, wrapped, or already retired generations must fail closed.

That stage should reuse existing handle and frame-completion conventions where
they fit and remain independently testable without Vulkan, a viewer global, or
a loader call site. Native shader modules, pipeline layout and cache policy,
descriptor allocation, backend selection, startup integration, and GPU work
remain later stages.
