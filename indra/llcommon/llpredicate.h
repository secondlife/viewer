/** 
 * @file llpredicate.h
 * @brief abstraction for filtering objects by predicates, with arbitrary boolean expressions
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LLPREDICATE_H
#define LL_LLPREDICATE_H

#include "llerror.h"

namespace LLPredicate
{
	template<typename ENUM> class Rule;

	extern const U32 cPredicateFlagsFromEnum[5];

	template<typename ENUM>
	class Value
	{
	public:
		typedef U32 predicate_flag_t;
		static const S32 cMaxEnum = 5;

		Value(ENUM e, bool predicate_value = true)
		:	mPredicateFlags(predicate_value ? cPredicateFlagsFromEnum[e] : ~cPredicateFlagsFromEnum[e])
		{
			llassert(0 <= e && e < cMaxEnum);
		}

		Value()
		:	mPredicateFlags(0xFFFFffff)
		{}

		Value operator!() const
		{
			Value new_value;
			new_value.mPredicateFlags = ~mPredicateFlags;
			return new_value;
		}

		Value operator &&(const Value other) const
		{
			Value new_value;
			new_value.mPredicateFlags = mPredicateFlags & other.mPredicateFlags;
			return new_value;
		}

		Value operator ||(const Value other) const
		{
			Value new_value;
			new_value.mPredicateFlags = mPredicateFlags | other.mPredicateFlags;
			return new_value;
		}

		void set(ENUM e, bool value = true)
		{
			llassert(0 <= e && e < cMaxEnum);
			predicate_flag_t flags_to_modify;
			predicate_flag_t mask = cPredicateFlagsFromEnum[e];
			if (value)
			{	// add predicate "e" to flags that don't contain it already
				flags_to_modify = (mPredicateFlags & ~mask);
				// clear flags not containing e
				mPredicateFlags &= mask;
				// add back flags shifted to contain e
				mPredicateFlags |= flags_to_modify << (0x1 << e);
			}
			else
			{	// remove predicate "e" from flags that contain it
				flags_to_modify = (mPredicateFlags & mask);
				// clear flags containing e
				mPredicateFlags &= ~mask;
				// add back flags shifted to not contain e
				mPredicateFlags |= flags_to_modify >> (0x1 << e);
			}
		}

		void forget(ENUM e)
		{
			set(e, true);
			U32 flags_with_predicate = mPredicateFlags;
			set(e, false);
			// ambiguous value is result of adding and removing predicate at the same time!
			mPredicateFlags |= flags_with_predicate;
		}

		bool allSet() const
		{
			return mPredicateFlags == ~0;
		}

		bool noneSet() const
		{
			return mPredicateFlags == 0;
		}

		bool someSet() const
		{
			return mPredicateFlags != 0;
		}

	private:
		predicate_flag_t mPredicateFlags;
	};

	template<typename ENUM>
	class Rule
	{
	public:
		Rule(ENUM value)
		:	mRule(value)
		{}

		Rule(const Value<ENUM> other)
		:	mRule(other)
		{}

		Rule()
		{}

		void require(ENUM e, bool match)
		{
			mRule.set(e, match);
		}

		void allow(ENUM e)
		{
			mRule.forget(e);
		}

		bool check(const Value<ENUM> value) const
		{
			return (mRule && value).someSet();
		}

		bool requires(const Value<ENUM> value) const
		{
			return (mRule && value).someSet() && (!mRule && value).noneSet();
		}

		bool isAmbivalent(const Value<ENUM> value) const
		{
			return (mRule && value).someSet() && (!mRule && value).someSet();
		}

		bool acceptsAll() const
		{
			return mRule.allSet();
		}

		bool acceptsNone() const
		{
			return mRule.noneSet();
		}

		Rule operator!() const
		{
			Rule new_rule;
			new_rule.mRule = !mRule;
			return new_rule;
		}

		Rule operator &&(const Rule other) const
		{
			Rule new_rule;
			new_rule.mRule = mRule && other.mRule;
			return new_rule;
		}

		Rule operator ||(const Rule other) const
		{
			Rule new_rule;
			new_rule.mRule = mRule || other.mRule;
			return new_rule;
		}

	private:
		Value<ENUM> mRule;
	};
}

template<typename ENUM>
LLPredicate::Value<ENUM> ll_make_predicate(ENUM e, bool predicate_value = true)
{
	 return LLPredicate::Value<ENUM>(e, predicate_value);
}


#endif // LL_LLPREDICATE_H
