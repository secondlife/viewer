/** 
 * @file lldqueueptr.h
 * @brief LLDynamicQueuePtr declaration
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#ifndef LL_LLDQUEUEPTR_H
#define LL_LLDQUEUEPTR_H

template <class Type> 
class LLDynamicQueuePtr
{
public:
	enum
	{
		OKAY = 0,
		FAIL = -1
	};
	
	LLDynamicQueuePtr(const S32 size=8);
	~LLDynamicQueuePtr();

	void init();
	void destroy();
	void reset();
	void realloc(U32 newsize);

	// ACCESSORS
	const Type& get(const S32 index) const;					// no bounds checking
	Type&       get(const S32 index);						// no bounds checking
	const Type& operator []	(const S32 index) const			{ return get(index); }
	Type&       operator []	(const S32 index)				{ return get(index); }
	S32			find(const Type &obj) const;

	S32			count() const								{ return (mLastObj >= mFirstObj ? mLastObj - mFirstObj : mLastObj + mMaxObj - mFirstObj); }
	S32			getMax() const								{ return mMaxObj; }
	S32			getFirst() const       { return mFirstObj; }
	S32			getLast () const       { return mLastObj; }

	// MANIPULATE
	S32         push(const Type &obj);					// add to end of Queue, returns index from start
	S32			pull(      Type &obj);			        // pull from Queue, returns index from start

	S32			remove   (S32 index);				    // remove by index
	S32			removeObj(const Type &obj);				// remove by object

protected:
	S32           mFirstObj, mLastObj, mMaxObj;
	Type*		  mMemory;

public:

	void print()
	{
		/*
		Convert this to llinfos if it's intended to be used - djs 08/30/02

		printf("Printing from %d to %d (of %d): ",mFirstObj, mLastObj, mMaxObj);

		if (mFirstObj <= mLastObj)
		{
			for (S32 i=mFirstObj;i<mLastObj;i++)
			{
				printf("%d ",mMemory[i]);
			}
		}
		else
		{
			for (S32 i=mFirstObj;i<mMaxObj;i++)
			{
				printf("%d ",mMemory[i]);
			}
			for (i=0;i<mLastObj;i++)
			{
				printf("%d ",mMemory[i]);
			}
		}
		printf("\n");
		*/
	}

};


//--------------------------------------------------------
// LLDynamicQueuePtrPtr implementation
//--------------------------------------------------------


template <class Type>
inline LLDynamicQueuePtr<Type>::LLDynamicQueuePtr(const S32 size)
{
	init();
	realloc(size);
}

template <class Type>
inline LLDynamicQueuePtr<Type>::~LLDynamicQueuePtr()
{
	destroy();
}

template <class Type>
inline void LLDynamicQueuePtr<Type>::init()
{ 
	mFirstObj    = 0;
	mLastObj     = 0;
	mMaxObj      = 0;
	mMemory      = NULL;
}

template <class Type>
inline void LLDynamicQueuePtr<Type>::realloc(U32 newsize)
{ 
	if (newsize)
	{
		if (mFirstObj > mLastObj && newsize > mMaxObj)
		{
			Type* new_memory = new Type[newsize];

			llassert(new_memory);

			S32 _count = count();
			S32 i, m = 0;
			for (i=mFirstObj; i < mMaxObj; i++)
			{
				new_memory[m++] = mMemory[i];
			}
			for (i=0; i <=mLastObj; i++)
			{
				new_memory[m++] = mMemory[i];
			}

			delete[] mMemory;
			mMemory = new_memory;

			mFirstObj = 0;
			mLastObj  = _count;
		}
		else
		{
			Type* new_memory = new Type[newsize];

			llassert(new_memory);

			S32 i, m = 0;
			for (i=0; i < mLastObj; i++)
			{
				new_memory[m++] = mMemory[i];
			}
			delete[] mMemory;
			mMemory = new_memory;
		}
	}
	else if (mMemory)
	{
		delete[] mMemory;
		mMemory = NULL;
	}

	mMaxObj = newsize;
}

template <class Type>
inline void LLDynamicQueuePtr<Type>::destroy()
{
	reset();
	delete[] mMemory;
	mMemory = NULL;
}


template <class Type>
void LLDynamicQueuePtr<Type>::reset()	   
{ 
	for (S32 i=0; i < mMaxObj; i++)
	{
		get(i) = NULL; // unrefs for pointers
	}

	mFirstObj    = 0;
	mLastObj     = 0;
}


template <class Type>
inline S32 LLDynamicQueuePtr<Type>::find(const Type &obj) const
{
	S32 i;
	if (mFirstObj <= mLastObj)
	{
		for ( i = mFirstObj; i < mLastObj; i++ )
		{
			if (mMemory[i] == obj)
			{
				return i;
			}
		}
	}
	else
	{
		for ( i = mFirstObj; i < mMaxObj; i++ )
		{
			if (mMemory[i] == obj)
			{
				return i;
			}
		}
		for ( i = 0; i < mLastObj; i++ )
		{
			if (mMemory[i] == obj)
			{
				return i;
			}
		}
	}

	return FAIL;
}

template <class Type>
inline S32 LLDynamicQueuePtr<Type>::remove(S32 i)
{
	if (mFirstObj > mLastObj)
	{
		if (i >= mFirstObj && i < mMaxObj)
		{
			while( i > mFirstObj)
			{
				mMemory[i] = mMemory[i-1];
				i--;
			}
			mMemory[mFirstObj] = NULL;
			mFirstObj++;
			if (mFirstObj >= mMaxObj) mFirstObj = 0;

			return count();
		}
		else if (i < mLastObj && i >= 0)
		{
			while(i < mLastObj)
			{
				mMemory[i] = mMemory[i+1];
				i++;
			}
			mMemory[mLastObj] = NULL;
			mLastObj--;
			if (mLastObj < 0) mLastObj = mMaxObj-1;

			return count();
		}
	}
	else if (i <= mLastObj && i >= mFirstObj)
	{
		while(i < mLastObj)
		{
			mMemory[i] = mMemory[i+1];
			i++;
		}
		mMemory[mLastObj] = NULL;
		mLastObj--;
		if (mLastObj < 0) mLastObj = mMaxObj-1;

		return count();
	}

	
	return FAIL;
}

template <class Type>
inline S32 LLDynamicQueuePtr<Type>::removeObj(const Type& obj)
{
	S32 ind = find(obj);
	if (ind >= 0)
	{
		return remove(ind);
	}
	return FAIL;
}

template <class Type>
inline S32	LLDynamicQueuePtr<Type>::push(const Type &obj) 
{
	if (mMaxObj - count() <= 1)
	{
		realloc(mMaxObj * 2);
	}

	mMemory[mLastObj++] = obj;

	if (mLastObj >= mMaxObj) 
	{
		mLastObj = 0;
	}

	return count();
}

template <class Type>
inline S32	LLDynamicQueuePtr<Type>::pull(Type &obj) 
{
	obj = NULL;

	if (count() < 1) return -1;

	obj = mMemory[mFirstObj];
	mMemory[mFirstObj] = NULL;

	mFirstObj++;

	if (mFirstObj >= mMaxObj) 
	{
		mFirstObj = 0;
	}

	return count();
}

template <class Type>
inline const Type& LLDynamicQueuePtr<Type>::get(const S32 i) const
{
	return mMemory[i];
}

template <class Type>
inline Type& LLDynamicQueuePtr<Type>::get(const S32 i)
{
	return mMemory[i];
}


#endif // LL_LLDQUEUEPTR_H
