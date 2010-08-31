/** 
 * @file llpointer.h
 * @brief A reference-counted pointer for objects derived from LLRefCount
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#ifndef LLPOINTER_H
#define LLPOINTER_H

#include "llerror.h"	// *TODO: consider eliminating this

//----------------------------------------------------------------------------
// RefCount objects should generally only be accessed by way of LLPointer<>'s
// NOTE: LLPointer<LLFoo> x = new LLFoo(); MAY NOT BE THREAD SAFE
//   if LLFoo::LLFoo() does anything like put itself in an update queue.
//   The queue may get accessed before it gets assigned to x.
// The correct implementation is:
//   LLPointer<LLFoo> x = new LLFoo; // constructor does not do anything interesting
//   x->instantiate(); // does stuff like place x into an update queue

// see llthread.h for LLThreadSafeRefCount

//----------------------------------------------------------------------------

// Note: relies on Type having ref() and unref() methods
template <class Type> class LLPointer
{
public:

	LLPointer() : 
		mPointer(NULL)
	{
	}

	LLPointer(Type* ptr) : 
		mPointer(ptr)
	{
		ref();
	}

	LLPointer(const LLPointer<Type>& ptr) : 
		mPointer(ptr.mPointer)
	{
		ref();
	}

	// support conversion up the type hierarchy.  See Item 45 in Effective C++, 3rd Ed.
	template<typename Subclass>
	LLPointer(const LLPointer<Subclass>& ptr) : 
		mPointer(ptr.get())
	{
		ref();
	}

	~LLPointer()								
	{
		unref();
	}

	Type*	get() const							{ return mPointer; }
	const Type*	operator->() const				{ return mPointer; }
	Type*	operator->()						{ return mPointer; }
	const Type&	operator*() const				{ return *mPointer; }
	Type&	operator*()							{ return *mPointer; }

	operator BOOL()  const						{ return (mPointer != NULL); }
	operator bool()  const						{ return (mPointer != NULL); }
	bool operator!() const						{ return (mPointer == NULL); }
	bool isNull() const							{ return (mPointer == NULL); }
	bool notNull() const						{ return (mPointer != NULL); }

	operator Type*()       const				{ return mPointer; }
	bool operator !=(Type* ptr) const           { return (mPointer != ptr); 	}
	bool operator ==(Type* ptr) const           { return (mPointer == ptr); 	}
	bool operator ==(const LLPointer<Type>& ptr) const           { return (mPointer == ptr.mPointer); 	}
	bool operator < (const LLPointer<Type>& ptr) const           { return (mPointer < ptr.mPointer); 	}
	bool operator > (const LLPointer<Type>& ptr) const           { return (mPointer > ptr.mPointer); 	}

	LLPointer<Type>& operator =(Type* ptr)                   
	{ 
		if( mPointer != ptr )
		{
			unref(); 
			mPointer = ptr; 
			ref();
		}

		return *this; 
	}

	LLPointer<Type>& operator =(const LLPointer<Type>& ptr)  
	{ 
		if( mPointer != ptr.mPointer )
		{
			unref(); 
			mPointer = ptr.mPointer;
			ref();
		}
		return *this; 
	}

	// support assignment up the type hierarchy. See Item 45 in Effective C++, 3rd Ed.
	template<typename Subclass>
	LLPointer<Type>& operator =(const LLPointer<Subclass>& ptr)  
	{ 
		if( mPointer != ptr.get() )
		{
			unref(); 
			mPointer = ptr.get();
			ref();
		}
		return *this; 
	}
	
	// Just exchange the pointers, which will not change the reference counts.
	static void swap(LLPointer<Type>& a, LLPointer<Type>& b)
	{
		Type* temp = a.mPointer;
		a.mPointer = b.mPointer;
		b.mPointer = temp;
	}

protected:
	void ref()                             
	{ 
		if (mPointer)
		{
			mPointer->ref();
		}
	}

	void unref()
	{
		if (mPointer)
		{
			Type *tempp = mPointer;
			mPointer = NULL;
			tempp->unref();
			if (mPointer != NULL)
			{
				llwarns << "Unreference did assignment to non-NULL because of destructor" << llendl;
				unref();
			}
		}
	}

protected:
	Type*	mPointer;
};

#endif
