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
struct LLResultTypeAdd
{
	typedef LL_TYPEOF(S() + T()) type_t;
};

template<typename S, typename T>
struct LLResultTypeSubtract
{
	typedef LL_TYPEOF(S() - T()) type_t;
};

template<typename S, typename T>
struct LLResultTypeMultiply
{
	typedef LL_TYPEOF(S() * T()) type_t;
};

template<typename S, typename T>
struct LLResultTypeDivide
{
	typedef LL_TYPEOF(S() / T(1)) type_t;
};

template<typename S, typename T>
struct LLResultTypePromote
{
	typedef LL_TYPEOF((true) ? S() : T()) type_t;
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
		typedef typename LLResultTypePromote<STORAGE_TYPE, SOURCE_STORAGE>::type_t result_storage_t;
		LLUnit<result_storage_t, UNIT_TYPE> result;
		result_storage_t divisor = ll_convert_units(v, result);
		result.value(result.value() / divisor);
		return self_t(result.value());
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
	:	base_t(other)
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

	// this overload exists to explicitly catch use of another implicit unit
	// without ambiguity between conversion to storage_t vs conversion to base_t
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

	// this overload exists to explicitly catch use of another implicit unit
	// without ambiguity between conversion to storage_t vs conversion to base_t
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
LL_FORCE_INLINE S2 ll_convert_units(LLUnit<S1, T1> in, LLUnit<S2, T2>& out, ...)
{
	S2 divisor(1);

	LL_STATIC_ASSERT((LLIsSameType<T1, T2>::value 
						|| !LLIsSameType<T1, typename T1::base_unit_t>::value 
						|| !LLIsSameType<T2, typename T2::base_unit_t>::value), 
						"conversion requires compatible units");

	if (LLIsSameType<T1, T2>::value)
	{
		// T1 and T2 same type, just assign
		out.value((S2)in.value());
	}
	else if (LLIsSameType<T2, typename T2::base_unit_t>::value)
	{
		// reduce T1
		LLUnit<S2, typename T1::base_unit_t> new_in;
		divisor *= (S2)ll_convert_units(in, new_in);
		divisor *= (S2)ll_convert_units(new_in, out);
	}
	else
	{
		// reduce T2
		LLUnit<S2, typename T2::base_unit_t> new_out;
		divisor *= (S2)ll_convert_units(in, new_out);
		divisor *= (S2)ll_convert_units(new_out, out);
	}
	return divisor;
}

template<typename T>
struct LLStorageType
{
	typedef T type_t;
};

template<typename STORAGE_TYPE, typename UNIT_TYPE>
struct LLStorageType<LLUnit<STORAGE_TYPE, UNIT_TYPE> >
{
	typedef STORAGE_TYPE type_t;
};

//
// operator +
//
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> operator + (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> result(first);
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
LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> operator + (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> result(first);
	result += LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>(second);
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE> operator + (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultTypeAdd<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::
	type_t, UNIT_TYPE> operator + (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LLUnitImplicit<typename LLResultTypeAdd<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNIT_TYPE> result(first);
	result += second;
	return result;
}

//
// operator -
//
template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> operator - (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> result(first);
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
LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> operator - (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> operator - (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> operator - (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNIT_TYPE1> result(first);
	result -= LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>(second);
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE> operator - (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultTypeSubtract<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNIT_TYPE> operator - (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNIT_TYPE> result(first);
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
LLUnit<typename LLResultTypeMultiply<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE> operator * (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	return LLUnit<typename LLResultTypeMultiply<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE>(first.value() * second);
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnit<typename LLResultTypeMultiply<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNIT_TYPE> operator * (UNITLESS_TYPE first, LLUnit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return LLUnit<typename LLResultTypeMultiply<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNIT_TYPE>(first * second.value());
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> operator * (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE1, "multiplication of unit types results in new unit type - not supported.");
	return LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1>();
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultTypeMultiply<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE> operator * (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	return LLUnitImplicit<typename LLResultTypeMultiply<STORAGE_TYPE, UNITLESS_TYPE>::type_t, UNIT_TYPE>(first.value() * second);
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultTypeMultiply<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNIT_TYPE> operator * (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> second)
{
	return LLUnitImplicit<typename LLResultTypeMultiply<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNIT_TYPE>(first * second.value());
}


//
// operator /
//

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnit<typename LLResultTypeDivide<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE> operator / (LLUnit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	return LLUnit<typename LLResultTypeDivide<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE>(first.value() / second);
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t operator / (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return first.value() / first.convert(second).value();
}

template<typename STORAGE_TYPE, typename UNIT_TYPE, typename UNITLESS_TYPE>
LLUnitImplicit<typename LLResultTypeDivide<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE> operator / (LLUnitImplicit<STORAGE_TYPE, UNIT_TYPE> first, UNITLESS_TYPE second)
{
	return LLUnitImplicit<typename LLResultTypeDivide<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNIT_TYPE>(first.value() / second);
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t operator / (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return (typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t)(first.value() / first.convert(second).value());
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t operator / (LLUnit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnitImplicit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return (typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t)(first.value() / first.convert(second).value());
}

template<typename STORAGE_TYPE1, typename UNIT_TYPE1, typename STORAGE_TYPE2, typename UNIT_TYPE2>
typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t operator / (LLUnitImplicit<STORAGE_TYPE1, UNIT_TYPE1> first, LLUnit<STORAGE_TYPE2, UNIT_TYPE2> second)
{
	return (typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t)(first.value() / first.convert(second).value());
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

template<typename T>
struct LLUnitLinearOps
{
	typedef LLUnitLinearOps<T> self_t;

	LLUnitLinearOps(T val) 
	:	mValue(val),
		mDivisor(1)
	{}

	template<typename OTHER_T>
	self_t operator * (OTHER_T other)
	{
		return mValue * other;
	}

	template<typename OTHER_T>
	self_t operator / (OTHER_T other)
	{
		mDivisor *= other;
		return *this;
	}

	template<typename OTHER_T>
	self_t operator + (OTHER_T other)
	{
		mValue /= mDivisor;
		mValue += other;
		return *this;
	}

	template<typename OTHER_T>
	self_t operator - (OTHER_T other)
	{
		mValue /= mDivisor;
		mValue -= other;
		return *this;
	}

	T mValue;
	T mDivisor;
};

template<typename T>
struct LLUnitInverseLinearOps
{
	typedef LLUnitInverseLinearOps<T> self_t;

	LLUnitInverseLinearOps(T val) 
	:	mValue(val),
		mDivisor(1)
	{}

	template<typename OTHER_T>
	self_t operator * (OTHER_T other)
	{
		mDivisor *= other;
		return *this;
	}

	template<typename OTHER_T>
	self_t operator / (OTHER_T other)
	{
		mValue *= other;
		return *this;
	}

	template<typename OTHER_T>
	self_t operator + (OTHER_T other)
	{
		mValue /= mDivisor;
		mValue -= other;
		return *this;
	}

	template<typename OTHER_T>
	self_t operator - (OTHER_T other)
	{
		mValue /= mDivisor;
		mValue += other;
		return *this;
	}

	T mValue;
	T mDivisor;
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


#define LL_DECLARE_DERIVED_UNIT(base_unit_name, conversion_operation, unit_name, unit_label)	 \
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
	                                                                                             \
template<typename S1, typename S2>                                                               \
S2 ll_convert_units(LLUnit<S1, unit_name> in, LLUnit<S2, base_unit_name>& out)                   \
{                                                                                                \
	typedef typename LLResultTypePromote<S1, S2>::type_t result_storage_t;                       \
	LLUnitLinearOps<result_storage_t> op =                                                       \
		LLUnitLinearOps<result_storage_t>(in.value()) conversion_operation;                      \
	out = LLUnit<S2, base_unit_name>((S2)op.mValue);	                                         \
	return op.mDivisor;                                                                          \
}                                                                                                \
                                                                                                 \
template<typename S1, typename S2>                                                               \
S2 ll_convert_units(LLUnit<S1, base_unit_name> in, LLUnit<S2, unit_name>& out)                   \
{                                                                                                \
	typedef typename LLResultTypePromote<S1, S2>::type_t result_storage_t;                       \
	LLUnitInverseLinearOps<result_storage_t> op =                                                \
		LLUnitInverseLinearOps<result_storage_t>(in.value()) conversion_operation;               \
	out = LLUnit<S2, unit_name>((S2)op.mValue);                                                  \
	return op.mDivisor;                                                                          \
}                                                                                               

#define LL_DECLARE_UNIT_TYPEDEFS(ns, unit_name)                         \
	typedef LLUnit<F32, ns::unit_name> F32##unit_name;                  \
	typedef LLUnitImplicit<F32, ns::unit_name> F32##unit_name##Implicit;\
	typedef LLUnit<F64, ns::unit_name> F64##unit_name;                  \
	typedef LLUnitImplicit<F64, ns::unit_name> F64##unit_name##Implicit;\
	typedef LLUnit<S32, ns::unit_name> S32##unit_name;                  \
	typedef LLUnitImplicit<S32, ns::unit_name> S32##unit_name##Implicit;\
	typedef LLUnit<S64, ns::unit_name> S64##unit_name;                  \
	typedef LLUnitImplicit<S64, ns::unit_name> S64##unit_name##Implicit;\
	typedef LLUnit<U32, ns::unit_name> U32##unit_name;                  \
	typedef LLUnitImplicit<U32, ns::unit_name> U32##unit_name##Implicit;\
	typedef LLUnit<U64, ns::unit_name> U64##unit_name;                  \
	typedef LLUnitImplicit<U64, ns::unit_name> U64##unit_name##Implicit

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

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Bytes);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Kilobytes);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Megabytes);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Gigabytes);

namespace LLUnits
{
// technically, these are kibibits, mibibits, etc. but we should stick with commonly accepted terminology
LL_DECLARE_DERIVED_UNIT(Bytes, / 8,				Bits, "b");
LL_DECLARE_DERIVED_UNIT(Bits, * 1024,			Kilobits, "Kb");
LL_DECLARE_DERIVED_UNIT(Kilobits, * 1024,		Megabits, "Mb");  
LL_DECLARE_DERIVED_UNIT(Megabits, * 1024,		Gigabits, "Gb");
}

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Bits);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Kilobits);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Megabits);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Gigabits);

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

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Seconds);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Minutes);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Hours);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Days);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Milliseconds);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Microseconds);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Nanoseconds);

namespace LLUnits
{
LL_DECLARE_BASE_UNIT(Meters, "m");
LL_DECLARE_DERIVED_UNIT(Meters, * 1000,			Kilometers, "km");
LL_DECLARE_DERIVED_UNIT(Meters, / 100,			Centimeters, "cm");
LL_DECLARE_DERIVED_UNIT(Meters, / 1000,			Millimeters, "mm");
}

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Meters);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Kilometers);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Centimeters);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Millimeters);

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
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Hertz);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Kilohertz);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Megahertz);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Gigahertz);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Radians);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Degrees);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Percent);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Ratio);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Triangles);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Kilotriangles);


#endif // LL_LLUNIT_H
