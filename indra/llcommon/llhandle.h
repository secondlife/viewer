/** 
* @file llhandle.h
* @brief "Handle" to an object (usually a floater) whose lifetime you don't
* control.
*
* $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LLHANDLE_H
#define LLHANDLE_H

#include "llpointer.h"
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

/**
 * Helper object for LLHandle. Don't instantiate these directly, used
 * exclusively by LLHandle.
 */
class LLTombStone : public LLRefCount
{
public:
	LLTombStone(void* target = NULL) : mTarget(target) {}

	void setTarget(void* target) { mTarget = target; }
	void* getTarget() const { return mTarget; }
private:
	mutable void* mTarget;
};

/**
 *	LLHandles are used to refer to objects whose lifetime you do not control or influence.  
 *	Calling get() on a handle will return a pointer to the referenced object or NULL, 
 *	if the object no longer exists.  Note that during the lifetime of the returned pointer, 
 *	you are assuming that the object will not be deleted by any action you perform, 
 *	or any other thread, as normal when using pointers, so avoid using that pointer outside of
 *	the local code block.
 * 
 *  https://wiki.lindenlab.com/mediawiki/index.php?title=LLHandle&oldid=79669
 *
 * The implementation is like some "weak pointer" implementations. When we
 * can't control the lifespan of the referenced object of interest, we can
 * still instantiate a proxy object whose lifespan we DO control, and store in
 * the proxy object a dumb pointer to the actual target. Then we just have to
 * ensure that on destruction of the target object, the proxy's dumb pointer
 * is set NULL.
 *
 * LLTombStone is our proxy object. LLHandle contains an LLPointer to the
 * LLTombStone, so every copy of an LLHandle increments the LLTombStone's ref
 * count as usual.
 *
 * One copy of the LLHandle, specifically the LLRootHandle, must be stored in
 * the referenced object. Destroying the LLRootHandle is what NULLs the
 * proxy's target pointer.
 *
 * Minor optimization: we want LLHandle's mTombStone to always be a valid
 * LLPointer, saving some conditionals in dereferencing. That's the
 * getDefaultTombStone() mechanism. The default LLTombStone object's target
 * pointer is always NULL, so it's semantically identical to allowing
 * mTombStone to be invalid.
 */
template <typename T>
class LLHandle
{
	template <typename U> friend class LLHandle;
	template <typename U> friend class LLHandleProvider;
public:
	LLHandle() : mTombStone(getDefaultTombStone()) {}

	template<typename U>
	LLHandle(const LLHandle<U>& other, typename boost::enable_if< typename boost::is_convertible<U*, T*> >::type* dummy = 0)
	: mTombStone(other.mTombStone)
	{}

	bool isDead() const 
	{ 
		return mTombStone->getTarget() == NULL; 
	}

	void markDead() 
	{ 
		mTombStone = getDefaultTombStone();
	}

	T* get() const
	{
		return reinterpret_cast<T*>(mTombStone->getTarget());
	}

	friend bool operator== (const LLHandle<T>& lhs, const LLHandle<T>& rhs)
	{
		return lhs.mTombStone == rhs.mTombStone;
	}
	friend bool operator!= (const LLHandle<T>& lhs, const LLHandle<T>& rhs)
	{
		return !(lhs == rhs);
	}
	friend bool	operator< (const LLHandle<T>& lhs, const LLHandle<T>& rhs)
	{
		return lhs.mTombStone < rhs.mTombStone;
	}
	friend bool	operator> (const LLHandle<T>& lhs, const LLHandle<T>& rhs)
	{
		return lhs.mTombStone > rhs.mTombStone;
	}

protected:
	LLPointer<LLTombStone> mTombStone;

private:
	typedef T* pointer_t;
	static LLPointer<LLTombStone>& getDefaultTombStone()
	{
		static LLPointer<LLTombStone> sDefaultTombStone = new LLTombStone;
		return sDefaultTombStone;
	}
};

/**
 * LLRootHandle isa LLHandle which must be stored in the referenced object.
 * You can either store it directly and explicitly bind(this), or derive from
 * LLHandleProvider (q.v.) which automates that for you. The essential point
 * is that destroying the LLRootHandle (as a consequence of destroying the
 * referenced object) calls unbind(), setting the LLTombStone's target pointer
 * NULL.
 */
template <typename T>
class LLRootHandle : public LLHandle<T>
{
public:
	typedef LLRootHandle<T> self_t;
	typedef LLHandle<T> base_t;

	LLRootHandle(T* object) { bind(object); }
	LLRootHandle() {};
	~LLRootHandle() { unbind(); }

	// this is redundant, since an LLRootHandle *is* an LLHandle
	//LLHandle<T> getHandle() { return LLHandle<T>(*this); }

	void bind(T* object) 
	{ 
		// unbind existing tombstone
		if (LLHandle<T>::mTombStone.notNull())
		{
			if (LLHandle<T>::mTombStone->getTarget() == (void*)object) return;
			LLHandle<T>::mTombStone->setTarget(NULL);
		}
		// tombstone reference counted, so no paired delete
		LLHandle<T>::mTombStone = new LLTombStone((void*)object);
	}

	void unbind() 
	{
		LLHandle<T>::mTombStone->setTarget(NULL);
	}

	//don't allow copying of root handles, since there should only be one
private:
	LLRootHandle(const LLRootHandle& other) {};
};

/**
 * Use this as a mixin for simple classes that need handles and when you don't
 * want handles at multiple points of the inheritance hierarchy
 */
template <typename T>
class LLHandleProvider
{
public:
	LLHandle<T> getHandle() const
	{ 
		// perform lazy binding to avoid small tombstone allocations for handle
		// providers whose handles are never referenced
		mHandle.bind(static_cast<T*>(const_cast<LLHandleProvider<T>* >(this))); 
		return mHandle; 
	}

protected:
	typedef LLHandle<T> handle_type_t;
	LLHandleProvider() 
	{
		// provided here to enforce T deriving from LLHandleProvider<T>
	} 

	template <typename U>
	LLHandle<U> getDerivedHandle(typename boost::enable_if< typename boost::is_convertible<U*, T*> >::type* dummy = 0) const
	{
		LLHandle<U> downcast_handle;
		downcast_handle.mTombStone = getHandle().mTombStone;
		return downcast_handle;
	}


private:
	mutable LLRootHandle<T> mHandle;
};

#endif
