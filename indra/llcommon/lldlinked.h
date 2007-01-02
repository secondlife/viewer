/** 
 * @file lldlinked.h
 * @brief Declaration of the LLDLinked class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LL_LLDLINKED_H
#define LL_LLDLINKED_H

#include <stdlib.h>

template <class Type> class LLDLinked
{
	LLDLinked* mNextp;
	LLDLinked* mPrevp;
public:

	Type*   getNext()  { return (Type*)mNextp; }
	Type*   getPrev()  { return (Type*)mPrevp; }
	Type*   getFirst() { return (Type*)mNextp; }

	void    init()
	{
		mNextp = mPrevp = NULL;
	}

	void    unlink()
	{
		if (mPrevp) mPrevp->mNextp = mNextp;
		if (mNextp) mNextp->mPrevp = mPrevp;
	}

	 LLDLinked() { mNextp = mPrevp = NULL; }
	virtual ~LLDLinked() { unlink(); }

	virtual void    deleteAll()
	{
		Type *curp = getFirst();
		while(curp)
		{
			Type *nextp = curp->getNext();
			curp->unlink();
			delete curp;
			curp = nextp;
		}
	}

	void relink(Type &after)
	{
		LLDLinked *afterp = (LLDLinked*)&after;
		afterp->mPrevp = this;
		mNextp = afterp;
	}

	virtual void    append(Type& after)
	{
		LLDLinked *afterp = (LLDLinked*)&after;
		afterp->mPrevp    = this;
		afterp->mNextp    = mNextp;
		if (mNextp) mNextp->mPrevp = afterp;
		mNextp            = afterp;
	}

	virtual void    insert(Type& before)
	{
		LLDLinked *beforep = (LLDLinked*)&before;
		beforep->mNextp    = this;
		beforep->mPrevp    = mPrevp;
		if (mPrevp) mPrevp->mNextp = beforep;
		mPrevp             = beforep;
	}

	virtual void    put(Type& obj) { append(obj); }
};

#endif
