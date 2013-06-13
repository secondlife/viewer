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
#include "llerrorlegacy.h"

namespace LLUnits
{

template<typename DERIVED_UNITS_TAG, typename BASE_UNITS_TAG, typename VALUE_TYPE>
struct Convert
{
	static VALUE_TYPE get(VALUE_TYPE val)
	{
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		llstatic_assert_template(DERIVED_UNITS_TAG, false,  "Cannot convert between types.");
        return val;
	}
};

template<typename BASE_UNITS_TAG, typename VALUE_TYPE>
struct Convert<BASE_UNITS_TAG, BASE_UNITS_TAG, VALUE_TYPE>
{
	static VALUE_TYPE get(VALUE_TYPE val)
	{ 
		return val; 
	}
};

}

template<typename STORAGE_TYPE, typename UNIT_TYPE>
struct LLUnit
{
	typedef LLUnit<STORAGE_TYPE, UNIT_TYPE> self_t;
	typedef STORAGE_TYPE storage_t;

	// value initialization
	LLUnit(storage_t value = storage_t())
	:	mValue(value)
	{}

	// unit initialization and conversion
	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	LLUnit(LLUnit<OTHER_STORAGE, OTHER_UNIT> other)
	:	mValue(convert(other))
	{}
	
	bool operator == (const self_t& other)
	{
		return mValue = other.mValue;
	}

	// value assignment
	self_t& operator = (storage_t value)
	{
		mValue = value;
		return *this;
	}

	// unit assignment
	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	self_t& operator = (LLUnit<OTHER_STORAGE, OTHER_UNIT> other)
	{
		mValue = convert(other);
		return *this;
	}

	storage_t value() const
	{
		return mValue;
	}

	template<typename NEW_UNIT_TYPE> LLUnit<STORAGE_TYPE, NEW_UNIT_TYPE> as()
	{
		return LLUnit<STORAGE_TYPE, NEW_UNIT_TYPE>(*this);
	}


	void operator += (storage_t value)
	{
		mValue += value;
	}

	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	void operator += (LLUnit<OTHER_STORAGE, OTHER_UNIT> other)
	{
		mValue += convert(other);
	}

	void operator -= (storage_t value)
	{
		mValue -= value;
	}

	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	void operator -= (LLUnit<OTHER_STORAGE, OTHER_UNIT> other)
	{
		mValue -= convert(other);
	}

	void operator *= (storage_t multiplicand)
	{
		mValue *= multiplicand;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	void operator *= (LLUnit<OTHER_STORAGE, OTHER_UNIT> multiplicand)
	{
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		llstatic_assert_template(OTHER_UNIT, false, "Multiplication of unit types not supported.");
	}

	void operator /= (storage_t divisor)
	{
		mValue /= divisor;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	void operator /= (LLUnit<OTHER_STORAGE, OTHER_UNIT> divisor)
	{
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		llstatic_assert_template(OTHER_UNIT, false, "Illegal in-place division of unit types.");
	}

	template<typename SOURCE_STORAGE, typename SOURCE_UNITS>
	static storage_t convert(LLUnit<SOURCE_STORAGE, SOURCE_UNITS> v) 
	{ 
		return (storage_t)LLUnits::Convert<typename UNIT_TYPE::base_unit_t, UNIT_TYPE, STORAGE_TYPE>::get((STORAGE_TYPE)
							LLUnits::Convert<SOURCE_UNITS, typename UNIT_TYPE::base_unit_t, SOURCE_STORAGE>::get(v.value())); 
	}

	template<typename SOURCE_STORAGE>
	static storage_t convert(LLUnit<SOURCE_STORAGE, UNIT_TYPE> v) 
	{ 
		return (storage_t)(v.value());
	}


protected:
	storage_t mValue;
};

template<typename STORAGE_TYPE, typename UNIT_TYPE>
struct LLUnitImplicit : public LLUnit<STORAGE_TYPE, UNIT_TYPE>
{
	typedef LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> self_t;
	typedef typename LLUnit<STORAGE_TYPE, UNIT_TYPE>::storage_t storage_t;
	typedef LLUnit<STORAGE_TYPE, UNIT_TYPE> base_t;

	LLUnitImplicit(storage_t value = storage_t())
	:	base_t(value)
	{}

	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	LLUnitImplicit(LLUnit<OTHER_STORAGE, OTHER_UNIT> other)
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

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator + (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
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

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator - (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator - (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator - (SCALAR_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> result(first);
	result -= second;
	return result;
}

//
// operator *
//
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator * (SCALAR_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>((STORAGE_TYPE)(first * second.value()));
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator * (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>((STORAGE_TYPE)(first.value() * second));
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnit<STORAGE_TYPE1, UNIT_TYPE1> operator * (LLUnit<STORAGE_TYPE1, UNIT_TYPE1>, LLUnit<STORAGE_TYPE2, UNIT_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	llstatic_assert_template(STORAGE_TYPE1, false, "Multiplication of unit types results in new unit type - not supported.");
	return LLUnit<STORAGE_TYPE1, UNIT_TYPE1>();
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator * (SCALAR_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE>(first * second.value());
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator * (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	return LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE>(first.value() * second);
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator * (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	llstatic_assert_template(STORAGE_TYPE1, false, "Multiplication of unit types results in new unit type - not supported.");
	return LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>();
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
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>((STORAGE_TYPE)(first.value() / second));
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
STORAGE_TYPE1 operator / (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return STORAGE_TYPE1(first.value() / first.convert(second));
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator / (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	return LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE>((STORAGE_TYPE)(first.value() / second));
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
STORAGE_TYPE1 operator / (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return STORAGE_TYPE1(first.value() / first.convert(second));
}

#define COMPARISON_OPERATORS(op)                                                                                     \
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>                                            \
bool operator op (SCALAR_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)                                         \
{                                                                                                                    \
	return first op second.value();                                                                                  \
}                                                                                                                    \
	                                                                                                                 \
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>                                            \
bool operator op (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)                                         \
{                                                                                                                    \
	return first.value() op second;                                                                                  \
}                                                                                                                    \
	                                                                                                                 \
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>                   \
bool operator op (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second) \
{                                                                                                                    \
	return first.value() op first.convert(second);                                                                   \
}                                                                                                                    \
	                                                                                                                 \
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>                   \
	bool operator op (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)             \
{                                                                                                                    \
	return first.value() op first.convert(second);                                                                   \
}

COMPARISON_OPERATORS(<)
COMPARISON_OPERATORS(<=)
COMPARISON_OPERATORS(>)
COMPARISON_OPERATORS(>=)
COMPARISON_OPERATORS(==)
COMPARISON_OPERATORS(!=)


template<typename T> 
struct LLGetUnitLabel
{
	static const char* getUnitLabel() { return ""; }
};

template<typename T, typename STORAGE_T>
struct LLGetUnitLabel<LLUnit<STORAGE_T, T> >
{
	static const char* getUnitLabel() { return T::getUnitLabel(); }
};

//
// Unit declarations
//
namespace LLUnits
{

template<typename VALUE_TYPE>
struct LinearOps
{
	typedef LinearOps<VALUE_TYPE> self_t;
	LinearOps(VALUE_TYPE val) : mValue (val) {}

	operator VALUE_TYPE() const { return mValue; }
	VALUE_TYPE mValue;

	template<typename T>
	self_t operator * (T other)
	{
		return mValue * other;
	}

	template<typename T>
	self_t operator / (T other)
	{
		return mValue / other;
	}

	template<typename T>
	self_t operator + (T other)
	{
		return mValue + other;
	}

	template<typename T>
	self_t operator - (T other)
	{
		return mValue - other;
	}
};

template<typename VALUE_TYPE>
struct InverseLinearOps
{
	typedef InverseLinearOps<VALUE_TYPE> self_t;

	InverseLinearOps(VALUE_TYPE val) : mValue (val) {}
	operator VALUE_TYPE() const { return mValue; }
	VALUE_TYPE mValue;

	template<typename T>
	self_t operator * (T other)
	{
		return mValue / other;
	}

	template<typename T>
	self_t operator / (T other)
	{
		return mValue * other;
	}

	template<typename T>
	self_t operator + (T other)
	{
		return mValue - other;
	}

	template<typename T>
	self_t operator - (T other)
	{
		return mValue + other;
	}
};


template<typename T>
T storageValue(T val) { return val; }

template<typename UNIT_TYPE, typename STORAGE_TYPE> 
STORAGE_TYPE storageValue(LLUnit<STORAGE_TYPE, UNIT_TYPE> val) { return val.value(); }

template<typename UNIT_TYPE, typename STORAGE_TYPE> 
STORAGE_TYPE storageValue(LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> val) { return val.value(); }

#define LL_DECLARE_BASE_UNIT(base_unit_name, unit_label) \
struct base_unit_name { typedef base_unit_name base_unit_t; static const char* getUnitLabel() { return unit_label; }}

#define LL_DECLARE_DERIVED_UNIT(unit_name, unit_label, base_unit_name, conversion_operation)	\
struct unit_name                                                                                \
{                                                                                               \
	typedef base_unit_name base_unit_t;                                                         \
	static const char* getUnitLabel() { return unit_label; }									\
};                                                                                              \
template<typename STORAGE_TYPE>                                                                 \
struct Convert<unit_name, base_unit_name, STORAGE_TYPE>                                         \
{                                                                                               \
	static STORAGE_TYPE get(STORAGE_TYPE val)                                                   \
	{                                                                                           \
		return (LinearOps<STORAGE_TYPE>(val) conversion_operation).mValue;                      \
	}                                                                                           \
};                                                                                              \
	                                                                                            \
template<typename STORAGE_TYPE>                                                                 \
struct Convert<base_unit_name, unit_name, STORAGE_TYPE>						                    \
{                                                                                               \
	static STORAGE_TYPE get(STORAGE_TYPE val)                                                   \
	{                                                                                           \
		return (InverseLinearOps<STORAGE_TYPE>(val) conversion_operation).mValue;               \
	}                                                                                           \
}

LL_DECLARE_BASE_UNIT(Bytes, "B");
LL_DECLARE_DERIVED_UNIT(Kilobytes, "KB", Bytes, * 1000);
LL_DECLARE_DERIVED_UNIT(Megabytes, "MB", Bytes, * 1000 * 1000);
LL_DECLARE_DERIVED_UNIT(Gigabytes, "GB", Bytes, * 1000 * 1000 * 1000);
LL_DECLARE_DERIVED_UNIT(Kibibytes, "KiB", Bytes, * 1024);
LL_DECLARE_DERIVED_UNIT(Mibibytes, "MiB", Bytes, * 1024 * 1024);
LL_DECLARE_DERIVED_UNIT(Gibibytes, "GiB", Bytes, * 1024 * 1024 * 1024);

LL_DECLARE_DERIVED_UNIT(Bits, "b", Bytes, / 8);
LL_DECLARE_DERIVED_UNIT(Kilobits, "Kb", Bytes, * (1000 / 8));
LL_DECLARE_DERIVED_UNIT(Megabits, "Mb", Bytes, * (1000 / 8));
LL_DECLARE_DERIVED_UNIT(Gigabits, "Gb", Bytes, * (1000 * 1000 * 1000 / 8));
LL_DECLARE_DERIVED_UNIT(Kibibits, "Kib", Bytes, * (1024 / 8));
LL_DECLARE_DERIVED_UNIT(Mibibits, "Mib", Bytes, * (1024 / 8));
LL_DECLARE_DERIVED_UNIT(Gibibits, "Gib", Bytes, * (1024 * 1024 * 1024 / 8));

LL_DECLARE_BASE_UNIT(Seconds, "s");
LL_DECLARE_DERIVED_UNIT(Minutes, "min", Seconds, * 60);
LL_DECLARE_DERIVED_UNIT(Hours, "h", Seconds, * 60 * 60);
LL_DECLARE_DERIVED_UNIT(Milliseconds, "ms", Seconds, / 1000);
LL_DECLARE_DERIVED_UNIT(Microseconds, "\x09\x3cs", Seconds, / 1000000);
LL_DECLARE_DERIVED_UNIT(Nanoseconds, "ns", Seconds, / 1000000000);

LL_DECLARE_BASE_UNIT(Meters, "m");
LL_DECLARE_DERIVED_UNIT(Kilometers, "km", Meters, * 1000);
LL_DECLARE_DERIVED_UNIT(Centimeters, "cm", Meters, * 100);
LL_DECLARE_DERIVED_UNIT(Millimeters, "mm", Meters, * 1000);

LL_DECLARE_BASE_UNIT(Hertz, "Hz");
LL_DECLARE_DERIVED_UNIT(Kilohertz, "KHz", Hertz, * 1000);
LL_DECLARE_DERIVED_UNIT(Megahertz, "MHz", Hertz, * 1000 * 1000);
LL_DECLARE_DERIVED_UNIT(Gigahertz, "GHz", Hertz, * 1000 * 1000 * 1000);

LL_DECLARE_BASE_UNIT(Radians, "rad");
LL_DECLARE_DERIVED_UNIT(Degrees, "deg", Radians, * 0.01745329251994);


} // namespace LLUnits

#endif // LL_LLUNIT_H
