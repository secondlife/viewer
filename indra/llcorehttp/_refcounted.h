/** 
 * @file _refcounted.h
 * @brief Atomic, thread-safe ref counting and destruction mixin class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LLCOREINT__REFCOUNTED_H_
#define LLCOREINT__REFCOUNTED_H_


#include "linden_common.h"

#include <boost/thread.hpp>

#include "llapr.h"


namespace LLCoreInt
{


class RefCounted
{
private:
	RefCounted();								// Not defined - may not be default constructed
	void operator=(const RefCounted &);			// Not defined
	
public:
	explicit RefCounted(bool const implicit)
		: mRefCount(implicit)
		{}

	// ref-count interface
	void addRef() const;
	void release() const;
	bool isLastRef() const;
	S32 getRefCount() const;
	void noRef() const;

	static const S32			NOT_REF_COUNTED = -1;
	
protected:
	virtual ~RefCounted();
	virtual void destroySelf();

private:
	mutable LLAtomicS32			mRefCount;

}; // end class RefCounted


inline void RefCounted::addRef() const
{
	S32 count(mRefCount++);
	llassert_always(count >= 0);
}


inline void RefCounted::release() const
{
	S32 count(mRefCount);
	llassert_always(count != NOT_REF_COUNTED);
	llassert_always(count > 0);
	count = mRefCount--;

	// clean ourselves up if that was the last reference
	if (0 == count)
	{
		const_cast<RefCounted *>(this)->destroySelf();
	}
}


inline bool RefCounted::isLastRef() const
{
	const S32 count(mRefCount);
	llassert_always(count != NOT_REF_COUNTED);
	llassert_always(count >= 1);
	return (1 == count);
}


inline S32 RefCounted::getRefCount() const
{
	const S32 result(mRefCount);
	return result;
}


inline void RefCounted::noRef() const
{
	llassert_always(mRefCount <= 1);
	mRefCount = NOT_REF_COUNTED;
}


inline void RefCounted::destroySelf()
{
	delete this;
}

} // end namespace LLCoreInt

#endif	// LLCOREINT__REFCOUNTED_H_

