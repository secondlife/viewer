/** 
 * @file llfloateravatartextures.h
 * @brief Debugging view showing underlying avatar textures and baked textures.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLFLOATERAVATARTEXTURES_H
#define LL_LLFLOATERAVATARTEXTURES_H

#include "llfloater.h"
#include "lluuid.h"
#include "llstring.h"
#include "llvoavatardefines.h"

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
	LLTextureCtrl* mTextures[LLVOAvatarDefines::TEX_NUM_INDICES];
};

#endif
