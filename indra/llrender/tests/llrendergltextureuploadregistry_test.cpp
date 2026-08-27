/**
 * @file llrendergltextureuploadregistry_test.cpp
 * @brief Context-free tests for streamed-upload OpenGL object resolution.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the license only.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llrendergltextureupload.h"
#include "lltut.h"

#include <cstdint>
#include <limits>

namespace tut
{

struct render_gl_texture_upload_registry_test
{
};

using render_gl_texture_upload_registry_group = test_group<render_gl_texture_upload_registry_test>;
using render_gl_texture_upload_registry_object = render_gl_texture_upload_registry_group::object;
render_gl_texture_upload_registry_group render_gl_texture_upload_registry_tests("render GL texture upload registry");

template<>
template<>
void render_gl_texture_upload_registry_object::test<1>()
{
    using namespace LLRenderContract;
    using namespace LLRenderGLTextureUpload;

    Registry registry;
    auto* screen = reinterpret_cast<LLVertexBuffer*>(std::uintptr_t{ 0x10 });
    auto* old_image = reinterpret_cast<LLImageGL*>(std::uintptr_t{ 0x20 });
    auto* replacement = reinterpret_cast<LLImageGL*>(std::uintptr_t{ 0x30 });
    auto* output = reinterpret_cast<LLRenderTarget*>(std::uintptr_t{ 0x40 });
    auto* shader = reinterpret_cast<LLGLSLShader*>(std::uintptr_t{ 0x50 });
    LifecycleLedger ledger{ { 11, 4 }, 22 };

    ensure("screen registration succeeds", registry.addScreenTriangle({ 1, 7 }, screen));
    ensure("consecutive image generations register",
           registry.addImageGenerations({ 11, 4 }, old_image, { 11, 5 }, replacement));
    ensure("output registration succeeds", registry.addOutput({ 12, 3 }, output));
    ensure("canonical GL sampler registration succeeds", registry.addSampler({ 2, 8 }, { 91 }));
    ensure("canonical pipeline registration succeeds",
           registry.addPipeline({ 3, 9 }, { "contract.sample-texture", 0 }, shader));
    ensure("lifecycle registration succeeds", registry.addLifecycle(&ledger));

    ensure("exact screen generation resolves", registry.resolve(BufferHandle{ 1, 7 }) == screen);
    ensure("exact old generation resolves", registry.resolveRegisteredImage({ 11, 4 }) == old_image);
    ensure("exact replacement generation resolves", registry.resolveRegisteredImage({ 11, 5 }) == replacement);
    ensure("exact output generation resolves", registry.resolveOutput({ 12, 3 }) == output);
    ensure("exact sampler generation resolves", registry.resolve(SamplerHandle{ 2, 8 }) != nullptr);
    ensure("exact pipeline key resolves",
           registry.resolve(PipelineHandle{ 3, 9 }, { "contract.sample-texture", 0 }) == shader);
    ensure("registry borrows the ledger", registry.lifecycle() == &ledger);
    ensure("only the published old generation is logically resolvable", registry.isResolvable({ 11, 4 }));
    ensure("replacement is registered but not published", !registry.isResolvable({ 11, 5 }));
}

template<>
template<>
void render_gl_texture_upload_registry_object::test<2>()
{
    using namespace LLRenderContract;
    using namespace LLRenderGLTextureUpload;

    auto* screen = reinterpret_cast<LLVertexBuffer*>(std::uintptr_t{ 0x10 });
    auto* old_image = reinterpret_cast<LLImageGL*>(std::uintptr_t{ 0x20 });
    auto* replacement = reinterpret_cast<LLImageGL*>(std::uintptr_t{ 0x30 });
    auto* output = reinterpret_cast<LLRenderTarget*>(std::uintptr_t{ 0x40 });
    auto* shader = reinterpret_cast<LLGLSLShader*>(std::uintptr_t{ 0x50 });

    ensure("null screen is rejected", !Registry{}.addScreenTriangle({ 1, 1 }, nullptr));
    ensure("invalid screen handle is rejected", !Registry{}.addScreenTriangle({}, screen));
    ensure("aliased image objects are rejected",
           !Registry{}.addImageGenerations({ 11, 1 }, old_image, { 11, 2 }, old_image));
    ensure("different image indices are rejected",
           !Registry{}.addImageGenerations({ 11, 1 }, old_image, { 12, 2 }, replacement));
    ensure("a skipped generation is rejected",
           !Registry{}.addImageGenerations({ 11, 1 }, old_image, { 11, 3 }, replacement));
    ensure("generation overflow is rejected",
           !Registry{}.addImageGenerations({ 11, std::numeric_limits<std::uint32_t>::max() }, old_image, { 11, 1 }, replacement));

    Registry output_first;
    ensure("output-first setup succeeds", output_first.addOutput({ 11, 9 }, output));
    ensure("image index cannot alias an existing output",
           !output_first.addImageGenerations({ 11, 1 }, old_image, { 11, 2 }, replacement));
    Registry images_first;
    ensure("image-first setup succeeds", images_first.addImageGenerations({ 11, 1 }, old_image, { 11, 2 }, replacement));
    ensure("output index cannot alias an image generation", !images_first.addOutput({ 11, 7 }, output));

    ensure("zero sampler name is rejected", !Registry{}.addSampler({ 1, 1 }, {}));
    Sampler wrong_filter{ 7 };
    wrong_filter.mMinFilter = Filter::Nearest;
    ensure("noncanonical sampler descriptor is rejected", !Registry{}.addSampler({ 1, 1 }, wrong_filter));
    Sampler nonfinite{ 7 };
    nonfinite.mMaxAnisotropy = std::numeric_limits<float>::infinity();
    ensure("nonfinite anisotropy is rejected", !Registry{}.addSampler({ 1, 1 }, nonfinite));

    ensure("wrong program key is rejected",
           !Registry{}.addPipeline({ 1, 1 }, { "interface.copy", 0 }, shader));
    ensure("wrong program variant is rejected",
           !Registry{}.addPipeline({ 1, 1 }, { "contract.sample-texture", 1 }, shader));
    ensure("null shader is rejected",
           !Registry{}.addPipeline({ 1, 1 }, { "contract.sample-texture", 0 }, nullptr));

    LifecycleLedger pending{ { 11, 1 }, 22, true };
    ensure("pending completion is rejected", !Registry{}.addLifecycle(&pending));
    LifecycleLedger already_completed{ { 11, 1 }, 22 };
    already_completed.mCompletionCount = 1;
    ensure("preexisting completion evidence is rejected", !Registry{}.addLifecycle(&already_completed));
}

template<>
template<>
void render_gl_texture_upload_registry_object::test<3>()
{
    using namespace LLRenderContract;
    using namespace LLRenderGLTextureUpload;

    Registry registry;
    LifecycleLedger ledger{ { 11, 1 }, 22 };
    ensure("lifecycle setup succeeds", registry.addLifecycle(&ledger));
    const LifecycleLedger before = ledger;

    ExecutionResult result = makeTextureUploadArtifact();
    result.mSampledRGBA8 = { 0x5a };
    const ExecutionResult result_before = result;
    ensure("an invalid packet fails without requiring a GL context", !execute({}, registry, result));
    ensure("packet rejection leaves the lifecycle ledger unchanged", ledger == before);
    ensure("packet rejection leaves the caller result unchanged", result == result_before);

    const TextureUploadCase upload_case = makeTextureUploadCase();
    ensure("a canonical packet with missing borrowed resources fails before GL preflight",
           !execute(upload_case.mFrame, registry, result));
    ensure("registry rejection leaves the lifecycle ledger unchanged", ledger == before);
    ensure("registry rejection leaves the caller result unchanged", result == result_before);
}

} // namespace tut
