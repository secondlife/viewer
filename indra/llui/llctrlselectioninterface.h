/** 
 * @file llctrlselectioninterface.h
 * @brief Programmatic selection of items in a list.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LLCTRLSELECTIONINTERFACE_H
#define LLCTRLSELECTIONINTERFACE_H

#include "stdtypes.h"
#include "stdenums.h"
#include "llstring.h"

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

	virtual BOOL	getCanSelect() const = 0;

	virtual S32		getItemCount() const = 0;

	virtual BOOL	selectFirstItem() = 0;
	virtual BOOL	selectNthItem( S32 index ) = 0;

	virtual S32		getFirstSelectedIndex() const = 0;

	// TomY TODO: Simply cast the UUIDs to LLSDs, using the selectByValue function
	virtual BOOL	setCurrentByID( const LLUUID& id ) = 0;
	virtual LLUUID	getCurrentID() = 0;

			BOOL	selectByValue(LLSD value);
			BOOL	deselectByValue(LLSD value);
	virtual BOOL	setSelectedByValue(LLSD value, BOOL selected) = 0;
	virtual LLSD	getSimpleSelectedValue() = 0;

	virtual BOOL	isSelected(LLSD value) = 0;

	virtual BOOL	operateOnSelection(EOperation op) = 0;
	virtual BOOL	operateOnAll(EOperation op) = 0;
};

class LLCtrlListInterface : public LLCtrlSelectionInterface
{
public:
	virtual ~LLCtrlListInterface();
	
	virtual void addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM) = 0;
	virtual void clearColumns() = 0;
	virtual void setColumnLabel(const LLString& column, const LLString& label) = 0;
	// TomY TODO: Document this
	virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL) = 0;

	LLScrollListItem* addSimpleElement(const LLString& value); // defaults to bottom
	LLScrollListItem* addSimpleElement(const LLString& value, EAddPosition pos); // defaults to no LLSD() id
	virtual LLScrollListItem* addSimpleElement(const LLString& value, EAddPosition pos, const LLSD& id) = 0;

	virtual void clearRows() = 0;
	virtual void sortByColumn(LLString name, BOOL ascending) = 0;
};

class LLCtrlScrollInterface
{
public:
	virtual ~LLCtrlScrollInterface();
	
	virtual S32 getScrollPos() = 0;
	virtual void setScrollPos( S32 pos ) = 0;
	virtual void scrollToShowSelected() = 0;
};

#endif
