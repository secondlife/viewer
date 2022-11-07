/** 
 * @file llundo.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"
#include "llundo.h"


// TODO:
// implement doubly linked circular list for ring buffer
// this will allow us to easily change the size of an undo buffer on the fly

//-----------------------------------------------------------------------------
// LLUndoBuffer()
//-----------------------------------------------------------------------------
LLUndoBuffer::LLUndoBuffer( LLUndoAction (*create_func()), S32 initial_count )
{
    mNextAction = 0;
    mLastAction = 0;
    mFirstAction = 0;
    mOperationID = 0;

    mNumActions = initial_count;    

    mActions = new LLUndoAction *[initial_count];

    //initialize buffer with actions
    for (S32 i = 0; i < initial_count; i++)
    {
        mActions[i] = create_func();
        if (!mActions[i])
        {
            LL_ERRS() << "Unable to create action for undo buffer" << LL_ENDL;
        }
    }
}

//-----------------------------------------------------------------------------
// ~LLUndoBuffer()
//-----------------------------------------------------------------------------
LLUndoBuffer::~LLUndoBuffer()
{
    for (S32 i = 0; i < mNumActions; i++)
    {
        delete mActions[i];
    }

    delete [] mActions;
}

//-----------------------------------------------------------------------------
// getNextAction()
//-----------------------------------------------------------------------------
LLUndoBuffer::LLUndoAction* LLUndoBuffer::getNextAction(BOOL setClusterBegin)
{
    LLUndoAction *nextAction = mActions[mNextAction];

    if (setClusterBegin)
    {
        mOperationID++;
    }
    mActions[mNextAction]->mClusterID = mOperationID;

    mNextAction = (mNextAction + 1) % mNumActions;
    mLastAction = mNextAction;

    if (mNextAction == mFirstAction)
    {
        mActions[mFirstAction]->cleanup();
        mFirstAction = (mFirstAction + 1) % mNumActions;
    }

    return nextAction;
}

//-----------------------------------------------------------------------------
// undoAction()
//-----------------------------------------------------------------------------
BOOL LLUndoBuffer::undoAction()
{
    if (!canUndo())
    {
        return FALSE;
    }

    S32 prevAction = (mNextAction + mNumActions - 1) % mNumActions;

    while(mActions[prevAction]->mClusterID == mOperationID)
    {
        // go ahead and decrement action index
        mNextAction = prevAction;

        // undo this action
        mActions[mNextAction]->undo();

        // we're at the first action, so we don't know if we've actually undid everything
        if (mNextAction == mFirstAction)
        {
            mOperationID--;
            return FALSE;
        }

        // do wrap-around of index, but avoid negative numbers for modulo operator
        prevAction = (mNextAction + mNumActions - 1) % mNumActions;
    }

    mOperationID--;

    return TRUE;
}

//-----------------------------------------------------------------------------
// redoAction()
//-----------------------------------------------------------------------------
BOOL LLUndoBuffer::redoAction()
{
    if (!canRedo())
    {
        return FALSE;
    }

    mOperationID++;

    while(mActions[mNextAction]->mClusterID == mOperationID)
    {
        if (mNextAction == mLastAction)
        {
            return FALSE;
        }

        mActions[mNextAction]->redo();

        // do wrap-around of index
        mNextAction = (mNextAction + 1) % mNumActions;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// flushActions()
//-----------------------------------------------------------------------------
void LLUndoBuffer::flushActions()
{
    for (S32 i = 0; i < mNumActions; i++)
    {
        mActions[i]->cleanup();
    }
    mNextAction = 0;
    mLastAction = 0;
    mFirstAction = 0;
    mOperationID = 0;
}
