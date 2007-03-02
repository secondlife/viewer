/** 
 * @file llstrider.h
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSTRIDER_H
#define LL_LLSTRIDER_H

#include "stdtypes.h"

template <class Object> class LLStrider
{
	union
	{
		Object* mObjectp;
		U8*		mBytep;
	};
	U32     mSkip;
public:

	LLStrider()  { mObjectp = NULL; mSkip = sizeof(Object); } 
	~LLStrider() { } 

	const LLStrider<Object>& operator =  (Object *first)    { mObjectp = first; return *this;}
	void setStride (S32 skipBytes)	{ mSkip = (skipBytes ? skipBytes : sizeof(Object));}

	void skip(const U32 index)     { mBytep += mSkip*index;}

	Object* get()                  { return mObjectp; }
	Object* operator->()           { return mObjectp; }
	Object& operator *()           { return *mObjectp; }
	Object* operator ++(int)       { Object* old = mObjectp; mBytep += mSkip; return old; }
	Object* operator +=(int i)     { mBytep += mSkip*i; return mObjectp; }
	Object& operator[](U32 index)  { return *(Object*)(mBytep + (mSkip * index)); }
};

#endif // LL_LLSTRIDER_H
