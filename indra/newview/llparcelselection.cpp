/**
 * @file llparcelselection.cpp
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
