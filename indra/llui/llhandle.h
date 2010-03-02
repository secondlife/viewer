/** 
* @file llhandle.h
* @brief "Handle" to an object (usually a floater) whose lifetime you don't
* control.
*
* $LicenseInfo:firstyear=2001&license=viewergpl$
* 
* Copyright (c) 2001-2009, Linden Research, Inc.
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

	template<typename Subclass>
	LLHandle<T>& operator =(const LLHandle<Subclass>& other)  
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
