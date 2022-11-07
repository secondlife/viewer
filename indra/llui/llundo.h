/** 
 * @file llundo.h
 * @brief Generic interface for undo/redo circular buffer.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLUNDO_H
#define LL_LLUNDO_H


class LLUndoBuffer
{
public:
    class LLUndoAction
    {
        friend class LLUndoBuffer;
    public:
        virtual void undo() = 0;
        virtual void redo() = 0;
        virtual void cleanup() {};
    protected:
        LLUndoAction(): mClusterID(0) {};
        virtual ~LLUndoAction(){};
    private:
        S32     mClusterID;
    };

    LLUndoBuffer( LLUndoAction (*create_func()), S32 initial_count );
    virtual ~LLUndoBuffer();

    LLUndoAction *getNextAction(BOOL setClusterBegin = TRUE);
    BOOL undoAction();
    BOOL redoAction();
    BOOL canUndo() { return (mNextAction != mFirstAction); }
    BOOL canRedo() { return (mNextAction != mLastAction); }

    void flushActions();

private:
    LLUndoAction **mActions;    // array of pointers to undoactions
    S32         mNumActions;    // total number of actions in ring buffer
    S32         mNextAction;    // next action to perform undo/redo on
    S32         mLastAction;    // last action actually added to undo buffer
    S32         mFirstAction;   // beginning of ring buffer (don't undo any further)
    S32         mOperationID;   // current operation id, for undoing and redoing in clusters
};

#endif //LL_LLUNDO_H
