/**
 * @file lldrawinfotranslator.cpp
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

#include "llviewerprecompiledheaders.h"

#include "lldrawinfotranslator.h"

#include "lldrawpool.h"
#include "llspatialpartition.h"

#include <algorithm>
#include <cstddef>

namespace LLDrawInfoAdapter
{
namespace
{

    constexpr std::uint32_t REQUIRED_VERTEX_MASK = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0 |
                                                   LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2 |
                                                   LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TANGENT;
    constexpr std::uint32_t KNOWN_VERTEX_MASK = (1u << LLVertexBuffer::TYPE_MAX) - 1u;

    bool productionPipelineKey(const LLRenderContract::LegacyNormSpecPipelineKey& key)
    {
        static const LLRenderContract::LegacyNormSpecPipelineKey modern        = LLRenderContract::legacyNormSpecModernHDRPipelineKey();
        static const LLRenderContract::LegacyNormSpecPipelineKey compatibility = LLRenderContract::legacyNormSpecCompatibilityPipelineKey();
        return key == modern || key == compatibility;
    }

    LLRenderContract::DrawMatrix4 copyMatrix(const LLMatrix4* matrix)
    {
        if (!matrix)
        {
            return LLRenderContract::DRAW_IDENTITY_MATRIX4;
        }

        LLRenderContract::DrawMatrix4 result;
        for (std::size_t row = 0; row < 4; ++row)
        {
            for (std::size_t column = 0; column < 4; ++column)
            {
                result[row * 4 + column] = matrix->mMatrix[row][column];
            }
        }
        return result;
    }

    bool supportedSource(const LLDrawInfo& draw_info)
    {
        if (draw_info.mVertexBuffer.isNull() || draw_info.mTexture.isNull() || draw_info.mNormalMap.isNull() ||
            draw_info.mSpecularMap.isNull() || draw_info.mMaterial.isNull() || draw_info.mGLTFMaterial.notNull() ||
            draw_info.mAvatar.notNull() || draw_info.mSkinInfo || draw_info.mHasGlow || draw_info.mNormalMapMatrix ||
            draw_info.mSpecularMapMatrix)
        {
            return false;
        }

        const std::uint32_t vertex_mask = draw_info.mVertexBuffer->getTypeMask();
        if ((vertex_mask & REQUIRED_VERTEX_MASK) != REQUIRED_VERTEX_MASK || (vertex_mask & ~KNOWN_VERTEX_MASK) != 0)
        {
            return false;
        }

        for (const LLPointer<LLViewerTexture>& texture : draw_info.mTextureList)
        {
            if (texture.notNull() && texture.get() != draw_info.mTexture.get())
            {
                return false;
            }
        }
        return true;
    }

    bool supportedGeometry(const LLDrawInfo& draw_info, const ResolvedGeometry& geometry)
    {
        if (!geometry.mVertexBuffer || !geometry.mIndexBuffer || geometry.mVertexBuffer == geometry.mIndexBuffer ||
            geometry.mVertexCount == 0 || geometry.mIndexCount == 0 || geometry.mVertexBufferSize == 0 || geometry.mIndexBufferSize == 0 ||
            draw_info.mCount == 0 || draw_info.mStart > draw_info.mEnd || draw_info.mEnd >= geometry.mVertexCount ||
            draw_info.mOffset > geometry.mIndexCount || draw_info.mCount > geometry.mIndexCount - draw_info.mOffset)
        {
            return false;
        }

        std::uint64_t index_size = 0;
        switch (geometry.mIndexType)
        {
            case LLRenderContract::IndexType::UInt16:
                index_size = sizeof(std::uint16_t);
                break;
            case LLRenderContract::IndexType::UInt32:
                index_size = sizeof(std::uint32_t);
                break;
            default:
                return false;
        }

        std::uint64_t required_vertex_bytes = 0;
        for (std::uint32_t type = 0; type < LLVertexBuffer::TYPE_TEXTURE_INDEX; ++type)
        {
            if ((draw_info.mVertexBuffer->getTypeMask() & (1u << type)) != 0)
            {
                required_vertex_bytes += static_cast<std::uint64_t>(LLVertexBuffer::sTypeSize[type]) * geometry.mVertexCount;
                required_vertex_bytes = (required_vertex_bytes + 15u) & ~std::uint64_t{ 15u };
            }
        }
        const std::uint64_t required_index_bytes = index_size * geometry.mIndexCount;
        const std::uint64_t draw_index_end =
            index_size * (static_cast<std::uint64_t>(draw_info.mOffset) + static_cast<std::uint64_t>(draw_info.mCount));

        return required_vertex_bytes <= geometry.mVertexBufferSize && required_index_bytes <= geometry.mIndexBufferSize &&
               draw_index_end <= geometry.mIndexBufferSize;
    }

    bool consistentImageIdentity(const LLViewerTexture*                    left_source,
                                 const LLRenderContract::DrawTextureInput& left,
                                 const LLViewerTexture*                    right_source,
                                 const LLRenderContract::DrawTextureInput& right)
    {
        return left_source != right_source || left.mImage == right.mImage;
    }

} // namespace

std::optional<LLRenderContract::LegacyNormSpecDrawPacket> translateNonRiggedNormSpecDraw(const LLDrawInfo& draw_info,
                                                                                         std::uint32_t render_type, const Context& context,
                                                                                         const Resolver& resolver)
{
    if (render_type != static_cast<std::uint32_t>(LLRenderPass::PASS_NORMSPEC) || context.mSubmission != SubmissionKind::DeferredMaterial ||
        context.mRenderDomain != RenderDomain::World || context.mFrame == 0 || !context.mPass ||
        !productionPipelineKey(context.mPipelineKey) || !supportedSource(draw_info))
    {
        return std::nullopt;
    }

    const std::optional<ResolvedGeometry> geometry = resolver.resolveGeometry(*draw_info.mVertexBuffer);
    if (!geometry || !supportedGeometry(draw_info, *geometry))
    {
        return std::nullopt;
    }

    const std::optional<LLRenderContract::DrawTextureInput> diffuse = resolver.resolveImage(*draw_info.mTexture, TextureRole::Diffuse);
    const std::optional<LLRenderContract::DrawTextureInput> normal  = resolver.resolveImage(*draw_info.mNormalMap, TextureRole::Normal);
    const std::optional<LLRenderContract::DrawTextureInput> specular =
        resolver.resolveImage(*draw_info.mSpecularMap, TextureRole::Specular);
    if (!diffuse || !normal || !specular ||
        !consistentImageIdentity(draw_info.mTexture.get(), *diffuse, draw_info.mNormalMap.get(), *normal) ||
        !consistentImageIdentity(draw_info.mTexture.get(), *diffuse, draw_info.mSpecularMap.get(), *specular) ||
        !consistentImageIdentity(draw_info.mNormalMap.get(), *normal, draw_info.mSpecularMap.get(), *specular))
    {
        return std::nullopt;
    }

    const std::optional<LLRenderContract::PipelineHandle> pipeline = resolver.resolvePipeline(context.mPipelineKey);
    if (!pipeline)
    {
        return std::nullopt;
    }

    LLRenderContract::LegacyNormSpecDrawInputs inputs;
    inputs.mFrame                 = context.mFrame;
    inputs.mPass                  = context.mPass;
    inputs.mHandles.mVertexBuffer = geometry->mVertexBuffer;
    inputs.mHandles.mIndexBuffer  = geometry->mIndexBuffer;
    inputs.mHandles.mPipeline     = *pipeline;
    inputs.mDescriptors.mDiffuse  = *diffuse;
    inputs.mDescriptors.mNormal   = *normal;
    inputs.mDescriptors.mSpecular = *specular;
    inputs.mPipelineKey           = context.mPipelineKey;
    inputs.mIndexType             = geometry->mIndexType;
    inputs.mFirstIndex            = draw_info.mOffset;
    inputs.mIndexCount            = draw_info.mCount;
    inputs.mMinVertex             = draw_info.mStart;
    inputs.mMaxVertex             = draw_info.mEnd;
    inputs.mModelMatrix           = copyMatrix(draw_info.mModelMatrix);
    inputs.mDiffuseTextureMatrix  = copyMatrix(draw_info.mTextureMatrix);
    std::copy_n(draw_info.mSpecColor.mV, inputs.mSpecularRGBA.size(), inputs.mSpecularRGBA.begin());
    inputs.mEnvironmentIntensity = draw_info.mEnvIntensity;
    inputs.mAlphaCutoff          = draw_info.mAlphaMaskCutoff;
    inputs.mEmissiveBrightness   = draw_info.mFullbright ? 1.f : 0.f;

    return LLRenderContract::buildLegacyNormSpecDrawPacket(inputs);
}

} // namespace LLDrawInfoAdapter
