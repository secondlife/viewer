/**
 * @file llparcelselection.h
 * @brief Information about the currently selected parcel
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LLPARCELSELECTION_H
#define LLPARCELSELECTION_H

#include "llmemory.h"

class LLParcel;

class LLParcelSelection : public LLRefCount
{
	friend class LLViewerParcelMgr;

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
	BOOL	getMultipleOwners() const;

	// Is the entire parcel selected, or just a part?
	BOOL	getWholeParcelSelected() const;

	static LLParcelSelection* getNullParcelSelection();

private:
	void setParcel(LLParcel* parcel) { mParcel = parcel; }

private:
	LLParcel*	mParcel;
	BOOL		mSelectedMultipleOwners;
	BOOL		mWholeParcelSelected;
	S32			mSelectedSelfCount;
	S32			mSelectedOtherCount;
	S32			mSelectedPublicCount;

	static LLPointer<LLParcelSelection> sNullSelection;
};

typedef LLSafeHandle<LLParcelSelection> LLParcelSelectionHandle;

#endif
