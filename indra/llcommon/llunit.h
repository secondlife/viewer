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
	typedef STORAGE_TYPE storage_t;

	// value initialization
	LLUnit(storage_t value = storage_t())
	:	mValue(value)
	{}

	// unit initialization and conversion
	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	LLUnit(LLUnit<OTHER_UNIT, OTHER_STORAGE> other)
	:	mValue(convert(other))
	{}
	
	// value assignment
	self_t& operator = (storage_t value)
	{
		mValue = value;
		return *this;
	}

	// unit assignment
	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	self_t& operator = (LLUnit<OTHER_UNIT, OTHER_STORAGE> other)
	{
		mValue = convert(other);
		return *this;
	}

	storage_t value() const
	{
		return mValue;
	}

	template<typename NEW_UNIT_TYPE, typename NEW_STORAGE_TYPE> LLUnit<NEW_UNIT_TYPE, NEW_STORAGE_TYPE> as()
	{
		return LLUnit<NEW_UNIT_TYPE, NEW_STORAGE_TYPE>(*this);
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
		llstatic_assert(sizeof(OTHER_UNIT) == 0, "Illegal in-place division of unit types.");
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
struct LLUnitImplicit : public LLUnit<UNIT_TYPE, STORAGE_TYPE>
{
	typedef LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> self_t;
	typedef typename LLUnit<UNIT_TYPE, STORAGE_TYPE>::storage_t storage_t;
	typedef LLUnit<UNIT_TYPE, STORAGE_TYPE> base_t;

	LLUnitImplicit(storage_t value = storage_t())
	:	base_t(value)
	{}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	LLUnitImplicit(LLUnit<OTHER_UNIT, OTHER_STORAGE> other)
	:	base_t(convert(other))
	{}

	// unlike LLUnit, LLUnitImplicit is *implicitly* convertable to a POD scalar (F32, S32, etc)
	// this allows for interoperability with legacy code
	operator storage_t() const
	{
		return base_t::value();
	}
};

//
// operator +
//
template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
LLUnit<UNIT_TYPE1, STORAGE_TYPE1> operator + (LLUnit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnit<UNIT_TYPE2, STORAGE_TYPE2> second)
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

template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> operator + (LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnit<UNIT_TYPE2, STORAGE_TYPE2> second)
{
	LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> result(first);
	result += second;
	return result;
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> operator + (LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)
{
	LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> result(first);
	result += second;
	return result;
}

template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> operator + (LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnitImplicit<UNIT_TYPE2, STORAGE_TYPE2> second)
{
	LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> result(first);
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

template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> operator - (LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnitImplicit<UNIT_TYPE2, STORAGE_TYPE2> second)
{
	LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> result(first);
	result -= second;
	return result;
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> operator - (LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)
{
	LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> result(first);
	result -= second;
	return result;
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> operator - (SCALAR_TYPE first, LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> second)
{
	LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> result(first);
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
LLUnit<UNIT_TYPE1, STORAGE_TYPE1> operator * (LLUnit<UNIT_TYPE1, STORAGE_TYPE1>, LLUnit<UNIT_TYPE2, STORAGE_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	llstatic_assert(sizeof(STORAGE_TYPE1) == 0, "Multiplication of unit types results in new unit type - not supported.");
	return LLUnit<UNIT_TYPE1, STORAGE_TYPE1>();
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> operator * (SCALAR_TYPE first, LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> second)
{
	return LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE>(first * second.value());
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> operator * (LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)
{
	return LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE>(first.value() * second);
}

template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> operator * (LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1>, LLUnitImplicit<UNIT_TYPE2, STORAGE_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	llstatic_assert(sizeof(STORAGE_TYPE1) == 0, "Multiplication of unit types results in new unit type - not supported.");
	return LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1>();
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
STORAGE_TYPE1 operator / (LLUnit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnit<UNIT_TYPE2, STORAGE_TYPE2> second)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	return STORAGE_TYPE1(first.value() / second.value());
}

template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> operator / (LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)
{
	return LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE>(first.value() / second);
}

template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>
STORAGE_TYPE1 operator / (LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnitImplicit<UNIT_TYPE2, STORAGE_TYPE2> second)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	return STORAGE_TYPE1(first.value() / second.value());
}

#define COMPARISON_OPERATORS(op)                                                                                     \
template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>                                            \
bool operator op (SCALAR_TYPE first, LLUnit<UNIT_TYPE, STORAGE_TYPE> second)                                         \
{                                                                                                                    \
	return first op second.value();                                                                                  \
}                                                                                                                    \
	                                                                                                                 \
template<typename UNIT_TYPE, typename STORAGE_TYPE, typename SCALAR_TYPE>                                            \
bool operator op (LLUnit<UNIT_TYPE, STORAGE_TYPE> first, SCALAR_TYPE second)                                         \
{                                                                                                                    \
	return first.value() op second;                                                                                  \
}                                                                                                                    \
	                                                                                                                 \
template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>                   \
bool operator op (LLUnitImplicit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnitImplicit<UNIT_TYPE2, STORAGE_TYPE2> second) \
{                                                                                                                    \
	return first.value() op first.convert(second);                                                                   \
}                                                                                                                    \
	                                                                                                                 \
template<typename UNIT_TYPE1, typename STORAGE_TYPE1, typename UNIT_TYPE2, typename STORAGE_TYPE2>                   \
	bool operator op (LLUnit<UNIT_TYPE1, STORAGE_TYPE1> first, LLUnit<UNIT_TYPE2, STORAGE_TYPE2> second)             \
{                                                                                                                    \
	return first.value() op first.convert(second);                                                                   \
}

COMPARISON_OPERATORS(<)
COMPARISON_OPERATORS(<=)
COMPARISON_OPERATORS(>)
COMPARISON_OPERATORS(>=)
COMPARISON_OPERATORS(==)
COMPARISON_OPERATORS(!=)

namespace LLUnits
{
template<typename T>
T rawValue(T val) { return val; }

template<typename UNIT_TYPE, typename STORAGE_TYPE> 
STORAGE_TYPE rawValue(LLUnit<UNIT_TYPE, STORAGE_TYPE> val) { return val.value(); }

template<typename UNIT_TYPE, typename STORAGE_TYPE> 
STORAGE_TYPE rawValue(LLUnitImplicit<UNIT_TYPE, STORAGE_TYPE> val) { return val.value(); }

template<typename UNIT_TYPE, typename STORAGE_TYPE> 
struct HighestPrecisionType<LLUnit<UNIT_TYPE, STORAGE_TYPE> >
{
	typedef typename HighestPrecisionType<STORAGE_TYPE>::type_t type_t;
};

#define LL_DECLARE_DERIVED_UNIT(conversion_factor, base_unit_name, unit_name)                   \
struct unit_name                                                                                \
{                                                                                               \
	typedef base_unit_name base_unit_t;                                                         \
};                                                                                              \
template<typename STORAGE_TYPE>                                                                 \
struct ConversionFactor<unit_name, base_unit_name, STORAGE_TYPE>                                \
{                                                                                               \
	static typename HighestPrecisionType<STORAGE_TYPE>::type_t get()                            \
	{                                                                                           \
		return typename HighestPrecisionType<STORAGE_TYPE>::type_t(conversion_factor);          \
	}                                                                                           \
};                                                                                              \
	                                                                                            \
template<typename STORAGE_TYPE>                                                                 \
struct ConversionFactor<base_unit_name, unit_name, STORAGE_TYPE>						        \
{                                                                                               \
	static typename HighestPrecisionType<STORAGE_TYPE>::type_t get()                            \
	{                                                                                           \
		return typename HighestPrecisionType<STORAGE_TYPE>::type_t(1.0 / (conversion_factor));  \
	}                                                                                           \
}

struct Bytes { typedef Bytes base_unit_t; };
LL_DECLARE_DERIVED_UNIT(1024, Bytes, Kilobytes);
LL_DECLARE_DERIVED_UNIT(1024 * 1024, Bytes, Megabytes);
LL_DECLARE_DERIVED_UNIT(1024 * 1024 * 1024, Bytes, Gigabytes);
LL_DECLARE_DERIVED_UNIT(1.0 / 8.0, Bytes, Bits);
LL_DECLARE_DERIVED_UNIT(1024 / 8, Bytes, Kilobits);
LL_DECLARE_DERIVED_UNIT(1024 / 8, Bytes, Megabits);
LL_DECLARE_DERIVED_UNIT(1024 * 1024 * 1024 / 8, Bytes, Gigabits);

struct Seconds { typedef Seconds base_unit_t; };
LL_DECLARE_DERIVED_UNIT(60, Seconds, Minutes);
LL_DECLARE_DERIVED_UNIT(60 * 60, Seconds, Hours);
LL_DECLARE_DERIVED_UNIT(1.0 / 1000.0, Seconds, Milliseconds);
LL_DECLARE_DERIVED_UNIT(1.0 / 1000000.0, Seconds, Microseconds);
LL_DECLARE_DERIVED_UNIT(1.0 / 1000000000.0, Seconds, Nanoseconds);

struct Meters { typedef Meters base_unit_t; };
LL_DECLARE_DERIVED_UNIT(1000, Meters, Kilometers);
LL_DECLARE_DERIVED_UNIT(1.0 / 100.0, Meters, Centimeters);
LL_DECLARE_DERIVED_UNIT(1.0 / 1000.0, Meters, Millimeters);

struct Hertz { typedef Hertz base_unit_t; };
LL_DECLARE_DERIVED_UNIT(1000, Hertz, Kilohertz);
LL_DECLARE_DERIVED_UNIT(1000 * 1000, Hertz, Megahertz);
LL_DECLARE_DERIVED_UNIT(1000 * 1000 * 1000, Hertz, Gigahertz);
} // namespace LLUnits

#endif // LL_LLUNIT_H
