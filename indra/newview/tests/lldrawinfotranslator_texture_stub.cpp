/**
 * @file lldrawinfotranslator_texture_stub.cpp
 * @brief CPU-only LLViewerTexture implementation for draw-info translation tests.
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

#include "llspatialpartition.h"
#include "llviewertexture.h"

#include <algorithm>

LLDrawInfo::~LLDrawInfo() = default;

LLViewerTexture::LLViewerTexture(bool usemipmaps) :
    LLGLTexture(usemipmaps),
    mTextureListType(LOCAL_TEXTURE),
    mMaxVirtualSizeResetCounter(1),
    mMaxVirtualSizeResetInterval(1),
    mParcelMedia(nullptr)
{
    std::fill(std::begin(mNumFaces), std::end(mNumFaces), 0);
    std::fill(std::begin(mNumVolumes), std::end(mNumVolumes), 0);
}

LLViewerTexture::~LLViewerTexture() = default;

S8 LLViewerTexture::getType() const
{
    return LOCAL_TEXTURE;
}

bool LLViewerTexture::isMissingAsset() const
{
    return false;
}

void LLViewerTexture::dump()
{
}

bool LLViewerTexture::bindDefaultImage(S32)
{
    return false;
}

bool LLViewerTexture::bindDebugImage(S32)
{
    return false;
}

void LLViewerTexture::forceImmediateUpdate()
{
}

bool LLViewerTexture::isActiveFetching()
{
    return false;
}

void LLViewerTexture::setBoostLevel(S32 level)
{
    mBoostLevel = level;
}

F32 LLViewerTexture::getMaxVirtualSize()
{
    return mMaxVirtualSize;
}

void LLViewerTexture::setKnownDrawSize(S32 width, S32 height)
{
    mFullWidth  = width;
    mFullHeight = height;
}

void LLViewerTexture::addFace(U32, LLFace*)
{
}

void LLViewerTexture::removeFace(U32, LLFace*)
{
}

void LLViewerTexture::addVolume(U32, LLVOVolume*)
{
}

void LLViewerTexture::removeVolume(U32, LLVOVolume*)
{
}

void LLViewerTexture::updateBindStatsForTester()
{
}
