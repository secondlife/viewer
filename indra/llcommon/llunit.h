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

template<typename T>
struct HighestPrecisionType
{
	typedef T type_t;
};

template<> struct HighestPrecisionType<F32> { typedef F64 type_t; };
template<> struct HighestPrecisionType<S32> { typedef S64 type_t; };
template<> struct HighestPrecisionType<U32> { typedef S64 type_t; };
template<> struct HighestPrecisionType<S16> { typedef S64 type_t; };
template<> struct HighestPrecisionType<U16> { typedef S64 type_t; };
template<> struct HighestPrecisionType<S8> { typedef S64 type_t; };
template<> struct HighestPrecisionType<U8> { typedef S64 type_t; };

template<typename DERIVED_UNITS_TAG, typename BASE_UNITS_TAG, typename VALUE_TYPE>
struct ConversionFactor
{
	static typename HighestPrecisionType<VALUE_TYPE>::type_t get()
	{
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		llstatic_assert(sizeof(DERIVED_UNITS_TAG) == 0, "Cannot convert between types.");
	}
};

template<typename BASE_UNITS_TAG, typename VALUE_TYPE>
struct ConversionFactor<BASE_UNITS_TAG, BASE_UNITS_TAG, VALUE_TYPE>
{
	static typename HighestPrecisionType<VALUE_TYPE>::type_t get() 
	{ 
		return 1; 
	}
};
}

template<typename UNIT_TYPE, typename STORAGE_TYPE>
struct LLUnit
{
	typedef LLUnit<UNIT_TYPE, STORAGE_TYPE> self_t;
	typedef typename STORAGE_TYPE storage_t;

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

	template<typename NEW_UNIT_TYPE> LLUnit<NEW_UNIT_TYPE, STORAGE_TYPE> as()
	{
		return LLUnit<NEW_UNIT_TYPE, STORAGE_TYPE>(*this);
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
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		llstatic_assert(sizeof(OTHER_UNIT) == 0, "Multiplication of unit types not supported.");
	}

	void operator /= (storage_t divisor)
	{
		mValue /= divisor;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	void operator /= (LLUnit<OTHER_UNIT, OTHER_STORAGE> divisor)
	{
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		llstatic_assert(sizeof(OTHER_UNIT) == 0, "Division of unit types not supported.");
	}

	template<typename SOURCE_UNITS, typename SOURCE_STORAGE>
	static storage_t convert(LLUnit<SOURCE_UNITS, SOURCE_STORAGE> v) 
	{ 
		return (storage_t)(v.value() 
			* LLUnits::ConversionFactor<SOURCE_UNITS, typename UNIT_TYPE::base_unit_t, SOURCE_STORAGE>::get() 
			* LLUnits::ConversionFactor<typename UNIT_TYPE::base_unit_t, UNIT_TYPE, STORAGE_TYPE>::get()); 
	}

protected:

	storage_t mValue;
};

template<typename UNIT_TYPE, typename STORAGE_TYPE>
struct LLUnitStrict : public LLUnit<UNIT_TYPE, STORAGE_TYPE>
{
	typedef LLUnitStrict<UNIT_TYPE, STORAGE_TYPE> self_t;

	explicit LLUnitStrict(storage_t value = storage_t())
	:	LLUnit(value)
	{}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	LLUnitStrict(LLUnit<OTHER_UNIT, OTHER_STORAGE> other)
	:	LLUnit(convert(other))
	{}

	LLUnitStrict(self_t& other)
	:	LLUnit(other)
	{}


private:
	operator storage_t() const
	{
		return value();
	}
};

//
// operator +
//
template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
LLUnit<STORAGE_TYPE1, UNIT_TYPE1> operator + (LLUnit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnit<UNIT_TYPE2, STORAGE_TYPE2> second)
{
	LLUnit<UNIT_TYPE1, STORAGE_TYPE1> result(first);
	result += second;
	return result;
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnit<UNIT_TYPE, STORAGE_TYPE> operator + (LLUnit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)
{
	LLUnit<UNIT_TYPE, STORAGE_TYPE> result(first);
	result += second;
	return result;
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnit<UNIT_TYPE, STORAGE_TYPE> operator + (SCALAR_TYPE first, LLUnit<UNIT_TYPE, STORAGE_TYPE> second)
{
	LLUnit<UNIT_TYPE, STORAGE_TYPE> result(first);
	result += second;
	return result;
}

//
// operator -
//
template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
LLUnit<UNIT_TYPE1, STORAGE_TYPE1> operator - (LLUnit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnit<UNIT_TYPE2, STORAGE_TYPE2> second)
{
	LLUnit<UNIT_TYPE1, STORAGE_TYPE1> result(first);
	result -= second;
	return result;
}


template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnit<UNIT_TYPE, STORAGE_TYPE> operator - (LLUnit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)
{
	LLUnit<UNIT_TYPE, STORAGE_TYPE> result(first);
	result -= second;
	return result;
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnit<UNIT_TYPE, STORAGE_TYPE> operator - (SCALAR_TYPE first, LLUnit<UNIT_TYPE, STORAGE_TYPE> second)
{
	LLUnit<UNIT_TYPE, STORAGE_TYPE> result(first);
	result -= second;
	return result;
}

//
// operator *
//
template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnit<UNIT_TYPE, STORAGE_TYPE> operator * (SCALAR_TYPE first, LLUnit<UNIT_TYPE, STORAGE_TYPE> second)
{
	return LLUnit<UNIT_TYPE, STORAGE_TYPE>(first * second.value());
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnit<UNIT_TYPE, STORAGE_TYPE> operator * (LLUnit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)
{
	return LLUnit<UNIT_TYPE, STORAGE_TYPE>(first.value() * second);
}

template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
void operator * (LLUnit<UNIT_TYPE1, STORAGE_TYPE1>, LLUnit<UNIT_TYPE2, STORAGE_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	llstatic_assert(sizeof(STORAGE_TYPE1) == 0, "Multiplication of unit types results in new unit type - not supported.");
}

//
// operator /
//
template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
SCALAR_TYPE operator / (SCALAR_TYPE first, LLUnit<UNIT_TYPE, STORAGE_TYPE> second)
{
	return SCALAR_TYPE(first / second.value());
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnit<UNIT_TYPE, STORAGE_TYPE> operator / (LLUnit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)
{
	return LLUnit<UNIT_TYPE, STORAGE_TYPE>(first.value() / second);
}

template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
void operator / (LLUnit<UNIT_TYPE1, STORAGE_TYPE1>, LLUnit<UNIT_TYPE2, STORAGE_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	llstatic_assert(sizeof(STORAGE_TYPE1) == 0, "Multiplication of unit types results in new unit type - not supported.");
}

#define COMPARISON_OPERATORS(op)                                                                     \
template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>                            \
bool operator op (SCALAR_TYPE first, LLUnit<UNIT_TYPE, STORAGE_TYPE> second)                         \
{                                                                                                    \
	return first op second.value();                                                                  \
}                                                                                                    \
	                                                                                                 \
template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>                            \
bool operator op (LLUnit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)                         \
{                                                                                                    \
	return first.value() op second;                                                                  \
}                                                                                                    \
	                                                                                                 \
template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>   \
bool operator op (LLUnit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnit<UNIT_TYPE2, STORAGE_TYPE2> second) \
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
template<typename UNIT_TYPE, typename STORAGE_TYPE> 
struct HighestPrecisionType<LLUnit<UNIT_TYPE, STORAGE_TYPE> >
{
	typedef typename HighestPrecisionType<STORAGE_TYPE>::type_t type_t;
};

#define LL_DECLARE_DERIVED_UNIT(base_unit_name, unit_name, conversion_factor)                                                                                   \
struct unit_name                                                                                                                                                \
{                                                                                                                                                               \
	typedef base_unit_name base_unit_t;                                                                                                                         \
};                                                                                                                                                              \
template<typename STORAGE_TYPE>                                                                                                                                 \
struct ConversionFactor<unit_name, base_unit_name, STORAGE_TYPE>                                                                                                \
{                                                                                                                                                               \
	static typename HighestPrecisionType<STORAGE_TYPE>::type_t get() { return typename HighestPrecisionType<STORAGE_TYPE>::type_t(conversion_factor); }         \
};                                                                                                                                                              \
	                                                                                                                                                            \
template<typename STORAGE_TYPE>                                                                                                                                 \
struct ConversionFactor<base_unit_name, unit_name, STORAGE_TYPE>						                                                                        \
{                                                                                                                                                               \
	static typename HighestPrecisionType<STORAGE_TYPE>::type_t get() { return typename HighestPrecisionType<STORAGE_TYPE>::type_t(1.0 / (conversion_factor)); } \
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
LL_DECLARE_DERIVED_UNIT(Seconds, Milliseconds,	(1.0 / 1000.0));
LL_DECLARE_DERIVED_UNIT(Seconds, Microseconds,	(1.0 / (1000000.0)));
LL_DECLARE_DERIVED_UNIT(Seconds, Nanoseconds,	(1.0 / (1000000000.0)));

struct Meters { typedef Meters base_unit_t; };
LL_DECLARE_DERIVED_UNIT(Meters, Kilometers, 1000);
LL_DECLARE_DERIVED_UNIT(Meters, Centimeters, (1.0 / 100.0));
LL_DECLARE_DERIVED_UNIT(Meters, Millimeters, (1.0 / 1000.0));

struct Hertz { typedef Hertz base_unit_t; };
LL_DECLARE_DERIVED_UNIT(Hertz, Kilohertz, 1000);
LL_DECLARE_DERIVED_UNIT(Hertz, Megahertz, 1000 * 1000);
LL_DECLARE_DERIVED_UNIT(Hertz, Gigahertz, 1000 * 1000 * 1000);
}

#endif // LL_LLUNIT_H
