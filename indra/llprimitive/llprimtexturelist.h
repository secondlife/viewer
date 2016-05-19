/** 
 * @file llprimtexturelist.h
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

#ifndef LL_LLPRIMTEXTURELIST_H
#define LL_LLPRIMTEXTURELIST_H

#include <vector>
#include "lluuid.h"
#include "v3color.h"
#include "v4color.h"
#include "llmaterial.h"


class LLTextureEntry;
class LLMaterialID;

// this is a list of LLTextureEntry*'s because in practice the list's elements
// are of some derived class: LLFooTextureEntry
typedef std::vector<LLTextureEntry*> texture_list_t;

class LLPrimTextureList
{
public:
	// the LLPrimTextureList needs to know what type of LLTextureEntry 
	// to generate when it needs a new one, so we may need to set a 
	// callback for generating it, (or else use the base class default: 
	// static LLPrimTextureEntry::newTextureEntry() )
	//typedef LLTextureEntry* (__stdcall *NewTextureEntryFunction)();
	//static NewTextureEntryFunction sNewTextureEntryCallback;
	static LLTextureEntry* newTextureEntry();
	static void setNewTextureEntryCallback( LLTextureEntry* (*callback)() );
	static LLTextureEntry* (*sNewTextureEntryCallback)(); 

	LLPrimTextureList();
	virtual ~LLPrimTextureList();

	void clear();

	// clears current entries
	// copies contents of other_list
	// this is somewhat expensive, so it must be called explicitly
	void copy(const LLPrimTextureList& other_list);

	// clears current copies
	// takes contents of other_list
	// clears other_list
	void take(LLPrimTextureList& other_list);

	// copies LLTextureEntry 'te'
	// returns TEM_CHANGE_TEXTURE if successful, otherwise TEM_CHANGE_NONE
	S32 copyTexture(const U8 index, const LLTextureEntry& te);

	// takes ownership of LLTextureEntry* 'te'
	// returns TEM_CHANGE_TEXTURE if successful, otherwise TEM_CHANGE_NONE
	// IMPORTANT! -- if you use this function you must check the return value
	S32 takeTexture(const U8 index, LLTextureEntry* te);

//	// copies contents of 'entry' and stores it in 'index' slot
//	void copyTexture(const U8 index, const LLTextureEntry* entry);

	// returns pointer to texture at 'index' slot
	LLTextureEntry* getTexture(const U8 index) const;

	S32 setID(const U8 index, const LLUUID& id);
	S32 setColor(const U8 index, const LLColor3& color);
	S32 setColor(const U8 index, const LLColor4& color);
	S32 setAlpha(const U8 index, const F32 alpha);
	S32 setScale(const U8 index, const F32 s, const F32 t);
	S32 setScaleS(const U8 index, const F32 s);
	S32 setScaleT(const U8 index, const F32 t);
	S32 setOffset(const U8 index, const F32 s, const F32 t);
	S32 setOffsetS(const U8 index, const F32 s);
	S32 setOffsetT(const U8 index, const F32 t);
	S32 setRotation(const U8 index, const F32 r);
	S32 setBumpShinyFullbright(const U8 index, const U8 bump);
	S32 setMediaTexGen(const U8 index, const U8 media);
	S32 setBumpMap(const U8 index, const U8 bump);
	S32 setBumpShiny(const U8 index, const U8 bump_shiny);
	S32 setTexGen(const U8 index, const U8 texgen);
	S32 setShiny(const U8 index, const U8 shiny);
	S32 setFullbright(const U8 index, const U8 t);
	S32 setMediaFlags(const U8 index, const U8 media_flags);
	S32 setGlow(const U8 index, const F32 glow);
	S32 setMaterialID(const U8 index, const LLMaterialID& pMaterialID);
	S32 setMaterialParams(const U8 index, const LLMaterialPtr pMaterialParams);

	LLMaterialPtr getMaterialParams(const U8 index);

	S32 size() const;

//	void forceResize(S32 new_size);
	void setSize(S32 new_size);

	void setAllIDs(const LLUUID& id);
protected:
	texture_list_t mEntryList;
private:
	LLPrimTextureList(const LLPrimTextureList& other_list)
	{
		// private so that it can't be used
	}
};

#endif
