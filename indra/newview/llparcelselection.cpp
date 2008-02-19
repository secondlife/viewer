/** 
 * @file llparcelselection.cpp
 * @brief Information about the currently selected parcel
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llparcelselection.h"

#include "llparcel.h"


// static
LLPointer<LLParcelSelection> LLParcelSelection::sNullSelection;

template<> 
	const LLSafeHandle<LLParcelSelection>::NullFunc 
		LLSafeHandle<LLParcelSelection>::sNullFunc = LLParcelSelection::getNullParcelSelection;

//
// LLParcelSelection
//
LLParcelSelection::LLParcelSelection() : 
	mParcel(NULL),
	mSelectedMultipleOwners(FALSE),
	mWholeParcelSelected(FALSE),
	mSelectedSelfCount(0),
	mSelectedOtherCount(0),
	mSelectedPublicCount(0)
{
}

LLParcelSelection::LLParcelSelection(LLParcel* parcel)  : 
	mParcel(parcel),
	mSelectedMultipleOwners(FALSE),
	mWholeParcelSelected(FALSE),
	mSelectedSelfCount(0),
	mSelectedOtherCount(0),
	mSelectedPublicCount(0)
{
}

LLParcelSelection::~LLParcelSelection()
{
}

BOOL LLParcelSelection::getMultipleOwners() const
{
	return mSelectedMultipleOwners;
}


BOOL LLParcelSelection::getWholeParcelSelected() const
{
	return mWholeParcelSelected;
}


S32 LLParcelSelection::getClaimableArea() const
{
	const S32 UNIT_AREA = S32( PARCEL_GRID_STEP_METERS * PARCEL_GRID_STEP_METERS );
	return mSelectedPublicCount * UNIT_AREA;
}

bool LLParcelSelection::hasOthersSelected() const
{
	return mSelectedOtherCount != 0;
}

// static
LLParcelSelection* LLParcelSelection::getNullParcelSelection()
{
	if (sNullSelection.isNull())
	{
		sNullSelection = new LLParcelSelection;
	}
	
	return sNullSelection;
}
