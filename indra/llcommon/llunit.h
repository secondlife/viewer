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

//lightweight replacement of type traits for simple type equality check
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

// workaround for decltype() not existing and typeof() not working inline in gcc 4.2
template<typename S, typename T>
struct LLResultType
{
	typedef LL_TYPEOF(S() + T()) add_t;
	typedef LL_TYPEOF(S() - T()) subtract_t;
	typedef LL_TYPEOF(S() * T()) multiply_t;
	typedef LL_TYPEOF(S() / T(1)) divide_t;
};

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

	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	void operator += (LLUnit<OTHER_STORAGE, OTHER_UNIT> other)
	{
		mValue += convert(other).mValue;
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
        unit.value(val);
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

	// unlike LLUnit, LLUnitImplicit is *implicitly* convertable to a POD value (F32, S32, etc)
	// this allows for interoperability with legacy code
	operator storage_t() const
	{
		return base_t::value();
	}

	using base_t::operator +=;
	void operator += (storage_t value)
	{
        base_t::mValue += value;
	}

	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	void operator += (LLUnitImplicit<OTHER_STORAGE, OTHER_UNIT> other)
	{
        base_t::mValue += convert(other).value();
	}

	using base_t::operator -=;
	void operator -= (storage_t value)
	{
        base_t::mValue -= value;
	}

	template<typename OTHER_STORAGE, typename OTHER_UNIT>
	void operator -= (LLUnitImplicit<OTHER_STORAGE, OTHER_UNIT> other)
	{
        base_t::mValue -= convert(other).value();
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

template<typename S1, typename T1, typename S2, typename T2>
LL_FORCE_INLINE void ll_convert_units(LLUnit<S1, T1> in, LLUnit<S2, T2>& out, ...)
{
	LL_STATIC_ASSERT((LLIsSameType<T1, T2>::value 
		|| !LLIsSameType<T1, typename T1::base_unit_t>::value 
		|| !LLIsSameType<T2, typename T2::base_unit_t>::value), "invalid conversion: incompatible units");

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
LLUnit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::add_t, UNIT_TYPE1> operator + (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::add_t, UNIT_TYPE1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator + (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS second)
{
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "operator + requires compatible unit types");
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>(0);
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator + (UNITLESS first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "operator + requires compatible unit types");
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>(0);
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::add_t, UNIT_TYPE1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::add_t, UNIT_TYPE1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::add_t, UNIT_TYPE1> operator + (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::add_t, UNIT_TYPE1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::add_t, UNIT_TYPE1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::add_t, UNIT_TYPE1> result(first);
	result += LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>(second);
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::add_t, UNIT_TYPE> operator + (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	LLUnitImplicit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::add_t, UNIT_TYPE> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultType<UNITLESS_TYPE, STORAGE_TYPE>::add_t, UNIT_TYPE> operator + (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LLUnitImplicit<typename LLResultType<UNITLESS_TYPE, STORAGE_TYPE>::add_t, UNIT_TYPE> result(first);
	result += second;
	return result;
}

//
// operator -
//
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::subtract_t, UNIT_TYPE1> operator - (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::subtract_t, UNIT_TYPE1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator - (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS second)
{
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "operator - requires compatible unit types");
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>(0);
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS>
LLUnit<STORAGE_TYPE, UNIT_TYPE> operator - (UNITLESS first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "operator - requires compatible unit types");
	return LLUnit<STORAGE_TYPE, UNIT_TYPE>(0);
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::subtract_t, UNIT_TYPE1> operator - (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::subtract_t, UNIT_TYPE1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::subtract_t, UNIT_TYPE1> operator - (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::subtract_t, UNIT_TYPE1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::subtract_t, UNIT_TYPE1> operator - (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::subtract_t, UNIT_TYPE1> result(first);
	result -= LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>(second);
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::subtract_t, UNIT_TYPE> operator - (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	LLUnitImplicit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::subtract_t, UNIT_TYPE> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultType<UNITLESS_TYPE, STORAGE_TYPE>::subtract_t, UNIT_TYPE> operator - (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LLUnitImplicit<typename LLResultType<UNITLESS_TYPE, STORAGE_TYPE>::subtract_t, UNIT_TYPE> result(first);
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
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE1, "multiplication of unit types results in new unit type - not supported.");
	return LLUnit<STORAGE_TYPE1, UNIT_TYPE1>();
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::multiply_t, UNIT_TYPE> operator * (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	return LLUnit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::multiply_t, UNIT_TYPE>(first.value() * second);
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnit<typename LLResultType<UNITLESS_TYPE, STORAGE_TYPE>::multiply_t, UNIT_TYPE> operator * (UNITLESS_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return LLUnit<typename LLResultType<UNITLESS_TYPE, STORAGE_TYPE>::multiply_t, UNIT_TYPE>(first * second.value());
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator * (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE1, "multiplication of unit types results in new unit type - not supported.");
	return LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>();
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::multiply_t, UNIT_TYPE> operator * (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	return LLUnitImplicit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::multiply_t, UNIT_TYPE>(first.value() * second);
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultType<UNITLESS_TYPE, STORAGE_TYPE>::multiply_t, UNIT_TYPE> operator * (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return LLUnitImplicit<typename LLResultType<UNITLESS_TYPE, STORAGE_TYPE>::multiply_t, UNIT_TYPE>(first * second.value());
}


//
// operator /
//

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::divide_t, UNIT_TYPE> operator / (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	return LLUnit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::divide_t, UNIT_TYPE>(first.value() / second);
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::divide_t operator / (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return first.value() / first.convert(second).value();
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::divide_t, UNIT_TYPE> operator / (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	return LLUnitImplicit<typename LLResultType<STORAGE_TYPE, UNITLESS_TYPE>::divide_t, UNIT_TYPE>(first.value() / second);
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::divide_t operator / (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return (typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::divide_t)(first.value() / first.convert(second).value());
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::divide_t operator / (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return (typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::divide_t)(first.value() / first.convert(second).value());
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::divide_t operator / (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return (typename LLResultType<STORAGE_TYPE1, STORAGE_TYPE2>::divide_t)(first.value() / first.convert(second).value());
}

//
// comparison operators
//

#define LL_UNIT_DECLARE_COMPARISON_OPERATOR(op)                                                                            \
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>                         \
bool operator op (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)       \
{                                                                                                                          \
	return first.value() op first.convert(second).value();                                                                 \
}                                                                                                                          \
	                                                                                                                       \
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>                                                \
bool operator op (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)                                     \
{                                                                                                                          \
	return first.value() op second;                                                                                        \
}                                                                                                                          \
                                                                                                                           \
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>                                                \
bool operator op (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)                                     \
{                                                                                                                          \
	return first op second.value();                                                                                        \
}                                                                                                                          \
                                                                                                                           \
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>                         \
bool operator op (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)                       \
{                                                                                                                          \
	return first.value() op first.convert(second).value();                                                                 \
}                                                                                                                          \
                                                                                                                           \
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>                                                \
bool operator op (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)                                             \
{                                                                                                                          \
	LL_BAD_TEMPLATE_INSTANTIATION(UNITLESS_TYPE, "operator " #op " requires compatible unit types");                       \
	return false;                                                                                                          \
}                                                                                                                          \
                                                                                                                           \
template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>                                                \
bool operator op (UNITLESS_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)                                             \
{                                                                                                                          \
	LL_BAD_TEMPLATE_INSTANTIATION(UNITLESS_TYPE, "operator " #op " requires compatible unit types");                       \
	return false;                                                                                                          \
}                                                                                                                          \
	                                                                                                                       \
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>                         \
bool operator op (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)               \
{                                                                                                                          \
	return first.value() op first.convert(second).value();                                                                 \
}                                                                                                                          \
                                                                                                                           \
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>                         \
bool operator op (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)               \
{                                                                                                                          \
	return first.value() op first.convert(second).value();                                                                 \
}

LL_UNIT_DECLARE_COMPARISON_OPERATOR(<);
LL_UNIT_DECLARE_COMPARISON_OPERATOR(<=);
LL_UNIT_DECLARE_COMPARISON_OPERATOR(>);
LL_UNIT_DECLARE_COMPARISON_OPERATOR(>=);
LL_UNIT_DECLARE_COMPARISON_OPERATOR(==);
LL_UNIT_DECLARE_COMPARISON_OPERATOR(!=);


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
		return typename LLResultType<INPUT_TYPE, OUTPUT_TYPE>::multiply_t(mInput) * other;
	}

	template<typename T>
	output_t operator / (T other)
	{
		return typename LLResultType<INPUT_TYPE, OUTPUT_TYPE>::divide_t(mInput) / other;
	}

	template<typename T>
	output_t operator + (T other)
	{
		return typename LLResultType<INPUT_TYPE, OUTPUT_TYPE>::add_t(mInput) + other;
	}

	template<typename T>
	output_t operator - (T other)
	{
		return typename LLResultType<INPUT_TYPE, OUTPUT_TYPE>::subtract_t(mInput) - other;
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
		return typename LLResultType<INPUT_TYPE, OUTPUT_TYPE>::divide_t(mInput) / other;
	}

	template<typename T>
	output_t operator / (T other)
	{
		return typename LLResultType<INPUT_TYPE, OUTPUT_TYPE>::multiply_t(mInput) * other;
	}

	template<typename T>
	output_t operator + (T other)
	{
		return typename LLResultType<INPUT_TYPE, OUTPUT_TYPE>::subtract_t(mInput) - other;
	}

	template<typename T>
	output_t operator - (T other)
	{
		return typename LLResultType<INPUT_TYPE, OUTPUT_TYPE>::add_t(mInput) + other;
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
	out = LLUnit<S2, base_unit_name>((S2)(LLUnitLinearOps<S1, S2>(in.value()) conversion_operation));	\
}                                                                                                       \
                                                                                                        \
template<typename S1, typename S2>                                                                      \
void ll_convert_units(LLUnit<S1, base_unit_name> in, LLUnit<S2, unit_name>& out)                        \
{                                                                                                       \
	out = LLUnit<S2, unit_name>((S2)(LLUnitInverseLinearOps<S1, S2>(in.value()) conversion_operation)); \
}                                                                                               

//
// Unit declarations
//

namespace LLUnits
{
LL_DECLARE_BASE_UNIT(Bytes, "B");
// technically, these are kibibytes, mibibytes, etc. but we should stick with commonly accepted terminology
LL_DECLARE_DERIVED_UNIT(Bytes, * 1024,			Kilobytes, "KB");
LL_DECLARE_DERIVED_UNIT(Kilobytes, * 1024,		Megabytes, "MB");
LL_DECLARE_DERIVED_UNIT(Megabytes, * 1024,		Gigabytes, "GB");
}

typedef LLUnit<F32, LLUnits::Bytes>     F32Bytes;
typedef LLUnit<F32, LLUnits::Kilobytes> F32Kilobytes;
typedef LLUnit<F32, LLUnits::Megabytes> F32Megabytes;
typedef LLUnit<F32, LLUnits::Gigabytes> F32Gigabytes;

typedef LLUnitImplicit<F32, LLUnits::Bytes>     F32BytesImplicit;
typedef LLUnitImplicit<F32, LLUnits::Kilobytes> F32KilobytesImplicit;
typedef LLUnitImplicit<F32, LLUnits::Megabytes> F32MegabytesImplicit;
typedef LLUnitImplicit<F32, LLUnits::Gigabytes> F32GigabytesImplicit;

typedef LLUnit<F64, LLUnits::Bytes>     F64Bytes;
typedef LLUnit<F64, LLUnits::Kilobytes> F64Kilobytes;
typedef LLUnit<F64, LLUnits::Megabytes> F64Megabytes;
typedef LLUnit<F64, LLUnits::Gigabytes> F64Gigabytes;

typedef LLUnitImplicit<F64, LLUnits::Bytes>     F64BytesImplicit;
typedef LLUnitImplicit<F64, LLUnits::Kilobytes> F64KilobytesImplicit;
typedef LLUnitImplicit<F64, LLUnits::Megabytes> F64MegabytesImplicit;
typedef LLUnitImplicit<F64, LLUnits::Gigabytes> F64GigabytesImplicit;

typedef LLUnit<S32, LLUnits::Bytes>     S32Bytes;
typedef LLUnit<S32, LLUnits::Kilobytes> S32Kilobytes;
typedef LLUnit<S32, LLUnits::Megabytes> S32Megabytes;
typedef LLUnit<S32, LLUnits::Gigabytes> S32Gigabytes;

typedef LLUnitImplicit<S32, LLUnits::Bytes>     S32BytesImplicit;
typedef LLUnitImplicit<S32, LLUnits::Kilobytes> S32KilobytesImplicit;
typedef LLUnitImplicit<S32, LLUnits::Megabytes> S32MegabytesImplicit;
typedef LLUnitImplicit<S32, LLUnits::Gigabytes> S32GigabytesImplicit;

typedef LLUnit<S64, LLUnits::Bytes>     S64Bytes;
typedef LLUnit<S64, LLUnits::Kilobytes> S64Kilobytes;
typedef LLUnit<S64, LLUnits::Megabytes> S64Megabytes;
typedef LLUnit<S64, LLUnits::Gigabytes> S64Gigabytes;

typedef LLUnitImplicit<S64, LLUnits::Bytes>     S64BytesImplicit;
typedef LLUnitImplicit<S64, LLUnits::Kilobytes> S64KilobytesImplicit;
typedef LLUnitImplicit<S64, LLUnits::Megabytes> S64MegabytesImplicit;
typedef LLUnitImplicit<S64, LLUnits::Gigabytes> S64GigabytesImplicit;

typedef LLUnit<U32, LLUnits::Bytes>     U32Bytes;
typedef LLUnit<U32, LLUnits::Kilobytes> U32Kilobytes;
typedef LLUnit<U32, LLUnits::Megabytes> U32Megabytes;
typedef LLUnit<U32, LLUnits::Gigabytes> U32Gigabytes;

typedef LLUnitImplicit<U32, LLUnits::Bytes>     U32BytesImplicit;
typedef LLUnitImplicit<U32, LLUnits::Kilobytes> U32KilobytesImplicit;
typedef LLUnitImplicit<U32, LLUnits::Megabytes> U32MegabytesImplicit;
typedef LLUnitImplicit<U32, LLUnits::Gigabytes> U32GigabytesImplicit;

typedef LLUnit<U64, LLUnits::Bytes>     U64Bytes;
typedef LLUnit<U64, LLUnits::Kilobytes> U64Kilobytes;
typedef LLUnit<U64, LLUnits::Megabytes> U64Megabytes;
typedef LLUnit<U64, LLUnits::Gigabytes> U64Gigabytes;

typedef LLUnitImplicit<U64, LLUnits::Bytes>     U64BytesImplicit;
typedef LLUnitImplicit<U64, LLUnits::Kilobytes> U64KilobytesImplicit;
typedef LLUnitImplicit<U64, LLUnits::Megabytes> U64MegabytesImplicit;
typedef LLUnitImplicit<U64, LLUnits::Gigabytes> U64GigabytesImplicit;

namespace LLUnits
{
// technically, these are kibibits, mibibits, etc. but we should stick with commonly accepted terminology
LL_DECLARE_DERIVED_UNIT(Bytes, / 8,				Bits, "b");
LL_DECLARE_DERIVED_UNIT(Bits, * 1024,			Kilobits, "Kb");
LL_DECLARE_DERIVED_UNIT(Kilobits, * 1024,		Megabits, "Mb");  
LL_DECLARE_DERIVED_UNIT(Megabits, * 1024,		Gigabits, "Gb");
}

typedef LLUnit<F32, LLUnits::Bits>     F32Bits;
typedef LLUnit<F32, LLUnits::Kilobits> F32Kilobits;
typedef LLUnit<F32, LLUnits::Megabits> F32Megabits;
typedef LLUnit<F32, LLUnits::Gigabits> F32Gigabits;

typedef LLUnitImplicit<F32, LLUnits::Bits>     F32BitsImplicit;
typedef LLUnitImplicit<F32, LLUnits::Kilobits> F32KilobitsImplicit;
typedef LLUnitImplicit<F32, LLUnits::Megabits> F32MegabitsImplicit;
typedef LLUnitImplicit<F32, LLUnits::Gigabits> F32GigabitsImplicit;

typedef LLUnit<F64, LLUnits::Bits>     F64Bits;
typedef LLUnit<F64, LLUnits::Kilobits> F64Kilobits;
typedef LLUnit<F64, LLUnits::Megabits> F64Megabits;
typedef LLUnit<F64, LLUnits::Gigabits> F64Gigabits;

typedef LLUnitImplicit<F64, LLUnits::Bits>     F64BitsImplicit;
typedef LLUnitImplicit<F64, LLUnits::Kilobits> F64KilobitsImplicit;
typedef LLUnitImplicit<F64, LLUnits::Megabits> F64MegabitsImplicit;
typedef LLUnitImplicit<F64, LLUnits::Gigabits> F64GigabitsImplicit;

typedef LLUnit<S32, LLUnits::Bits>     S32Bits;
typedef LLUnit<S32, LLUnits::Kilobits> S32Kilobits;
typedef LLUnit<S32, LLUnits::Megabits> S32Megabits;
typedef LLUnit<S32, LLUnits::Gigabits> S32Gigabits;

typedef LLUnitImplicit<S32, LLUnits::Bits>     S32BitsImplicit;
typedef LLUnitImplicit<S32, LLUnits::Kilobits> S32KilobitsImplicit;
typedef LLUnitImplicit<S32, LLUnits::Megabits> S32MegabitsImplicit;
typedef LLUnitImplicit<S32, LLUnits::Gigabits> S32GigabitsImplicit;

typedef LLUnit<S64, LLUnits::Bits>     S64Bits;
typedef LLUnit<S64, LLUnits::Kilobits> S64Kilobits;
typedef LLUnit<S64, LLUnits::Megabits> S64Megabits;
typedef LLUnit<S64, LLUnits::Gigabits> S64Gigabits;

typedef LLUnitImplicit<S64, LLUnits::Bits>     S64BitsImplicit;
typedef LLUnitImplicit<S64, LLUnits::Kilobits> S64KilobitsImplicit;
typedef LLUnitImplicit<S64, LLUnits::Megabits> S64MegabitsImplicit;
typedef LLUnitImplicit<S64, LLUnits::Gigabits> S64GigabitsImplicit;

typedef LLUnit<U32, LLUnits::Bits>     U32Bits;
typedef LLUnit<U32, LLUnits::Kilobits> U32Kilobits;
typedef LLUnit<U32, LLUnits::Megabits> U32Megabits;
typedef LLUnit<U32, LLUnits::Gigabits> U32Gigabits;

typedef LLUnitImplicit<U32, LLUnits::Bits>     U32BitsImplicit;
typedef LLUnitImplicit<U32, LLUnits::Kilobits> U32KilobitsImplicit;
typedef LLUnitImplicit<U32, LLUnits::Megabits> U32MegabitsImplicit;
typedef LLUnitImplicit<U32, LLUnits::Gigabits> U32GigabitsImplicit;

typedef LLUnit<U64, LLUnits::Bits>     U64Bits;
typedef LLUnit<U64, LLUnits::Kilobits> U64Kilobits;
typedef LLUnit<U64, LLUnits::Megabits> U64Megabits;
typedef LLUnit<U64, LLUnits::Gigabits> U64Gigabits;

typedef LLUnitImplicit<U64, LLUnits::Bits>     U64BitsImplicit;
typedef LLUnitImplicit<U64, LLUnits::Kilobits> U64KilobitsImplicit;
typedef LLUnitImplicit<U64, LLUnits::Megabits> U64MegabitsImplicit;
typedef LLUnitImplicit<U64, LLUnits::Gigabits> U64GigabitsImplicit;

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

typedef LLUnitImplicit<F32, LLUnits::Seconds>      F32SecondsImplicit;
typedef LLUnitImplicit<F32, LLUnits::Minutes>      F32MinutesImplicit;
typedef LLUnitImplicit<F32, LLUnits::Hours>        F32HoursImplicit;
typedef LLUnitImplicit<F32, LLUnits::Days>         F32DaysImplicit;
typedef LLUnitImplicit<F32, LLUnits::Milliseconds> F32MillisecondsImplicit;
typedef LLUnitImplicit<F32, LLUnits::Microseconds> F32MicrosecondsImplicit;
typedef LLUnitImplicit<F32, LLUnits::Nanoseconds>  F32NanosecondsImplicit;

typedef LLUnit<F64, LLUnits::Seconds>      F64Seconds;
typedef LLUnit<F64, LLUnits::Minutes>      F64Minutes;
typedef LLUnit<F64, LLUnits::Hours>        F64Hours;
typedef LLUnit<F64, LLUnits::Days>         F64Days;
typedef LLUnit<F64, LLUnits::Milliseconds> F64Milliseconds;
typedef LLUnit<F64, LLUnits::Microseconds> F64Microseconds;
typedef LLUnit<F64, LLUnits::Nanoseconds>  F64Nanoseconds;

typedef LLUnitImplicit<F64, LLUnits::Seconds>      F64SecondsImplicit;
typedef LLUnitImplicit<F64, LLUnits::Minutes>      F64MinutesImplicit;
typedef LLUnitImplicit<F64, LLUnits::Hours>        F64HoursImplicit;
typedef LLUnitImplicit<F64, LLUnits::Days>         F64DaysImplicit;
typedef LLUnitImplicit<F64, LLUnits::Milliseconds> F64MillisecondsImplicit;
typedef LLUnitImplicit<F64, LLUnits::Microseconds> F64MicrosecondsImplicit;
typedef LLUnitImplicit<F64, LLUnits::Nanoseconds>  F64NanosecondsImplicit;

typedef LLUnit<S32, LLUnits::Seconds>      S32Seconds;
typedef LLUnit<S32, LLUnits::Minutes>      S32Minutes;
typedef LLUnit<S32, LLUnits::Hours>        S32Hours;
typedef LLUnit<S32, LLUnits::Days>         S32Days;
typedef LLUnit<S32, LLUnits::Milliseconds> S32Milliseconds;
typedef LLUnit<S32, LLUnits::Microseconds> S32Microseconds;
typedef LLUnit<S32, LLUnits::Nanoseconds>  S32Nanoseconds;

typedef LLUnitImplicit<S32, LLUnits::Seconds>      S32SecondsImplicit;
typedef LLUnitImplicit<S32, LLUnits::Minutes>      S32MinutesImplicit;
typedef LLUnitImplicit<S32, LLUnits::Hours>        S32HoursImplicit;
typedef LLUnitImplicit<S32, LLUnits::Days>         S32DaysImplicit;
typedef LLUnitImplicit<S32, LLUnits::Milliseconds> S32MillisecondsImplicit;
typedef LLUnitImplicit<S32, LLUnits::Microseconds> S32MicrosecondsImplicit;
typedef LLUnitImplicit<S32, LLUnits::Nanoseconds>  S32NanosecondsImplicit;

typedef LLUnit<S64, LLUnits::Seconds>      S64Seconds;
typedef LLUnit<S64, LLUnits::Minutes>      S64Minutes;
typedef LLUnit<S64, LLUnits::Hours>        S64Hours;
typedef LLUnit<S64, LLUnits::Days>         S64Days;
typedef LLUnit<S64, LLUnits::Milliseconds> S64Milliseconds;
typedef LLUnit<S64, LLUnits::Microseconds> S64Microseconds;
typedef LLUnit<S64, LLUnits::Nanoseconds>  S64Nanoseconds;

typedef LLUnitImplicit<S64, LLUnits::Seconds>      S64SecondsImplicit;
typedef LLUnitImplicit<S64, LLUnits::Minutes>      S64MinutesImplicit;
typedef LLUnitImplicit<S64, LLUnits::Hours>        S64HoursImplicit;
typedef LLUnitImplicit<S64, LLUnits::Days>         S64DaysImplicit;
typedef LLUnitImplicit<S64, LLUnits::Milliseconds> S64MillisecondsImplicit;
typedef LLUnitImplicit<S64, LLUnits::Microseconds> S64MicrosecondsImplicit;
typedef LLUnitImplicit<S64, LLUnits::Nanoseconds>  S64NanosecondsImplicit;

typedef LLUnit<U32, LLUnits::Seconds>      U32Seconds;
typedef LLUnit<U32, LLUnits::Minutes>      U32Minutes;
typedef LLUnit<U32, LLUnits::Hours>        U32Hours;
typedef LLUnit<U32, LLUnits::Days>         U32Days;
typedef LLUnit<U32, LLUnits::Milliseconds> U32Milliseconds;
typedef LLUnit<U32, LLUnits::Microseconds> U32Microseconds;
typedef LLUnit<U32, LLUnits::Nanoseconds>  U32Nanoseconds;

typedef LLUnitImplicit<U32, LLUnits::Seconds>      U32SecondsImplicit;
typedef LLUnitImplicit<U32, LLUnits::Minutes>      U32MinutesImplicit;
typedef LLUnitImplicit<U32, LLUnits::Hours>        U32HoursImplicit;
typedef LLUnitImplicit<U32, LLUnits::Days>         U32DaysImplicit;
typedef LLUnitImplicit<U32, LLUnits::Milliseconds> U32MillisecondsImplicit;
typedef LLUnitImplicit<U32, LLUnits::Microseconds> U32MicrosecondsImplicit;
typedef LLUnitImplicit<U32, LLUnits::Nanoseconds>  U32NanosecondsImplicit;

typedef LLUnit<U64, LLUnits::Seconds>      U64Seconds;
typedef LLUnit<U64, LLUnits::Minutes>      U64Minutes;
typedef LLUnit<U64, LLUnits::Hours>        U64Hours;
typedef LLUnit<U64, LLUnits::Days>         U64Days;
typedef LLUnit<U64, LLUnits::Milliseconds> U64Milliseconds;
typedef LLUnit<U64, LLUnits::Microseconds> U64Microseconds;
typedef LLUnit<U64, LLUnits::Nanoseconds>  U64Nanoseconds;

typedef LLUnitImplicit<U64, LLUnits::Seconds>      U64SecondsImplicit;
typedef LLUnitImplicit<U64, LLUnits::Minutes>      U64MinutesImplicit;
typedef LLUnitImplicit<U64, LLUnits::Hours>        U64HoursImplicit;
typedef LLUnitImplicit<U64, LLUnits::Days>         U64DaysImplicit;
typedef LLUnitImplicit<U64, LLUnits::Milliseconds> U64MillisecondsImplicit;
typedef LLUnitImplicit<U64, LLUnits::Microseconds> U64MicrosecondsImplicit;
typedef LLUnitImplicit<U64, LLUnits::Nanoseconds>  U64NanosecondsImplicit;

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

typedef LLUnitImplicit<F32, LLUnits::Meters>      F32MetersImplicit;
typedef LLUnitImplicit<F32, LLUnits::Kilometers>  F32KilometersImplicit;
typedef LLUnitImplicit<F32, LLUnits::Centimeters> F32CentimetersImplicit;
typedef LLUnitImplicit<F32, LLUnits::Millimeters> F32MillimetersImplicit;

typedef LLUnit<F64, LLUnits::Meters>      F64Meters;
typedef LLUnit<F64, LLUnits::Kilometers>  F64Kilometers;
typedef LLUnit<F64, LLUnits::Centimeters> F64Centimeters;
typedef LLUnit<F64, LLUnits::Millimeters> F64Millimeters;

typedef LLUnitImplicit<F64, LLUnits::Meters>      F64MetersImplicit;
typedef LLUnitImplicit<F64, LLUnits::Kilometers>  F64KilometersImplicit;
typedef LLUnitImplicit<F64, LLUnits::Centimeters> F64CentimetersImplicit;
typedef LLUnitImplicit<F64, LLUnits::Millimeters> F64MillimetersImplicit;

typedef LLUnit<S32, LLUnits::Meters>      S32Meters;
typedef LLUnit<S32, LLUnits::Kilometers>  S32Kilometers;
typedef LLUnit<S32, LLUnits::Centimeters> S32Centimeters;
typedef LLUnit<S32, LLUnits::Millimeters> S32Millimeters;

typedef LLUnitImplicit<S32, LLUnits::Meters>      S32MetersImplicit;
typedef LLUnitImplicit<S32, LLUnits::Kilometers>  S32KilometersImplicit;
typedef LLUnitImplicit<S32, LLUnits::Centimeters> S32CentimetersImplicit;
typedef LLUnitImplicit<S32, LLUnits::Millimeters> S32MillimetersImplicit;

typedef LLUnit<S64, LLUnits::Meters>      S64Meters;
typedef LLUnit<S64, LLUnits::Kilometers>  S64Kilometers;
typedef LLUnit<S64, LLUnits::Centimeters> S64Centimeters;
typedef LLUnit<S64, LLUnits::Millimeters> S64Millimeters;

typedef LLUnitImplicit<S64, LLUnits::Meters>      S64MetersImplicit;
typedef LLUnitImplicit<S64, LLUnits::Kilometers>  S64KilometersImplicit;
typedef LLUnitImplicit<S64, LLUnits::Centimeters> S64CentimetersImplicit;
typedef LLUnitImplicit<S64, LLUnits::Millimeters> S64MillimetersImplicit;

typedef LLUnit<U32, LLUnits::Meters>      U32Meters;
typedef LLUnit<U32, LLUnits::Kilometers>  U32Kilometers;
typedef LLUnit<U32, LLUnits::Centimeters> U32Centimeters;
typedef LLUnit<U32, LLUnits::Millimeters> U32Millimeters;

typedef LLUnitImplicit<U32, LLUnits::Meters>      U32MetersImplicit;
typedef LLUnitImplicit<U32, LLUnits::Kilometers>  U32KilometersImplicit;
typedef LLUnitImplicit<U32, LLUnits::Centimeters> U32CentimetersImplicit;
typedef LLUnitImplicit<U32, LLUnits::Millimeters> U32MillimetersImplicit;

typedef LLUnit<U64, LLUnits::Meters>      U64Meters;
typedef LLUnit<U64, LLUnits::Kilometers>  U64Kilometers;
typedef LLUnit<U64, LLUnits::Centimeters> U64Centimeters;
typedef LLUnit<U64, LLUnits::Millimeters> U64Millimeters;

typedef LLUnitImplicit<U64, LLUnits::Meters>      U64MetersImplicit;
typedef LLUnitImplicit<U64, LLUnits::Kilometers>  U64KilometersImplicit;
typedef LLUnitImplicit<U64, LLUnits::Centimeters> U64CentimetersImplicit;
typedef LLUnitImplicit<U64, LLUnits::Millimeters> U64MillimetersImplicit;

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

// rare units
typedef LLUnit<F32, LLUnits::Hertz>			F32Hertz;
typedef LLUnit<F32, LLUnits::Kilohertz>		F32Kilohertz;
typedef LLUnit<F32, LLUnits::Megahertz>		F32Megahertz;
typedef LLUnit<F32, LLUnits::Gigahertz>		F32Gigahertz;
typedef LLUnit<F32, LLUnits::Radians>		F32Radians;
typedef LLUnit<F32, LLUnits::Degrees>		F32Degrees;
typedef LLUnit<F32, LLUnits::Percent>		F32Percent;
typedef LLUnit<F32, LLUnits::Ratio>			F32Ratio;
typedef LLUnit<F32, LLUnits::Triangles>		F32Triangles;
typedef LLUnit<F32, LLUnits::Kilotriangles>	F32KiloTriangles;

typedef LLUnitImplicit<F32, LLUnits::Hertz>			F32HertzImplicit;
typedef LLUnitImplicit<F32, LLUnits::Kilohertz>		F32KilohertzImplicit;
typedef LLUnitImplicit<F32, LLUnits::Megahertz>		F32MegahertzImplicit;
typedef LLUnitImplicit<F32, LLUnits::Gigahertz>		F32GigahertzImplicit;
typedef LLUnitImplicit<F32, LLUnits::Radians>		F32RadiansImplicit;
typedef LLUnitImplicit<F32, LLUnits::Degrees>		F32DegreesImplicit;
typedef LLUnitImplicit<F32, LLUnits::Percent>		F32PercentImplicit;
typedef LLUnitImplicit<F32, LLUnits::Ratio>			F32RatioImplicit;
typedef LLUnitImplicit<F32, LLUnits::Triangles>		F32TrianglesImplicit;
typedef LLUnitImplicit<F32, LLUnits::Kilotriangles>	F32KiloTrianglesImplicit;

typedef LLUnit<F64, LLUnits::Hertz>			F64Hertz;
typedef LLUnit<F64, LLUnits::Kilohertz>		F64Kilohertz;
typedef LLUnit<F64, LLUnits::Megahertz>		F64Megahertz;
typedef LLUnit<F64, LLUnits::Gigahertz>		F64Gigahertz;
typedef LLUnit<F64, LLUnits::Radians>		F64Radians;
typedef LLUnit<F64, LLUnits::Degrees>		F64Degrees;
typedef LLUnit<F64, LLUnits::Percent>		F64Percent;
typedef LLUnit<F64, LLUnits::Ratio>			F64Ratio;
typedef LLUnit<F64, LLUnits::Triangles>		F64Triangles;
typedef LLUnit<F64, LLUnits::Kilotriangles>	F64KiloTriangles;

typedef LLUnitImplicit<F64, LLUnits::Hertz>			F64HertzImplicit;
typedef LLUnitImplicit<F64, LLUnits::Kilohertz>		F64KilohertzImplicit;
typedef LLUnitImplicit<F64, LLUnits::Megahertz>		F64MegahertzImplicit;
typedef LLUnitImplicit<F64, LLUnits::Gigahertz>		F64GigahertzImplicit;
typedef LLUnitImplicit<F64, LLUnits::Radians>		F64RadiansImplicit;
typedef LLUnitImplicit<F64, LLUnits::Degrees>		F64DegreesImplicit;
typedef LLUnitImplicit<F64, LLUnits::Percent>		F64PercentImplicit;
typedef LLUnitImplicit<F64, LLUnits::Ratio>			F64RatioImplicit;
typedef LLUnitImplicit<F64, LLUnits::Triangles>		F64TrianglesImplicit;
typedef LLUnitImplicit<F64, LLUnits::Kilotriangles>	F64KiloTrianglesImplicit;

typedef LLUnit<S32, LLUnits::Hertz>			S32Hertz;
typedef LLUnit<S32, LLUnits::Kilohertz>		S32Kilohertz;
typedef LLUnit<S32, LLUnits::Megahertz>		S32Megahertz;
typedef LLUnit<S32, LLUnits::Gigahertz>		S32Gigahertz;
typedef LLUnit<S32, LLUnits::Radians>		S32Radians;
typedef LLUnit<S32, LLUnits::Degrees>		S32Degrees;
typedef LLUnit<S32, LLUnits::Percent>		S32Percent;
typedef LLUnit<S32, LLUnits::Ratio>			S32Ratio;
typedef LLUnit<S32, LLUnits::Triangles>		S32Triangles;
typedef LLUnit<S32, LLUnits::Kilotriangles>	S32KiloTriangles;

typedef LLUnitImplicit<S32, LLUnits::Hertz>			S32HertzImplicit;
typedef LLUnitImplicit<S32, LLUnits::Kilohertz>		S32KilohertzImplicit;
typedef LLUnitImplicit<S32, LLUnits::Megahertz>		S32MegahertzImplicit;
typedef LLUnitImplicit<S32, LLUnits::Gigahertz>		S32GigahertzImplicit;
typedef LLUnitImplicit<S32, LLUnits::Radians>		S32RadiansImplicit;
typedef LLUnitImplicit<S32, LLUnits::Degrees>		S32DegreesImplicit;
typedef LLUnitImplicit<S32, LLUnits::Percent>		S32PercentImplicit;
typedef LLUnitImplicit<S32, LLUnits::Ratio>			S32RatioImplicit;
typedef LLUnitImplicit<S32, LLUnits::Triangles>		S32TrianglesImplicit;
typedef LLUnitImplicit<S32, LLUnits::Kilotriangles>	S32KiloTrianglesImplicit;

typedef LLUnit<S64, LLUnits::Hertz>			S64Hertz;
typedef LLUnit<S64, LLUnits::Kilohertz>		S64Kilohertz;
typedef LLUnit<S64, LLUnits::Megahertz>		S64Megahertz;
typedef LLUnit<S64, LLUnits::Gigahertz>		S64Gigahertz;
typedef LLUnit<S64, LLUnits::Radians>		S64Radians;
typedef LLUnit<S64, LLUnits::Degrees>		S64Degrees;
typedef LLUnit<S64, LLUnits::Percent>		S64Percent;
typedef LLUnit<S64, LLUnits::Ratio>			S64Ratio;
typedef LLUnit<S64, LLUnits::Triangles>		S64Triangles;
typedef LLUnit<S64, LLUnits::Kilotriangles>	S64KiloTriangles;

typedef LLUnitImplicit<S64, LLUnits::Hertz>			S64HertzImplicit;
typedef LLUnitImplicit<S64, LLUnits::Kilohertz>		S64KilohertzImplicit;
typedef LLUnitImplicit<S64, LLUnits::Megahertz>		S64MegahertzImplicit;
typedef LLUnitImplicit<S64, LLUnits::Gigahertz>		S64GigahertzImplicit;
typedef LLUnitImplicit<S64, LLUnits::Radians>		S64RadiansImplicit;
typedef LLUnitImplicit<S64, LLUnits::Degrees>		S64DegreesImplicit;
typedef LLUnitImplicit<S64, LLUnits::Percent>		S64PercentImplicit;
typedef LLUnitImplicit<S64, LLUnits::Ratio>			S64RatioImplicit;
typedef LLUnitImplicit<S64, LLUnits::Triangles>		S64TrianglesImplicit;
typedef LLUnitImplicit<S64, LLUnits::Kilotriangles>	S64KiloTrianglesImplicit;

typedef LLUnit<U32, LLUnits::Hertz>			U32Hertz;
typedef LLUnit<U32, LLUnits::Kilohertz>		U32Kilohertz;
typedef LLUnit<U32, LLUnits::Megahertz>		U32Megahertz;
typedef LLUnit<U32, LLUnits::Gigahertz>		U32Gigahertz;
typedef LLUnit<U32, LLUnits::Radians>		U32Radians;
typedef LLUnit<U32, LLUnits::Degrees>		U32Degrees;
typedef LLUnit<U32, LLUnits::Percent>		U32Percent;
typedef LLUnit<U32, LLUnits::Ratio>			U32Ratio;
typedef LLUnit<U32, LLUnits::Triangles>		U32Triangles;
typedef LLUnit<U32, LLUnits::Kilotriangles>	U32KiloTriangles;

typedef LLUnitImplicit<U32, LLUnits::Hertz>			U32HertzImplicit;
typedef LLUnitImplicit<U32, LLUnits::Kilohertz>		U32KilohertzImplicit;
typedef LLUnitImplicit<U32, LLUnits::Megahertz>		U32MegahertzImplicit;
typedef LLUnitImplicit<U32, LLUnits::Gigahertz>		U32GigahertzImplicit;
typedef LLUnitImplicit<U32, LLUnits::Radians>		U32RadiansImplicit;
typedef LLUnitImplicit<U32, LLUnits::Degrees>		U32DegreesImplicit;
typedef LLUnitImplicit<U32, LLUnits::Percent>		U32PercentImplicit;
typedef LLUnitImplicit<U32, LLUnits::Ratio>			U32RatioImplicit;
typedef LLUnitImplicit<U32, LLUnits::Triangles>		U32TrianglesImplicit;
typedef LLUnitImplicit<U32, LLUnits::Kilotriangles>	U32KiloTrianglesImplicit;

typedef LLUnit<U64, LLUnits::Hertz>			U64Hertz;
typedef LLUnit<U64, LLUnits::Kilohertz>		U64Kilohertz;
typedef LLUnit<U64, LLUnits::Megahertz>		U64Megahertz;
typedef LLUnit<U64, LLUnits::Gigahertz>		U64Gigahertz;
typedef LLUnit<U64, LLUnits::Radians>		U64Radians;
typedef LLUnit<U64, LLUnits::Degrees>		U64Degrees;
typedef LLUnit<U64, LLUnits::Percent>		U64Percent;
typedef LLUnit<U64, LLUnits::Ratio>			U64Ratio;
typedef LLUnit<U64, LLUnits::Triangles>		U64Triangles;
typedef LLUnit<U64, LLUnits::Kilotriangles>	U64KiloTriangles;

typedef LLUnitImplicit<U64, LLUnits::Hertz>			U64HertzImplicit;
typedef LLUnitImplicit<U64, LLUnits::Kilohertz>		U64KilohertzImplicit;
typedef LLUnitImplicit<U64, LLUnits::Megahertz>		U64MegahertzImplicit;
typedef LLUnitImplicit<U64, LLUnits::Gigahertz>		U64GigahertzImplicit;
typedef LLUnitImplicit<U64, LLUnits::Radians>		U64RadiansImplicit;
typedef LLUnitImplicit<U64, LLUnits::Degrees>		U64DegreesImplicit;
typedef LLUnitImplicit<U64, LLUnits::Percent>		U64PercentImplicit;
typedef LLUnitImplicit<U64, LLUnits::Ratio>			U64RatioImplicit;
typedef LLUnitImplicit<U64, LLUnits::Triangles>		U64TrianglesImplicit;
typedef LLUnitImplicit<U64, LLUnits::Kilotriangles>	U64KiloTrianglesImplicit;

#endif // LL_LLUNIT_H
