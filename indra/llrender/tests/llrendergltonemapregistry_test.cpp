/**
 * @file llrendergltonemapregistry_test.cpp
 * @brief Tests for generation-safe tonemap OpenGL object resolution.
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

#include "llrendergltonemap.h"
#include "lltut.h"

#include <cstdint>

namespace tut
{

struct render_gl_tonemap_registry_test
{
};

using render_gl_tonemap_registry_group = test_group<render_gl_tonemap_registry_test>;
using render_gl_tonemap_registry_object = render_gl_tonemap_registry_group::object;
render_gl_tonemap_registry_group render_gl_tonemap_registry_tests("render GL tonemap registry");

template<>
template<>
void render_gl_tonemap_registry_object::test<1>()
{
    using namespace LLRenderContract;
    using namespace LLRenderGLTonemap;

    Registry registry;
    auto* buffer = reinterpret_cast<LLVertexBuffer*>(std::uintptr_t{ 0x10 });
    auto* image = reinterpret_cast<LLRenderTarget*>(std::uintptr_t{ 0x20 });
    auto* shader = reinterpret_cast<LLGLSLShader*>(std::uintptr_t{ 0x30 });

    ensure("buffer registration succeeds", registry.addBuffer({ 1, 4 }, buffer));
    ensure("image registration succeeds", registry.addImage({ 2, 7 }, image));
    ensure("sampler registration succeeds", registry.addSampler({ 3, 2 }, Sampler::Point));
    ensure("pipeline registration succeeds", registry.addPipeline({ 4, 9 }, { "deferred.tonemap", 3 }, shader));

    ensure("exact buffer generation resolves", registry.resolve(BufferHandle{ 1, 4 }) == buffer);
    ensure("exact image generation resolves", registry.resolve(ImageHandle{ 2, 7 }) == image);
    ensure("exact sampler generation resolves", registry.resolve(SamplerHandle{ 3, 2 }) &&
                                                *registry.resolve(SamplerHandle{ 3, 2 }) == Sampler::Point);
    ensure("exact pipeline key resolves", registry.resolve(PipelineHandle{ 4, 9 }, { "deferred.tonemap", 3 }) == shader);
}

template<>
template<>
void render_gl_tonemap_registry_object::test<2>()
{
    using namespace LLRenderContract;
    using namespace LLRenderGLTonemap;

    Registry registry;
    auto* image = reinterpret_cast<LLRenderTarget*>(std::uintptr_t{ 0x20 });
    auto* shader = reinterpret_cast<LLGLSLShader*>(std::uintptr_t{ 0x30 });
    ensure("image registration succeeds", registry.addImage({ 2, 7 }, image));
    ensure("pipeline registration succeeds", registry.addPipeline({ 4, 9 }, { "deferred.tonemap", 3 }, shader));

    ensure("stale image generation is rejected", registry.resolve(ImageHandle{ 2, 6 }) == nullptr);
    ensure("unknown image index is rejected", registry.resolve(ImageHandle{ 8, 7 }) == nullptr);
    ensure("program variant mismatch is rejected", registry.resolve(PipelineHandle{ 4, 9 }, { "deferred.tonemap", 2 }) == nullptr);
    ensure("program name mismatch is rejected", registry.resolve(PipelineHandle{ 4, 9 }, { "other.tonemap", 3 }) == nullptr);
    ensure("stale pipeline generation is rejected", registry.resolve(PipelineHandle{ 4, 8 }, { "deferred.tonemap", 3 }) == nullptr);
    ensure("duplicate live index is rejected", !registry.addImage({ 2, 8 }, image));
    ensure("null object is rejected", !registry.addImage({ 9, 1 }, nullptr));
}

}
