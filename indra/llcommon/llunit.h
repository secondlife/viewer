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
#include "llerror.h"

template<typename STORAGE_TYPE, typename UNIT_TYPE>
struct LLUnit
{
	typedef LLUnit<STORAGE_TYPE, UNIT_TYPE> self_t;

	typedef STORAGE_TYPE storage_t;

	// value initialization
	explicit LLUnit(storage_t value = storage_t())
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

	void value(storage_t value)
	{
		mValue = value;
	}

	template<typename NEW_UNIT_TYPE> 
	storage_t valueInUnits()
	{
		return LLUnit<storage_t, NEW_UNIT_TYPE>(*this).value();
	}

	template<typename NEW_UNIT_TYPE> 
	void valueInUnits(storage_t value)
	{
		*this = LLUnit<storage_t, NEW_UNIT_TYPE>(value);
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
std::ostream& operator <<(std::ostream& s, const LLUnit<STORAGE_TYPE, UNIT_TYPE>& unit)
{
        s << unit.value() << UNIT_TYPE::getUnitLabel();
        return s;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE>
std::istream& operator >>(std::istream& s, LLUnit<STORAGE_TYPE, UNIT_TYPE>& unit)
{
        STORAGE_TYPE val;
        s >> val;
        unit = val;
        return s;
}

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

template<typename STORAGE_TYPE, typename UNIT_TYPE>
std::ostream& operator <<(std::ostream& s, const LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE>& unit)
{
        s << unit.value() << UNIT_TYPE::getUnitLabel();
        return s;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE>
std::istream& operator >>(std::istream& s, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE>& unit)
{
        STORAGE_TYPE val;
        s >> val;
        unit = val;
        return s;
}

template<typename S, typename T> 
struct LLIsSameType
{
	static const bool value = false;
};

template<typename T>
struct LLIsSameType<T, T>
{
	static const bool value = true;
};

template<typename S1, typename T1, typename S2, typename T2>
LL_FORCE_INLINE void ll_convert_units(LLUnit<S1, T1> in, LLUnit<S2, T2>& out, ...)
{
	LL_STATIC_ASSERT((LLIsSameType<T1, T2>::value 
		|| !LLIsSameType<T1, typename T1::base_unit_t>::value 
		|| !LLIsSameType<T2, typename T2::base_unit_t>::value), "invalid conversion");

	if (LLIsSameType<T1, typename T1::base_unit_t>::value)
	{
		if (LLIsSameType<T2, typename T2::base_unit_t>::value)

		{
			// T1 and T2 fully reduced and equal...just copy
			out = LLUnit<S2, T2>((S2)in.value());
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
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator + (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
	result += LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>(second);
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator + (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator + (SCALAR_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> result(first);
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

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator - (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator - (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> result(first);
	result -= LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>(second);
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
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnit<STORAGE_TYPE1, UNIT_TYPE1> operator * (LLUnit<STORAGE_TYPE1, UNIT_TYPE1>, LLUnit<STORAGE_TYPE2, UNIT_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE1, "Multiplication of unit types results in new unit type - not supported.");
	return LLUnit<STORAGE_TYPE1, UNIT_TYPE1>();
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator * (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>((STORAGE_TYPE)(first.value() * second));
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator * (SCALAR_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>((STORAGE_TYPE)(first * second.value()));
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator * (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE1, "Multiplication of unit types results in new unit type - not supported.");
	return LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>();
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator * (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, SCALAR_TYPE second)
{
	return LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE>(first.value() * second);
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename SCALAR_TYPE>
LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> operator * (SCALAR_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE>(first * second.value());
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

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
STORAGE_TYPE1 operator / (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return STORAGE_TYPE1(first.value() / first.convert(second));
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
STORAGE_TYPE1 operator / (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
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

#define LL_UNIT_PROMOTE_VALUE(output_type, value) ((true ? (output_type)(1) : (value/value)) * value)

template<typename INPUT_TYPE, typename OUTPUT_TYPE>
struct LLUnitLinearOps
{
	typedef LLUnitLinearOps<OUTPUT_TYPE, OUTPUT_TYPE> output_t;

	LLUnitLinearOps(INPUT_TYPE val) 
	:	mInput (val)
	{}

	operator OUTPUT_TYPE() const { return (OUTPUT_TYPE)mInput; }
	INPUT_TYPE mInput;

	template<typename T>
	output_t operator * (T other)
	{
		return mInput * other;
	}

	template<typename T>
	output_t operator / (T other)
	{
		return LL_UNIT_PROMOTE_VALUE(OUTPUT_TYPE, mInput) / other;
	}

	template<typename T>
	output_t operator + (T other)
	{
		return mInput + other;
	}

	template<typename T>
	output_t operator - (T other)
	{
		return mInput - other;
	}
};

template<typename INPUT_TYPE, typename OUTPUT_TYPE>
struct LLUnitInverseLinearOps
{
	typedef LLUnitInverseLinearOps<OUTPUT_TYPE, OUTPUT_TYPE> output_t;

	LLUnitInverseLinearOps(INPUT_TYPE val) 
	:	mInput(val)
	{}

	operator OUTPUT_TYPE() const { return (OUTPUT_TYPE)mInput; }
	INPUT_TYPE mInput;

	template<typename T>
	output_t operator * (T other)
	{
		return LL_UNIT_PROMOTE_VALUE(OUTPUT_TYPE, mInput) / other;
	}

	template<typename T>
	output_t operator / (T other)
	{
		return mInput * other;
	}

	template<typename T>
	output_t operator + (T other)
	{
		return mInput - other;
	}

	template<typename T>
	output_t operator - (T other)
	{
		return mInput + other;
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
}


#define LL_DECLARE_DERIVED_UNIT(base_unit_name, conversion_operation, unit_name, unit_label)	        \
struct unit_name                                                                                        \
{                                                                                                       \
	typedef base_unit_name base_unit_t;                                                                 \
	static const char* getUnitLabel() { return unit_label; }									        \
	template<typename T>                                                                                \
	static LLUnit<T, unit_name> fromValue(T value) { return LLUnit<T, unit_name>(value); }		        \
	template<typename STORAGE_T, typename UNIT_T>                                                       \
	static LLUnit<STORAGE_T, unit_name> fromValue(LLUnit<STORAGE_T, UNIT_T> value)				        \
	{ return LLUnit<STORAGE_T, unit_name>(value); }												        \
};                                                                                                      \
	                                                                                                    \
template<typename S1, typename S2>                                                                      \
void ll_convert_units(LLUnit<S1, unit_name> in, LLUnit<S2, base_unit_name>& out)                        \
{                                                                                                       \
	out = LLUnit<S2, base_unit_name>((S2)(LLUnitLinearOps<S1, S2>(in.value()) conversion_operation));		\
}                                                                                                       \
                                                                                                        \
template<typename S1, typename S2>                                                                      \
void ll_convert_units(LLUnit<S1, base_unit_name> in, LLUnit<S2, unit_name>& out)                        \
{                                                                                                       \
	out = LLUnit<S2, unit_name>((S2)(LLUnitInverseLinearOps<S1, S2>(in.value()) conversion_operation));     \
}                                                                                               

//
// Unit declarations
//

namespace LLUnits
{
LL_DECLARE_BASE_UNIT(Bytes, "B");
LL_DECLARE_DERIVED_UNIT(Bytes, * 1000,			Kilobytes, "KB");
LL_DECLARE_DERIVED_UNIT(Kilobytes, * 1000,		Megabytes, "MB");
LL_DECLARE_DERIVED_UNIT(Megabytes, * 1000,		Gigabytes, "GB");
LL_DECLARE_DERIVED_UNIT(Bytes, * 1024,			Kibibytes, "KiB");
LL_DECLARE_DERIVED_UNIT(Kibibytes, * 1024,		Mibibytes, "MiB");
LL_DECLARE_DERIVED_UNIT(Mibibytes, * 1024,		Gibibytes, "GiB");
}

typedef LLUnit<F32, LLUnits::Bytes>     F32Bytes;
typedef LLUnit<F32, LLUnits::Kilobytes> F32Kilobytes;
typedef LLUnit<F32, LLUnits::Megabytes> F32Megabytes;
typedef LLUnit<F32, LLUnits::Gigabytes> F32Gigabytes;
typedef LLUnit<F32, LLUnits::Kibibytes> F32Kibibytes;
typedef LLUnit<F32, LLUnits::Mibibytes> F32Mibibytes;
typedef LLUnit<F32, LLUnits::Gibibytes> F32Gibibytes;

typedef LLUnit<F64, LLUnits::Bytes>     F64Bytes;
typedef LLUnit<F64, LLUnits::Kilobytes> F64Kilobytes;
typedef LLUnit<F64, LLUnits::Megabytes> F64Megabytes;
typedef LLUnit<F64, LLUnits::Gigabytes> F64Gigabytes;
typedef LLUnit<F64, LLUnits::Kibibytes> F64Kibibytes;
typedef LLUnit<F64, LLUnits::Mibibytes> F64Mibibytes;
typedef LLUnit<F64, LLUnits::Gibibytes> F64Gibibytes;

typedef LLUnit<S32, LLUnits::Bytes>     S32Bytes;
typedef LLUnit<S32, LLUnits::Kilobytes> S32Kilobytes;
typedef LLUnit<S32, LLUnits::Megabytes> S32Megabytes;
typedef LLUnit<S32, LLUnits::Gigabytes> S32Gigabytes;
typedef LLUnit<S32, LLUnits::Kibibytes> S32Kibibytes;
typedef LLUnit<S32, LLUnits::Mibibytes> S32Mibibytes;
typedef LLUnit<S32, LLUnits::Gibibytes> S32Gibibytes;

typedef LLUnit<U32, LLUnits::Bytes>     U32Bytes;
typedef LLUnit<U32, LLUnits::Kilobytes> U32Kilobytes;
typedef LLUnit<U32, LLUnits::Megabytes> U32Megabytes;
typedef LLUnit<U32, LLUnits::Gigabytes> U32Gigabytes;
typedef LLUnit<U32, LLUnits::Kibibytes> U32Kibibytes;
typedef LLUnit<U32, LLUnits::Mibibytes> U32Mibibytes;
typedef LLUnit<U32, LLUnits::Gibibytes> U32Gibibytes;

typedef LLUnit<S64, LLUnits::Bytes>     S64Bytes;
typedef LLUnit<S64, LLUnits::Kilobytes> S64Kilobytes;
typedef LLUnit<S64, LLUnits::Megabytes> S64Megabytes;
typedef LLUnit<S64, LLUnits::Gigabytes> S64Gigabytes;
typedef LLUnit<S64, LLUnits::Kibibytes> S64Kibibytes;
typedef LLUnit<S64, LLUnits::Mibibytes> S64Mibibytes;
typedef LLUnit<S64, LLUnits::Gibibytes> S64Gibibytes;

typedef LLUnit<U64, LLUnits::Bytes>     U64Bytes;
typedef LLUnit<U64, LLUnits::Kilobytes> U64Kilobytes;
typedef LLUnit<U64, LLUnits::Megabytes> U64Megabytes;
typedef LLUnit<U64, LLUnits::Gigabytes> U64Gigabytes;
typedef LLUnit<U64, LLUnits::Kibibytes> U64Kibibytes;
typedef LLUnit<U64, LLUnits::Mibibytes> U64Mibibytes;
typedef LLUnit<U64, LLUnits::Gibibytes> U64Gibibytes;

namespace LLUnits
{
LL_DECLARE_DERIVED_UNIT(Bytes, / 8,				Bits, "b");
LL_DECLARE_DERIVED_UNIT(Bits, * 1000,			Kilobits, "Kb");
LL_DECLARE_DERIVED_UNIT(Kilobits, * 1000,		Megabits, "Mb");
LL_DECLARE_DERIVED_UNIT(Megabits, * 1000,		Gigabits, "Gb");
LL_DECLARE_DERIVED_UNIT(Bits, * 1024,			Kibibits, "Kib");
LL_DECLARE_DERIVED_UNIT(Kibibits, * 1024,		Mibibits, "Mib");  
LL_DECLARE_DERIVED_UNIT(Mibibits, * 1024,		Gibibits, "Gib");
}

typedef LLUnit<F32, LLUnits::Bits>     F32Bits;
typedef LLUnit<F32, LLUnits::Kilobits> F32Kilobits;
typedef LLUnit<F32, LLUnits::Megabits> F32Megabits;
typedef LLUnit<F32, LLUnits::Gigabits> F32Gigabits;
typedef LLUnit<F32, LLUnits::Kibibits> F32Kibibits;
typedef LLUnit<F32, LLUnits::Mibibits> F32Mibibits;
typedef LLUnit<F32, LLUnits::Gibibits> F32Gibibits;
					
typedef LLUnit<F64, LLUnits::Bits>     F64Bits;
typedef LLUnit<F64, LLUnits::Kilobits> F64Kilobits;
typedef LLUnit<F64, LLUnits::Megabits> F64Megabits;
typedef LLUnit<F64, LLUnits::Gigabits> F64Gigabits;
typedef LLUnit<F64, LLUnits::Kibibits> F64Kibibits;
typedef LLUnit<F64, LLUnits::Mibibits> F64Mibibits;
typedef LLUnit<F64, LLUnits::Gibibits> F64Gibibits;

typedef LLUnit<S32, LLUnits::Bits>     S32Bits;
typedef LLUnit<S32, LLUnits::Kilobits> S32Kilobits;
typedef LLUnit<S32, LLUnits::Megabits> S32Megabits;
typedef LLUnit<S32, LLUnits::Gigabits> S32Gigabits;
typedef LLUnit<S32, LLUnits::Kibibits> S32Kibibits;
typedef LLUnit<S32, LLUnits::Mibibits> S32Mibibits;
typedef LLUnit<S32, LLUnits::Gibibits> S32Gibibits;

typedef LLUnit<U32, LLUnits::Bits>     U32Bits;
typedef LLUnit<U32, LLUnits::Kilobits> U32Kilobits;
typedef LLUnit<U32, LLUnits::Megabits> U32Megabits;
typedef LLUnit<U32, LLUnits::Gigabits> U32Gigabits;
typedef LLUnit<U32, LLUnits::Kibibits> U32Kibibits;
typedef LLUnit<U32, LLUnits::Mibibits> U32Mibibits;
typedef LLUnit<U32, LLUnits::Gibibits> U32Gibibits;

typedef LLUnit<S64, LLUnits::Bits>     S64Bits;
typedef LLUnit<S64, LLUnits::Kilobits> S64Kilobits;
typedef LLUnit<S64, LLUnits::Megabits> S64Megabits;
typedef LLUnit<S64, LLUnits::Gigabits> S64Gigabits;
typedef LLUnit<S64, LLUnits::Kibibits> S64Kibibits;
typedef LLUnit<S64, LLUnits::Mibibits> S64Mibibits;
typedef LLUnit<S64, LLUnits::Gibibits> S64Gibibits;

typedef LLUnit<U64, LLUnits::Bits>     U64Bits;
typedef LLUnit<U64, LLUnits::Kilobits> U64Kilobits;
typedef LLUnit<U64, LLUnits::Megabits> U64Megabits;
typedef LLUnit<U64, LLUnits::Gigabits> U64Gigabits;
typedef LLUnit<U64, LLUnits::Kibibits> U64Kibibits;
typedef LLUnit<U64, LLUnits::Mibibits> U64Mibibits;
typedef LLUnit<U64, LLUnits::Gibibits> U64Gibibits;

namespace LLUnits
{
LL_DECLARE_BASE_UNIT(Seconds, "s");
LL_DECLARE_DERIVED_UNIT(Seconds, * 60,			Minutes, "min");
LL_DECLARE_DERIVED_UNIT(Minutes, * 60,			Hours, "h");
LL_DECLARE_DERIVED_UNIT(Hours, * 24,			Days, "d");
LL_DECLARE_DERIVED_UNIT(Seconds, / 1000,		Milliseconds, "ms");
LL_DECLARE_DERIVED_UNIT(Milliseconds, / 1000,	Microseconds, "\x09\x3cs");
LL_DECLARE_DERIVED_UNIT(Microseconds, / 1000,	Nanoseconds, "ns");
}

typedef LLUnit<F32, LLUnits::Seconds>      F32Seconds;
typedef LLUnit<F32, LLUnits::Minutes>      F32Minutes;
typedef LLUnit<F32, LLUnits::Hours>        F32Hours;
typedef LLUnit<F32, LLUnits::Days>         F32Days;
typedef LLUnit<F32, LLUnits::Milliseconds> F32Milliseconds;
typedef LLUnit<F32, LLUnits::Microseconds> F32Microseconds;
typedef LLUnit<F32, LLUnits::Nanoseconds>  F32Nanoseconds;

typedef LLUnit<F64, LLUnits::Seconds>      F64Seconds;
typedef LLUnit<F64, LLUnits::Minutes>      F64Minutes;
typedef LLUnit<F64, LLUnits::Hours>        F64Hours;
typedef LLUnit<F64, LLUnits::Days>         F64Days;
typedef LLUnit<F64, LLUnits::Milliseconds> F64Milliseconds;
typedef LLUnit<F64, LLUnits::Microseconds> F64Microseconds;
typedef LLUnit<F64, LLUnits::Nanoseconds>  F64Nanoseconds;

typedef LLUnit<S32, LLUnits::Seconds>      S32Seconds;
typedef LLUnit<S32, LLUnits::Minutes>      S32Minutes;
typedef LLUnit<S32, LLUnits::Hours>        S32Hours;
typedef LLUnit<S32, LLUnits::Days>         S32Days;
typedef LLUnit<S32, LLUnits::Milliseconds> S32Milliseconds;
typedef LLUnit<S32, LLUnits::Microseconds> S32Microseconds;
typedef LLUnit<S32, LLUnits::Nanoseconds>  S32Nanoseconds;

typedef LLUnit<U32, LLUnits::Seconds>      U32Seconds;
typedef LLUnit<U32, LLUnits::Minutes>      U32Minutes;
typedef LLUnit<U32, LLUnits::Hours>        U32Hours;
typedef LLUnit<U32, LLUnits::Days>         U32Days;
typedef LLUnit<U32, LLUnits::Milliseconds> U32Milliseconds;
typedef LLUnit<U32, LLUnits::Microseconds> U32Microseconds;
typedef LLUnit<U32, LLUnits::Nanoseconds>  U32Nanoseconds;

typedef LLUnit<S64, LLUnits::Seconds>      S64Seconds;
typedef LLUnit<S64, LLUnits::Minutes>      S64Minutes;
typedef LLUnit<S64, LLUnits::Hours>        S64Hours;
typedef LLUnit<S64, LLUnits::Days>         S64Days;
typedef LLUnit<S64, LLUnits::Milliseconds> S64Milliseconds;
typedef LLUnit<S64, LLUnits::Microseconds> S64Microseconds;
typedef LLUnit<S64, LLUnits::Nanoseconds>  S64Nanoseconds;
					
typedef LLUnit<U64, LLUnits::Seconds>      U64Seconds;
typedef LLUnit<U64, LLUnits::Minutes>      U64Minutes;
typedef LLUnit<U64, LLUnits::Hours>        U64Hours;
typedef LLUnit<U64, LLUnits::Days>         U64Days;
typedef LLUnit<U64, LLUnits::Milliseconds> U64Milliseconds;
typedef LLUnit<U64, LLUnits::Microseconds> U64Microseconds;
typedef LLUnit<U64, LLUnits::Nanoseconds>  U64Nanoseconds;

namespace LLUnits
{
LL_DECLARE_BASE_UNIT(Meters, "m");
LL_DECLARE_DERIVED_UNIT(Meters, * 1000,			Kilometers, "km");
LL_DECLARE_DERIVED_UNIT(Meters, / 100,			Centimeters, "cm");
LL_DECLARE_DERIVED_UNIT(Meters, / 1000,			Millimeters, "mm");
}

typedef LLUnit<F32, LLUnits::Meters>      F32Meters;
typedef LLUnit<F32, LLUnits::Kilometers>  F32Kilometers;
typedef LLUnit<F32, LLUnits::Centimeters> F32Centimeters;
typedef LLUnit<F32, LLUnits::Millimeters> F32Millimeters;

typedef LLUnit<F64, LLUnits::Meters>      F64Meters;
typedef LLUnit<F64, LLUnits::Kilometers>  F64Kilometers;
typedef LLUnit<F64, LLUnits::Centimeters> F64Centimeters;
typedef LLUnit<F64, LLUnits::Millimeters> F64Millimeters;

typedef LLUnit<S32, LLUnits::Meters>      S32Meters;
typedef LLUnit<S32, LLUnits::Kilometers>  S32Kilometers;
typedef LLUnit<S32, LLUnits::Centimeters> S32Centimeters;
typedef LLUnit<S32, LLUnits::Millimeters> S32Millimeters;

typedef LLUnit<U32, LLUnits::Meters>      U32Meters;
typedef LLUnit<U32, LLUnits::Kilometers>  U32Kilometers;
typedef LLUnit<U32, LLUnits::Centimeters> U32Centimeters;
typedef LLUnit<U32, LLUnits::Millimeters> U32Millimeters;

typedef LLUnit<S64, LLUnits::Meters>      S64Meters;
typedef LLUnit<S64, LLUnits::Kilometers>  S64Kilometers;
typedef LLUnit<S64, LLUnits::Centimeters> S64Centimeters;
typedef LLUnit<S64, LLUnits::Millimeters> S64Millimeters;

typedef LLUnit<U64, LLUnits::Meters>      U64Meters;
typedef LLUnit<U64, LLUnits::Kilometers>  U64Kilometers;
typedef LLUnit<U64, LLUnits::Centimeters> U64Centimeters;
typedef LLUnit<U64, LLUnits::Millimeters> U64Millimeters;

namespace LLUnits
{
// rare units
LL_DECLARE_BASE_UNIT(Hertz, "Hz");
LL_DECLARE_DERIVED_UNIT(Hertz, * 1000,			Kilohertz, "KHz");
LL_DECLARE_DERIVED_UNIT(Kilohertz, * 1000,		Megahertz, "MHz");
LL_DECLARE_DERIVED_UNIT(Megahertz, * 1000,		Gigahertz, "GHz");

LL_DECLARE_BASE_UNIT(Radians, "rad");
LL_DECLARE_DERIVED_UNIT(Radians, / 57.29578f,	Degrees, "deg");

LL_DECLARE_BASE_UNIT(Percent, "%");
LL_DECLARE_DERIVED_UNIT(Percent, * 100,			Ratio, "x");

LL_DECLARE_BASE_UNIT(Triangles, "tris");
LL_DECLARE_DERIVED_UNIT(Triangles, * 1000,		Kilotriangles, "ktris");

} // namespace LLUnits

#endif // LL_LLUNIT_H
