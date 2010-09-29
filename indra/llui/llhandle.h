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

template <typename T>
class LLTombStone : public LLRefCount
{
public:
	LLTombStone(T* target = NULL) : mTarget(target) {}

	void setTarget(T* target) { mTarget = target; }
	T* getTarget() const { return mTarget; }
private:
	T* mTarget;
};

//	LLHandles are used to refer to objects whose lifetime you do not control or influence.  
//	Calling get() on a handle will return a pointer to the referenced object or NULL, 
//	if the object no longer exists.  Note that during the lifetime of the returned pointer, 
//	you are assuming that the object will not be deleted by any action you perform, 
//	or any other thread, as normal when using pointers, so avoid using that pointer outside of
//	the local code block.
// 
//  https://wiki.lindenlab.com/mediawiki/index.php?title=LLHandle&oldid=79669

template <typename T>
class LLHandle
{
public:
	LLHandle() : mTombStone(getDefaultTombStone()) {}
	const LLHandle<T>& operator =(const LLHandle<T>& other)  
	{ 
		mTombStone = other.mTombStone;
		return *this; 
	}

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
		return mTombStone->getTarget();
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
	LLPointer<LLTombStone<T> > mTombStone;

private:
	static LLPointer<LLTombStone<T> >& getDefaultTombStone()
	{
		static LLPointer<LLTombStone<T> > sDefaultTombStone = new LLTombStone<T>;
		return sDefaultTombStone;
	}
};

template <typename T>
class LLRootHandle : public LLHandle<T>
{
public:
	LLRootHandle(T* object) { bind(object); }
	LLRootHandle() {};
	~LLRootHandle() { unbind(); }

	// this is redundant, since a LLRootHandle *is* an LLHandle
	LLHandle<T> getHandle() { return LLHandle<T>(*this); }

	void bind(T* object) 
	{ 
		// unbind existing tombstone
		if (LLHandle<T>::mTombStone.notNull())
		{
			if (LLHandle<T>::mTombStone->getTarget() == object) return;
			LLHandle<T>::mTombStone->setTarget(NULL);
		}
		// tombstone reference counted, so no paired delete
		LLHandle<T>::mTombStone = new LLTombStone<T>(object);
	}

	void unbind() 
	{
		LLHandle<T>::mTombStone->setTarget(NULL);
	}

	//don't allow copying of root handles, since there should only be one
private:
	LLRootHandle(const LLRootHandle& other) {};
};

// Use this as a mixin for simple classes that need handles and when you don't
// want handles at multiple points of the inheritance hierarchy
template <typename T>
class LLHandleProvider
{
protected:
	typedef LLHandle<T> handle_type_t;
	LLHandleProvider() 
	{
		// provided here to enforce T deriving from LLHandleProvider<T>
	} 

	LLHandle<T> getHandle() 
	{ 
		// perform lazy binding to avoid small tombstone allocations for handle
		// providers whose handles are never referenced
		mHandle.bind(static_cast<T*>(this)); 
		return mHandle; 
	}

private:
	LLRootHandle<T> mHandle;
};

#endif
