/**
 * @file llrendercontract.h
 * @brief API-neutral renderer work descriptions.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * $/LicenseInfo$
 */

#ifndef LL_LLRENDERCONTRACT_H
#define LL_LLRENDERCONTRACT_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace LLRenderContract
{

template<typename Tag>
struct Handle
{
    std::uint32_t mIndex      = 0;
    std::uint32_t mGeneration = 0;

    constexpr explicit operator bool() const noexcept { return mIndex != 0 && mGeneration != 0; }

    friend constexpr bool operator==(const Handle&, const Handle&) = default;
};

template<typename Tag>
constexpr std::optional<Handle<Tag>> nextHandleGeneration(Handle<Tag> current) noexcept
{
    if (!current || current.mGeneration == std::numeric_limits<std::uint32_t>::max())
    {
        return std::nullopt;
    }
    return Handle<Tag>{ current.mIndex, current.mGeneration + 1 };
}

struct BufferTag;
struct ImageTag;
struct SamplerTag;
struct PipelineTag;
struct ShaderTag;

using BufferHandle   = Handle<BufferTag>;
using ImageHandle    = Handle<ImageTag>;
using SamplerHandle  = Handle<SamplerTag>;
using PipelineHandle = Handle<PipelineTag>;
using ShaderHandle   = Handle<ShaderTag>;
using ResourceHandle = std::variant<BufferHandle, ImageHandle, SamplerHandle, PipelineHandle>;

struct PassId
{
    std::uint32_t mValue = 0;

    constexpr explicit    operator bool() const noexcept { return mValue != 0; }
    friend constexpr bool operator==(const PassId&, const PassId&) = default;
};

struct Extent2D
{
    std::uint32_t mWidth  = 0;
    std::uint32_t mHeight = 0;
};

struct Offset2D
{
    std::uint32_t mX = 0;
    std::uint32_t mY = 0;
};

struct ImageSubresource
{
    std::uint32_t mMipLevel   = 0;
    std::uint32_t mArrayLayer = 0;

    friend constexpr bool operator==(const ImageSubresource&, const ImageSubresource&) = default;
};

struct ImageSubresourceRange
{
    std::uint32_t mBaseMipLevel    = 0;
    std::uint32_t mMipLevelCount   = 1;
    std::uint32_t mBaseArrayLayer  = 0;
    std::uint32_t mArrayLayerCount = 1;

    friend constexpr bool operator==(const ImageSubresourceRange&, const ImageSubresourceRange&) = default;
};

struct Viewport
{
    float mX        = 0.f;
    float mY        = 0.f;
    float mWidth    = 0.f;
    float mHeight   = 0.f;
    float mMinDepth = 0.f;
    float mMaxDepth = 1.f;
};

struct Scissor
{
    std::uint32_t mX      = 0;
    std::uint32_t mY      = 0;
    std::uint32_t mWidth  = 0;
    std::uint32_t mHeight = 0;
};

struct ClearColor
{
    float mRed   = 0.f;
    float mGreen = 0.f;
    float mBlue  = 0.f;
    float mAlpha = 0.f;
};

struct ByteRange
{
    std::shared_ptr<const std::vector<std::uint8_t>> mStorage;
    std::size_t                                      mOffset = 0;
    std::size_t                                      mSize   = 0;
};

enum class ResourceLifetime
{
    External,
    Persistent,
    Frame
};

enum class PixelFormat
{
    R8Unorm,
    RG8Unorm,
    RGB8Unorm,
    RGBA8Unorm,
    RGBA8Srgb,
    RGB10A2Unorm,
    RGBA16Unorm,
    R16Float,
    RGBA16Float,
    Depth24Unorm,
    Depth32Float,
    RGB16Float
};

enum class ImageState
{
    Undefined,
    TransferDestination,
    ShaderRead,
    ColorAttachment,
    DepthAttachment
};

enum class ImageAccessKind
{
    SampledRead,
    ColorAttachmentWrite,
    DepthAttachmentReadWrite
};

enum class BufferAccessKind
{
    VertexRead,
    IndexRead
};

enum class LoadOp
{
    Load,
    Clear,
    DontCare
};

enum class StoreOp
{
    Store,
    DontCare
};

enum class Filter
{
    Nearest,
    Linear
};

enum class MipFilter
{
    Disabled,
    Nearest,
    Linear
};

enum class AddressMode
{
    Clamp,
    Repeat,
    Mirror
};

enum class PrimitiveTopology
{
    TriangleList
};

enum class CullMode
{
    Disabled,
    Back
};

enum class FrontFace
{
    Clockwise,
    CounterClockwise
};

enum class CompareOp
{
    AlwaysPass,
    LessOrEqual
};

enum class VertexSemantic
{
    Position,
    Normal,
    TexCoord0,
    Color,
    Tangent,
    TexCoord1,
    TexCoord2
};

enum class VertexFormat
{
    Float2,
    Float3,
    Float4,
    UNorm8x4
};

enum class IndexType
{
    UInt16,
    UInt32
};

enum class RowOrigin
{
    TopLeft,
    BottomLeft
};

enum class MipGeneration
{
    Disabled,
    GenerateRemaining
};

struct BufferResource
{
    BufferHandle     mHandle;
    std::uint64_t    mSize     = 0;
    ResourceLifetime mLifetime = ResourceLifetime::Persistent;
};

struct ImageResource
{
    ImageHandle      mHandle;
    Extent2D         mExtent;
    std::uint32_t    mMipLevels   = 1;
    std::uint32_t    mArrayLayers = 1;
    std::uint32_t    mSamples     = 1;
    PixelFormat      mFormat      = PixelFormat::RGBA8Unorm;
    ResourceLifetime mLifetime    = ResourceLifetime::Persistent;
};

struct SamplerResource
{
    SamplerHandle    mHandle;
    Filter           mMinFilter     = Filter::Linear;
    Filter           mMagFilter     = Filter::Linear;
    MipFilter        mMipFilter     = MipFilter::Disabled;
    AddressMode      mAddressU      = AddressMode::Clamp;
    AddressMode      mAddressV      = AddressMode::Clamp;
    float            mMaxAnisotropy = 1.f;
    ResourceLifetime mLifetime      = ResourceLifetime::Persistent;
};

struct VertexAttribute
{
    VertexSemantic mSemantic = VertexSemantic::Position;
    VertexFormat   mFormat   = VertexFormat::Float3;
    std::uint32_t  mBinding  = 0;
    std::uint32_t  mOffset   = 0;
};

struct VertexBindingLayout
{
    std::uint32_t mBinding = 0;
    std::uint32_t mStride  = 0;
};

struct ShaderProgramKey
{
    std::string   mName;
    std::uint64_t mVariant = 0;
};

struct ColorTargetState
{
    PixelFormat  mFormat       = PixelFormat::RGBA8Unorm;
    bool         mBlendEnabled = false;
    std::uint8_t mWriteMask    = 0xf;
};

struct ParameterLayout
{
    std::uint32_t mBinding = 0;
    std::uint32_t mSize    = 0;
};

struct PipelineResource
{
    PipelineHandle                   mHandle;
    ShaderProgramKey                 mProgram;
    PrimitiveTopology                mTopology          = PrimitiveTopology::TriangleList;
    CullMode                         mCullMode          = CullMode::Disabled;
    FrontFace                        mFrontFace         = FrontFace::CounterClockwise;
    bool                             mDepthTestEnabled  = false;
    bool                             mDepthWriteEnabled = false;
    CompareOp                        mDepthCompare      = CompareOp::AlwaysPass;
    std::uint32_t                    mSamples           = 1;
    std::vector<ColorTargetState>    mColorTargets;
    std::optional<PixelFormat>       mDepthFormat;
    std::vector<VertexBindingLayout> mVertexBindings;
    std::vector<VertexAttribute>     mVertexAttributes;
    std::vector<std::uint32_t>       mSampledImageBindings;
    std::vector<ParameterLayout>     mParameterBindings;
    ResourceLifetime                 mLifetime = ResourceLifetime::Persistent;
};

struct BufferAccess
{
    BufferHandle     mBuffer;
    BufferAccessKind mKind = BufferAccessKind::VertexRead;
};

struct ImageAccess
{
    ImageHandle           mImage;
    ImageSubresourceRange mRange;
    ImageAccessKind       mKind   = ImageAccessKind::SampledRead;
    ImageState            mBefore = ImageState::ShaderRead;
    ImageState            mDuring = ImageState::ShaderRead;
    ImageState            mAfter  = ImageState::ShaderRead;
};

struct ColorAttachment
{
    ImageHandle      mImage;
    ImageSubresource mSubresource;
    LoadOp           mLoad  = LoadOp::DontCare;
    StoreOp          mStore = StoreOp::Store;
    ClearColor       mClear;
};

struct DepthAttachment
{
    ImageHandle      mImage;
    ImageSubresource mSubresource;
    LoadOp           mLoad       = LoadOp::Load;
    StoreOp          mStore      = StoreOp::Store;
    float            mClearDepth = 1.f;
};

struct VertexBufferBinding
{
    std::uint32_t mBinding = 0;
    BufferHandle  mBuffer;
    std::uint64_t mOffset = 0;
};

struct IndexBufferBinding
{
    BufferHandle  mBuffer;
    std::uint64_t mOffset = 0;
    IndexType     mType   = IndexType::UInt16;
};

struct SampledImageBinding
{
    std::uint32_t         mBinding = 0;
    ImageHandle           mImage;
    ImageSubresourceRange mRange;
    SamplerHandle         mSampler;
};

struct ParameterBinding
{
    std::uint32_t mBinding = 0;
    ByteRange     mBytes;
};

struct DrawResources
{
    PipelineHandle                   mPipeline;
    std::vector<VertexBufferBinding> mVertexBuffers;
    std::vector<SampledImageBinding> mSampledImages;
    std::vector<ParameterBinding>    mParameters;
};

struct Draw
{
    DrawResources mResources;
    std::uint32_t mFirstVertex   = 0;
    std::uint32_t mVertexCount   = 0;
    std::uint32_t mFirstInstance = 0;
    std::uint32_t mInstanceCount = 1;
};

struct DrawIndexed
{
    DrawResources      mResources;
    IndexBufferBinding mIndexBuffer;
    std::uint32_t      mFirstIndex    = 0;
    std::uint32_t      mIndexCount    = 0;
    std::int32_t       mBaseVertex    = 0;
    std::uint32_t      mMinVertex     = 0;
    std::uint32_t      mMaxVertex     = 0;
    std::uint32_t      mFirstInstance = 0;
    std::uint32_t      mInstanceCount = 1;
};

using DrawCommand = std::variant<Draw, DrawIndexed>;

struct RenderPass
{
    PassId                         mId;
    std::string                    mLabel;
    Extent2D                       mExtent;
    Viewport                       mViewport;
    Scissor                        mScissor;
    std::vector<PassId>            mDependencies;
    std::vector<BufferAccess>      mBufferAccesses;
    std::vector<ImageAccess>       mImageAccesses;
    std::vector<ColorAttachment>   mColorAttachments;
    std::optional<DepthAttachment> mDepthAttachment;
    std::vector<DrawCommand>       mDraws;
};

struct TextureUpload
{
    ImageHandle      mDestination;
    std::uint64_t    mRevision = 0;
    ImageSubresource mSubresource;
    Offset2D         mOffset;
    Extent2D         mExtent;
    Extent2D         mLogicalExtent;
    std::uint32_t    mResidentDiscard = 0;
    PixelFormat      mSourceFormat    = PixelFormat::RGBA8Unorm;
    std::uint32_t    mRowPitch        = 0;
    RowOrigin        mRowOrigin       = RowOrigin::TopLeft;
    MipGeneration    mMipGeneration   = MipGeneration::Disabled;
    ByteRange        mPixels;
    ImageState       mBefore = ImageState::Undefined;
    ImageState       mDuring = ImageState::TransferDestination;
    ImageState       mAfter  = ImageState::ShaderRead;
};

struct ReleaseAfterFrame
{
    ResourceHandle mResource;
    std::uint64_t  mFrame = 0;
};

struct FrameSnapshot
{
    std::uint64_t                  mFrame = 0;
    std::vector<BufferResource>    mBuffers;
    std::vector<ImageResource>     mImages;
    std::vector<SamplerResource>   mSamplers;
    std::vector<PipelineResource>  mPipelines;
    std::vector<TextureUpload>     mUploads;
    std::vector<RenderPass>        mPasses;
    std::vector<ReleaseAfterFrame> mReleases;
};

enum class ValidationCode
{
    InvalidFrame,
    InvalidHandle,
    DuplicateResource,
    MissingResource,
    InvalidResource,
    InvalidDependency,
    InvalidPass,
    MissingAccess,
    InvalidState,
    StateMismatch,
    InvalidBinding,
    OutOfBounds,
    InvalidUpload,
    InvalidRelease
};

struct ValidationError
{
    ValidationCode mCode = ValidationCode::InvalidFrame;
    std::string    mPath;
    std::string    mMessage;
};

struct ValidationResult
{
    std::vector<ValidationError> mErrors;

    explicit operator bool() const noexcept { return mErrors.empty(); }
};

ValidationResult validate(const FrameSnapshot& frame);

} // namespace LLRenderContract

#endif // LL_LLRENDERCONTRACT_H
