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
	class Literal
	{
		friend Rule<ENUM>;

	public:
		typedef U32 predicate_flag_t;
		static const S32 cMaxEnum = 5;

		Literal(ENUM e)
		:	mPredicateFlags(cPredicateFlagsFromEnum[e])
		{
			llassert(0 <= e && e < cMaxEnum);
		}

		Literal()
		:	mPredicateFlags(0xFFFFffff)
		{}

		Literal operator~()
		{
			Literal new_rule;
			new_rule.mPredicateFlags = ~mPredicateFlags;
			return new_rule;
		}

		Literal operator &&(const Literal& other)
		{
			Literal new_rule;
			new_rule.mPredicateFlags = mPredicateFlags & other.mPredicateFlags;
			return new_rule;
		}

		Literal operator ||(const Literal& other)
		{
			Literal new_rule;
			new_rule.mPredicateFlags = mPredicateFlags | other.mPredicateFlags;
			return new_rule;
		}

		void set(ENUM e, bool value)
		{
			llassert(0 <= e && e < cMaxEnum);
			modifyPredicate(0x1 << (S32)e, cPredicateFlagsFromEnum[e], value);
		}

		void set(const Literal& other, bool value)
		{
			U32 predicate_flags = other.mPredicateFlags;
			while(predicate_flags)
			{
				U32 next_flags = clearLSB(predicate_flags);
				lsb_flag = predicate_flags ^ next_flags;
				U32 mask = 0;
				for (S32 i = 0; i < cMaxEnum; i++)
				{
					if (cPredicateFlagsFromEnum[i] & lsb_flag)
					{
						mask |= cPredicateFlagsFromEnum[i];
					}
				}

				modifyPredicate(lsb_flag, mask, value);
				
				predicate_flags = next_flags;
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

		void forget(const Literal& literal)
		{
			set(literal, true);
			U32 flags_with_predicate = mPredicateFlags;
			set(literal, false);
			// ambiguous value is result of adding and removing predicate at the same time!
			mPredicateFlags |= flags_with_predicate;
		}

	private:

		predicate_flag_t clearLSB(predicate_flag_t value)
		{
			return value & (value - 1);
		}

		void modifyPredicate(predicate_flag_t predicate_flag, predicate_flag_t mask, bool value)
		{
			llassert(clearLSB(predicate_flag) == 0);
			predicate_flag_t flags_to_modify;

			if (value)
			{
				flags_to_modify = (mPredicateFlags & ~mask);
				// clear flags not containing predicate to be added
				mPredicateFlags &= mask;
				// shift flags, in effect adding predicate
				flags_to_modify *= predicate_flag;
			}
			else
			{
				flags_to_modify = mPredicateFlags & mask;
				// clear flags containing predicate to be removed
				mPredicateFlags &= ~mask;
				// shift flags, in effect removing predicate
				flags_to_modify /= predicate_flag;
			}
			// put modified flags back
			mPredicateFlags |= flags_to_modify;
		}

		predicate_flag_t mPredicateFlags;
	};

	template<typename ENUM>
	struct Value
	:	public Literal<ENUM>
	{
	public:
		Value(ENUM e)
		:	Literal(e)
		{}

		Value()
		{}
	};

	template<typename ENUM>
	class Rule
	:	public Literal<ENUM>
	{
	public:
		Rule(ENUM value)
		:	Literal(value)
		{}

		Rule(const Literal other)
		:	Literal(other)
		{}

		Rule()
		{}

		bool check(const Value<ENUM>& value) const
		{
			return (value.mPredicateFlags & mPredicateFlags) != 0;
		}

		bool isTriviallyTrue() const
		{
			return mPredicateFlags == 0xFFFFffff;
		}

		bool isTriviallyFalse() const
		{
			return mPredicateFlags == 0;
		}
	};
}

template<typename ENUM>
LLPredicate::Literal<ENUM> ll_predicate(ENUM e)
{
	 return LLPredicate::Literal<ENUM>(e);
}


#endif // LL_LLPREDICATE_H
