/** 
 * @file llsafehandle.h
 * @brief Reference-counted object where Object() is valid, not NULL.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
