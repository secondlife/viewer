/**
 * @file lldrawinfotranslator.h
 * @brief Translation boundary from viewer draw state to an owned draw packet.
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

#ifndef LL_LLDRAWINFOTRANSLATOR_H
#define LL_LLDRAWINFOTRANSLATOR_H

#include "lldrawpacketcontract.h"

#include <cstdint>
#include <optional>

class LLDrawInfo;
class LLVertexBuffer;
class LLViewerTexture;

namespace LLDrawInfoAdapter
{

enum class SubmissionKind : std::uint8_t
{
    Invalid,
    DeferredMaterial
};

enum class RenderDomain : std::uint8_t
{
    Invalid,
    World,
    HUD,
    Impostor,
    Reflection,
    Cube
};

enum class TextureRole : std::uint8_t
{
    Diffuse,
    Normal,
    Specular
};

struct Context
{
    std::uint64_t                               mFrame = 0;
    LLRenderContract::PassId                    mPass;
    LLRenderContract::LegacyNormSpecPipelineKey mPipelineKey;
    RenderDomain                                mRenderDomain = RenderDomain::Invalid;
    SubmissionKind                              mSubmission   = SubmissionKind::Invalid;
};

struct ResolvedGeometry
{
    LLRenderContract::BufferHandle mVertexBuffer;
    LLRenderContract::BufferHandle mIndexBuffer;
    std::uint64_t                  mVertexBufferSize = 0;
    std::uint64_t                  mIndexBufferSize  = 0;
    std::uint32_t                  mVertexCount      = 0;
    std::uint32_t                  mIndexCount       = 0;
    LLRenderContract::IndexType    mIndexType        = LLRenderContract::IndexType::UInt16;
};

class Resolver
{
public:
    virtual ~Resolver() = default;

    // Successful resolutions describe the source's current live generation and
    // validated immutable metadata. Retired, stale, or policy-incompatible sources return nullopt.
    virtual std::optional<ResolvedGeometry>                   resolveGeometry(const LLVertexBuffer& buffer) const                  = 0;
    virtual std::optional<LLRenderContract::DrawTextureInput> resolveImage(const LLViewerTexture& texture, TextureRole role) const = 0;
    virtual std::optional<LLRenderContract::PipelineHandle>   resolvePipeline(
          const LLRenderContract::LegacyNormSpecPipelineKey& key) const = 0;
};

std::optional<LLRenderContract::LegacyNormSpecDrawPacket> translateNonRiggedNormSpecDraw(const LLDrawInfo& draw_info,
                                                                                         std::uint32_t render_type, const Context& context,
                                                                                         const Resolver& resolver);

} // namespace LLDrawInfoAdapter

#endif // LL_LLDRAWINFOTRANSLATOR_H
