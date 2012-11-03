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
			mPredicateFlags = modifyPredicate(0x1 << (S32)e, cPredicateFlagsFromEnum[e], value, mPredicateFlags);
		}

		void set(const Value other, bool value)
		{
			predicate_flag_t predicate_flags_to_set = other.mPredicateFlags;
			predicate_flag_t cumulative_flags = 0;
			while(predicate_flags_to_set)
			{
				predicate_flag_t next_flags = clearLSB(predicate_flags_to_set);
				predicate_flag_t lsb_flag = predicate_flags_to_set ^ next_flags;

				predicate_flag_t mask = 0;
				predicate_flag_t cur_flags = mPredicateFlags;
				for (S32 i = 0; i < cMaxEnum; i++)
				{
					if (cPredicateFlagsFromEnum[i] & lsb_flag)
					{
						mask |= cPredicateFlagsFromEnum[i];
						cur_flags = modifyPredicate(0x1 << (0x1 << i ), cPredicateFlagsFromEnum[i], !value, cur_flags);
					}
				}

				cumulative_flags |= modifyPredicate(lsb_flag, mask, value, cur_flags);
				
				predicate_flags_to_set = next_flags;
			}
			mPredicateFlags = cumulative_flags;
		}

		void forget(const Value value)
		{
			set(value, true);
			U32 flags_with_predicate = mPredicateFlags;
			set(value, false);
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

		predicate_flag_t clearLSB(predicate_flag_t value)
		{
			return value & (value - 1);
		}

		predicate_flag_t modifyPredicate(predicate_flag_t predicate_flag, predicate_flag_t mask, predicate_flag_t value, bool set)
		{
			llassert(clearLSB(predicate_flag) == 0);
			predicate_flag_t flags_to_modify;

			if (set)
			{
				flags_to_modify = (mPredicateFlags & ~mask);
				// clear flags not containing predicate to be added
				value &= mask;
				// shift flags, in effect adding predicate
				flags_to_modify *= predicate_flag;
			}
			else
			{
				flags_to_modify = mPredicateFlags & mask;
				// clear flags containing predicate to be removed
				value &= ~mask;
				// shift flags, in effect removing predicate
				flags_to_modify /= predicate_flag;
			}
			// put modified flags back
			value |= flags_to_modify;
			return value;
		}

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

		void require(const Value<ENUM> value)
		{
			mRule.set(value, require);
		}

		void allow(const Value<ENUM> value)
		{
			mRule.forget(value);
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
