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
#include "llsingleton.h"

/*==========================================================================*|
 ____   ___    _   _  ___ _____   _   _ ____  _____ _
|  _ \ / _ \  | \ | |/ _ \_   _| | | | / ___|| ____| |
| | | | | | | |  \| | | | || |   | | | \___ \|  _| | |
| |_| | |_| | | |\  | |_| || |   | |_| |___) | |___|_|
|____/ \___/  |_| \_|\___/ |_|    \___/|____/|_____(_)

This handle class is deprecated. Unfortunately it is already in widespread use
to reference the LLObjectSelection and LLParcelSelection classes, but do not
apply LLSafeHandle to other classes, or declare new instances.

Instead, use LLPointer or other smart pointer types with appropriate checks
for NULL. If you're certain the reference cannot (or must not) be NULL,
consider storing a C++ reference instead -- or use (e.g.) LLCheckedHandle.

When an LLSafeHandle<T> containing NULL is dereferenced, it resolves to a
canonical "null" T instance. This raises issues about the lifespan of the
"null" instance. In addition to encouraging sloppy coding practices, it
potentially masks bugs when code that performs some mutating operation
inadvertently applies it to the "null" instance. That result might or might
not ever affect subsequent computations.
|*==========================================================================*/

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
				LL_WARNS() << "Unreference did assignment to non-NULL because of destructor" << LL_ENDL;
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

	// Define an LLSingleton whose sole purpose is to hold a "null instance"
	// of the subject Type: the canonical instance to dereference if this
	// LLSafeHandle actually holds a null pointer. We use LLSingleton
	// specifically so that the "null instance" can be cleaned up at a well-
	// defined time, specifically LLSingletonBase::deleteAll().
	// Of course, as with any LLSingleton, the "null instance" is only
	// instantiated on demand -- in this case, if you actually try to
	// dereference an LLSafeHandle containing null.
	class NullInstanceHolder: public LLSingleton<NullInstanceHolder>
	{
		LLSINGLETON_EMPTY_CTOR(NullInstanceHolder);
	public:
		Type mNullInstance;
	};

	static Type* nonNull(Type* ptr)
	{
		return ptr? ptr : &NullInstanceHolder::instance().mNullInstance;
	}

protected:
	Type*	mPointer;
};

#endif
