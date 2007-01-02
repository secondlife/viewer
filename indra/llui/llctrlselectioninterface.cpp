/** 
 * @file llctrlselectioninterface.cpp
 * @brief Programmatic selection of items in a list.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llctrlselectioninterface.h"

#include "llsd.h"

// virtual
LLCtrlSelectionInterface::~LLCtrlSelectionInterface()
{ }

BOOL LLCtrlSelectionInterface::selectByValue(LLSD value)
{
	return setSelectedByValue(value, TRUE);
}

BOOL LLCtrlSelectionInterface::deselectByValue(LLSD value)
{ 
	return setSelectedByValue(value, FALSE); 
}


// virtual
LLCtrlListInterface::~LLCtrlListInterface()
{ }

LLScrollListItem* LLCtrlListInterface::addSimpleElement(const LLString& value)
{
	return addSimpleElement(value, ADD_BOTTOM, LLSD());
}

LLScrollListItem* LLCtrlListInterface::addSimpleElement(const LLString& value, EAddPosition pos)
{
	return addSimpleElement(value, pos, LLSD());
}

// virtual
LLCtrlScrollInterface::~LLCtrlScrollInterface()
{ }
