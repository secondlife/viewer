/** 
 * @file llparcelselection.h
 * @brief Information about the currently selected parcel
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
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

typedef LLHandle<LLParcelSelection> LLParcelSelectionHandle;

#endif
