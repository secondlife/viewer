/**
 * @file llprimtexturelist_stub.cpp
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

LLTextureEntry* LLPrimTextureList::newTextureEntry()
{
    return new LLTextureEntry();
}

LLPrimTextureList::LLPrimTextureList() { }
LLPrimTextureList::~LLPrimTextureList() { }

S32 LLPrimTextureList::copyTexture(const U8 index, const LLTextureEntry& te)
{
    if (size_t(index) >= mEntryList.size())
    {
        auto current_size = mEntryList.size();
        return TEM_CHANGE_NONE;
    }

    // we're changing an existing entry
    llassert(mEntryList[index]);
    delete (mEntryList[index]);
    if  (&te)
    {
        mEntryList[index] = te.newCopy();
    }
    else
    {
        mEntryList[index] = LLPrimTextureList::newTextureEntry();
    }
    return TEM_CHANGE_TEXTURE;
}

S32 LLPrimTextureList::setFullbright(const U8 index, const U8 t) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setMaterialParams(const U8 index, const LLMaterialPtr pMaterialParams) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setShiny(const U8 index, const U8 shiny) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setTexGen(const U8 index, const U8 texgen) { return TEM_CHANGE_NONE; }

LLMaterialPtr LLPrimTextureList::getMaterialParams(const U8 index) { return LLMaterialPtr(); }
void LLPrimTextureList::copy(LLPrimTextureList const & ptl) { mEntryList = ptl.mEntryList; } // do we need to call getTexture()->newCopy()?
void LLPrimTextureList::take(LLPrimTextureList &other_list) { }

// sets the size of the mEntryList container
void LLPrimTextureList::setSize(S32 new_size)
{
    if (new_size < 0)
    {
        new_size = 0;
    }

    auto current_size = mEntryList.size();

    if (new_size > current_size)
    {
        mEntryList.resize(new_size);
        for (size_t index = current_size; index < new_size; ++index)
        {
            if (current_size > 0
                && mEntryList[current_size - 1])
            {
                // copy the last valid entry for the new one
                mEntryList[index] = mEntryList[current_size - 1]->newCopy();
            }
            else
            {
                // no valid enries to copy, so we new one up
                LLTextureEntry* new_entry = LLPrimTextureList::newTextureEntry();
                mEntryList[index] = new_entry;
            }
        }
    }
    else if (new_size < current_size)
    {
        for (size_t index = current_size-1; index >= new_size; --index)
        {
            delete mEntryList[index];
        }
        mEntryList.resize(new_size);
    }
}

void LLPrimTextureList::setAllIDs(const LLUUID &id)
{
    llassert(false); // implement this if you get here
}

// returns pointer to texture at 'index' slot
LLTextureEntry* LLPrimTextureList::getTexture(const U8 index) const
{
    if (index < mEntryList.size())
    {
        return mEntryList[index];
    }
    return nullptr;
}

S32 LLPrimTextureList::size() const { return static_cast<S32>(mEntryList.size()); }

S32 LLPrimTextureList::setID(const U8 index, const LLUUID& id)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setID(id);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setColor(const U8 index, const LLColor3& color)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setColor(color);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setColor(const U8 index, const LLColor4& color)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setColor(color);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setAlpha(const U8 index, const F32 alpha)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setAlpha(alpha);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setScale(const U8 index, const F32 s, const F32 t)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setScale(s, t);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setScaleS(const U8 index, const F32 s)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setScaleS(s);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setScaleT(const U8 index, const F32 t)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setScaleT(t);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setOffset(const U8 index, const F32 s, const F32 t)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setOffset(s, t);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setOffsetS(const U8 index, const F32 s)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setOffsetS(s);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setOffsetT(const U8 index, const F32 t)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setOffsetT(t);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setRotation(const U8 index, const F32 r)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setRotation(r);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setBumpShinyFullbright(const U8 index, const U8 bump)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setBumpShinyFullbright(bump);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setMediaTexGen(const U8 index, const U8 media)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setMediaTexGen(media);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setBumpMap(const U8 index, const U8 bump)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setBumpmap(bump);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setBumpShiny(const U8 index, const U8 bump_shiny)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setBumpShiny(bump_shiny);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setMediaFlags(const U8 index, const U8 media_flags)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setMediaFlags(media_flags);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setGlow(const U8 index, const F32 glow)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setGlow(glow);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setMaterialID(const U8 index, const LLMaterialID& pMaterialID)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setMaterialID(pMaterialID);
    }
    return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setAlphaGamma(const U8 index, const U8 gamma)
{
    if (index < mEntryList.size())
    {
        return mEntryList[index]->setAlphaGamma(gamma);
    }
    return TEM_CHANGE_NONE;
}
