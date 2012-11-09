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

namespace LLUnits
{
template<typename DERIVED_UNITS_TAG, typename BASE_UNITS_TAG>
struct ConversionFactor
{
	static F64 get()
	{
		llstatic_assert(sizeof(DERIVED_UNITS_TAG) == 0, "Cannot convert between types.");
	}
};

template<typename BASE_UNITS_TAG>
struct ConversionFactor<BASE_UNITS_TAG, BASE_UNITS_TAG>
{
	static F64 get() { return 1.0; }
};
}

template<typename UNIT_TYPE, typename STORAGE_TYPE>
struct LLUnit
{
	typedef LLUnit<UNIT_TYPE, STORAGE_TYPE> self_t;
	typedef typename STORAGE_TYPE storage_t;
	typedef void is_unit_tag_t;

	LLUnit(storage_t value = storage_t())
	:	mValue(value)
	{}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	LLUnit(LLUnit<OTHER_UNIT, OTHER_STORAGE> other)
	:	mValue(convert(other))
	{}

	LLUnit(self_t& other)
	:	mValue(other.mValue)
	{}

	self_t& operator = (storage_t value)
	{
		mValue = value;
		return *this;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	self_t& operator = (LLUnit<OTHER_UNIT, OTHER_STORAGE> other)
	{
		mValue = convert(other);
		return *this;
	}

	operator storage_t() const
	{
		return value();
	}

	storage_t value() const
	{
		return mValue;
	}

	void operator += (storage_t value)
	{
		mValue += value;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	void operator += (LLUnit<OTHER_UNIT, OTHER_STORAGE> other)
	{
		mValue += convert(other);
	}

	void operator -= (storage_t value)
	{
		mValue -= value;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	void operator -= (LLUnit<OTHER_UNIT, OTHER_STORAGE> other)
	{
		mValue -= convert(other);
	}

	void operator *= (storage_t multiplicand)
	{
		mValue *= multiplicand;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	void operator *= (LLUnit<OTHER_UNIT, OTHER_STORAGE> multiplicand)
	{
		llstatic_assert(sizeof(OTHER_UNIT) == false, "Multiplication of unit types not supported.");
	}

	void operator /= (storage_t divisor)
	{
		mValue /= divisor;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	void operator /= (LLUnit<OTHER_UNIT, OTHER_STORAGE> divisor)
	{
		llstatic_assert(sizeof(OTHER_UNIT) == false, "Division of unit types not supported.");
	}

	template<typename SOURCE_UNITS, typename SOURCE_VALUE>
	static storage_t convert(LLUnit<SOURCE_UNITS, SOURCE_VALUE> v) 
	{ 
		return (storage_t)(v.value() 
			* LLUnits::ConversionFactor<SOURCE_UNITS, typename UNIT_TYPE::base_unit_t>::get() 
			* LLUnits::ConversionFactor<typename UNIT_TYPE::base_unit_t, UNIT_TYPE>::get()); 
	}

protected:

	storage_t mValue;
};

//
// operator +
//
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnit<STORAGE_TYPE1, UNIT_TYPE1> operator + (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator + (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	LLUnit<STORAGE_TYPE, UNIT_TYPE> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator + (SCALAR_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LLUnit<STORAGE_TYPE, UNIT_TYPE> result(first);
	result += second;
	return result;
}

//
// operator -
//
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnit<STORAGE_TYPE1, UNIT_TYPE1> operator - (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
	result -= second;
	return result;
}


template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator - (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	LLUnit<STORAGE_TYPE, UNIT_TYPE> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator - (SCALAR_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LLUnit<STORAGE_TYPE, UNIT_TYPE> result(first);
	result -= second;
	return result;
}

//
// operator *
//
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator * (SCALAR_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>(first * second.value());
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator * (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>(first.value() * second);
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
void operator * (LLUnit<STORAGE_TYPE1, UNIT_TYPE1>, LLUnit<STORAGE_TYPE2, UNIT_TYPE2>)
{
	llstatic_assert(sizeof(STORAGE_TYPE1) == false, "Multiplication of unit types results in new unit type - not supported.");
}

//
// operator /
//
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
SCALAR_TYPE operator / (SCALAR_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return SCALAR_TYPE(first / second.value());
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator / (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>(first.value() / second);
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
void operator / (LLUnit<STORAGE_TYPE1, UNIT_TYPE1>, LLUnit<STORAGE_TYPE2, UNIT_TYPE2>)
{
	llstatic_assert(sizeof(STORAGE_TYPE1) == false, "Multiplication of unit types results in new unit type - not supported.");
}

#define COMPARISON_OPERATORS(op)                                                                     \
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>                            \
bool operator op (SCALAR_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)                         \
{                                                                                                    \
	return first op second.value();                                                                  \
}                                                                                                    \
	                                                                                                 \
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>                            \
bool operator op (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)                         \
{                                                                                                    \
	return first.value() op second;                                                                  \
}                                                                                                    \
	                                                                                                 \
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>   \
bool operator op (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second) \
{                                                                                                    \
	return first.value() op first.convert(second);                                                   \
}

COMPARISON_OPERATORS(<)
COMPARISON_OPERATORS(<=)
COMPARISON_OPERATORS(>)
COMPARISON_OPERATORS(>=)
COMPARISON_OPERATORS(==)
COMPARISON_OPERATORS(!=)

namespace LLUnits
{
#define LL_DECLARE_DERIVED_UNIT(base_unit_name, unit_name, conversion_factor)\
struct unit_name                                                             \
{                                                                            \
	typedef base_unit_name base_unit_t;                                      \
};                                                                           \
template<>                                                                   \
struct ConversionFactor<unit_name, base_unit_name>                           \
{                                                                            \
	static F64 get() { return (conversion_factor); }                         \
};                                                                           \
	                                                                         \
template<>                                                                   \
struct ConversionFactor<base_unit_name, unit_name>						     \
{                                                                            \
	static F64 get() { return 1.0 / (conversion_factor); }                   \
}

struct Bytes { typedef Bytes base_unit_t; };
LL_DECLARE_DERIVED_UNIT(Bytes, Kilobytes, 1024);
LL_DECLARE_DERIVED_UNIT(Bytes, Megabytes, 1024 * 1024);
LL_DECLARE_DERIVED_UNIT(Bytes, Gigabytes, 1024 * 1024 * 1024);
LL_DECLARE_DERIVED_UNIT(Bytes, Bits,	  (1.0 / 8.0));
LL_DECLARE_DERIVED_UNIT(Bytes, Kilobits,  (1024 / 8));
LL_DECLARE_DERIVED_UNIT(Bytes, Megabits,  (1024 / 8));
LL_DECLARE_DERIVED_UNIT(Bytes, Gigabits,  (1024 * 1024 * 1024 / 8));

struct Seconds { typedef Seconds base_unit_t; };
LL_DECLARE_DERIVED_UNIT(Seconds, Minutes,		60);
LL_DECLARE_DERIVED_UNIT(Seconds, Hours,			60 * 60);
LL_DECLARE_DERIVED_UNIT(Seconds, Days,			60 * 60 * 24);
LL_DECLARE_DERIVED_UNIT(Seconds, Weeks,			60 * 60 * 24 * 7);
LL_DECLARE_DERIVED_UNIT(Seconds, Milliseconds,	(1.0 / 1000.0));
LL_DECLARE_DERIVED_UNIT(Seconds, Microseconds,	(1.0 / (1000000.0)));
LL_DECLARE_DERIVED_UNIT(Seconds, Nanoseconds,	(1.0 / (1000000000.0)));

struct Meters { typedef Meters base_unit_t; };
LL_DECLARE_DERIVED_UNIT(Meters, Kilometers, 1000);
LL_DECLARE_DERIVED_UNIT(Meters, Centimeters, (1.0 / 100));
LL_DECLARE_DERIVED_UNIT(Meters, Millimeters, (1.0 / 1000));
}

#endif // LL_LLUNIT_H
