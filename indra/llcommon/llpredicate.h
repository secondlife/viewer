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

	template<typename ENUM>
	struct Value
	{
		friend Rule<ENUM>;
	public:
		Value(ENUM e)
		:	mPredicateFlags(0x1),
			mPredicateCombinationFlags(0x1)
		{
			set(e);
		}

		Value()
			:	mPredicateFlags(0x1),
			mPredicateCombinationFlags(0x1)
		{}

		void set(ENUM predicate)
		{
			llassert(predicate <= 5);
			int predicate_flag = 0x1 << (0x1 << (int)predicate);
			if (!(mPredicateFlags & predicate_flag))
			{
				mPredicateCombinationFlags *= predicate_flag;
				mPredicateFlags |= predicate_flag;
			}
		}

		bool get(ENUM predicate)
		{
			int predicate_flag = 0x1 << (0x1 << (int)predicate);
			return (mPredicateFlags & predicate_flag) != 0;
		}

		void clear(ENUM predicate)
		{
			llassert(predicate <= 5);
			int predicate_flag = 0x1 << (0x1 << (int)predicate);
			if (mPredicateFlags & predicate_flag)
			{
				mPredicateCombinationFlags /= predicate_flag;
				mPredicateFlags &= ~predicate_flag;
			}
		}

	private:
		int mPredicateCombinationFlags;
		int mPredicateFlags;
	};

	struct EmptyRule {};

	template<typename ENUM>
	class Rule
	{
	public:
		Rule(EmptyRule e)
			:	mPredicateRequirements(0x1)
		{}

		Rule(ENUM value)
			:	mPredicateRequirements(predicateFromValue(value))
		{}

		Rule()
			:	mPredicateRequirements(0x1)
		{}

		Rule operator~()
		{
			Rule new_rule;
			new_rule.mPredicateRequirements = ~mPredicateRequirements;
			return new_rule;
		}

		Rule operator &&(const Rule& other)
		{
			Rule new_rule;
			new_rule.mPredicateRequirements = mPredicateRequirements & other.mPredicateRequirements;
			return new_rule;
		}

		Rule operator ||(const Rule& other)
		{
			Rule new_rule;
			new_rule.mPredicateRequirements = mPredicateRequirements | other.mPredicateRequirements;
			return new_rule;
		}

		bool check(const Value<ENUM>& value) const
		{
			return ((value.mPredicateCombinationFlags | 0x1) & mPredicateRequirements) != 0;
		}

		static int predicateFromValue(ENUM value)
		{
			int countdown = value;
			bool bit_val = false;

			int predicate = 0x0;

			for (int bit_index = 0; bit_index < 32; bit_index++)
			{
				if (bit_val)
				{
					predicate |= 0x1 << bit_index;
				}

				if (countdown-- == 0)
				{
					countdown = value;
					bit_val = !bit_val;
				}
			}
			return predicate;
		}

		bool isTriviallyTrue() const
		{
			return mPredicateRequirements & 0x1;
		}

		bool isTriviallyFalse() const
		{
			return mPredicateRequirements == 0;
		}

	private:
		int mPredicateRequirements;
	};

	template<typename ENUM>
	Rule<ENUM> make_rule(ENUM e) { return Rule<ENUM>(e);}

	EmptyRule make_rule();

}
#endif // LL_LLPREDICATE_H
