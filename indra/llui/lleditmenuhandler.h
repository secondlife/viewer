/** 
* @file lleditmenuhandler.h
* @authors Aaron Yonas, James Cook
*
* Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
* $License$
*/

#ifndef LLEDITMENUHANDLER_H
#define LLEDITMENUHANDLER_H

// Interface used by menu system for plug-in hotkey/menu handling
class LLEditMenuHandler
{
public:
	// this is needed even though this is just an interface class.
	virtual ~LLEditMenuHandler();
	
	virtual void	undo();
	virtual BOOL	canUndo();
	
	virtual void	redo();
	virtual BOOL	canRedo();
	
	virtual void	cut();
	virtual BOOL	canCut();
	
	virtual void	copy();
	virtual BOOL	canCopy();
	
	virtual void	paste();
	virtual BOOL	canPaste();
	
	// "delete" is a keyword
	virtual void	doDelete();
	virtual BOOL	canDoDelete();
	
	virtual void	selectAll();
	virtual BOOL	canSelectAll();
	
	virtual void	deselect();
	virtual BOOL	canDeselect();
	
	virtual void	duplicate();
	virtual BOOL	canDuplicate();
};

extern LLEditMenuHandler* gEditMenuHandler;

#endif
