/** 
* @file lleditmenuhandler.h
* @authors Aaron Yonas, James Cook
*
* $LicenseInfo:firstyear=2006&license=viewergpl$
* 
* Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LLEDITMENUHANDLER_H
#define LLEDITMENUHANDLER_H

// Interface used by menu system for plug-in hotkey/menu handling
class LLEditMenuHandler
{
public:
	// this is needed even though this is just an interface class.
	virtual ~LLEditMenuHandler();
	
	virtual void	undo() {};
	virtual BOOL	canUndo() const { return FALSE; }
	
	virtual void	redo() {};
	virtual BOOL	canRedo() const { return FALSE; }
	
	virtual void	cut() {};
	virtual BOOL	canCut() const { return FALSE; }
	
	virtual void	copy() {};
	virtual BOOL	canCopy() const { return FALSE; }
	
	virtual void	paste() {};
	virtual BOOL	canPaste() const { return FALSE; }
	
	// "delete" is a keyword
	virtual void	doDelete() {};
	virtual BOOL	canDoDelete() const { return FALSE; }
	
	virtual void	selectAll() {};
	virtual BOOL	canSelectAll() const { return FALSE; }
	
	virtual void	deselect() {};
	virtual BOOL	canDeselect() const { return FALSE; }
	
	virtual void	duplicate() {};
	virtual BOOL	canDuplicate() const { return FALSE; }

	// TODO: Instead of being a public data member, it would be better to hide it altogether
	// and have a "set" method and then a bunch of static versions of the cut, copy, paste
	// methods, etc that operate on the current global instance. That would drastically
	// simplify the existing code that accesses this global variable by putting all the
	// null checks in the one implementation of those static methods. -MG
	static LLEditMenuHandler* gEditMenuHandler;
};


#endif
