/** 
 * @file llsafehandle.h
 * @brief Reference-counted object where Object() is valid, not NULL.
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
#ifndef LLSAFEHANDLE_H
#define LLSAFEHANDLE_H

#include "llerror.h"	// *TODO: consider eliminating this

// Expands LLPointer to return a pointer to a special instance of class Type instead of NULL.
// This is useful in instances where operations on NULL pointers are semantically safe and/or
// when error checking occurs at a different granularity or in a different part of the code
// than when referencing an object via a LLSafeHandle.

template <class Type> 
class LLSafeHandle
{
public:
	LLSafeHandle() :
		mPointer(NULL)
	{
	}

	LLSafeHandle(Type* ptr) : 
		mPointer(NULL)
	{
		assign(ptr);
	}

	LLSafeHandle(const LLSafeHandle<Type>& ptr) : 
		mPointer(NULL)
	{
		assign(ptr.mPointer);
	}

	// support conversion up the type hierarchy.  See Item 45 in Effective C++, 3rd Ed.
	template<typename Subclass>
	LLSafeHandle(const LLSafeHandle<Subclass>& ptr) : 
		mPointer(NULL)
	{
		assign(ptr.get());
	}

	~LLSafeHandle()								
	{
		unref();
	}

	const Type*	operator->() const				{ return nonNull(mPointer); }
	Type*	operator->()						{ return nonNull(mPointer); }

	Type*	get() const							{ return mPointer; }
	void	clear()								{ assign(NULL); }
	// we disallow these operations as they expose our null objects to direct manipulation
	// and bypass the reference counting semantics
	//const Type&	operator*() const			{ return *nonNull(mPointer); }
	//Type&	operator*()							{ return *nonNull(mPointer); }

	operator BOOL()  const						{ return mPointer != NULL; }
	operator bool()  const						{ return mPointer != NULL; }
	bool operator!() const						{ return mPointer == NULL; }
	bool isNull() const							{ return mPointer == NULL; }
	bool notNull() const						{ return mPointer != NULL; }


	operator Type*()       const				{ return mPointer; }
	operator const Type*() const				{ return mPointer; }
	bool operator !=(Type* ptr) const           { return (mPointer != ptr); 	}
	bool operator ==(Type* ptr) const           { return (mPointer == ptr); 	}
	bool operator ==(const LLSafeHandle<Type>& ptr) const           { return (mPointer == ptr.mPointer); 	}
	bool operator < (const LLSafeHandle<Type>& ptr) const           { return (mPointer < ptr.mPointer); 	}
	bool operator > (const LLSafeHandle<Type>& ptr) const           { return (mPointer > ptr.mPointer); 	}

	LLSafeHandle<Type>& operator =(Type* ptr)                   
	{ 
		assign(ptr);
		return *this; 
	}

	LLSafeHandle<Type>& operator =(const LLSafeHandle<Type>& ptr)  
	{ 
		assign(ptr.mPointer);
		return *this; 
	}

	// support assignment up the type hierarchy. See Item 45 in Effective C++, 3rd Ed.
	template<typename Subclass>
	LLSafeHandle<Type>& operator =(const LLSafeHandle<Subclass>& ptr)  
	{ 
		assign(ptr.get());
		return *this; 
	}

public:
	typedef Type* (*NullFunc)();
	static const NullFunc sNullFunc;

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

	void assign(Type* ptr)
	{
		if( mPointer != ptr )
		{
			unref(); 
			mPointer = ptr; 
			ref();
		}
	}

	static Type* nonNull(Type* ptr)
	{
		return ptr == NULL ? sNullFunc() : ptr;
	}

protected:
	Type*	mPointer;
};

#endif
