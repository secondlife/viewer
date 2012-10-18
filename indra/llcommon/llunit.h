/** 
 * @file llunit.h
 * @brief Unit conversion classes
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLUNIT_H
#define LL_LLUNIT_H

#include "stdtypes.h"
#include "llpreprocessor.h"

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT = BASE_UNIT>
struct LLUnitType : public BASE_UNIT
{
	typedef DERIVED_UNIT unit_t;

	typedef typename STORAGE_TYPE storage_t;
	typedef void is_unit_tag_t;

	LLUnitType()
	{}

	explicit LLUnitType(storage_t value)
	:	BASE_UNIT(convertToBase(value))
	{}

	// implicit downcast
	operator unit_t& ()
	{
		return static_cast<unit_t&>(*this);
	}

	storage_t value() const
	{
		return convertToDerived(mBaseValue);
	}

	template<typename CONVERTED_TYPE>
	storage_t as() const
	{
		return CONVERTED_TYPE(*this).value();
	}

protected:
	static storage_t convertToBase(storage_t derived_value)
	{
		return (storage_t)((F32)derived_value * unit_t::conversionToBaseFactor());
	}

	static storage_t convertToDerived(storage_t base_value)
	{
		return (storage_t)((F32)base_value / unit_t::conversionToBaseFactor());
	}

};

template<typename STORAGE_TYPE, typename T>
struct LLUnitType<STORAGE_TYPE, T, T>
{
	typedef T unit_t;
	typedef STORAGE_TYPE storage_t;
	typedef void is_unit_tag_t;

	LLUnitType()
	:	mBaseValue()
	{}

	explicit LLUnitType(storage_t value)
	:	mBaseValue(value)
	{}

	unit_t& operator=(storage_t value)
	{
		setBaseValue(value);
		return *this;
	}

	//implicit downcast
	operator unit_t& ()
	{
		return static_cast<unit_t&>(*this);
	}

	storage_t value() const { return mBaseValue; }

	template<typename CONVERTED_TYPE>
	storage_t as() const
	{
		return CONVERTED_TYPE(*this).value();
	}


	static storage_t convertToBase(storage_t derived_value)
	{
		return (storage_t)derived_value;
	}

	static storage_t convertToDerived(storage_t base_value)
	{
		return (storage_t)base_value;
	}

	void operator += (const unit_t other)
	{
		mBaseValue += other.mBaseValue;
	}

	void operator -= (const unit_t other)
	{
		mBaseValue -= other.mBaseValue;
	}

	void operator *= (storage_t multiplicand)
	{
		mBaseValue *= multiplicand;
	}

	void operator /= (storage_t divisor)
	{
		mBaseValue /= divisor;
	}

protected:
	void setBaseValue(storage_t value)
	{
		mBaseValue = value;
	}

	storage_t mBaseValue;
};

//
// operator +
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
DERIVED_UNIT operator + (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return DERIVED_UNIT(first + second.value());
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
DERIVED_UNIT operator + (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return DERIVED_UNIT(first.value() + second);
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT, typename OTHER_DERIVED_UNIT>
DERIVED_UNIT operator + (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, LLUnitType<STORAGE_TYPE, BASE_UNIT, OTHER_DERIVED_UNIT> second)
{
	return DERIVED_UNIT(first.value() + second.value());
}

//
// operator -
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
DERIVED_UNIT operator - (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return DERIVED_UNIT(first - second.value());
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
DERIVED_UNIT operator - (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return DERIVED_UNIT(first.value() - second);
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT, typename OTHER_DERIVED_UNIT>
DERIVED_UNIT operator - (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, LLUnitType<STORAGE_TYPE, BASE_UNIT, OTHER_DERIVED_UNIT> second)
{
	return DERIVED_UNIT(first.value() - second.value());
}

//
// operator *
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
DERIVED_UNIT operator * (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return DERIVED_UNIT(first * second.value());
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
DERIVED_UNIT operator * (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return DERIVED_UNIT(first.value() * second);
}

//
// operator /
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
DERIVED_UNIT operator / (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return DERIVED_UNIT(first * second.value());
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
DERIVED_UNIT operator / (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return DERIVED_UNIT(first.value() * second);
}

//
// operator <
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator < (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return first < second.value();
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator < (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return first.value() < second;
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT, typename OTHER_DERIVED_UNIT>
bool operator < (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, LLUnitType<STORAGE_TYPE, BASE_UNIT, OTHER_DERIVED_UNIT> second)
{
	return first.value() < second.value();
}

//
// operator <=
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator <= (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return first <= second.value();
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator <= (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return first.value() <= second;
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT, typename OTHER_DERIVED_UNIT>
bool operator <= (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, LLUnitType<STORAGE_TYPE, BASE_UNIT, OTHER_DERIVED_UNIT> second)
{
	return first.value() <= second.value();
}

//
// operator >
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator > (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return first > second.value();
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator > (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return first.value() > second;
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT, typename OTHER_DERIVED_UNIT>
bool operator > (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, LLUnitType<STORAGE_TYPE, BASE_UNIT, OTHER_DERIVED_UNIT> second)
{
	return first.value() > second.value();
}
//
// operator >=
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator >= (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return first >= second.value();
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator >= (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return first.value() >= second;
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT, typename OTHER_DERIVED_UNIT>
bool operator >= (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, LLUnitType<STORAGE_TYPE, BASE_UNIT, OTHER_DERIVED_UNIT> second)
{
	return first.value() >= second.value();
}

//
// operator ==
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator == (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return first == second.value();
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator == (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return first.value() == second;
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT, typename OTHER_DERIVED_UNIT>
bool operator == (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, LLUnitType<STORAGE_TYPE, BASE_UNIT, OTHER_DERIVED_UNIT> second)
{
	return first.value() == second.value();
}

//
// operator !=
//
template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator != (typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t first, LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> second)
{
	return first != second.value();
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT>
bool operator != (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, typename LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT>::storage_t second)
{
	return first.value() != second;
}

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT, typename OTHER_DERIVED_UNIT>
bool operator != (LLUnitType<STORAGE_TYPE, BASE_UNIT, DERIVED_UNIT> first, LLUnitType<STORAGE_TYPE, BASE_UNIT, OTHER_DERIVED_UNIT> second)
{
	return first.value() != second.value();
}

#define LL_DECLARE_BASE_UNIT(unit_name)                                                                          \
	template<typename STORAGE>                                                                                   \
	struct unit_name : public LLUnitType<STORAGE, unit_name<STORAGE>, unit_name<STORAGE> >						 \
	{                                                                                                            \
		typedef LLUnitType<STORAGE, unit_name> unit_t;						                                     \
	                                                                                                             \
		unit_name(storage_t value = 0)                                                                           \
		:	LLUnitType(value)                                                                                    \
		{}                                                                                                       \
		                                                                                                         \
		template <typename SOURCE_STORAGE_TYPE, typename SOURCE_TYPE>                                            \
		unit_name(LLUnitType<SOURCE_STORAGE_TYPE, unit_name<SOURCE_STORAGE_TYPE>, SOURCE_TYPE> source)			 \
		{                                                                                                        \
			assignFrom(source);			                                                                         \
		}                                                                                                        \
		                                                                                                         \
		template <typename SOURCE_STORAGE_TYPE, typename SOURCE_TYPE>                                            \
		void assignFrom(LLUnitType<SOURCE_STORAGE_TYPE, unit_name<SOURCE_STORAGE_TYPE>, SOURCE_TYPE> source)     \
		{                                                                                                        \
			setBaseValue((storage_t)source.unit_name<SOURCE_STORAGE_TYPE>::unit_t::value());                     \
		}                                                                                                        \
	                                                                                                             \
	};                                                                                                           \

#define LL_DECLARE_DERIVED_UNIT(base_unit, derived_unit, conversion_factor)                                      \
	template<typename STORAGE>	                                                                                 \
	struct derived_unit : public LLUnitType<STORAGE, base_unit<STORAGE>, derived_unit<STORAGE> >                 \
	{                                                                                                            \
		typedef LLUnitType<STORAGE, base_unit<STORAGE>, derived_unit<STORAGE> > unit_t;				             \
		                                                                                                         \
		derived_unit(storage_t value = 0)                                                                        \
		:	LLUnitType(value)                                                                                    \
		{}                                                                                                       \
		                                                                                                         \
		template <typename SOURCE_STORAGE_TYPE, typename SOURCE_TYPE>                                            \
		derived_unit(LLUnitType<SOURCE_STORAGE_TYPE, base_unit<SOURCE_STORAGE_TYPE>, SOURCE_TYPE> source)		 \
		{                                                                                                        \
			assignFrom(source);					                                                                 \
		}                                                                                                        \
		                                                                                                         \
		template <typename SOURCE_STORAGE_TYPE, typename SOURCE_TYPE>                                            \
		void assignFrom(LLUnitType<SOURCE_STORAGE_TYPE, base_unit<SOURCE_STORAGE_TYPE>, SOURCE_TYPE> source)     \
		{                                                                                                        \
			setBaseValue((storage_t)source.base_unit<SOURCE_STORAGE_TYPE>::unit_t::value());                     \
		}                                                                                                        \
		                                                                                                         \
		static F32 conversionToBaseFactor() { return (F32)(conversion_factor); }                                 \
		                                                                                                         \
	};                                                                                                           \

namespace LLUnit
{
	LL_DECLARE_BASE_UNIT(Bytes);
	LL_DECLARE_DERIVED_UNIT(Bytes, Kilobytes, 1024);
	LL_DECLARE_DERIVED_UNIT(Bytes, Megabytes, 1024 * 1024);
	LL_DECLARE_DERIVED_UNIT(Bytes, Gigabytes, 1024 * 1024 * 1024);
	LL_DECLARE_DERIVED_UNIT(Bytes, Bits,	  (1.f / 8.f));
	LL_DECLARE_DERIVED_UNIT(Bytes, Kilobits,  (1024 / 8));
	LL_DECLARE_DERIVED_UNIT(Bytes, Megabits,  (1024 / 8));
	LL_DECLARE_DERIVED_UNIT(Bytes, Gigabits,  (1024 * 1024 * 1024 / 8));

	LL_DECLARE_BASE_UNIT(Seconds);
	LL_DECLARE_DERIVED_UNIT(Seconds, Minutes,		60);
	LL_DECLARE_DERIVED_UNIT(Seconds, Hours,			60 * 60);
	LL_DECLARE_DERIVED_UNIT(Seconds, Days,			60 * 60 * 24);
	LL_DECLARE_DERIVED_UNIT(Seconds, Weeks,			60 * 60 * 24 * 7);
	LL_DECLARE_DERIVED_UNIT(Seconds, Milliseconds,	(1.f / 1000.f));
	LL_DECLARE_DERIVED_UNIT(Seconds, Microseconds,	(1.f / (1000000.f)));
	LL_DECLARE_DERIVED_UNIT(Seconds, Nanoseconds,	(1.f / (1000000000.f)));

	LL_DECLARE_BASE_UNIT(Meters);
	LL_DECLARE_DERIVED_UNIT(Meters, Kilometers, 1000);
	LL_DECLARE_DERIVED_UNIT(Meters, Centimeters, 1 / 100);
	LL_DECLARE_DERIVED_UNIT(Meters, Millimeters, 1 / 1000);
}

#endif // LL_LLUNIT_H
