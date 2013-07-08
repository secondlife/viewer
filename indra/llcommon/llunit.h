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
#include <boost/type_traits/is_same.hpp>

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
	:	mValue(convert(other).mValue)
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
		mValue = convert(other).mValue;
		return *this;
	}

	storage_t value() const
	{
		return mValue;
	}

	template<typename NEW_UNIT_TYPE> 
	STORAGE_TYPE valueAs()
	{
		return LLUnit<STORAGE_TYPE, NEW_UNIT_TYPE>(*this).value();
	}

	void operator += (storage_t value)
	{
		mValue += value;
	}

	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	void operator += (LLUnit<OTHER_STORAGE, OTHER_UNIT> other)
	{
		mValue += convert(other).mValue;
	}

	void operator -= (storage_t value)
	{
		mValue -= value;
	}

	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	void operator -= (LLUnit<OTHER_STORAGE, OTHER_UNIT> other)
	{
		mValue -= convert(other).mValue;
	}

	void operator *= (storage_t multiplicand)
	{
		mValue *= multiplicand;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	void operator *= (LLUnit<OTHER_STORAGE, OTHER_UNIT> multiplicand)
	{
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		LL_BAD_TEMPLATE_INSTANTIATION(OTHER_UNIT, "Multiplication of unit types not supported.");
	}

	void operator /= (storage_t divisor)
	{
		mValue /= divisor;
	}

	template<typename OTHER_UNIT, typename OTHER_STORAGE>
	void operator /= (LLUnit<OTHER_STORAGE, OTHER_UNIT> divisor)
	{
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		LL_BAD_TEMPLATE_INSTANTIATION(OTHER_UNIT, "Illegal in-place division of unit types.");
	}

	template<typename SOURCE_STORAGE, typename SOURCE_UNITS>
	static self_t convert(LLUnit<SOURCE_STORAGE, SOURCE_UNITS> v) 
	{ 
		self_t result;
		ll_convert_units(v, result);
		return result;
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


template<typename S1, typename T1, typename S2, typename T2>
LL_FORCE_INLINE void ll_convert_units(LLUnit<S1, T1> in, LLUnit<S2, T2>& out, ...)
{
	typedef boost::integral_constant<bool, 
									boost::is_same<T1, T2>::value 
										|| !boost::is_same<T1, typename T1::base_unit_t>::value 
										|| !boost::is_same<T2, typename T2::base_unit_t>::value> conversion_valid_t;
	LL_STATIC_ASSERT(conversion_valid_t::value, "invalid conversion");

	if (boost::is_same<T1, typename T1::base_unit_t>::value)
	{
		if (boost::is_same<T2, typename T2::base_unit_t>::value)

		{
			// T1 and T2 fully reduced and equal...just copy
			out = (S2)in.value();
		}
		else
		{
			// reduce T2
			LLUnit<S2, typename T2::base_unit_t> new_out;
			ll_convert_units(in, new_out);
			ll_convert_units(new_out, out);
		}
	}
	else
	{
		// reduce T1
		LLUnit<S1, typename T1::base_unit_t> new_in;
		ll_convert_units(in, new_in);
		ll_convert_units(new_in, out);
	}
}

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
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE1, "Multiplication of unit types results in new unit type - not supported.");
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
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE1, "Multiplication of unit types results in new unit type - not supported.");
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

template<typename VALUE_TYPE>
struct LLUnitLinearOps
{
	typedef LLUnitLinearOps<VALUE_TYPE> self_t;
	LLUnitLinearOps(VALUE_TYPE val) : mValue (val) {}

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
struct LLUnitInverseLinearOps
{
	typedef LLUnitInverseLinearOps<VALUE_TYPE> self_t;

	LLUnitInverseLinearOps(VALUE_TYPE val) : mValue (val) {}
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

#define LL_DECLARE_BASE_UNIT(base_unit_name, unit_label)                                             \
struct base_unit_name                                                                                \
{                                                                                                    \
	typedef base_unit_name base_unit_t;                                                              \
	static const char* getUnitLabel() { return unit_label; }                                         \
	template<typename T>                                                                             \
	static LLUnit<T, base_unit_name> fromValue(T value) { return LLUnit<T, base_unit_name>(value); } \
	template<typename STORAGE_T, typename UNIT_T>                                                    \
	static LLUnit<STORAGE_T, base_unit_name> fromValue(LLUnit<STORAGE_T, UNIT_T> value)              \
	{ return LLUnit<STORAGE_T, base_unit_name>(value); }                                             \
};                                                                                                   \
template<typename T> std::ostream& operator<<(std::ostream& s, const LLUnit<T, base_unit_name>& val) \
{ s << val.value() << base_unit_name::getUnitLabel; return s; }


#define LL_DECLARE_DERIVED_UNIT(unit_name, unit_label, base_unit_name, conversion_operation)	 \
struct unit_name                                                                                 \
{                                                                                                \
	typedef base_unit_name base_unit_t;                                                          \
	static const char* getUnitLabel() { return unit_label; }									 \
	template<typename T>                                                                         \
	static LLUnit<T, unit_name> fromValue(T value) { return LLUnit<T, unit_name>(value); }		 \
	template<typename STORAGE_T, typename UNIT_T>                                                \
	static LLUnit<STORAGE_T, unit_name> fromValue(LLUnit<STORAGE_T, UNIT_T> value)				 \
	{ return LLUnit<STORAGE_T, unit_name>(value); }												 \
};                                                                                               \
template<typename T> std::ostream& operator<<(std::ostream& s, const LLUnit<T, unit_name>& val)  \
{ s << val.value() << unit_name::getUnitLabel; return s; }                                       \
	                                                                                             \
template<typename S1, typename S2>                                                               \
void ll_convert_units(LLUnit<S1, unit_name> in, LLUnit<S2, base_unit_name>& out)                 \
{                                                                                                \
	out = (S2)(LLUnitLinearOps<S1>(in.value()) conversion_operation).mValue;                     \
}                                                                                                \
                                                                                                 \
template<typename S1, typename S2>                                                               \
void ll_convert_units(LLUnit<S1, base_unit_name> in, LLUnit<S2, unit_name>& out)                 \
{                                                                                                \
	out = (S2)(LLUnitInverseLinearOps<S1>(in.value()) conversion_operation).mValue;              \
}                                                                                               

//
// Unit declarations
//

namespace LLUnits
{
LL_DECLARE_BASE_UNIT(Bytes, "B");
LL_DECLARE_DERIVED_UNIT(Kilobytes, "KB", Bytes, * 1000);
LL_DECLARE_DERIVED_UNIT(Megabytes, "MB", Kilobytes, * 1000);
LL_DECLARE_DERIVED_UNIT(Gigabytes, "GB", Megabytes, * 1000);
LL_DECLARE_DERIVED_UNIT(Kibibytes, "KiB", Bytes, * 1024);
LL_DECLARE_DERIVED_UNIT(Mibibytes, "MiB", Kibibytes, * 1024);
LL_DECLARE_DERIVED_UNIT(Gibibytes, "GiB", Mibibytes, * 1024);

LL_DECLARE_DERIVED_UNIT(Bits, "b", Bytes, / 8);
LL_DECLARE_DERIVED_UNIT(Kilobits, "Kb", Bytes, * 1000 / 8);
LL_DECLARE_DERIVED_UNIT(Megabits, "Mb", Kilobits, * 1000 / 8);
LL_DECLARE_DERIVED_UNIT(Gigabits, "Gb", Megabits, * 1000 / 8);
LL_DECLARE_DERIVED_UNIT(Kibibits, "Kib", Bytes, * 1024 / 8);
LL_DECLARE_DERIVED_UNIT(Mibibits, "Mib", Kibibits, * 1024 / 8);
LL_DECLARE_DERIVED_UNIT(Gibibits, "Gib", Mibibits, * 1024 / 8);

LL_DECLARE_BASE_UNIT(Seconds, "s");
LL_DECLARE_DERIVED_UNIT(Minutes, "min", Seconds, * 60);
LL_DECLARE_DERIVED_UNIT(Hours, "h", Seconds, * 60 * 60);
LL_DECLARE_DERIVED_UNIT(Milliseconds, "ms", Seconds, / 1000);
LL_DECLARE_DERIVED_UNIT(Microseconds, "\x09\x3cs", Milliseconds, / 1000);
LL_DECLARE_DERIVED_UNIT(Nanoseconds, "ns", Microseconds, / 1000);

LL_DECLARE_BASE_UNIT(Meters, "m");
LL_DECLARE_DERIVED_UNIT(Kilometers, "km", Meters, * 1000);
LL_DECLARE_DERIVED_UNIT(Centimeters, "cm", Meters, / 100);
LL_DECLARE_DERIVED_UNIT(Millimeters, "mm", Meters, / 1000);

LL_DECLARE_BASE_UNIT(Hertz, "Hz");
LL_DECLARE_DERIVED_UNIT(Kilohertz, "KHz", Hertz, * 1000);
LL_DECLARE_DERIVED_UNIT(Megahertz, "MHz", Kilohertz, * 1000);
LL_DECLARE_DERIVED_UNIT(Gigahertz, "GHz", Megahertz, * 1000);

LL_DECLARE_BASE_UNIT(Radians, "rad");
LL_DECLARE_DERIVED_UNIT(Degrees, "deg", Radians, * 0.01745329251994);

LL_DECLARE_BASE_UNIT(Percent, "%");
LL_DECLARE_DERIVED_UNIT(Ratio, "x", Percent, / 100);

LL_DECLARE_BASE_UNIT(Triangles, "tris");
LL_DECLARE_DERIVED_UNIT(Kilotriangles, "ktris", Triangles, * 1000);

} // namespace LLUnits

#endif // LL_LLUNIT_H
