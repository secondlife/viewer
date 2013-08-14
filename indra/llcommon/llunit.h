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

typedef LLUnit<F32, Bytes>     F32Bytes;
typedef LLUnit<F32, Kilobytes> F32KiloBytes;
typedef LLUnit<F32, Megabytes> F32MegaBytes;
typedef LLUnit<F32, Gigabytes> F32GigaBytes;
typedef LLUnit<F32, Kibibytes> F32KibiBytes;
typedef LLUnit<F32, Mibibytes> F32MibiBytes;
typedef LLUnit<F32, Gibibytes> F32GibiBytes;

typedef LLUnit<F64, Bytes>     F64Bytes;
typedef LLUnit<F64, Kilobytes> F64KiloBytes;
typedef LLUnit<F64, Megabytes> F64MegaBytes;
typedef LLUnit<F64, Gigabytes> F64GigaBytes;
typedef LLUnit<F64, Kibibytes> F64KibiBytes;
typedef LLUnit<F64, Mibibytes> F64MibiBytes;
typedef LLUnit<F64, Gibibytes> F64GibiBytes;

typedef LLUnit<S32, Bytes>     S32Bytes;
typedef LLUnit<S32, Kilobytes> S32KiloBytes;
typedef LLUnit<S32, Megabytes> S32MegaBytes;
typedef LLUnit<S32, Gigabytes> S32GigaBytes;
typedef LLUnit<S32, Kibibytes> S32KibiBytes;
typedef LLUnit<S32, Mibibytes> S32MibiBytes;
typedef LLUnit<S32, Gibibytes> S32GibiBytes;

typedef LLUnit<U32, Bytes>     U32Bytes;
typedef LLUnit<U32, Kilobytes> U32KiloBytes;
typedef LLUnit<U32, Megabytes> U32MegaBytes;
typedef LLUnit<U32, Gigabytes> U32GigaBytes;
typedef LLUnit<U32, Kibibytes> U32KibiBytes;
typedef LLUnit<U32, Mibibytes> U32MibiBytes;
typedef LLUnit<U32, Gibibytes> U32GibiBytes;

typedef LLUnit<S64, Bytes>     S64Bytes;
typedef LLUnit<S64, Kilobytes> S64KiloBytes;
typedef LLUnit<S64, Megabytes> S64MegaBytes;
typedef LLUnit<S64, Gigabytes> S64GigaBytes;
typedef LLUnit<S64, Kibibytes> S64KibiBytes;
typedef LLUnit<S64, Mibibytes> S64MibiBytes;
typedef LLUnit<S64, Gibibytes> S64GibiBytes;

typedef LLUnit<U64, Bytes>     U64Bytes;
typedef LLUnit<U64, Kilobytes> U64KiloBytes;
typedef LLUnit<U64, Megabytes> U64MegaBytes;
typedef LLUnit<U64, Gigabytes> U64GigaBytes;
typedef LLUnit<U64, Kibibytes> U64KibiBytes;
typedef LLUnit<U64, Mibibytes> U64MibiBytes;
typedef LLUnit<U64, Gibibytes> U64GibiBytes;

LL_DECLARE_DERIVED_UNIT(Bytes, / 8,				Bits, "b");
LL_DECLARE_DERIVED_UNIT(Bits, * 1000,			Kilobits, "Kb");
LL_DECLARE_DERIVED_UNIT(Kilobits, * 1000,		Megabits, "Mb");
LL_DECLARE_DERIVED_UNIT(Megabits, * 1000,		Gigabits, "Gb");
LL_DECLARE_DERIVED_UNIT(Bits, * 1024,			Kibibits, "Kib");
LL_DECLARE_DERIVED_UNIT(Kibibits, * 1024,		Mibibits, "Mib");  
LL_DECLARE_DERIVED_UNIT(Mibibits, * 1024,		Gibibits, "Gib");

typedef LLUnit<F32, Bits>     F32Bits;
typedef LLUnit<F32, Kilobits> F32KiloBits;
typedef LLUnit<F32, Megabits> F32MegaBits;
typedef LLUnit<F32, Gigabits> F32GigaBits;
typedef LLUnit<F32, Kibibits> F32KibiBits;
typedef LLUnit<F32, Mibibits> F32MibiBits;
typedef LLUnit<F32, Gibibits> F32GibiBits;

typedef LLUnit<F64, Bits>     F64Bits;
typedef LLUnit<F64, Kilobits> F64KiloBits;
typedef LLUnit<F64, Megabits> F64MegaBits;
typedef LLUnit<F64, Gigabits> F64GigaBits;
typedef LLUnit<F64, Kibibits> F64KibiBits;
typedef LLUnit<F64, Mibibits> F64MibiBits;
typedef LLUnit<F64, Gibibits> F64GibiBits;

typedef LLUnit<S32, Bits>     S32Bits;
typedef LLUnit<S32, Kilobits> S32KiloBits;
typedef LLUnit<S32, Megabits> S32MegaBits;
typedef LLUnit<S32, Gigabits> S32GigaBits;
typedef LLUnit<S32, Kibibits> S32KibiBits;
typedef LLUnit<S32, Mibibits> S32MibiBits;
typedef LLUnit<S32, Gibibits> S32GibiBits;

typedef LLUnit<U32, Bits>     U32Bits;
typedef LLUnit<U32, Kilobits> U32KiloBits;
typedef LLUnit<U32, Megabits> U32MegaBits;
typedef LLUnit<U32, Gigabits> U32GigaBits;
typedef LLUnit<U32, Kibibits> U32KibiBits;
typedef LLUnit<U32, Mibibits> U32MibiBits;
typedef LLUnit<U32, Gibibits> U32GibiBits;

typedef LLUnit<S64, Bits>     S64Bits;
typedef LLUnit<S64, Kilobits> S64KiloBits;
typedef LLUnit<S64, Megabits> S64MegaBits;
typedef LLUnit<S64, Gigabits> S64GigaBits;
typedef LLUnit<S64, Kibibits> S64KibiBits;
typedef LLUnit<S64, Mibibits> S64MibiBits;
typedef LLUnit<S64, Gibibits> S64GibiBits;

typedef LLUnit<U64, Bits>     U64Bits;
typedef LLUnit<U64, Kilobits> U64KiloBits;
typedef LLUnit<U64, Megabits> U64MegaBits;
typedef LLUnit<U64, Gigabits> U64GigaBits;
typedef LLUnit<U64, Kibibits> U64KibiBits;
typedef LLUnit<U64, Mibibits> U64MibiBits;
typedef LLUnit<U64, Gibibits> U64GibiBits;

LL_DECLARE_BASE_UNIT(Seconds, "s");
LL_DECLARE_DERIVED_UNIT(Seconds, * 60,			Minutes, "min");
LL_DECLARE_DERIVED_UNIT(Minutes, * 60,			Hours, "h");
LL_DECLARE_DERIVED_UNIT(Hours, * 24,			Days, "d");
LL_DECLARE_DERIVED_UNIT(Seconds, / 1000,		Milliseconds, "ms");
LL_DECLARE_DERIVED_UNIT(Milliseconds, / 1000,	Microseconds, "\x09\x3cs");
LL_DECLARE_DERIVED_UNIT(Microseconds, / 1000,	Nanoseconds, "ns");

typedef LLUnit<F32, Seconds>      F32Seconds;
typedef LLUnit<F32, Minutes>      F32Minutes;
typedef LLUnit<F32, Hours>        F32Hours;
typedef LLUnit<F32, Days>         F32Days;
typedef LLUnit<F32, Milliseconds> F32Milliseconds;
typedef LLUnit<F32, Microseconds> F32Microseconds;
typedef LLUnit<F32, Nanoseconds>  F32Nanoseconds;

typedef LLUnit<F64, Seconds>      F64Seconds;
typedef LLUnit<F64, Minutes>      F64Minutes;
typedef LLUnit<F64, Hours>        F64Hours;
typedef LLUnit<F64, Days>         F64Days;
typedef LLUnit<F64, Milliseconds> F64Milliseconds;
typedef LLUnit<F64, Microseconds> F64Microseconds;
typedef LLUnit<F64, Nanoseconds>  F64Nanoseconds;

typedef LLUnit<S32, Seconds>      S32Seconds;
typedef LLUnit<S32, Minutes>      S32Minutes;
typedef LLUnit<S32, Hours>        S32Hours;
typedef LLUnit<S32, Days>         S32Days;
typedef LLUnit<S32, Milliseconds> S32Milliseconds;
typedef LLUnit<S32, Microseconds> S32Microseconds;
typedef LLUnit<S32, Nanoseconds>  S32Nanoseconds;

typedef LLUnit<U32, Seconds>      U32Seconds;
typedef LLUnit<U32, Minutes>      U32Minutes;
typedef LLUnit<U32, Hours>        U32Hours;
typedef LLUnit<U32, Days>         U32Days;
typedef LLUnit<U32, Milliseconds> U32Milliseconds;
typedef LLUnit<U32, Microseconds> U32Microseconds;
typedef LLUnit<U32, Nanoseconds>  U32Nanoseconds;

typedef LLUnit<S64, Seconds>      S64Seconds;
typedef LLUnit<S64, Minutes>      S64Minutes;
typedef LLUnit<S64, Hours>        S64Hours;
typedef LLUnit<S64, Days>         S64Days;
typedef LLUnit<S64, Milliseconds> S64Milliseconds;
typedef LLUnit<S64, Microseconds> S64Microseconds;
typedef LLUnit<S64, Nanoseconds>  S64Nanoseconds;

typedef LLUnit<U64, Seconds>      U64Seconds;
typedef LLUnit<U64, Minutes>      U64Minutes;
typedef LLUnit<U64, Hours>        U64Hours;
typedef LLUnit<U64, Days>         U64Days;
typedef LLUnit<U64, Milliseconds> U64Milliseconds;
typedef LLUnit<U64, Microseconds> U64Microseconds;
typedef LLUnit<U64, Nanoseconds>  U64Nanoseconds;

LL_DECLARE_BASE_UNIT(Meters, "m");
LL_DECLARE_DERIVED_UNIT(Meters, * 1000,			Kilometers, "km");
LL_DECLARE_DERIVED_UNIT(Meters, / 100,			Centimeters, "cm");
LL_DECLARE_DERIVED_UNIT(Meters, / 1000,			Millimeters, "mm");

typedef LLUnit<F32, Meters>      F32Meters;
typedef LLUnit<F32, Kilometers>  F32Kilometers;
typedef LLUnit<F32, Centimeters> F32Centimeters;
typedef LLUnit<F32, Millimeters> F32Millimeters;

typedef LLUnit<F64, Meters>      F64Meters;
typedef LLUnit<F64, Kilometers>  F64Kilometers;
typedef LLUnit<F64, Centimeters> F64Centimeters;
typedef LLUnit<F64, Millimeters> F64Millimeters;

typedef LLUnit<S32, Meters>      S32Meters;
typedef LLUnit<S32, Kilometers>  S32Kilometers;
typedef LLUnit<S32, Centimeters> S32Centimeters;
typedef LLUnit<S32, Millimeters> S32Millimeters;

typedef LLUnit<U32, Meters>      U32Meters;
typedef LLUnit<U32, Kilometers>  U32Kilometers;
typedef LLUnit<U32, Centimeters> U32Centimeters;
typedef LLUnit<U32, Millimeters> U32Millimeters;

typedef LLUnit<S64, Meters>      S64Meters;
typedef LLUnit<S64, Kilometers>  S64Kilometers;
typedef LLUnit<S64, Centimeters> S64Centimeters;
typedef LLUnit<S64, Millimeters> S64Millimeters;

typedef LLUnit<U64, Meters>      U64Meters;
typedef LLUnit<U64, Kilometers>  U64Kilometers;
typedef LLUnit<U64, Centimeters> U64Centimeters;
typedef LLUnit<U64, Millimeters> U64Millimeters;

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
