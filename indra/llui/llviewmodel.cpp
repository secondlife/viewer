/**
 * @file   llviewmodel.cpp
 * @author Nat Goodspeed
 * @date   2008-08-08
 * @brief  Implementation for llviewmodel.
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

// Precompiled header
#include "linden_common.h"
// associated header
#include "llviewmodel.h"
// STL headers
// std headers
// external library headers
// other Linden headers

///
LLViewModel::LLViewModel()
:   mDirty(false)
{
}

/// Instantiate an LLViewModel with an existing data value
LLViewModel::LLViewModel(const LLSD& value)
:   mDirty(false)
{
    setValue(value);
}

/// Update the stored value
void LLViewModel::setValue(const LLSD& value)
{
    mValue = value;
    mDirty = true;
}

LLSD LLViewModel::getValue() const
{
    return mValue;
}

////////////////////////////////////////////////////////////////////////////

///
LLTextViewModel::LLTextViewModel()
  : LLViewModel(false),
    mUpdateFromDisplay(false)
{
}

/// Instantiate an LLViewModel with an existing data value
LLTextViewModel::LLTextViewModel(const LLSD& value)
  : LLViewModel(value),
    mUpdateFromDisplay(false)
{
}

/// Update the stored value
void LLTextViewModel::setValue(const LLSD& value)
{
    // approximate LLSD storage usage
    LLViewModel::setValue(value);
    mDisplay = utf8str_to_wstring(value.asString());

    // mDisplay and mValue agree
    mUpdateFromDisplay = false;
}

void LLTextViewModel::setDisplay(const LLWString& value)
{
    // This is the strange way to alter the value. Normally we'd setValue()
    // and do the utf8str_to_wstring() to get the corresponding mDisplay
    // value. But a text editor might want to edit the display string
    // directly, then convert back to UTF8 on commit.
    mDisplay = value;
    mDirty = true;
    // Don't immediately convert to UTF8 -- do it lazily -- we expect many
    // more setDisplay() calls than getValue() calls. Just flag that it needs
    // doing.
    mUpdateFromDisplay = true;
}

LLSD LLTextViewModel::getValue() const
{
    // Has anyone called setDisplay() since the last setValue()? If so, have
    // to convert mDisplay back to UTF8.
    if (mUpdateFromDisplay)
    {
        // The fact that we're lazily updating fields in this object should be
        // transparent to clients, which is why this method is left
        // conventionally const. Nor do we particularly want to make these
        // members mutable. Just cast away constness in this one place.
        LLTextViewModel* nthis = const_cast<LLTextViewModel*>(this);
        nthis->mUpdateFromDisplay = false;
        nthis->mValue = wstring_to_utf8str(mDisplay);
    }
    return LLViewModel::getValue();
}


////////////////////////////////////////////////////////////////////////////

LLListViewModel::LLListViewModel(const LLSD& values)
  : LLViewModel()
{
}

void LLListViewModel::addColumn(const LLSD& column, EAddPosition pos)
{
}

void LLListViewModel::clearColumns()
{
}

void LLListViewModel::setColumnLabel(const std::string& column, const std::string& label)
{
}

LLScrollListItem* LLListViewModel::addElement(const LLSD& value, EAddPosition pos,
                                         void* userdata)
{
    return NULL;
}

LLScrollListItem* LLListViewModel::addSimpleElement(const std::string& value, EAddPosition pos,
                                               const LLSD& id)
{
    return NULL;
}

void LLListViewModel::clearRows()
{
}

void LLListViewModel::sortByColumn(const std::string& name, bool ascending)
{
}
