/** 
 * @file llmemory.h
 * @brief Memory allocation/deallocation header-stuff goes here.
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
#ifndef LL_MEMORY_H
#define LL_MEMORY_H

#include <new>
#include <cstdlib>

#include "llerror.h"

extern S32 gTotalDAlloc;
extern S32 gTotalDAUse;
extern S32 gDACount;

const U32 LLREFCOUNT_SENTINEL_VALUE = 0xAAAAAAAA;

//----------------------------------------------------------------------------

class LLMemory
{
public:
	static void initClass();
	static void cleanupClass();
	static void freeReserve();
private:
	static char* reserveMem;
};

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

class LLRefCount
{
protected:
	LLRefCount(const LLRefCount&); // not implemented
private:
	LLRefCount&operator=(const LLRefCount&); // not implemented

protected:
	virtual ~LLRefCount(); // use unref()
	
public:
	LLRefCount();

	void ref()
	{ 
		mRef++; 
	} 

	S32 unref()
	{
		llassert(mRef >= 1);
		if (0 == --mRef) 
		{
			delete this; 
			return 0;
		}
		return mRef;
	}	

	S32 getNumRefs() const
	{
		return mRef;
	}

private: 
	S32	mRef; 
};

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
	operator const Type*() const				{ return mPointer; }
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

//template <class Type> 
//class LLPointerTraits
//{
//	static Type* null();
//};
//
// Expands LLPointer to return a pointer to a special instance of class Type instead of NULL.
// This is useful in instances where operations on NULL pointers are semantically safe and/or
// when error checking occurs at a different granularity or in a different part of the code
// than when referencing an object via a LLSafeHandle.
// 

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

// LLInitializedPointer is just a pointer with a default constructor that initializes it to NULL
// NOT a smart pointer like LLPointer<>
// Useful for example in std::map<int,LLInitializedPointer<LLFoo> >
//  (std::map uses the default constructor for creating new entries)
template <typename T> class LLInitializedPointer
{
public:
	LLInitializedPointer() : mPointer(NULL) {}
	~LLInitializedPointer() { delete mPointer; }
	
	const T* operator->() const { return mPointer; }
	T* operator->() 			{ return mPointer; }
	const T& operator*() const 	{ return *mPointer; }
	T& operator*() 				{ return *mPointer; }
	operator const T*() const	{ return mPointer; }
	operator T*()				{ return mPointer; }
	T* operator=(T* x)				{ return (mPointer = x); }
	operator bool() const		{ return mPointer != NULL; }
	bool operator!() const		{ return mPointer == NULL; }
	bool operator==(T* rhs)		{ return mPointer == rhs; }
	bool operator==(const LLInitializedPointer<T>* rhs) { return mPointer == rhs.mPointer; }

protected:
	T* mPointer;
};	

//----------------------------------------------------------------------------

// LLSingleton implements the getInstance() method part of the Singleton
// pattern. It can't make the derived class constructors protected, though, so
// you have to do that yourself.
//
// There are two ways to use LLSingleton. The first way is to inherit from it
// while using the typename that you'd like to be static as the template
// parameter, like so:
//
//   class Foo: public LLSingleton<Foo>{};
//
//   Foo& instance = Foo::instance();
//
// The second way is to use the singleton class directly, without inheritance:
//
//   typedef LLSingleton<Foo> FooSingleton;
//
//   Foo& instance = FooSingleton::instance();
//
// In this case, the class being managed as a singleton needs to provide an
// initSingleton() method since the LLSingleton virtual method won't be
// available
//
// As currently written, it is not thread-safe.
template <typename T>
class LLSingleton
{
public:
	virtual ~LLSingleton() {}
#ifdef  LL_MSVC7
// workaround for VC7 compiler bug
// adapted from http://www.codeproject.com/KB/tips/VC2003MeyersSingletonBug.aspx
// our version doesn't introduce a nested struct so that you can still declare LLSingleton<MyClass>
// a friend and hide your constructor
	static T* getInstance()
    {
        LLSingleton<T> singleton;
        return singleton.vsHack();
    }

	T* vsHack()
#else
	static T* getInstance()
#endif
	{
		static T instance;
		static bool needs_init = true;
		if (needs_init)
		{
			needs_init = false;
			instance.initSingleton();
		}
		return &instance;
	}

	static T& instance()
	{
		return *getInstance();
	}

private:
	virtual void initSingleton() {}
};

//----------------------------------------------------------------------------

// Return the resident set size of the current process, in bytes.
// Return value is zero if not known.
U64 getCurrentRSS();

#endif
