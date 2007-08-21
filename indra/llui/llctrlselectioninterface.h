/** 
 * @file llctrlselectioninterface.h
 * @brief Programmatic selection of items in a list.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
