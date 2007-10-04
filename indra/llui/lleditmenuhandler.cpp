/** 
* @file lleditmenuhandler.cpp
* @authors Aaron Yonas, James Cook
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
