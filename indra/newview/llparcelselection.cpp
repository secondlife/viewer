/**
 * @file llparcelselection.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llparcelselection.h"

#include "llparcel.h"


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
