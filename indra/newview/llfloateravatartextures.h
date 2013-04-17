/** 
 * @file llfloateravatartextures.h
 * @brief Debugging view showing underlying avatar textures and baked textures.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLFLOATERAVATARTEXTURES_H
#define LL_LLFLOATERAVATARTEXTURES_H

#include "llfloater.h"
#include "lluuid.h"
#include "llstring.h"
#include "llavatarappearancedefines.h"

class LLTextureCtrl;

class LLFloaterAvatarTextures : public LLFloater
{
public:
	LLFloaterAvatarTextures(const LLSD& id);
	virtual ~LLFloaterAvatarTextures();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();

	void refresh();

private:
	static void onClickDump(void*);

private:
	LLUUID	mID;
	std::string mTitle;
	LLTextureCtrl* mTextures[LLAvatarAppearanceDefines::TEX_NUM_INDICES];
};

#endif
