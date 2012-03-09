/** 
 * @file llclipboard.h
 * @brief LLClipboard base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLCLIPBOARD_H
#define LL_LLCLIPBOARD_H


#include "llstring.h"
#include "lluuid.h"
#include "stdenums.h"
#include "llinventory.h"


class LLClipboard
{
public:
	LLClipboard();
	~LLClipboard();

	/* We support two flavors of clipboard.  The default is the explicitly
	   copy-and-pasted clipboard.  The second is the so-called 'primary' clipboard
	   which is implicitly copied upon selection on platforms which expect this
	   (i.e. X11/Linux). */

	void		copyFromSubstring(const LLWString &copy_from, S32 pos, S32 len, const LLUUID& source_id = LLUUID::null );
	void		copyFromString(const LLWString &copy_from, const LLUUID& source_id = LLUUID::null );
	BOOL		canPasteString() const;
	const LLWString&	getPasteWString(LLUUID* source_id = NULL);

	void		copyFromPrimarySubstring(const LLWString &copy_from, S32 pos, S32 len, const LLUUID& source_id = LLUUID::null );
	BOOL		canPastePrimaryString() const;
	const LLWString&	getPastePrimaryWString(LLUUID* source_id = NULL);	

	// Support clipboard for object known only by their uuid and asset type
	void		  setSourceObject(const LLUUID& source_id, LLAssetType::EType type);
	const LLInventoryObject* getSourceObject() { return mSourceItem; }
	
private:
	LLUUID      mSourceID;
	LLWString	mString;
	LLInventoryObject* mSourceItem;
};


// Global singleton
extern LLClipboard gClipboard;


#endif  // LL_LLCLIPBOARD_H
