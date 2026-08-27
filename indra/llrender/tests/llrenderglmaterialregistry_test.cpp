/**
 * @file llrenderglmaterialregistry_test.cpp
 * @brief Tests for generation-safe material OpenGL object resolution.
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

#include "llrenderglmaterial.h"
#include "lltut.h"

#include <array>
#include <cstdint>

namespace tut
{

struct render_gl_material_registry_test
{
};

using render_gl_material_registry_group  = test_group<render_gl_material_registry_test>;
using render_gl_material_registry_object = render_gl_material_registry_group::object;
render_gl_material_registry_group render_gl_material_registry_tests("render GL material registry");

template<>
template<>
void render_gl_material_registry_object::test<1>()
{
    using namespace LLRenderContract;
    using namespace LLRenderGLMaterial;

    Registry registry;
    auto*    vertex_buffer = reinterpret_cast<LLVertexBuffer*>(std::uintptr_t{ 0x10 });
    auto*    target        = reinterpret_cast<LLRenderTarget*>(std::uintptr_t{ 0x20 });
    auto*    shader        = reinterpret_cast<LLGLSLShader*>(std::uintptr_t{ 0x30 });

    ensure("paired buffer registration succeeds", registry.addVertexBuffer({ 1, 4 }, { 2, 8 }, vertex_buffer));
    ensure("diffuse registration succeeds", registry.addSampledImage({ 3, 2 }, { 101, PixelFormat::RGBA8Unorm, { 4, 4 }, 3 }));
    ensure("normal registration succeeds", registry.addSampledImage({ 4, 3 }, { 102, PixelFormat::RGBA8Unorm, { 4, 4 }, 3 }));
    ensure("specular registration succeeds", registry.addSampledImage({ 5, 5 }, { 103, PixelFormat::RGBA8Unorm, { 4, 4 }, 3 }));
    ensure("target registration succeeds",
           registry.addRenderTarget({ ImageHandle{ 6, 1 }, ImageHandle{ 7, 2 }, ImageHandle{ 8, 3 } }, { 9, 4 }, target));
    ensure("sampler registration succeeds", registry.addSampler({ 10, 6 }, {}));
    ensure("pipeline registration succeeds", registry.addPipeline({ 11, 7 }, { "deferred.material.normspec", 0 }, shader));

    ensure("exact vertex and index generations resolve", registry.resolveVertexBuffer({ 1, 4 }, { 2, 8 }) == vertex_buffer);
    ensure("either exact buffer handle resolves to the shared object", registry.resolve(BufferHandle{ 2, 8 }) == vertex_buffer);
    const SampledImage* diffuse = registry.resolveSampledImage({ 3, 2 });
    ensure("exact sampled image generation resolves", diffuse && diffuse->mTexture == 101 && diffuse->mMipLevels == 3);

    const TargetImage color = registry.resolveTargetImage({ 7, 2 });
    ensure("color handle resolves to the exact target attachment",
           color.mTarget == target && color.mAspect == TargetAspect::Color && color.mAttachment == 1);
    const TargetImage depth = registry.resolveTargetImage({ 9, 4 });
    ensure("depth handle resolves to the target depth aspect", depth.mTarget == target && depth.mAspect == TargetAspect::Depth);
    ensure("exact sampler generation resolves", registry.resolve(SamplerHandle{ 10, 6 }) != nullptr);
    ensure("exact pipeline key resolves", registry.resolve(PipelineHandle{ 11, 7 }, { "deferred.material.normspec", 0 }) == shader);
}

template<>
template<>
void render_gl_material_registry_object::test<2>()
{
    using namespace LLRenderContract;
    using namespace LLRenderGLMaterial;

    Registry registry;
    auto*    vertex_buffer = reinterpret_cast<LLVertexBuffer*>(std::uintptr_t{ 0x10 });
    auto*    target        = reinterpret_cast<LLRenderTarget*>(std::uintptr_t{ 0x20 });
    auto*    shader        = reinterpret_cast<LLGLSLShader*>(std::uintptr_t{ 0x30 });

    ensure("paired buffer registration succeeds", registry.addVertexBuffer({ 1, 4 }, { 2, 8 }, vertex_buffer));
    ensure("sampled image registration succeeds", registry.addSampledImage({ 3, 2 }, { 101, PixelFormat::RGBA8Unorm, { 4, 4 }, 3 }));
    ensure("target registration succeeds",
           registry.addRenderTarget({ ImageHandle{ 6, 1 }, ImageHandle{ 7, 2 }, ImageHandle{ 8, 3 } }, { 9, 4 }, target));
    ensure("sampler registration succeeds", registry.addSampler({ 10, 6 }, {}));
    ensure("pipeline registration succeeds", registry.addPipeline({ 11, 7 }, { "deferred.material.normspec", 0 }, shader));

    ensure("stale vertex generation is rejected", registry.resolveVertexBuffer({ 1, 3 }, { 2, 8 }) == nullptr);
    ensure("stale index generation is rejected", registry.resolveVertexBuffer({ 1, 4 }, { 2, 7 }) == nullptr);
    ensure("stale sampled generation is rejected", registry.resolveSampledImage({ 3, 1 }) == nullptr);
    ensure("stale output generation is rejected", registry.resolveTargetImage({ 7, 1 }).mTarget == nullptr);
    ensure("stale sampler generation is rejected", registry.resolve(SamplerHandle{ 10, 5 }) == nullptr);
    ensure("program variant mismatch is rejected",
           registry.resolve(PipelineHandle{ 11, 7 }, { "deferred.material.normspec", 12 }) == nullptr);
    ensure("program name mismatch is rejected", registry.resolve(PipelineHandle{ 11, 7 }, { "other.material", 0 }) == nullptr);
    ensure("stale pipeline generation is rejected",
           registry.resolve(PipelineHandle{ 11, 6 }, { "deferred.material.normspec", 0 }) == nullptr);

    ensure("a second buffer pair is rejected", !registry.addVertexBuffer({ 12, 1 }, { 13, 1 }, vertex_buffer));
    ensure("a null buffer object is rejected", !Registry{}.addVertexBuffer({ 1, 1 }, { 2, 1 }, nullptr));
    ensure("a null GL texture is rejected", !Registry{}.addSampledImage({ 1, 1 }, { 0, PixelFormat::RGBA8Unorm, { 4, 4 }, 3 }));
    ensure("a sampled image index cannot be reused with a new generation",
           !registry.addSampledImage({ 3, 9 }, { 104, PixelFormat::RGBA8Unorm, { 4, 4 }, 3 }));
    ensure("a sampled GL texture cannot alias an existing sampled image",
           !registry.addSampledImage({ 4, 1 }, { 101, PixelFormat::RGBA8Unorm, { 4, 4 }, 3 }));
    ensure("a second sampler is rejected", !registry.addSampler({ 12, 1 }, {}));
    Registry alias_registry;
    ensure("alias setup sampled image succeeds", alias_registry.addSampledImage({ 3, 1 }, { 201, PixelFormat::RGBA8Unorm, { 4, 4 }, 3 }));
    ensure("an output handle cannot alias a sampled handle index",
           !alias_registry.addRenderTarget({ ImageHandle{ 3, 2 }, ImageHandle{ 4, 1 }, ImageHandle{ 5, 1 } }, { 6, 1 }, target));
}

} // namespace tut
