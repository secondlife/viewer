/** 
 * @file llmemory.h
 * @brief Memory allocation/deallocation header-stuff goes here.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
// than when referencing an object via a LLHandle.
// 

template <class Type> 
class LLHandle
{
public:
	LLHandle() :
		mPointer(NULL)
	{
	}

	LLHandle(Type* ptr) : 
		mPointer(NULL)
	{
		assign(ptr);
	}

	LLHandle(const LLHandle<Type>& ptr) : 
		mPointer(NULL)
	{
		assign(ptr.mPointer);
	}

	// support conversion up the type hierarchy.  See Item 45 in Effective C++, 3rd Ed.
	template<typename Subclass>
	LLHandle(const LLHandle<Subclass>& ptr) : 
		mPointer(NULL)
	{
		assign(ptr.get());
	}

	~LLHandle()								
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
	bool operator ==(const LLHandle<Type>& ptr) const           { return (mPointer == ptr.mPointer); 	}
	bool operator < (const LLHandle<Type>& ptr) const           { return (mPointer < ptr.mPointer); 	}
	bool operator > (const LLHandle<Type>& ptr) const           { return (mPointer > ptr.mPointer); 	}

	LLHandle<Type>& operator =(Type* ptr)                   
	{ 
		assign(ptr);
		return *this; 
	}

	LLHandle<Type>& operator =(const LLHandle<Type>& ptr)  
	{ 
		assign(ptr.mPointer);
		return *this; 
	}

	// support assignment up the type hierarchy. See Item 45 in Effective C++, 3rd Ed.
	template<typename Subclass>
	LLHandle<Type>& operator =(const LLHandle<Subclass>& ptr)  
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

// LLSingleton implements the getInstance() method part of the Singleton pattern. It can't make
// the derived class constructors protected, though, so you have to do that yourself.
// The proper way to use LLSingleton is to inherit from it while using the typename that you'd 
// like to be static as the template parameter, like so:
//   class FooBar: public LLSingleton<FooBar>
// As currently written, it is not thread-safe.
template <typename T>
class LLSingleton
{
public:
	static T* getInstance()
	{
		static T instance;
		return &instance;
	}
};

//----------------------------------------------------------------------------

// Return the resident set size of the current process, in bytes.
// Return value is zero if not known.
U64 getCurrentRSS();

#endif
