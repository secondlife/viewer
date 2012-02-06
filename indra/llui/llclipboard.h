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
#include "llsingleton.h"
#include "llassettype.h"
#include "llinventory.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLClipboard
//
// This class is used to cut/copy/paste text strings and inventory items around 
// the world. Use LLClipboard::getInstance()->method() to use its methods.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLClipboard : public LLSingleton<LLClipboard>
{
public:
	LLClipboard();
	~LLClipboard();

	/* We support two flavors of clipboard.  The default is the explicitly
	   copy-and-pasted clipboard.  The second is the so-called 'primary' clipboard
	   which is implicitly copied upon selection on platforms which expect this
	   (i.e. X11/Linux). */

	// Text strings and single item management
	bool		copyToClipboard(const LLWString& src, S32 pos, S32 len, bool use_primary = false);
	bool		copyToClipboard(const LLUUID& src, const LLAssetType::EType type = LLAssetType::AT_NONE);
	bool		pasteFromClipboard(LLWString& dst, bool use_primary = false);
	bool		isTextAvailable(bool use_primary = false) const;
	
	// Object list management
	void add(const LLUUID& object);									// Adds to the current list of objects on the clipboard
	void store(const LLUUID& object);								// Stores a single inventory object
	void store(const LLDynamicArray<LLUUID>& inventory_objects);	// Stores an array of objects
	void cut(const LLUUID& object);									// Adds to the current list of cut objects on the clipboard
	void retrieve(LLDynamicArray<LLUUID>& inventory_objects) const;	// Gets a copy of the objects on the clipboard
	void reset();													// Clears the clipboard
	
	BOOL hasContents() const;										// true if the clipboard has something pasteable in it
	bool isCutMode() const { return mCutMode; }

private:
	LLDynamicArray<LLUUID> mObjects;
	bool mCutMode;
	LLWString mString;
};

#endif  // LL_LLCLIPBOARD_H
