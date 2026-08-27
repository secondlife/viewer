/**
 * @file lldrawinfo.cpp
 * @brief LLDrawInfo implementation.
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

#include "llspatialpartition.h"

LLDrawInfo::LLDrawInfo(U16 start, U16 end, U32 count, U32 offset, LLViewerTexture* texture, LLVertexBuffer* buffer, bool fullbright,
                       U8 bump) :
    mVertexBuffer(buffer),
    mTexture(texture),
    mStart(start),
    mEnd(end),
    mCount(count),
    mOffset(offset),
    mFullbright(fullbright),
    mBump(bump),
    mBlendFuncSrc(LLRender::BF_SOURCE_ALPHA),
    mBlendFuncDst(LLRender::BF_ONE_MINUS_SOURCE_ALPHA),
    mHasGlow(false),
    mEnvIntensity(0.0f),
    mAlphaMaskCutoff(0.5f)
{
    mVertexBuffer->validateRange(mStart, mEnd, mCount, mOffset);
}

LLColor4U LLDrawInfo::getDebugColor() const
{
    LLColor4U color;

    LLCRC hash;
    hash.update((U8*)this + sizeof(S32), sizeof(LLDrawInfo) - sizeof(S32));

    *((U32*)color.mV) = hash.getCRC();

    color.mV[3] = 200;

    return color;
}

void LLDrawInfo::validate()
{
    mVertexBuffer->validateRange(mStart, mEnd, mCount, mOffset);
}

U64 LLDrawInfo::getSkinHash()
{
    return mSkinInfo ? mSkinInfo->mHash : 0;
}
