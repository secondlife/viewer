/** 
 * @file llpreviewsound.h
 * @brief LLPreviewSound class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPREVIEWSOUND_H
#define LL_LLPREVIEWSOUND_H

#include "llpreview.h"

class LLPreviewSound : public LLPreview
{
public:
	LLPreviewSound(const std::string& name, const LLRect& rect, const std::string& title,
				   const LLUUID& item_uuid,
				   const LLUUID& object_uuid = LLUUID::null);

	static void playSound( void* userdata );
	static void auditionSound( void* userdata );

};

#endif  // LL_LLPREVIEWSOUND_H
