/** 
* @file lleditmenuhandler.cpp
* @authors Aaron Yonas, James Cook
*
* Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
* $License$
*/

#include "linden_common.h"

#include "lleditmenuhandler.h"

LLEditMenuHandler* gEditMenuHandler = NULL;

// virtual
LLEditMenuHandler::~LLEditMenuHandler()
{ }

// virtual
void LLEditMenuHandler::undo()
{ }

// virtual
BOOL LLEditMenuHandler::canUndo()
{
	return FALSE;
}

// virtual
void LLEditMenuHandler::redo()
{ }

// virtual
BOOL LLEditMenuHandler::canRedo()
{
	return FALSE;
}

// virtual
void LLEditMenuHandler::cut()
{ }

// virtual
BOOL LLEditMenuHandler::canCut()
{
	return FALSE;
}

// virtual
void LLEditMenuHandler::copy()
{ }

// virtual
BOOL LLEditMenuHandler::canCopy()
{
	return FALSE;
}

// virtual
void LLEditMenuHandler::paste()
{ }

// virtual
BOOL LLEditMenuHandler::canPaste()
{
	return FALSE;
}

// virtual
void LLEditMenuHandler::doDelete()
{ }

// virtual
BOOL LLEditMenuHandler::canDoDelete()
{
	return FALSE;
}

// virtual
void LLEditMenuHandler::selectAll()
{ }

// virtual
BOOL LLEditMenuHandler::canSelectAll()
{
	return FALSE;
}

// virtual
void LLEditMenuHandler::deselect()
{ }

// virtual
BOOL LLEditMenuHandler::canDeselect()
{
	return FALSE;
}

// virtual
void LLEditMenuHandler::duplicate()
{ }

// virtual
BOOL LLEditMenuHandler::canDuplicate()
{
	return FALSE;
}
