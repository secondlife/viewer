/** 
 * @file lldlinked.h
 * @brief Declaration of the LLDLinked class.
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
#ifndef LL_LLDLINKED_H
#define LL_LLDLINKED_H

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
