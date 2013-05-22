/** 
 * @file lltexturelist.cpp
 * @brief LLPrimTextureList (virtual) base class
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "linden_common.h"

#include "llprimtexturelist.h"
#include "lltextureentry.h"

// static 
//int (TMyClass::*pt2Member)(float, char, char) = NULL;                // C++
LLTextureEntry* (*LLPrimTextureList::sNewTextureEntryCallback)() = &(LLTextureEntry::newTextureEntry);

// static
void LLPrimTextureList::setNewTextureEntryCallback( LLTextureEntry* (*callback)() )
{
	if (callback)
	{
		LLPrimTextureList::sNewTextureEntryCallback = callback;
	}
	else
	{
		LLPrimTextureList::sNewTextureEntryCallback = &(LLTextureEntry::newTextureEntry);
	}
}

// static 
// call this to get a new texture entry
LLTextureEntry* LLPrimTextureList::newTextureEntry()
{
	return (*sNewTextureEntryCallback)();
}

LLPrimTextureList::LLPrimTextureList()
{
}

// virtual 
LLPrimTextureList::~LLPrimTextureList()
{
	clear();
}

void LLPrimTextureList::clear()
{
	texture_list_t::iterator itr = mEntryList.begin();
	while (itr != mEntryList.end())
	{
		delete (*itr);
		(*itr) = NULL;
		++itr;
	}
	mEntryList.clear();
}


// clears current entries
// copies contents of other_list
// this is somewhat expensive, so it must be called explicitly
void LLPrimTextureList::copy(const LLPrimTextureList& other_list)
{
	// compare the sizes
	S32 this_size = mEntryList.size();
	S32 other_size = other_list.mEntryList.size();

	if (this_size > other_size)
	{
		// remove the extra entries
		for (S32 index = this_size; index > other_size; --index)
		{
			delete mEntryList[index-1];
		}
		mEntryList.resize(other_size);
		this_size = other_size;
	}

	S32 index = 0;
	// copy for the entries that already exist
	for ( ; index < this_size; ++index)
	{
		delete mEntryList[index];
		mEntryList[index] = other_list.getTexture(index)->newCopy();
	}

	// add new entires if needed
	for ( ; index < other_size; ++index)
	{
		mEntryList.push_back( other_list.getTexture(index)->newCopy() );
	}
}

// clears current copies
// takes contents of other_list
// clears other_list
void LLPrimTextureList::take(LLPrimTextureList& other_list)
{
	clear();
	mEntryList = other_list.mEntryList;
	other_list.mEntryList.clear();
}

// virtual 
// copies LLTextureEntry 'te'
// returns TEM_CHANGE_TEXTURE if successful, otherwise TEM_CHANGE_NONE
S32 LLPrimTextureList::copyTexture(const U8 index, const LLTextureEntry& te)
{
	if (S32(index) >= mEntryList.size())
	{
		S32 current_size = mEntryList.size();
		llwarns << "ignore copy of index = " << S32(index) << " into texture entry list of size = " << current_size << llendl;
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

// virtual 
// takes ownership of LLTextureEntry* 'te'
// returns TEM_CHANGE_TEXTURE if successful, otherwise TEM_CHANGE_NONE
// IMPORTANT! -- if you use this function you must check the return value
S32 LLPrimTextureList::takeTexture(const U8 index, LLTextureEntry* te)
{
	if (S32(index) >= mEntryList.size())
	{
		return TEM_CHANGE_NONE;
	}

	// we're changing an existing entry
	llassert(mEntryList[index]);
	delete (mEntryList[index]);
	mEntryList[index] = te;
	return TEM_CHANGE_TEXTURE;
}

// returns pointer to texture at 'index' slot
LLTextureEntry* LLPrimTextureList::getTexture(const U8 index) const
{
	if (index < mEntryList.size())
	{
		return mEntryList[index];
	}
	return NULL;
}

//virtual 
//S32 setTE(const U8 index, const LLTextureEntry& te) = 0;

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

S32 LLPrimTextureList::setTexGen(const U8 index, const U8 texgen)
{
	if (index < mEntryList.size())
	{
		return mEntryList[index]->setTexGen(texgen);
	}
	return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setShiny(const U8 index, const U8 shiny)
{
	if (index < mEntryList.size())
	{
		return mEntryList[index]->setShiny(shiny);
	}
	return TEM_CHANGE_NONE;
}

S32 LLPrimTextureList::setFullbright(const U8 index, const U8 fullbright)
{
	if (index < mEntryList.size())
	{
		return mEntryList[index]->setFullbright(fullbright);
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

S32 LLPrimTextureList::size() const
{
	return mEntryList.size();
}

// sets the size of the mEntryList container
void LLPrimTextureList::setSize(S32 new_size)
{
	if (new_size < 0)
	{
		new_size = 0;
	}

	S32 current_size = mEntryList.size();

	if (new_size > current_size)
	{
		mEntryList.resize(new_size);
		for (S32 index = current_size; index < new_size; ++index)
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
		for (S32 index = current_size-1; index >= new_size; --index)
		{
			delete mEntryList[index];
		}
		mEntryList.resize(new_size);
	}
}


void LLPrimTextureList::setAllIDs(const LLUUID& id)
{
	texture_list_t::iterator itr = mEntryList.begin();
	while (itr != mEntryList.end())
	{
		(*itr)->setID(id);
		++itr;
	}
}


