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

	S32 predicateFlagsFromValue(S32 value);

	template<typename ENUM>
	struct Value
	{
		friend Rule<ENUM>;
	public:
		Value(ENUM e)
		:	mPredicateCombinationFlags(0xFFFFffff)
		{
			add(e);
		}

		Value()
		:	mPredicateCombinationFlags(0xFFFFffff)
		{}

		void add(ENUM predicate)
		{
			llassert(predicate < 5);
			S32 predicate_shift = 0x1 << (S32)predicate;
			S32 flag_mask = predicateFlagsFromValue(predicate);
			S32 flags_to_modify = mPredicateCombinationFlags & ~flag_mask;
			// clear flags containing predicate to be removed
			mPredicateCombinationFlags &= ~flag_mask;
			// shift flags, in effect removing predicate
			flags_to_modify <<= predicate_shift;
			// put modified flags back
			mPredicateCombinationFlags |= flags_to_modify;
		}

		void remove(ENUM predicate)
		{
			llassert(predicate < 5);
			S32 predicate_shift = 0x1 << (S32)predicate;
			S32 flag_mask = predicateFlagsFromValue(predicate);
			S32 flags_to_modify = mPredicateCombinationFlags & flag_mask;
			// clear flags containing predicate to be removed
			mPredicateCombinationFlags &= ~flag_mask;
			// shift flags, in effect removing predicate
			flags_to_modify >>= predicate_shift;
			// put modified flags back
			mPredicateCombinationFlags |= flags_to_modify;
		}

		void unknown(ENUM predicate)
		{
			add(predicate);
			S32 flags_with_predicate = mPredicateCombinationFlags;
			remove(predicate);
			// unknown is result of adding and removing predicate at the same time!
			mPredicateCombinationFlags |= flags_with_predicate;
		}

		bool has(ENUM predicate)
		{
			S32 flag_mask = predicateFlagsFromValue(predicate);
			return (mPredicateCombinationFlags & flag_mask) != 0;
		}

	private:
		S32 mPredicateCombinationFlags;
	};

	struct EmptyRule {};

	template<typename ENUM>
	class Rule
	{
	public:
		Rule(ENUM value)
		:	mPredicateRequirements(predicateFlagsFromValue(value))
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

		bool isTriviallyTrue() const
		{
			return mPredicateRequirements & 0x1;
		}

		bool isTriviallyFalse() const
		{
			return mPredicateRequirements == 0;
		}

	private:
		S32 mPredicateRequirements;
	};

	template<typename ENUM>
	Rule<ENUM> make_rule(ENUM e) { return Rule<ENUM>(e);}

	// return generic empty rule class to avoid requiring template argument to create an empty rule
	EmptyRule make_rule();

}
#endif // LL_LLPREDICATE_H
