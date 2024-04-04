/**
 * @file llparcelselection.h
 * @brief Information about the currently selected parcel
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LLPARCELSELECTION_H
#define LLPARCELSELECTION_H

#include "llrefcount.h"
#include "llsafehandle.h"

class LLParcel;

class LLParcelSelection : public LLRefCount
{
	friend class LLViewerParcelMgr;
	friend class LLSafeHandle<LLParcelSelection>;

protected:
	~LLParcelSelection();

public:
	LLParcelSelection(LLParcel* parcel);
	LLParcelSelection();

	// this can return NULL at any time, as parcel selection
	// might have been invalidated.
	LLParcel* getParcel() { return mParcel; }

	// Return the number of grid units that are owned by you within
	// the selection (computed by server).
	S32 getSelfCount() const { return mSelectedSelfCount; }

	// Returns area that will actually be claimed in meters squared.
	S32		getClaimableArea() const;
	bool	hasOthersSelected() const;

	// Does the selection have multiple land owners in it?
	bool	getMultipleOwners() const;

	// Is the entire parcel selected, or just a part?
	bool	getWholeParcelSelected() const;

private:
	void setParcel(LLParcel* parcel) { mParcel = parcel; }

private:
	LLParcel*	mParcel;
	bool		mSelectedMultipleOwners;
	bool		mWholeParcelSelected;
	S32			mSelectedSelfCount;
	S32			mSelectedOtherCount;
	S32			mSelectedPublicCount;
};

typedef LLSafeHandle<LLParcelSelection> LLParcelSelectionHandle;

#endif
