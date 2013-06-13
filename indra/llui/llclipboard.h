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

#include <boost/function.hpp>

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
// the world. Use LLClipboard::instance().method() to use its methods.
// Note that the text and UUIDs are loosely coupled only. There are few cases
// where the viewer does offer a serialized version of the UUID on the clipboard.
// In those case, the text is overridden when copying/cutting the item. 
// In all other cases, the text and the UUIDs are very much independent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLClipboard : public LLSingleton<LLClipboard>
{
public:
	LLClipboard();
	~LLClipboard();
	
	// Clears the clipboard
	void reset();
	// Returns the state of the clipboard so client can know if it has been modified (comparing with tracked state)
	int	getGeneration() const { return mGeneration; }

	// Text strings management:
	// ------------------------
	// We support two flavors of text clipboards. The default is the explicitly
	// copy-and-pasted clipboard. The second is the so-called 'primary' clipboard
	// which is implicitly copied upon selection on platforms which expect this
	// (i.e. X11/Linux, Mac).
	bool copyToClipboard(const LLWString& src, S32 pos, S32 len, bool use_primary = false);
	bool addToClipboard(const LLWString& src, S32 pos, S32 len, bool use_primary = false);
	bool pasteFromClipboard(LLWString& dst, bool use_primary = false);
	bool isTextAvailable(bool use_primary = false) const;
	
	// Object list management:
	// -----------------------
	// Clears and adds one single object to the clipboard
	bool copyToClipboard(const LLUUID& src, const LLAssetType::EType type = LLAssetType::AT_NONE);
	// Adds one object to the current list of objects on the clipboard
	bool addToClipboard(const LLUUID& src, const LLAssetType::EType type = LLAssetType::AT_NONE);
	// Gets a copy of the objects on the clipboard
	bool pasteFromClipboard(std::vector<LLUUID>& inventory_objects) const;
	
	bool hasContents() const;										// True if the clipboard has pasteable objects
	bool isOnClipboard(const LLUUID& object) const;					// True if the input object uuid is on the clipboard

	bool isCutMode() const { return mCutMode; }
	void setCutMode(bool mode) { mCutMode = mode; mGeneration++; }

private:
	std::vector<LLUUID> mObjects;		// Objects on the clipboard. Can be empty while mString contains something licit (e.g. text from chat)
	LLWString mString;					// The text string. If mObjects is not empty, this string is reflecting them (UUIDs for the moment) if the asset type is knowable.
	bool mCutMode;						// This is a convenience flag for the viewer.
	int mGeneration;					// Incremented when the clipboard changes so that interested parties can check for changes on the clipboard.	
};

#endif  // LL_LLCLIPBOARD_H
