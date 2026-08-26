# Stage 9 renderer contract decision

## Decision

Accept the backend-neutral renderer contract as the Stage 1A foundation. It is sufficient to record and validate the selected full-screen, indexed-material, and streaming-texture slices without importing OpenGL or Vulkan vocabulary.

This does not complete master Stage 1. No OpenGL executor, Vulkan executor, rendered-output comparison, performance result, or backend recommendation exists yet. Current rendering output is unchanged.

## Boundary

The contract is an immutable work and resource-reference stream. Viewer policy resolves scene objects, settings, materials, and decoded images above it. Backend-neutral registries own persistent resource contents and shader implementations. Executors resolve typed handles below it and own API objects, synchronization primitives, state translation, and deferred destruction.

The frame stream therefore describes resources used by a frame, not a general resource-creation protocol. A later replay harness can seed OpenGL and Vulkan registries from the same CPU fixture. The texture upload remains in the stream because streaming publication and old-generation retirement are one of the three selected slices.

`llrendercontract` is a separate standard C++ target. Its source and public header contain no graphics API or window-system include, symbol, handle, enum, or link dependency. Existing `llrender` and `llrenderheadless` link to it in one direction.

## Representative traces

### Tonemap full-screen pass

The boundary is immediately above `LLPipeline::tonemap()`. The fixture records the RGBA16F destination, RGBA16F scene input, 1x1 R16F exposure input, samplers, a stable shader-program key, the 16-byte parameter block, fixed pipeline state, viewport, scissor, image accesses, and the real three-position screen-triangle buffer with its 16-byte stride.

GLSL filenames, reflected uniform locations, texture units, FBOs, and the `LLGLSLShader`, `LLRenderTarget`, and `LLVertexBuffer` objects stay below the boundary.

### Indexed material draw

The boundary is after viewer draw preparation and before the deferred material pass issues API calls. The fixture snapshots the three material image views and mip ranges, seven planar vertex bindings, U16 index range, parameters, three G-buffer attachments, depth attachment, explicit access states, and pipeline identity. Position and normal strides match the viewer's 16-byte planar layout.

`LLDrawInfo`, viewer material objects, mutable transform pointers, GL buffer names, and draw-pool state stay outside the contract.

### Streaming texture upload

The boundary is after decoded-image admission and before `LLImageGL` creates or publishes a physical texture generation. The fixture owns the upload bytes and records revision, physical and logical extents, resident discard, row pitch and origin, format, generated mip range, transfer and final states, and the new image generation. The old generation remains declared until its completion frame and is then released.

Fetch priority, cache and asset identity, `LLImageRaw`, GL names, shared-context fences, viewer callbacks, and the current three-frame GL deletion queue remain outside the contract.

## Validation

The null validator checks:

- typed generational resource identity, declaration uniqueness, and lifetime;
- resource, enum, binding, byte-range, vertex, index, subresource, viewport, scissor, and upload bounds;
- stable shader keys, pipeline-owned vertex layouts, exact parameter sizes, and attachment compatibility;
- ordered pass dependencies and complete buffer and image access declarations;
- per-generation and per-subresource image-state continuity, including generated mip ranges;
- attachment load/store semantics, including invalidation after `DontCare` stores;
- immutable upload ownership, row pitch, logical-discard relation, revision uniqueness, and mip policy;
- completion-safe retirement and stale-generation rejection.

Fifteen focused tests cover the three valid fixtures and failures for missing access, invalid dependencies and enums, stale generations, draw and upload bounds, parameter size, mip-range coverage, index alignment, attachment discard, and early release.

## Measured surface

The stage adds 2,212 lines before this decision record:

| Surface | Size |
| --- | ---: |
| Public contract header | 530 lines |
| Null-validator implementation | 1,182 lines |
| Focused fixtures and tests | 478 lines |
| Build wiring | 22 lines |

The public vocabulary contains 20 enums and 40 structs, including four distinct generational handle types. This is a meaningful interface, so later stages should migrate one real slice at a time and resist adding fields until a concrete executor or pass requires them.

## Verification

- Linux built the independent contract target and passed all 15 tests.
- GCC 15 compiled the GL-free implementation with C++20, `-Wall -Wextra -Wpedantic -Werror`.
- Source inspection found no OpenGL or Vulkan include or symbol in the contract or fixtures.
- macOS built the contract archive for both `arm64` and `x86_64`; `lipo` reported both architectures.
- The host-architecture macOS integration executable passed all 15 tests. Test executables are intentionally host-only in this build system.
- The benchmark-enabled universal Release viewer linked against the contract for both architectures.

The local full `llrender` target was not used as evidence because this checkout's host environment lacks the GLX development header and has an incompatible cached precompiled-header fortify setting. Those failures predate and do not involve the GL-free target. The full macOS universal link covers integration with `llrender`.

No viewer was launched, no account or profile was opened, and no benchmark timing was collected.

## Reanalysis and next stage

The contract is large enough that adding a second executor for all three slices at once would hide boundary mistakes and produce a poor rollback unit. The smallest useful next commit is Stage 1B: record the existing tonemap inputs through a pure builder and execute that one packet through the current OpenGL objects while preserving the existing image output.

Stage 1B must establish the backend-neutral registry seam for the screen triangle, images, samplers, and tonemap program; add an OpenGL executor only for the vocabulary used by that packet; compare the legacy and contract-driven offscreen output from fixed inputs; and retain an immediate legacy fallback. It must not add Vulkan, convert the material or upload paths, or change the production default.
