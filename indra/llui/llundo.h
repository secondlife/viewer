/** 
 * @file llundo.h
 * @brief LLUndo class header file
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLUNDO_H
#define LL_LLUNDO_H

class LLUndoAction
{
	friend class LLUndoBuffer;
protected:
	S32		mClusterID;
protected:
	LLUndoAction(): mClusterID(0) {};
	virtual ~LLUndoAction(){};

public:
	static LLUndoAction *create() { return NULL; }

	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual void cleanup() {};
};

class LLUndoBuffer
{
protected:
	LLUndoAction **mActions;	// array of pointers to undoactions
	S32			mNumActions;	// total number of actions in ring buffer
	S32			mNextAction;	// next action to perform undo/redo on
	S32			mLastAction;	// last action actually added to undo buffer
	S32			mFirstAction;	// beginning of ring buffer (don't undo any further)
	S32			mOperationID;	// current operation id, for undoing and redoing in clusters

public:
	LLUndoBuffer( LLUndoAction (*create_func()), S32 initial_count );
	virtual ~LLUndoBuffer();

	LLUndoAction *getNextAction(BOOL setClusterBegin = TRUE);
	BOOL undoAction();
	BOOL redoAction();
	BOOL canUndo() { return (mNextAction != mFirstAction); }
	BOOL canRedo() { return (mNextAction != mLastAction); }

	void flushActions();
};

#endif //LL_LLUNDO_H
