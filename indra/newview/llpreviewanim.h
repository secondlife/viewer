/** 
 * @file llpreviewanim.h
 * @brief LLPreviewAnim class definition
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPREVIEWANIM_H
#define LL_LLPREVIEWANIM_H

#include "llpreview.h"
#include "llcharacter.h"

class LLPreviewAnim : public LLPreview
{
public:
	LLPreviewAnim(const std::string& name, const LLRect& rect, const std::string& title,
					const LLUUID& item_uuid,
					const S32&    activate,
					const LLUUID& object_uuid = LLUUID::null);

	static void playAnim( void* userdata );
	static void auditionAnim( void* userdata );
	static void saveAnim( void* userdata );
	static void endAnimCallback( void *userdata );

protected:
	virtual void onClose(bool app_quitting);

	LLAnimPauseRequest	mPauseRequest;
	LLUUID		mItemID;
	LLString	mTitle;
	LLUUID		mObjectID;
	LLButton*	mPlayBtn;
	LLButton*	mAuditionBtn;
};

#endif  // LL_LLPREVIEWSOUND_H
