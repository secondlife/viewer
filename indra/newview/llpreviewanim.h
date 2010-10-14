/** 
 * @file llpreviewanim.h
 * @brief LLPreviewAnim class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLPREVIEWANIM_H
#define LL_LLPREVIEWANIM_H

#include "llpreview.h"
#include "llcharacter.h"

class LLPreviewAnim : public LLPreview
{
public:
	enum e_activation_type { NONE = 0, PLAY = 1, AUDITION = 2 };
	LLPreviewAnim(const LLSD& key);

	static void playAnim( void* userdata );
	static void auditionAnim( void* userdata );
	static void endAnimCallback( void *userdata );
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);
	void activate(e_activation_type type);
	
protected:
	
	LLAnimPauseRequest	mPauseRequest;
	LLUUID		mItemID;
	std::string	mTitle;
	LLUUID		mObjectID;
	LLButton*	mPlayBtn;
	LLButton*	mAuditionBtn;
};

#endif  // LL_LLPREVIEWSOUND_H
