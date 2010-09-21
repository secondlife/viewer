/** 
 * @file llundo.h
 * @brief Generic interface for undo/redo circular buffer.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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
		S32		mClusterID;
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
	LLUndoAction **mActions;	// array of pointers to undoactions
	S32			mNumActions;	// total number of actions in ring buffer
	S32			mNextAction;	// next action to perform undo/redo on
	S32			mLastAction;	// last action actually added to undo buffer
	S32			mFirstAction;	// beginning of ring buffer (don't undo any further)
	S32			mOperationID;	// current operation id, for undoing and redoing in clusters
};

#endif //LL_LLUNDO_H
