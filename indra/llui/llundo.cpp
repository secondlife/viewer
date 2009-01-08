/** 
 * @file llundo.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
			llerrs << "Unable to create action for undo buffer" << llendl;
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
