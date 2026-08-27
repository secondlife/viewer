/**
 * @file llrendercontract.cpp
 * @brief Validation for API-neutral renderer work descriptions.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#include "llrendercontract.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace LLRenderContract
{
namespace
{

    template<typename Value>
    bool addWouldOverflow(Value left, Value right)
    {
        return right > std::numeric_limits<Value>::max() - left;
    }

    template<typename Value>
    bool multiplyWouldOverflow(Value left, Value right)
    {
        return left != 0 && right > std::numeric_limits<Value>::max() / left;
    }

    template<typename Enum>
    bool validEnum(Enum value, Enum last)
    {
        static_assert(std::is_enum_v<Enum>);
        using Underlying        = std::underlying_type_t<Enum>;
        const Underlying number = static_cast<Underlying>(value);
        return number >= 0 && number <= static_cast<Underlying>(last);
    }

    std::uint32_t bytesPerPixel(PixelFormat format)
    {
        switch (format)
        {
            case PixelFormat::R8Unorm:
                return 1;
            case PixelFormat::RG8Unorm:
                return 2;
            case PixelFormat::RGB8Unorm:
                return 3;
            case PixelFormat::R16Float:
                return 2;
            case PixelFormat::RGBA8Unorm:
            case PixelFormat::RGBA8Srgb:
            case PixelFormat::RGB10A2Unorm:
            case PixelFormat::Depth24Unorm:
            case PixelFormat::Depth32Float:
                return 4;
            case PixelFormat::RGBA16Unorm:
                return 8;
            case PixelFormat::RGB16Float:
                return 6;
            case PixelFormat::RGBA16Float:
                return 8;
        }
        return 0;
    }

    bool isDepthFormat(PixelFormat format)
    {
        return format == PixelFormat::Depth24Unorm || format == PixelFormat::Depth32Float;
    }

    std::uint32_t maxMipLevels(Extent2D extent)
    {
        std::uint32_t levels = 0;
        while (extent.mWidth != 0 || extent.mHeight != 0)
        {
            ++levels;
            extent.mWidth /= 2;
            extent.mHeight /= 2;
        }
        return levels;
    }

    std::uint32_t vertexFormatSize(VertexFormat format)
    {
        switch (format)
        {
            case VertexFormat::Float2:
                return 8;
            case VertexFormat::Float3:
                return 12;
            case VertexFormat::Float4:
                return 16;
            case VertexFormat::UNorm8x4:
                return 4;
        }
        return 0;
    }

    std::uint32_t indexSize(IndexType type)
    {
        switch (type)
        {
            case IndexType::UInt16:
                return 2;
            case IndexType::UInt32:
                return 4;
        }
        return 0;
    }

    Extent2D mipExtent(const ImageResource& image, std::uint32_t mip_level)
    {
        Extent2D extent = image.mExtent;
        for (std::uint32_t mip = 0; mip < mip_level; ++mip)
        {
            extent.mWidth  = std::max(1u, extent.mWidth / 2);
            extent.mHeight = std::max(1u, extent.mHeight / 2);
        }
        return extent;
    }

    bool contains(const std::vector<std::uint32_t>& values, std::uint32_t value)
    {
        return std::find(values.begin(), values.end(), value) != values.end();
    }

    template<typename Value>
    bool containsDuplicate(const std::vector<Value>& values)
    {
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            if (std::find(values.begin() + index + 1, values.end(), values[index]) != values.end())
            {
                return true;
            }
        }
        return false;
    }

    class Validator
    {
    public:
        explicit Validator(const FrameSnapshot& frame) : mFrame(frame) {}

        ValidationResult run()
        {
            validateResources();
            validateUploads();
            validatePasses();
            validateReleases();
            validateFrameLifetimes();
            return std::move(mResult);
        }

    private:
        template<typename Descriptor, typename ResourceHandleType>
        static const Descriptor* find(const std::vector<Descriptor>& resources, ResourceHandleType handle)
        {
            const auto found = std::find_if(resources.begin(), resources.end(),
                                            [handle](const Descriptor& resource) { return resource.mHandle == handle; });
            return found == resources.end() ? nullptr : &*found;
        }

        template<typename Descriptor>
        void validateResourceHandles(const std::vector<Descriptor>& resources, const std::string& path)
        {
            for (std::size_t index = 0; index < resources.size(); ++index)
            {
                if (!resources[index].mHandle)
                {
                    error(ValidationCode::InvalidHandle, path + "[" + std::to_string(index) + "]",
                          "resource handle must have a non-zero index and generation");
                }
                for (std::size_t prior = 0; prior < index; ++prior)
                {
                    if (resources[prior].mHandle == resources[index].mHandle)
                    {
                        error(ValidationCode::DuplicateResource, path + "[" + std::to_string(index) + "]",
                              "resource handle is declared more than once");
                        break;
                    }
                }
            }
        }

        void error(ValidationCode code, std::string path, std::string message)
        {
            mResult.mErrors.push_back({ code, std::move(path), std::move(message) });
        }

        bool validBytes(const ByteRange& bytes, const std::string& path)
        {
            if (!bytes.mStorage)
            {
                error(ValidationCode::InvalidBinding, path, "byte storage is not owned");
                return false;
            }
            if (bytes.mOffset > bytes.mStorage->size() || bytes.mSize > bytes.mStorage->size() - bytes.mOffset)
            {
                error(ValidationCode::OutOfBounds, path, "byte range exceeds its owned storage");
                return false;
            }
            return true;
        }

        bool validSubresource(const ImageResource& image, const ImageSubresource& subresource, const std::string& path)
        {
            if (subresource.mMipLevel >= image.mMipLevels || subresource.mArrayLayer >= image.mArrayLayers)
            {
                error(ValidationCode::OutOfBounds, path, "image subresource is outside the declared image");
                return false;
            }
            return true;
        }

        bool validSubresourceRange(const ImageResource& image, const ImageSubresourceRange& range, const std::string& path)
        {
            if (range.mMipLevelCount == 0 || range.mArrayLayerCount == 0 || range.mBaseMipLevel >= image.mMipLevels ||
                range.mBaseArrayLayer >= image.mArrayLayers || range.mMipLevelCount > image.mMipLevels - range.mBaseMipLevel ||
                range.mArrayLayerCount > image.mArrayLayers - range.mBaseArrayLayer)
            {
                error(ValidationCode::OutOfBounds, path, "image subresource range is outside the declared image");
                return false;
            }
            return true;
        }

        template<typename Function>
        static void forEachSubresource(const ImageSubresourceRange& range, Function&& function)
        {
            for (std::uint32_t layer = range.mBaseArrayLayer; layer < range.mBaseArrayLayer + range.mArrayLayerCount; ++layer)
            {
                for (std::uint32_t mip = range.mBaseMipLevel; mip < range.mBaseMipLevel + range.mMipLevelCount; ++mip)
                {
                    function(ImageSubresource{ mip, layer });
                }
            }
        }

        static bool rangesOverlap(const ImageSubresourceRange& left, const ImageSubresourceRange& right)
        {
            const std::uint64_t left_mip_end    = static_cast<std::uint64_t>(left.mBaseMipLevel) + left.mMipLevelCount;
            const std::uint64_t right_mip_end   = static_cast<std::uint64_t>(right.mBaseMipLevel) + right.mMipLevelCount;
            const std::uint64_t left_layer_end  = static_cast<std::uint64_t>(left.mBaseArrayLayer) + left.mArrayLayerCount;
            const std::uint64_t right_layer_end = static_cast<std::uint64_t>(right.mBaseArrayLayer) + right.mArrayLayerCount;
            return left.mBaseMipLevel < right_mip_end && right.mBaseMipLevel < left_mip_end && left.mBaseArrayLayer < right_layer_end &&
                   right.mBaseArrayLayer < left_layer_end;
        }

        static bool rangeContains(const ImageSubresourceRange& outer, const ImageSubresourceRange& inner)
        {
            const std::uint64_t outer_mip_end   = static_cast<std::uint64_t>(outer.mBaseMipLevel) + outer.mMipLevelCount;
            const std::uint64_t inner_mip_end   = static_cast<std::uint64_t>(inner.mBaseMipLevel) + inner.mMipLevelCount;
            const std::uint64_t outer_layer_end = static_cast<std::uint64_t>(outer.mBaseArrayLayer) + outer.mArrayLayerCount;
            const std::uint64_t inner_layer_end = static_cast<std::uint64_t>(inner.mBaseArrayLayer) + inner.mArrayLayerCount;
            return outer.mBaseMipLevel <= inner.mBaseMipLevel && outer_mip_end >= inner_mip_end &&
                   outer.mBaseArrayLayer <= inner.mBaseArrayLayer && outer_layer_end >= inner_layer_end;
        }

        static ImageSubresourceRange singleSubresource(ImageSubresource subresource)
        {
            return { subresource.mMipLevel, 1, subresource.mArrayLayer, 1 };
        }

        void validateResources()
        {
            if (mFrame.mFrame == 0)
            {
                error(ValidationCode::InvalidFrame, "frame", "frame serial must be non-zero");
            }

            validateResourceHandles(mFrame.mBuffers, "buffers");
            validateResourceHandles(mFrame.mImages, "images");
            validateResourceHandles(mFrame.mSamplers, "samplers");
            validateResourceHandles(mFrame.mPipelines, "pipelines");

            for (std::size_t index = 0; index < mFrame.mBuffers.size(); ++index)
            {
                if (mFrame.mBuffers[index].mSize == 0 || !validEnum(mFrame.mBuffers[index].mLifetime, ResourceLifetime::Frame))
                {
                    error(ValidationCode::InvalidResource, "buffers[" + std::to_string(index) + "]", "buffer size must be non-zero");
                }
            }

            for (std::size_t index = 0; index < mFrame.mImages.size(); ++index)
            {
                const ImageResource& image = mFrame.mImages[index];
                if (image.mExtent.mWidth == 0 || image.mExtent.mHeight == 0 || image.mMipLevels == 0 || image.mArrayLayers == 0 ||
                    image.mSamples == 0 || image.mMipLevels > maxMipLevels(image.mExtent) ||
                    !validEnum(image.mFormat, PixelFormat::RGB16Float) || !validEnum(image.mLifetime, ResourceLifetime::Frame))
                {
                    error(ValidationCode::InvalidResource, "images[" + std::to_string(index) + "]",
                          "image dimensions, subresource counts, and samples must be non-zero");
                }
            }

            for (std::size_t index = 0; index < mFrame.mPipelines.size(); ++index)
            {
                validatePipeline(mFrame.mPipelines[index], "pipelines[" + std::to_string(index) + "]");
            }

            for (std::size_t index = 0; index < mFrame.mSamplers.size(); ++index)
            {
                const SamplerResource& sampler = mFrame.mSamplers[index];
                if (!std::isfinite(sampler.mMaxAnisotropy) || sampler.mMaxAnisotropy < 1.f ||
                    !validEnum(sampler.mMinFilter, Filter::Linear) || !validEnum(sampler.mMagFilter, Filter::Linear) ||
                    !validEnum(sampler.mMipFilter, MipFilter::Linear) || !validEnum(sampler.mAddressU, AddressMode::Mirror) ||
                    !validEnum(sampler.mAddressV, AddressMode::Mirror) || !validEnum(sampler.mLifetime, ResourceLifetime::Frame))
                {
                    error(ValidationCode::InvalidResource, "samplers[" + std::to_string(index) + "]",
                          "sampler anisotropy must be finite and at least one");
                }
            }
        }

        void validatePipeline(const PipelineResource& pipeline, const std::string& path)
        {
            if (pipeline.mProgram.mName.empty() || pipeline.mSamples == 0 || pipeline.mColorTargets.empty() ||
                !validEnum(pipeline.mTopology, PrimitiveTopology::TriangleList) || !validEnum(pipeline.mCullMode, CullMode::Back) ||
                !validEnum(pipeline.mFrontFace, FrontFace::CounterClockwise) ||
                !validEnum(pipeline.mDepthCompare, CompareOp::LessOrEqual) || !validEnum(pipeline.mLifetime, ResourceLifetime::Frame))
            {
                error(ValidationCode::InvalidResource, path,
                      "graphics pipeline must declare a program, samples, and at least one color target");
            }
            if (pipeline.mDepthWriteEnabled && !pipeline.mDepthTestEnabled)
            {
                error(ValidationCode::InvalidResource, path, "depth writes require depth testing in this contract");
            }
            if ((pipeline.mDepthTestEnabled || pipeline.mDepthWriteEnabled) && !pipeline.mDepthFormat)
            {
                error(ValidationCode::InvalidResource, path, "depth state requires a depth format");
            }
            if (pipeline.mDepthFormat &&
                (!validEnum(*pipeline.mDepthFormat, PixelFormat::RGB16Float) || !isDepthFormat(*pipeline.mDepthFormat)))
            {
                error(ValidationCode::InvalidResource, path, "pipeline depth format is not a depth format");
            }
            for (const ColorTargetState& target : pipeline.mColorTargets)
            {
                if (!validEnum(target.mFormat, PixelFormat::RGB16Float) || isDepthFormat(target.mFormat) ||
                    (target.mWriteMask & ~0xfu) != 0)
                {
                    error(ValidationCode::InvalidResource, path, "color target format or write mask is invalid");
                }
            }

            std::vector<std::uint32_t> vertex_bindings;
            for (const VertexBindingLayout& binding : pipeline.mVertexBindings)
            {
                vertex_bindings.push_back(binding.mBinding);
                if (binding.mStride == 0)
                {
                    error(ValidationCode::InvalidBinding, path, "pipeline vertex binding stride must be non-zero");
                }
            }

            std::vector<VertexSemantic> semantics;
            for (const VertexAttribute& attribute : pipeline.mVertexAttributes)
            {
                semantics.push_back(attribute.mSemantic);
                if (!validEnum(attribute.mSemantic, VertexSemantic::TexCoord2) || !validEnum(attribute.mFormat, VertexFormat::UNorm8x4))
                {
                    error(ValidationCode::InvalidBinding, path, "pipeline vertex attribute enum is invalid");
                }
                const auto binding =
                    std::find_if(pipeline.mVertexBindings.begin(), pipeline.mVertexBindings.end(),
                                 [&attribute](const VertexBindingLayout& layout) { return layout.mBinding == attribute.mBinding; });
                if (binding == pipeline.mVertexBindings.end())
                {
                    error(ValidationCode::InvalidBinding, path, "pipeline vertex attribute names an absent binding layout");
                }
                else if (attribute.mOffset > binding->mStride || vertexFormatSize(attribute.mFormat) > binding->mStride - attribute.mOffset)
                {
                    error(ValidationCode::InvalidBinding, path, "pipeline vertex attribute does not fit its binding stride");
                }
            }
            std::vector<std::uint32_t> parameter_bindings;
            for (const ParameterLayout& parameter : pipeline.mParameterBindings)
            {
                parameter_bindings.push_back(parameter.mBinding);
                if (parameter.mSize == 0)
                {
                    error(ValidationCode::InvalidBinding, path, "pipeline parameter size must be non-zero");
                }
            }
            if (containsDuplicate(vertex_bindings) || containsDuplicate(semantics) || containsDuplicate(pipeline.mSampledImageBindings) ||
                containsDuplicate(parameter_bindings))
            {
                error(ValidationCode::InvalidBinding, path, "pipeline layout contains duplicate bindings");
            }
        }

        void validateUploads()
        {
            for (std::size_t index = 0; index < mFrame.mUploads.size(); ++index)
            {
                const TextureUpload& upload = mFrame.mUploads[index];
                const std::string    path   = "uploads[" + std::to_string(index) + "]";
                const ImageResource* image  = find(mFrame.mImages, upload.mDestination);

                if (!upload.mDestination)
                {
                    error(ValidationCode::InvalidHandle, path, "upload destination handle is invalid");
                    continue;
                }
                if (!image)
                {
                    error(ValidationCode::MissingResource, path, "upload destination is not declared");
                    continue;
                }
                if (upload.mRevision == 0)
                {
                    error(ValidationCode::InvalidUpload, path, "upload revision must be non-zero");
                }
                if (!validEnum(upload.mSourceFormat, PixelFormat::RGB16Float) || !validEnum(upload.mRowOrigin, RowOrigin::BottomLeft) ||
                    !validEnum(upload.mMipGeneration, MipGeneration::GenerateRemaining) ||
                    !validEnum(upload.mBefore, ImageState::DepthAttachment) || !validEnum(upload.mDuring, ImageState::DepthAttachment) ||
                    !validEnum(upload.mAfter, ImageState::DepthAttachment))
                {
                    error(ValidationCode::InvalidUpload, path, "upload enum value is outside the contract domain");
                    continue;
                }
                const std::uint64_t expected_logical_width =
                    upload.mResidentDiscard <= 31 ? static_cast<std::uint64_t>(image->mExtent.mWidth) << upload.mResidentDiscard : 0;
                const std::uint64_t expected_logical_height =
                    upload.mResidentDiscard <= 31 ? static_cast<std::uint64_t>(image->mExtent.mHeight) << upload.mResidentDiscard : 0;
                if (upload.mResidentDiscard > 31 || expected_logical_width > std::numeric_limits<std::uint32_t>::max() ||
                    expected_logical_height > std::numeric_limits<std::uint32_t>::max() ||
                    upload.mLogicalExtent.mWidth != expected_logical_width || upload.mLogicalExtent.mHeight != expected_logical_height)
                {
                    error(ValidationCode::InvalidUpload, path, "upload logical extent must equal resident extent shifted by discard");
                }
                if (!validSubresource(*image, upload.mSubresource, path))
                {
                    continue;
                }
                if (isDepthFormat(upload.mSourceFormat) || upload.mSourceFormat != image->mFormat)
                {
                    error(ValidationCode::InvalidUpload, path, "upload source format must match a non-depth destination format");
                }
                if (upload.mDuring != ImageState::TransferDestination || upload.mAfter == ImageState::Undefined)
                {
                    error(ValidationCode::InvalidState, path,
                          "upload must use transfer-destination state and publish a defined final state");
                }
                if (upload.mMipGeneration == MipGeneration::GenerateRemaining && upload.mSubresource.mMipLevel + 1 >= image->mMipLevels)
                {
                    error(ValidationCode::InvalidUpload, path, "mip generation requires destination levels below the uploaded level");
                }

                const Extent2D destination_extent = mipExtent(*image, upload.mSubresource.mMipLevel);
                if (upload.mExtent.mWidth == 0 || upload.mExtent.mHeight == 0 || upload.mOffset.mX > destination_extent.mWidth ||
                    upload.mOffset.mY > destination_extent.mHeight ||
                    upload.mExtent.mWidth > destination_extent.mWidth - upload.mOffset.mX ||
                    upload.mExtent.mHeight > destination_extent.mHeight - upload.mOffset.mY)
                {
                    error(ValidationCode::OutOfBounds, path, "upload region exceeds the destination image");
                }

                const std::uint64_t tight_row = static_cast<std::uint64_t>(upload.mExtent.mWidth) * bytesPerPixel(upload.mSourceFormat);
                if (upload.mRowPitch < tight_row)
                {
                    error(ValidationCode::InvalidUpload, path, "upload row pitch is smaller than one row");
                }
                else if (validBytes(upload.mPixels, path + ".pixels") && upload.mExtent.mHeight != 0)
                {
                    const std::uint64_t preceding_rows = upload.mExtent.mHeight - 1;
                    if (multiplyWouldOverflow(preceding_rows, static_cast<std::uint64_t>(upload.mRowPitch)) ||
                        addWouldOverflow(preceding_rows * upload.mRowPitch, tight_row) ||
                        preceding_rows * upload.mRowPitch + tight_row > upload.mPixels.mSize)
                    {
                        error(ValidationCode::OutOfBounds, path + ".pixels", "upload byte range does not contain the declared rows");
                    }
                }

                for (std::size_t prior = 0; prior < index; ++prior)
                {
                    if (mFrame.mUploads[prior].mDestination == upload.mDestination && mFrame.mUploads[prior].mRevision >= upload.mRevision)
                    {
                        error(ValidationCode::InvalidUpload, path,
                              "upload revisions for one destination must increase in declaration order");
                        break;
                    }
                }

                transition(upload.mDestination, upload.mSubresource, upload.mBefore, upload.mAfter, path);
                if (upload.mMipGeneration == MipGeneration::GenerateRemaining)
                {
                    for (std::uint32_t mip = upload.mSubresource.mMipLevel + 1; mip < image->mMipLevels; ++mip)
                    {
                        transition(upload.mDestination, { mip, upload.mSubresource.mArrayLayer }, ImageState::Undefined, upload.mAfter,
                                   path);
                    }
                }
            }
        }

        void validatePasses()
        {
            std::vector<PassId> seen;
            for (std::size_t index = 0; index < mFrame.mPasses.size(); ++index)
            {
                const RenderPass& pass = mFrame.mPasses[index];
                const std::string path = "passes[" + std::to_string(index) + "]";

                if (!pass.mId)
                {
                    error(ValidationCode::InvalidPass, path, "pass id must be non-zero");
                }
                else if (std::find(seen.begin(), seen.end(), pass.mId) != seen.end())
                {
                    error(ValidationCode::InvalidPass, path, "pass id is duplicated");
                }

                for (PassId dependency : pass.mDependencies)
                {
                    if (!dependency || std::find(seen.begin(), seen.end(), dependency) == seen.end())
                    {
                        error(ValidationCode::InvalidDependency, path, "pass dependency must name an earlier pass");
                    }
                }
                if (containsDuplicate(pass.mDependencies))
                {
                    error(ValidationCode::InvalidDependency, path, "pass dependency is duplicated");
                }

                validatePassShape(pass, path);
                validatePassAccesses(pass, path);
                validateAttachments(pass, path);
                for (std::size_t draw_index = 0; draw_index < pass.mDraws.size(); ++draw_index)
                {
                    const std::string draw_path = path + ".draws[" + std::to_string(draw_index) + "]";
                    std::visit([&](const auto& draw) { validateDraw(pass, draw, draw_path); }, pass.mDraws[draw_index]);
                }

                seen.push_back(pass.mId);
            }
        }

        void validatePassShape(const RenderPass& pass, const std::string& path)
        {
            if (pass.mExtent.mWidth == 0 || pass.mExtent.mHeight == 0 || pass.mDraws.empty() || pass.mColorAttachments.empty())
            {
                error(ValidationCode::InvalidPass, path, "render pass needs an extent, color attachment, and draw");
            }

            const Viewport& viewport = pass.mViewport;
            if (!std::isfinite(viewport.mX) || !std::isfinite(viewport.mY) || !std::isfinite(viewport.mWidth) ||
                !std::isfinite(viewport.mHeight) || !std::isfinite(viewport.mMinDepth) || !std::isfinite(viewport.mMaxDepth) ||
                viewport.mX < 0.f || viewport.mY < 0.f || viewport.mWidth <= 0.f || viewport.mHeight <= 0.f ||
                viewport.mX + viewport.mWidth > pass.mExtent.mWidth || viewport.mY + viewport.mHeight > pass.mExtent.mHeight ||
                viewport.mMinDepth < 0.f || viewport.mMaxDepth > 1.f || viewport.mMinDepth > viewport.mMaxDepth)
            {
                error(ValidationCode::InvalidPass, path + ".viewport", "viewport must be finite and inside the pass extent");
            }

            const Scissor& scissor = pass.mScissor;
            if (scissor.mWidth == 0 || scissor.mHeight == 0 || scissor.mX > pass.mExtent.mWidth || scissor.mY > pass.mExtent.mHeight ||
                scissor.mWidth > pass.mExtent.mWidth - scissor.mX || scissor.mHeight > pass.mExtent.mHeight - scissor.mY)
            {
                error(ValidationCode::InvalidPass, path + ".scissor", "scissor must be inside the pass extent");
            }
        }

        void validatePassAccesses(const RenderPass& pass, const std::string& path)
        {
            for (std::size_t index = 0; index < pass.mBufferAccesses.size(); ++index)
            {
                const BufferAccess& access = pass.mBufferAccesses[index];
                if (!validEnum(access.mKind, BufferAccessKind::IndexRead))
                {
                    error(ValidationCode::InvalidBinding, path + ".bufferAccesses[" + std::to_string(index) + "]",
                          "buffer access kind is outside the contract domain");
                }
                if (!access.mBuffer)
                {
                    error(ValidationCode::InvalidHandle, path + ".bufferAccesses[" + std::to_string(index) + "]",
                          "buffer access handle is invalid");
                }
                else if (!find(mFrame.mBuffers, access.mBuffer))
                {
                    error(ValidationCode::MissingResource, path + ".bufferAccesses[" + std::to_string(index) + "]",
                          "buffer access names an undeclared generation");
                }
            }

            for (std::size_t index = 0; index < pass.mImageAccesses.size(); ++index)
            {
                const ImageAccess&   access      = pass.mImageAccesses[index];
                const std::string    access_path = path + ".imageAccesses[" + std::to_string(index) + "]";
                const ImageResource* image       = find(mFrame.mImages, access.mImage);
                if (!access.mImage)
                {
                    error(ValidationCode::InvalidHandle, access_path, "image access handle is invalid");
                    continue;
                }
                if (!image)
                {
                    error(ValidationCode::MissingResource, access_path, "image access names an undeclared generation");
                    continue;
                }
                if (!validEnum(access.mKind, ImageAccessKind::DepthAttachmentReadWrite) ||
                    !validEnum(access.mBefore, ImageState::DepthAttachment) || !validEnum(access.mDuring, ImageState::DepthAttachment) ||
                    !validEnum(access.mAfter, ImageState::DepthAttachment))
                {
                    error(ValidationCode::InvalidState, access_path, "image access enum value is outside the contract domain");
                    continue;
                }
                if (!validSubresourceRange(*image, access.mRange, access_path))
                {
                    continue;
                }

                ImageState required_state = ImageState::ShaderRead;
                switch (access.mKind)
                {
                    case ImageAccessKind::SampledRead:
                        required_state = ImageState::ShaderRead;
                        if (access.mBefore == ImageState::Undefined)
                        {
                            error(ValidationCode::InvalidState, access_path, "sampled reads cannot discard prior image contents");
                        }
                        break;
                    case ImageAccessKind::ColorAttachmentWrite:
                        required_state = ImageState::ColorAttachment;
                        break;
                    case ImageAccessKind::DepthAttachmentReadWrite:
                        required_state = ImageState::DepthAttachment;
                        break;
                }
                if (access.mDuring != required_state ||
                    (access.mKind == ImageAccessKind::SampledRead && access.mAfter == ImageState::Undefined))
                {
                    error(ValidationCode::InvalidState, access_path, "image access kind and declared states disagree");
                }

                for (std::size_t prior = 0; prior < index; ++prior)
                {
                    if (pass.mImageAccesses[prior].mImage == access.mImage &&
                        rangesOverlap(pass.mImageAccesses[prior].mRange, access.mRange))
                    {
                        error(ValidationCode::InvalidPass, access_path, "one pass may declare an image subresource only once");
                        break;
                    }
                }
                forEachSubresource(access.mRange, [&](ImageSubresource subresource)
                                   { transition(access.mImage, subresource, access.mBefore, access.mAfter, access_path); });
            }
        }

        void validateAttachments(const RenderPass& pass, const std::string& path)
        {
            for (std::size_t index = 0; index < pass.mColorAttachments.size(); ++index)
            {
                const ColorAttachment& attachment      = pass.mColorAttachments[index];
                const std::string      attachment_path = path + ".colorAttachments[" + std::to_string(index) + "]";
                const ImageResource*   image           = find(mFrame.mImages, attachment.mImage);
                if (!validEnum(attachment.mLoad, LoadOp::DontCare) || !validEnum(attachment.mStore, StoreOp::DontCare) ||
                    (attachment.mLoad == LoadOp::Clear &&
                     (!std::isfinite(attachment.mClear.mRed) || !std::isfinite(attachment.mClear.mGreen) ||
                      !std::isfinite(attachment.mClear.mBlue) || !std::isfinite(attachment.mClear.mAlpha))))
                {
                    error(ValidationCode::InvalidPass, attachment_path, "color attachment operation or clear value is invalid");
                }
                if (!image)
                {
                    error(ValidationCode::MissingResource, attachment_path, "color attachment image is not declared");
                    continue;
                }
                if (isDepthFormat(image->mFormat) || !validSubresource(*image, attachment.mSubresource, attachment_path))
                {
                    error(ValidationCode::InvalidResource, attachment_path, "color attachment must name a color image subresource");
                    continue;
                }
                const Extent2D extent = mipExtent(*image, attachment.mSubresource.mMipLevel);
                if (extent.mWidth < pass.mExtent.mWidth || extent.mHeight < pass.mExtent.mHeight)
                {
                    error(ValidationCode::OutOfBounds, attachment_path, "color attachment is smaller than the render pass");
                }
                const ImageAccess* access = findImageAccess(pass, attachment.mImage, singleSubresource(attachment.mSubresource),
                                                            ImageAccessKind::ColorAttachmentWrite);
                if (!access)
                {
                    error(ValidationCode::MissingAccess, attachment_path, "color attachment write is not declared");
                }
                else if (attachment.mLoad == LoadOp::Load && access->mBefore == ImageState::Undefined)
                {
                    error(ValidationCode::InvalidState, attachment_path, "a load attachment cannot begin from undefined contents");
                }
                else if ((attachment.mStore == StoreOp::DontCare) != (access->mAfter == ImageState::Undefined))
                {
                    error(ValidationCode::InvalidState, attachment_path,
                          "attachment after-state must be undefined exactly when its contents are discarded");
                }
            }

            if (pass.mDepthAttachment)
            {
                const DepthAttachment& attachment      = *pass.mDepthAttachment;
                const std::string      attachment_path = path + ".depthAttachment";
                const ImageResource*   image           = find(mFrame.mImages, attachment.mImage);
                if (!validEnum(attachment.mLoad, LoadOp::DontCare) || !validEnum(attachment.mStore, StoreOp::DontCare) ||
                    (attachment.mLoad == LoadOp::Clear &&
                     (!std::isfinite(attachment.mClearDepth) || attachment.mClearDepth < 0.f || attachment.mClearDepth > 1.f)))
                {
                    error(ValidationCode::InvalidPass, attachment_path, "depth attachment operation or clear value is invalid");
                }
                if (!image || !isDepthFormat(image->mFormat))
                {
                    error(ValidationCode::MissingResource, attachment_path, "depth attachment must name a declared depth image");
                }
                else
                {
                    if (validSubresource(*image, attachment.mSubresource, attachment_path))
                    {
                        const Extent2D extent = mipExtent(*image, attachment.mSubresource.mMipLevel);
                        if (extent.mWidth < pass.mExtent.mWidth || extent.mHeight < pass.mExtent.mHeight)
                        {
                            error(ValidationCode::OutOfBounds, attachment_path, "depth attachment is smaller than the render pass");
                        }
                    }
                }
                const ImageAccess* access = findImageAccess(pass, attachment.mImage, singleSubresource(attachment.mSubresource),
                                                            ImageAccessKind::DepthAttachmentReadWrite);
                if (!access)
                {
                    error(ValidationCode::MissingAccess, attachment_path, "depth attachment access is not declared");
                }
                else if (attachment.mLoad == LoadOp::Load && access->mBefore == ImageState::Undefined)
                {
                    error(ValidationCode::InvalidState, attachment_path, "a load attachment cannot begin from undefined contents");
                }
                else if ((attachment.mStore == StoreOp::DontCare) != (access->mAfter == ImageState::Undefined))
                {
                    error(ValidationCode::InvalidState, attachment_path,
                          "attachment after-state must be undefined exactly when its contents are discarded");
                }
            }
        }

        const ImageAccess* findImageAccess(const RenderPass& pass, ImageHandle image, ImageSubresourceRange range,
                                           ImageAccessKind kind) const
        {
            const auto found =
                std::find_if(pass.mImageAccesses.begin(), pass.mImageAccesses.end(), [image, range, kind](const ImageAccess& access)
                             { return access.mImage == image && rangeContains(access.mRange, range) && access.mKind == kind; });
            return found == pass.mImageAccesses.end() ? nullptr : &*found;
        }

        bool hasBufferAccess(const RenderPass& pass, BufferHandle buffer, BufferAccessKind kind) const
        {
            return std::find_if(pass.mBufferAccesses.begin(), pass.mBufferAccesses.end(), [buffer, kind](const BufferAccess& access)
                                { return access.mBuffer == buffer && access.mKind == kind; }) != pass.mBufferAccesses.end();
        }

        const PipelineResource* validateDrawResources(const RenderPass& pass, const DrawResources& resources, const std::string& path)
        {
            const PipelineResource* pipeline = find(mFrame.mPipelines, resources.mPipeline);
            if (!resources.mPipeline)
            {
                error(ValidationCode::InvalidHandle, path, "draw pipeline handle is invalid");
                return nullptr;
            }
            if (!pipeline)
            {
                error(ValidationCode::MissingResource, path, "draw pipeline names an undeclared generation");
                return nullptr;
            }

            if (pipeline->mColorTargets.size() != pass.mColorAttachments.size())
            {
                error(ValidationCode::InvalidBinding, path, "pipeline color layout does not match the render pass");
            }
            else
            {
                for (std::size_t index = 0; index < pipeline->mColorTargets.size(); ++index)
                {
                    const ImageResource* image = find(mFrame.mImages, pass.mColorAttachments[index].mImage);
                    if (image && (image->mFormat != pipeline->mColorTargets[index].mFormat || image->mSamples != pipeline->mSamples))
                    {
                        error(ValidationCode::InvalidBinding, path, "pipeline color format or sample count does not match the attachment");
                    }
                }
            }

            if (pipeline->mDepthFormat.has_value() != pass.mDepthAttachment.has_value())
            {
                error(ValidationCode::InvalidBinding, path, "pipeline depth layout does not match the render pass");
            }
            else if (pipeline->mDepthFormat && pass.mDepthAttachment)
            {
                const ImageResource* image = find(mFrame.mImages, pass.mDepthAttachment->mImage);
                if (image && (image->mFormat != *pipeline->mDepthFormat || image->mSamples != pipeline->mSamples))
                {
                    error(ValidationCode::InvalidBinding, path, "pipeline depth format or sample count does not match the attachment");
                }
            }

            validateSampledBindings(pass, *pipeline, resources.mSampledImages, path);
            validateParameterBindings(*pipeline, resources.mParameters, path);
            validateVertexBindings(pass, *pipeline, resources.mVertexBuffers, path);
            return pipeline;
        }

        void validateSampledBindings(const RenderPass& pass, const PipelineResource& pipeline,
                                     const std::vector<SampledImageBinding>& bindings, const std::string& path)
        {
            std::vector<std::uint32_t> supplied;
            for (std::size_t index = 0; index < bindings.size(); ++index)
            {
                const SampledImageBinding& binding      = bindings[index];
                const std::string          binding_path = path + ".sampledImages[" + std::to_string(index) + "]";
                supplied.push_back(binding.mBinding);
                if (!contains(pipeline.mSampledImageBindings, binding.mBinding))
                {
                    error(ValidationCode::InvalidBinding, binding_path, "sampled-image binding is absent from the pipeline layout");
                }
                const ImageResource* image = find(mFrame.mImages, binding.mImage);
                if (!image || !find(mFrame.mSamplers, binding.mSampler))
                {
                    error(ValidationCode::MissingResource, binding_path, "sampled image or sampler is not declared");
                }
                else
                {
                    validSubresourceRange(*image, binding.mRange, binding_path);
                }
                if (!findImageAccess(pass, binding.mImage, binding.mRange, ImageAccessKind::SampledRead))
                {
                    error(ValidationCode::MissingAccess, binding_path, "sampled image read is not declared by the pass");
                }
            }
            if (containsDuplicate(supplied))
            {
                error(ValidationCode::InvalidBinding, path, "sampled-image binding is supplied more than once");
            }
            for (std::uint32_t required : pipeline.mSampledImageBindings)
            {
                if (!contains(supplied, required))
                {
                    error(ValidationCode::InvalidBinding, path, "required sampled-image binding is missing");
                }
            }
        }

        void validateParameterBindings(const PipelineResource& pipeline, const std::vector<ParameterBinding>& bindings,
                                       const std::string& path)
        {
            std::vector<std::uint32_t> supplied;
            for (std::size_t index = 0; index < bindings.size(); ++index)
            {
                const ParameterBinding& binding      = bindings[index];
                const std::string       binding_path = path + ".parameters[" + std::to_string(index) + "]";
                supplied.push_back(binding.mBinding);
                const auto layout =
                    std::find_if(pipeline.mParameterBindings.begin(), pipeline.mParameterBindings.end(),
                                 [&binding](const ParameterLayout& parameter) { return parameter.mBinding == binding.mBinding; });
                if (layout == pipeline.mParameterBindings.end())
                {
                    error(ValidationCode::InvalidBinding, binding_path, "parameter binding is absent from the pipeline layout");
                }
                else if (binding.mBytes.mSize != layout->mSize)
                {
                    error(ValidationCode::InvalidBinding, binding_path, "parameter byte count does not match the pipeline layout");
                }
                if (!validBytes(binding.mBytes, binding_path) || binding.mBytes.mSize == 0)
                {
                    error(ValidationCode::InvalidBinding, binding_path, "parameter binding must own a non-empty byte range");
                }
            }
            if (containsDuplicate(supplied))
            {
                error(ValidationCode::InvalidBinding, path, "parameter binding is supplied more than once");
            }
            for (const ParameterLayout& required : pipeline.mParameterBindings)
            {
                if (!contains(supplied, required.mBinding))
                {
                    error(ValidationCode::InvalidBinding, path, "required parameter binding is missing");
                }
            }
        }

        void validateVertexBindings(const RenderPass& pass, const PipelineResource& pipeline,
                                    const std::vector<VertexBufferBinding>& bindings, const std::string& path)
        {
            std::vector<std::uint32_t> required;
            for (const VertexBindingLayout& binding : pipeline.mVertexBindings)
            {
                required.push_back(binding.mBinding);
            }

            std::vector<std::uint32_t> supplied;
            for (std::size_t index = 0; index < bindings.size(); ++index)
            {
                const VertexBufferBinding& binding      = bindings[index];
                const std::string          binding_path = path + ".vertexBuffers[" + std::to_string(index) + "]";
                supplied.push_back(binding.mBinding);
                const BufferResource* buffer = find(mFrame.mBuffers, binding.mBuffer);
                if (!contains(required, binding.mBinding))
                {
                    error(ValidationCode::InvalidBinding, binding_path, "vertex binding is absent from the pipeline layout");
                }
                if (!buffer)
                {
                    error(ValidationCode::MissingResource, binding_path, "vertex buffer is not declared");
                }
                else if (binding.mOffset >= buffer->mSize)
                {
                    error(ValidationCode::OutOfBounds, binding_path, "vertex buffer offset does not describe accessible data");
                }
                if (!hasBufferAccess(pass, binding.mBuffer, BufferAccessKind::VertexRead))
                {
                    error(ValidationCode::MissingAccess, binding_path, "vertex-buffer read is not declared by the pass");
                }
            }
            if (containsDuplicate(supplied))
            {
                error(ValidationCode::InvalidBinding, path, "vertex binding is supplied more than once");
            }
            for (std::uint32_t binding : required)
            {
                if (!contains(supplied, binding))
                {
                    error(ValidationCode::InvalidBinding, path, "required vertex binding is missing");
                }
            }
        }

        void validateDraw(const RenderPass& pass, const Draw& draw, const std::string& path)
        {
            const PipelineResource* pipeline = validateDrawResources(pass, draw.mResources, path);
            if (draw.mVertexCount == 0 || draw.mInstanceCount == 0)
            {
                error(ValidationCode::InvalidBinding, path, "non-indexed draw counts must be non-zero");
                return;
            }
            if (!pipeline || pipeline->mVertexAttributes.empty())
            {
                return;
            }
            if (addWouldOverflow(draw.mFirstVertex, draw.mVertexCount - 1))
            {
                error(ValidationCode::OutOfBounds, path, "non-indexed vertex range overflows");
                return;
            }
            validateVertexRange(*pipeline, draw.mResources.mVertexBuffers, draw.mFirstVertex + draw.mVertexCount - 1, path);
        }

        void validateDraw(const RenderPass& pass, const DrawIndexed& draw, const std::string& path)
        {
            const PipelineResource* pipeline         = validateDrawResources(pass, draw.mResources, path);
            const BufferResource*   index_buffer     = find(mFrame.mBuffers, draw.mIndexBuffer.mBuffer);
            const bool              valid_index_type = validEnum(draw.mIndexBuffer.mType, IndexType::UInt32);
            if (!valid_index_type)
            {
                error(ValidationCode::InvalidBinding, path, "index type is outside the contract domain");
            }
            if (!index_buffer)
            {
                error(ValidationCode::MissingResource, path, "index buffer is not declared");
            }
            else if (valid_index_type)
            {
                const std::uint64_t element_size  = indexSize(draw.mIndexBuffer.mType);
                const std::uint64_t element_count = static_cast<std::uint64_t>(draw.mFirstIndex) + draw.mIndexCount;
                if (draw.mIndexBuffer.mOffset % element_size != 0)
                {
                    error(ValidationCode::InvalidBinding, path, "index buffer offset must be aligned to the index type");
                }
                if (multiplyWouldOverflow(element_count, element_size) ||
                    addWouldOverflow(draw.mIndexBuffer.mOffset, element_count * element_size) ||
                    draw.mIndexBuffer.mOffset + element_count * element_size > index_buffer->mSize)
                {
                    error(ValidationCode::OutOfBounds, path, "indexed draw exceeds its index buffer");
                }
            }
            if (!hasBufferAccess(pass, draw.mIndexBuffer.mBuffer, BufferAccessKind::IndexRead))
            {
                error(ValidationCode::MissingAccess, path, "index-buffer read is not declared by the pass");
            }
            if (draw.mIndexCount == 0 || draw.mInstanceCount == 0 || draw.mMinVertex > draw.mMaxVertex)
            {
                error(ValidationCode::InvalidBinding, path, "indexed draw counts and vertex bounds are invalid");
                return;
            }
            const std::int64_t min_vertex = static_cast<std::int64_t>(draw.mBaseVertex) + draw.mMinVertex;
            const std::int64_t max_vertex = static_cast<std::int64_t>(draw.mBaseVertex) + draw.mMaxVertex;
            if (min_vertex < 0 || max_vertex < min_vertex || max_vertex > std::numeric_limits<std::uint32_t>::max())
            {
                error(ValidationCode::OutOfBounds, path, "indexed draw base vertex makes its bounds invalid");
                return;
            }
            if (pipeline)
            {
                validateVertexRange(*pipeline, draw.mResources.mVertexBuffers, static_cast<std::uint32_t>(max_vertex), path);
            }
        }

        void validateVertexRange(const PipelineResource& pipeline, const std::vector<VertexBufferBinding>& bindings,
                                 std::uint32_t max_vertex, const std::string& path)
        {
            for (const VertexBufferBinding& binding : bindings)
            {
                const BufferResource* buffer = find(mFrame.mBuffers, binding.mBuffer);
                const auto            layout =
                    std::find_if(pipeline.mVertexBindings.begin(), pipeline.mVertexBindings.end(),
                                 [&binding](const VertexBindingLayout& candidate) { return candidate.mBinding == binding.mBinding; });
                if (!buffer || layout == pipeline.mVertexBindings.end())
                {
                    continue;
                }
                std::uint32_t attribute_end = 0;
                for (const VertexAttribute& attribute : pipeline.mVertexAttributes)
                {
                    if (attribute.mBinding == binding.mBinding)
                    {
                        attribute_end = std::max(attribute_end, attribute.mOffset + vertexFormatSize(attribute.mFormat));
                    }
                }
                const std::uint64_t vertex_offset = static_cast<std::uint64_t>(max_vertex) * layout->mStride;
                if (addWouldOverflow(binding.mOffset, vertex_offset) ||
                    addWouldOverflow(binding.mOffset + vertex_offset, static_cast<std::uint64_t>(attribute_end)) ||
                    binding.mOffset + vertex_offset + attribute_end > buffer->mSize)
                {
                    error(ValidationCode::OutOfBounds, path, "draw vertex range exceeds a bound vertex buffer");
                }
            }
        }

        void validateReleases()
        {
            for (std::size_t index = 0; index < mFrame.mReleases.size(); ++index)
            {
                const ReleaseAfterFrame&              release  = mFrame.mReleases[index];
                const std::string                     path     = "releases[" + std::to_string(index) + "]";
                const std::optional<ResourceLifetime> lifetime = resourceLifetime(release.mResource);
                if (!resourceHandleValid(release.mResource))
                {
                    error(ValidationCode::InvalidHandle, path, "release handle is invalid");
                }
                else if (!lifetime)
                {
                    error(ValidationCode::MissingResource, path, "release names an undeclared resource generation");
                }
                else if (*lifetime == ResourceLifetime::External)
                {
                    error(ValidationCode::InvalidRelease, path, "externally owned resources cannot be released by the renderer");
                }
                if (release.mFrame < mFrame.mFrame)
                {
                    error(ValidationCode::InvalidRelease, path, "resource cannot retire before this frame completes");
                }
                for (std::size_t prior = 0; prior < index; ++prior)
                {
                    if (mFrame.mReleases[prior].mResource == release.mResource)
                    {
                        error(ValidationCode::InvalidRelease, path, "resource release is duplicated");
                        break;
                    }
                }
            }
        }

        void validateFrameLifetimes()
        {
            validateFrameLifetimes(mFrame.mBuffers);
            validateFrameLifetimes(mFrame.mImages);
            validateFrameLifetimes(mFrame.mSamplers);
            validateFrameLifetimes(mFrame.mPipelines);
        }

        template<typename Descriptor>
        void validateFrameLifetimes(const std::vector<Descriptor>& resources)
        {
            for (const Descriptor& resource : resources)
            {
                if (resource.mLifetime != ResourceLifetime::Frame)
                {
                    continue;
                }
                const ResourceHandle handle = resource.mHandle;
                const auto           release =
                    std::find_if(mFrame.mReleases.begin(), mFrame.mReleases.end(), [&handle, this](const ReleaseAfterFrame& candidate)
                                 { return candidate.mResource == handle && candidate.mFrame == mFrame.mFrame; });
                if (release == mFrame.mReleases.end())
                {
                    error(ValidationCode::InvalidRelease, "releases", "frame-owned resource must retire when this frame completes");
                }
            }
        }

        bool resourceHandleValid(const ResourceHandle& handle) const
        {
            return std::visit([](const auto& typed_handle) { return static_cast<bool>(typed_handle); }, handle);
        }

        std::optional<ResourceLifetime> resourceLifetime(const ResourceHandle& handle) const
        {
            return std::visit(
                [this](const auto& typed_handle) -> std::optional<ResourceLifetime>
                {
                    using HandleType = std::decay_t<decltype(typed_handle)>;
                    if constexpr (std::is_same_v<HandleType, BufferHandle>)
                    {
                        if (const BufferResource* resource = find(mFrame.mBuffers, typed_handle))
                        {
                            return resource->mLifetime;
                        }
                    }
                    else if constexpr (std::is_same_v<HandleType, ImageHandle>)
                    {
                        if (const ImageResource* resource = find(mFrame.mImages, typed_handle))
                        {
                            return resource->mLifetime;
                        }
                    }
                    else if constexpr (std::is_same_v<HandleType, SamplerHandle>)
                    {
                        if (const SamplerResource* resource = find(mFrame.mSamplers, typed_handle))
                        {
                            return resource->mLifetime;
                        }
                    }
                    else
                    {
                        if (const PipelineResource* resource = find(mFrame.mPipelines, typed_handle))
                        {
                            return resource->mLifetime;
                        }
                    }
                    return std::nullopt;
                },
                handle);
        }

        struct TrackedImageState
        {
            ImageHandle      mImage;
            ImageSubresource mSubresource;
            ImageState       mState;
        };

        void transition(ImageHandle image, ImageSubresource subresource, ImageState before, ImageState after, const std::string& path)
        {
            const auto found = std::find_if(mImageStates.begin(), mImageStates.end(), [image, subresource](const TrackedImageState& state)
                                            { return state.mImage == image && state.mSubresource == subresource; });
            if (found != mImageStates.end())
            {
                if (found->mState != before)
                {
                    error(ValidationCode::StateMismatch, path, "image before-state does not match the preceding declared use");
                }
                found->mState = after;
            }
            else
            {
                mImageStates.push_back({ image, subresource, after });
            }
        }

        const FrameSnapshot&           mFrame;
        ValidationResult               mResult;
        std::vector<TrackedImageState> mImageStates;
    };

} // namespace

ValidationResult validate(const FrameSnapshot& frame)
{
    return Validator(frame).run();
}

} // namespace LLRenderContract
