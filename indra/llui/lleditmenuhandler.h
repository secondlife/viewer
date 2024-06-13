/**
* @file lleditmenuhandler.h
* @authors Aaron Yonas, James Cook
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

#ifndef LLEDITMENUHANDLER_H
#define LLEDITMENUHANDLER_H

// Interface used by menu system for plug-in hotkey/menu handling
class LLEditMenuHandler
{
public:
    // this is needed even though this is just an interface class.
    virtual ~LLEditMenuHandler();

    virtual void    undo() {};
    virtual bool    canUndo() const { return false; }

    virtual void    redo() {};
    virtual bool    canRedo() const { return false; }

    virtual void    cut() {};
    virtual bool    canCut() const { return false; }

    virtual void    copy() {};
    virtual bool    canCopy() const { return false; }

    virtual void    paste() {};
    virtual bool    canPaste() const { return false; }

    // "delete" is a keyword
    virtual void    doDelete() {};
    virtual bool    canDoDelete() const { return false; }

    virtual void    selectAll() {};
    virtual bool    canSelectAll() const { return false; }

    virtual void    deselect() {};
    virtual bool    canDeselect() const { return false; }

    // TODO: Instead of being a public data member, it would be better to hide it altogether
    // and have a "set" method and then a bunch of static versions of the cut, copy, paste
    // methods, etc that operate on the current global instance. That would drastically
    // simplify the existing code that accesses this global variable by putting all the
    // null checks in the one implementation of those static methods. -MG
    static LLEditMenuHandler* gEditMenuHandler;
};


#endif
