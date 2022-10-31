/** 
 * @file llctrlselectioninterface.h
 * @brief Programmatic selection of items in a list.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LLCTRLSELECTIONINTERFACE_H
#define LLCTRLSELECTIONINTERFACE_H

#include "stdtypes.h"
#include "llstring.h"
#include "llui.h"

class LLSD;
class LLUUID;
class LLScrollListItem;

class LLCtrlSelectionInterface
{
public:
    virtual ~LLCtrlSelectionInterface();
    
    enum EOperation
    {
        OP_DELETE = 1,
        OP_SELECT,
        OP_DESELECT,
    };

    virtual BOOL    getCanSelect() const = 0;

    virtual S32     getItemCount() const = 0;

    virtual BOOL    selectFirstItem() = 0;
    virtual BOOL    selectNthItem( S32 index ) = 0;
    virtual BOOL    selectItemRange( S32 first, S32 last ) = 0;

    virtual S32     getFirstSelectedIndex() const = 0;

    // TomY TODO: Simply cast the UUIDs to LLSDs, using the selectByValue function
    virtual BOOL    setCurrentByID( const LLUUID& id ) = 0;
    virtual LLUUID  getCurrentID() const = 0;

            BOOL    selectByValue(const LLSD value);
            BOOL    deselectByValue(const LLSD value);
    virtual BOOL    setSelectedByValue(const LLSD& value, BOOL selected) = 0;
    virtual LLSD    getSelectedValue() = 0;

    virtual BOOL    isSelected(const LLSD& value) const = 0;

    virtual BOOL    operateOnSelection(EOperation op) = 0;
    virtual BOOL    operateOnAll(EOperation op) = 0;
};

class LLCtrlListInterface : public LLCtrlSelectionInterface
{
public:
    virtual ~LLCtrlListInterface();
    
    virtual void addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM) = 0;
    virtual void clearColumns() = 0;
    virtual void setColumnLabel(const std::string& column, const std::string& label) = 0;
    // TomY TODO: Document this
    virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL) = 0;

    LLScrollListItem* addSimpleElement(const std::string& value); // defaults to bottom
    LLScrollListItem* addSimpleElement(const std::string& value, EAddPosition pos); // defaults to no LLSD() id
    virtual LLScrollListItem* addSimpleElement(const std::string& value, EAddPosition pos, const LLSD& id) = 0;

    virtual void clearRows() = 0;
    virtual void sortByColumn(const std::string& name, BOOL ascending) = 0;
};

class LLCtrlScrollInterface
{
public:
    virtual ~LLCtrlScrollInterface();
    
    virtual S32 getScrollPos() const = 0;
    virtual void setScrollPos( S32 pos ) = 0;
    virtual void scrollToShowSelected() = 0;
};

#endif
