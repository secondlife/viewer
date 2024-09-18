/**
 * @file lltextueentry_stub.cpp
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#pragma once

LLTextureEntry::LLTextureEntry()
  : mMediaEntry(NULL)
  , mSelected(false)
  , mMaterialUpdatePending(false)
{
    constexpr U8 DEFAULT_BUMP_CODE = 0;  // no bump or shininess
    constexpr U8 DEFAULT_ALPHA_GAMMA = 100;  // Gamma 1 (linear) for alpha blending
    init(LLUUID::null, 1.f, 1.f, 0.f, 0.f, 0.f, DEFAULT_BUMP_CODE, DEFAULT_ALPHA_GAMMA);
}

LLTextureEntry::LLTextureEntry(const LLTextureEntry &rhs)
  : mMediaEntry(NULL)
  , mSelected(false)
  , mMaterialUpdatePending(false)
{
    *this = rhs;
}

LLTextureEntry::~LLTextureEntry()
{
}

// virtual
LLTextureEntry* LLTextureEntry::newBlank() const
{
    return new LLTextureEntry();
}

// virtual
LLTextureEntry* LLTextureEntry::newCopy() const
{
    return new LLTextureEntry(*this);
}

LLTextureEntry &LLTextureEntry::operator=(const LLTextureEntry &rhs)
{
    if (this != &rhs)
    {
        mID = rhs.mID;
        mScaleS = rhs.mScaleS;
        mScaleT = rhs.mScaleT;
        mOffsetS = rhs.mOffsetS;
        mOffsetT = rhs.mOffsetT;
        mRotation = rhs.mRotation;
        mColor = rhs.mColor;
        mBump = rhs.mBump;
        mAlphaGamma = rhs.mAlphaGamma;
        mMediaFlags = rhs.mMediaFlags;
        mGlow = rhs.mGlow;
        mMaterialID = rhs.mMaterialID;
    }

    return *this;
}

void LLTextureEntry::init(const LLUUID& tex_id, F32 scale_s, F32 scale_t, F32 offset_s, F32 offset_t, F32 rotation, U8 bump, U8 alphagamma)
{
    mID = tex_id;

    mScaleS = scale_s;
    mScaleT = scale_t;
    mOffsetS = offset_s;
    mOffsetT = offset_t;
    mRotation = rotation;
    mBump = bump;
    mAlphaGamma = alphagamma;
    mMediaFlags = 0x0;
    mGlow = 0;
    mMaterialID.clear();

    mColor = LLColor4(1.f, 1.f, 1.f, 1.f);
}

S32 LLTextureEntry::setID(const LLUUID &tex_id)
{
    if (mID != tex_id)
    {
        mID = tex_id;
        return TEM_CHANGE_TEXTURE;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setColor(const LLColor4 &color)
{
    if (mColor != color)
    {
        mColor = color;
        return TEM_CHANGE_COLOR;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setColor(const LLColor3 &color)
{
    if (mColor != color)
    {
        // This preserves alpha.
        mColor.setVec(color);
        return TEM_CHANGE_COLOR;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setAlpha(const F32 alpha)
{
    if (mColor.mV[VALPHA] != alpha)
    {
        mColor.mV[VALPHA] = alpha;
        return TEM_CHANGE_COLOR;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setScale(F32 s, F32 t)
{
    S32 retval = 0;

    if (  (mScaleS != s)
        ||(mScaleT != t))
    {
        mScaleS = s;
        mScaleT = t;

        retval = TEM_CHANGE_TEXTURE;
    }
    return retval;
}

S32 LLTextureEntry::setScaleS(F32 s)
{
    S32 retval = TEM_CHANGE_NONE;
    if (mScaleS != s)
    {
        mScaleS = s;
        retval = TEM_CHANGE_TEXTURE;
    }
    return retval;
}

S32 LLTextureEntry::setScaleT(F32 s)
{
    S32 retval = TEM_CHANGE_NONE;
    if (mScaleT != s)
    {
        mScaleT = s;
        retval = TEM_CHANGE_TEXTURE;
    }
    return retval;
}

S32 LLTextureEntry::setOffset(F32 s, F32 t)
{
    S32 retval = 0;

    if (  (mOffsetS != s)
        ||(mOffsetT != t))
    {
        mOffsetS = s;
        mOffsetT = t;

        retval = TEM_CHANGE_TEXTURE;
    }
    return retval;
}

S32 LLTextureEntry::setOffsetS(F32 s)
{
    S32 retval = 0;
    if (mOffsetS != s)
    {
        mOffsetS = s;
        retval = TEM_CHANGE_TEXTURE;
    }
    return retval;
}

S32 LLTextureEntry::setOffsetT(F32 t)
{
    S32 retval = 0;
    if (mOffsetT != t)
    {
        mOffsetT = t;
        retval = TEM_CHANGE_TEXTURE;
    }
    return retval;
}

S32 LLTextureEntry::setRotation(F32 theta)
{
    if (mRotation != theta && llfinite(theta))
    {
        mRotation = theta;
        return TEM_CHANGE_TEXTURE;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setBumpmap(U8 bump)
{
    bump &= TEM_BUMP_MASK;
    if (getBumpmap() != bump)
    {
        mBump &= ~TEM_BUMP_MASK;
        mBump |= bump;
        return TEM_CHANGE_TEXTURE;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setGlow(F32 glow)
{
    if (mGlow != glow)
    {
        mGlow = glow;
        return TEM_CHANGE_TEXTURE;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setMaterialID(const LLMaterialID& pMaterialID)
{
    if (mMaterialID != pMaterialID)
    {
        mMaterialID = pMaterialID;
        return TEM_CHANGE_TEXTURE;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setBumpShinyFullbright(U8 bump)
{
    if (mBump != bump)
    {
        mBump = bump;
        return TEM_CHANGE_TEXTURE;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setBumpShiny(U8 bump_shiny)
{
    bump_shiny &= TEM_BUMP_SHINY_MASK;
    if (getBumpShiny() != bump_shiny)
    {
        mBump &= ~TEM_BUMP_SHINY_MASK;
        mBump |= bump_shiny;
        return TEM_CHANGE_TEXTURE;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setAlphaGamma(U8 alpha_gamma)
{
    if (mAlphaGamma != alpha_gamma)
    {
        mAlphaGamma = alpha_gamma;
        return TEM_CHANGE_TEXTURE;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setMediaTexGen(U8 media)
{
    S32 result = TEM_CHANGE_NONE;
    result |= setTexGen(media & TEM_TEX_GEN_MASK);
    result |= setMediaFlags(media & TEM_MEDIA_MASK);
    return result;
}

S32 LLTextureEntry::setTexGen(U8 tex_gen)
{
    tex_gen &= TEM_TEX_GEN_MASK;
    if (getTexGen() != tex_gen)
    {
        mMediaFlags &= ~TEM_TEX_GEN_MASK;
        mMediaFlags |= tex_gen;
        return TEM_CHANGE_TEXTURE;
    }
    return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setMediaFlags(U8 media_flags)
{
    media_flags &= TEM_MEDIA_MASK;
    mMediaFlags &= ~TEM_MEDIA_MASK;
    mMediaFlags |= media_flags;
    return TEM_CHANGE_NONE;
}
