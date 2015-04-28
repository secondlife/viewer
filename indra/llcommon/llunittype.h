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

#ifndef LL_UNITTYPE_H
#define LL_UNITTYPE_H

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

template<typename STORAGE_TYPE, typename UNITS>
struct LLUnit
{
	typedef LLUnit<STORAGE_TYPE, UNITS> self_t;
	typedef STORAGE_TYPE storage_t;
	typedef void is_unit_t;

	// value initialization
	LL_FORCE_INLINE explicit LLUnit(storage_t value = storage_t())
	:	mValue(value)
	{}


	LL_FORCE_INLINE static self_t convert(self_t v) 
	{ 
		return v;
	}

	template<typename FROM_STORAGE_TYPE>
	LL_FORCE_INLINE static self_t convert(LLUnit<FROM_STORAGE_TYPE, UNITS> v) 
	{
		self_t result;
		result.mValue = (STORAGE_TYPE)v.value();
		return result;
	}

	template<typename FROM_UNITS>
	LL_FORCE_INLINE static self_t convert(LLUnit<STORAGE_TYPE, FROM_UNITS> v) 
	{
		self_t result;
		STORAGE_TYPE divisor = ll_convert_units(v, result);
		result.mValue /= divisor;
		return result;
	}

	template<typename FROM_STORAGE_TYPE, typename FROM_UNITS>
	LL_FORCE_INLINE static self_t convert(LLUnit<FROM_STORAGE_TYPE, FROM_UNITS> v) 
	{ 
		typedef typename LLResultTypePromote<FROM_STORAGE_TYPE, STORAGE_TYPE>::type_t result_storage_t;
		LLUnit<result_storage_t, UNITS> result;
		result_storage_t divisor = ll_convert_units(v, result);
		result.value(result.value() / divisor);
		return self_t(result.value());
	}


	// unit initialization and conversion
	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE LLUnit(LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other)
	:	mValue(convert(other).mValue)
	{}
	
	LL_FORCE_INLINE storage_t value() const
	{
		return mValue;
	}

	LL_FORCE_INLINE void value(storage_t value)
	{
		mValue = value;
	}

	template<typename NEW_UNITS> 
	storage_t valueInUnits()
	{
		return LLUnit<storage_t, NEW_UNITS>(*this).value();
	}

	template<typename NEW_UNITS> 
	void valueInUnits(storage_t value)
	{
		*this = LLUnit<storage_t, NEW_UNITS>(value);
	}

	LL_FORCE_INLINE void operator += (self_t other)
	{
		mValue += convert(other).mValue;
	}

	LL_FORCE_INLINE void operator -= (self_t other)
	{
		mValue -= convert(other).mValue;
	}

	LL_FORCE_INLINE void operator *= (storage_t multiplicand)
	{
		mValue *= multiplicand;
	}

	LL_FORCE_INLINE void operator *= (self_t multiplicand)
	{
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "Multiplication of unit types not supported.");
	}

	LL_FORCE_INLINE void operator /= (storage_t divisor)
	{
		mValue /= divisor;
	}

	void operator /= (self_t divisor)
	{
		// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
		LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "Illegal in-place division of unit types.");
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator == (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return mValue == convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator != (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return mValue != convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator < (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return mValue < convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator <= (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return mValue <= convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator > (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return mValue > convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator >= (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return mValue >= convert(other).value();
	}

protected:
	storage_t mValue;
};

template<typename STORAGE_TYPE, typename UNITS>
std::ostream& operator <<(std::ostream& s, const LLUnit<STORAGE_TYPE, UNITS>& unit)
{
        s << unit.value() << UNITS::getUnitLabel();
        return s;
}

template<typename STORAGE_TYPE, typename UNITS>
std::istream& operator >>(std::istream& s, LLUnit<STORAGE_TYPE, UNITS>& unit)
{
        STORAGE_TYPE val;
        s >> val;
        unit.value(val);
        return s;
}

template<typename STORAGE_TYPE, typename UNITS>
struct LLUnitImplicit : public LLUnit<STORAGE_TYPE, UNITS>
{
	typedef LLUnitImplicit<STORAGE_TYPE, UNITS> self_t;
	typedef typename LLUnit<STORAGE_TYPE, UNITS>::storage_t storage_t;
	typedef LLUnit<STORAGE_TYPE, UNITS> base_t;

	LL_FORCE_INLINE LLUnitImplicit(storage_t value = storage_t())
	:	base_t(value)
	{}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE LLUnitImplicit(LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other)
	:	base_t(other)
	{}

	// unlike LLUnit, LLUnitImplicit is *implicitly* convertable to a POD value (F32, S32, etc)
	// this allows for interoperability with legacy code
	LL_FORCE_INLINE operator storage_t() const
	{
		return base_t::value();
	}

	using base_t::operator +=;
	LL_FORCE_INLINE void operator += (storage_t value)
	{
        base_t::mValue += value;
	}

	// this overload exists to explicitly catch use of another implicit unit
	// without ambiguity between conversion to storage_t vs conversion to base_t
	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE void operator += (LLUnitImplicit<OTHER_STORAGE_TYPE, OTHER_UNITS> other)
	{
        base_t::mValue += base_t::convert(other).value();
	}

	using base_t::operator -=;
	LL_FORCE_INLINE void operator -= (storage_t value)
	{
        base_t::mValue -= value;
	}

	// this overload exists to explicitly catch use of another implicit unit
	// without ambiguity between conversion to storage_t vs conversion to base_t
	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE void operator -= (LLUnitImplicit<OTHER_STORAGE_TYPE, OTHER_UNITS> other)
	{
        base_t::mValue -= base_t::convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator == (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue == base_t::convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator == (LLUnitImplicit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue == base_t::convert(other).value();
	}

	template<typename STORAGE_T>
	LL_FORCE_INLINE bool operator == (STORAGE_T other) const
	{
		return base_t::mValue == other;
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator != (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue != convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator != (LLUnitImplicit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue != base_t::convert(other).value();
	}

	template<typename STORAGE_T>
	LL_FORCE_INLINE bool operator != (STORAGE_T other) const
	{
		return base_t::mValue != other;
	}
	
	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator < (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue < base_t::convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator < (LLUnitImplicit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue < base_t::convert(other).value();
	}

	template<typename STORAGE_T>
	LL_FORCE_INLINE bool operator < (STORAGE_T other) const
	{
		return base_t::mValue < other;
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator <= (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue <= base_t::convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator <= (LLUnitImplicit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue <= base_t::convert(other).value();
	}

	template<typename STORAGE_T>
	LL_FORCE_INLINE bool operator <= (STORAGE_T other) const
	{
		return base_t::mValue <= other;
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator > (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue > base_t::convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator > (LLUnitImplicit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue > base_t::convert(other).value();
	}

	template<typename STORAGE_T>
	LL_FORCE_INLINE bool operator > (STORAGE_T other) const
	{
		return base_t::mValue > other;
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator >= (LLUnit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue >= base_t::convert(other).value();
	}

	template<typename OTHER_STORAGE_TYPE, typename OTHER_UNITS>
	LL_FORCE_INLINE bool operator >= (LLUnitImplicit<OTHER_STORAGE_TYPE, OTHER_UNITS> other) const
	{
		return base_t::mValue >= base_t::convert(other).value();
	}

	template<typename STORAGE_T>
	LL_FORCE_INLINE bool operator >= (STORAGE_T other) const
	{
		return base_t::mValue >= other;
	}
};

template<typename STORAGE_TYPE, typename UNITS>
std::ostream& operator <<(std::ostream& s, const LLUnitImplicit<STORAGE_TYPE, UNITS>& unit)
{
        s << unit.value() << UNITS::getUnitLabel();
        return s;
}

template<typename STORAGE_TYPE, typename UNITS>
std::istream& operator >>(std::istream& s, LLUnitImplicit<STORAGE_TYPE, UNITS>& unit)
{
        STORAGE_TYPE val;
        s >> val;
        unit = val;
        return s;
}

template<typename S1, typename T1, typename S2, typename T2>
LL_FORCE_INLINE S2 ll_convert_units(LLUnit<S1, T1> in, LLUnit<S2, T2>& out)
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
	else if (T1::sLevel > T2::sLevel)
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

template<typename STORAGE_TYPE, typename UNITS>
struct LLStorageType<LLUnit<STORAGE_TYPE, UNITS> >
{
	typedef STORAGE_TYPE type_t;
};

// all of these operators need to perform type promotion on the storage type of the units, so they 
// cannot be expressed as operations on identical types with implicit unit conversion 
// e.g. typeof(S32Bytes(x) + F32Megabytes(y)) <==> F32Bytes

//
// operator +
//
template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE LLUnit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> operator + (LLUnit<STORAGE_TYPE1, UNITS1> first, LLUnit<STORAGE_TYPE2, UNITS2> second)
{
	LLUnit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS>
LLUnit<STORAGE_TYPE, UNITS> operator + (LLUnit<STORAGE_TYPE, UNITS> first, UNITLESS second)
{
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "operator + requires compatible unit types");
	return LLUnit<STORAGE_TYPE, UNITS>(0);
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS>
LLUnit<STORAGE_TYPE, UNITS> operator + (UNITLESS first, LLUnit<STORAGE_TYPE, UNITS> second)
{
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "operator + requires compatible unit types");
	return LLUnit<STORAGE_TYPE, UNITS>(0);
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNITS1> first, LLUnitImplicit<STORAGE_TYPE2, UNITS2> second)
{
	LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> operator + (LLUnit<STORAGE_TYPE1, UNITS1> first, LLUnitImplicit<STORAGE_TYPE2, UNITS2> second)
{
	LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> operator + (LLUnitImplicit<STORAGE_TYPE1, UNITS1> first, LLUnit<STORAGE_TYPE2, UNITS2> second)
{
	LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> result(first);
	result += LLUnitImplicit<STORAGE_TYPE1, UNITS1>(second);
	return result;
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS> operator + (LLUnitImplicit<STORAGE_TYPE, UNITS> first, UNITLESS_TYPE second)
{
	LLUnitImplicit<typename LLResultTypeAdd<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS> result(first);
	result += second;
	return result;
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeAdd<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::
	type_t, UNITS> operator + (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNITS> second)
{
	LLUnitImplicit<typename LLResultTypeAdd<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNITS> result(first);
	result += second;
	return result;
}

//
// operator -
//
template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE LLUnit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> operator - (LLUnit<STORAGE_TYPE1, UNITS1> first, LLUnit<STORAGE_TYPE2, UNITS2> second)
{
	LLUnit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS>
LLUnit<STORAGE_TYPE, UNITS> operator - (LLUnit<STORAGE_TYPE, UNITS> first, UNITLESS second)
{
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "operator - requires compatible unit types");
	return LLUnit<STORAGE_TYPE, UNITS>(0);
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS>
LLUnit<STORAGE_TYPE, UNITS> operator - (UNITLESS first, LLUnit<STORAGE_TYPE, UNITS> second)
{
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE, "operator - requires compatible unit types");
	return LLUnit<STORAGE_TYPE, UNITS>(0);
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> operator - (LLUnitImplicit<STORAGE_TYPE1, UNITS1> first, LLUnitImplicit<STORAGE_TYPE2, UNITS2> second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> operator - (LLUnit<STORAGE_TYPE1, UNITS1> first, LLUnitImplicit<STORAGE_TYPE2, UNITS2> second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> operator - (LLUnitImplicit<STORAGE_TYPE1, UNITS1> first, LLUnit<STORAGE_TYPE2, UNITS2> second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE1, STORAGE_TYPE2>::type_t, UNITS1> result(first);
	result -= LLUnitImplicit<STORAGE_TYPE1, UNITS1>(second);
	return result;
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS> operator - (LLUnitImplicit<STORAGE_TYPE, UNITS> first, UNITLESS_TYPE second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS> result(first);
	result -= second;
	return result;
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeSubtract<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNITS> operator - (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNITS> second)
{
	LLUnitImplicit<typename LLResultTypeSubtract<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNITS> result(first);
	result -= second;
	return result;
}

//
// operator *
//
template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LLUnit<STORAGE_TYPE1, UNITS1> operator * (LLUnit<STORAGE_TYPE1, UNITS1>, LLUnit<STORAGE_TYPE2, UNITS2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE1, "multiplication of unit types results in new unit type - not supported.");
	return LLUnit<STORAGE_TYPE1, UNITS1>();
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnit<typename LLResultTypeMultiply<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS> operator * (LLUnit<STORAGE_TYPE, UNITS> first, UNITLESS_TYPE second)
{
	return LLUnit<typename LLResultTypeMultiply<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS>(first.value() * second);
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnit<typename LLResultTypeMultiply<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNITS> operator * (UNITLESS_TYPE first, LLUnit<STORAGE_TYPE, UNITS> second)
{
	return LLUnit<typename LLResultTypeMultiply<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNITS>(first * second.value());
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LLUnitImplicit<STORAGE_TYPE1, UNITS1> operator * (LLUnitImplicit<STORAGE_TYPE1, UNITS1>, LLUnitImplicit<STORAGE_TYPE2, UNITS2>)
{
	// spurious use of dependent type to stop gcc from triggering the static assertion before instantiating the template
	LL_BAD_TEMPLATE_INSTANTIATION(STORAGE_TYPE1, "multiplication of unit types results in new unit type - not supported.");
	return LLUnitImplicit<STORAGE_TYPE1, UNITS1>();
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeMultiply<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS> operator * (LLUnitImplicit<STORAGE_TYPE, UNITS> first, UNITLESS_TYPE second)
{
	return LLUnitImplicit<typename LLResultTypeMultiply<STORAGE_TYPE, UNITLESS_TYPE>::type_t, UNITS>(first.value() * second);
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeMultiply<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNITS> operator * (UNITLESS_TYPE first, LLUnitImplicit<STORAGE_TYPE, UNITS> second)
{
	return LLUnitImplicit<typename LLResultTypeMultiply<typename LLStorageType<UNITLESS_TYPE>::type_t, STORAGE_TYPE>::type_t, UNITS>(first * second.value());
}


//
// operator /
//

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnit<typename LLResultTypeDivide<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS> operator / (LLUnit<STORAGE_TYPE, UNITS> first, UNITLESS_TYPE second)
{
	return LLUnit<typename LLResultTypeDivide<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS>(first.value() / second);
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t operator / (LLUnit<STORAGE_TYPE1, UNITS1> first, LLUnit<STORAGE_TYPE2, UNITS2> second)
{
	return first.value() / first.convert(second).value();
}

template<typename STORAGE_TYPE, typename UNITS, typename UNITLESS_TYPE>
LL_FORCE_INLINE LLUnitImplicit<typename LLResultTypeDivide<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS> operator / (LLUnitImplicit<STORAGE_TYPE, UNITS> first, UNITLESS_TYPE second)
{
	return LLUnitImplicit<typename LLResultTypeDivide<STORAGE_TYPE, typename LLStorageType<UNITLESS_TYPE>::type_t>::type_t, UNITS>(first.value() / second);
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t operator / (LLUnitImplicit<STORAGE_TYPE1, UNITS1> first, LLUnitImplicit<STORAGE_TYPE2, UNITS2> second)
{
	return (typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t)(first.value() / first.convert(second).value());
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t operator / (LLUnit<STORAGE_TYPE1, UNITS1> first, LLUnitImplicit<STORAGE_TYPE2, UNITS2> second)
{
	return (typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t)(first.value() / first.convert(second).value());
}

template<typename STORAGE_TYPE1, typename UNITS1, typename STORAGE_TYPE2, typename UNITS2>
LL_FORCE_INLINE typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t operator / (LLUnitImplicit<STORAGE_TYPE1, UNITS1> first, LLUnit<STORAGE_TYPE2, UNITS2> second)
{
	return (typename LLResultTypeDivide<STORAGE_TYPE1, STORAGE_TYPE2>::type_t)(first.value() / first.convert(second).value());
}

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
		mValue += other * mDivisor;
		return *this;
	}

	template<typename OTHER_T>
	self_t operator - (OTHER_T other)
	{
		mValue -= other * mDivisor;
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
		mDivisor(1),
		mMultiplicand(1)
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
		mMultiplicand *= other;
		return *this;
	}

	template<typename OTHER_T>
	self_t operator + (OTHER_T other)
	{
		mValue -= other * mMultiplicand;
		return *this;
	}

	template<typename OTHER_T>
	self_t operator - (OTHER_T other)
	{
		mValue += other * mMultiplicand;
		return *this;
	}

	T mValue;
	T mDivisor;
	T mMultiplicand;
};

#define LL_DECLARE_BASE_UNIT(base_unit_name, unit_label)                                             \
struct base_unit_name                                                                                \
{                                                                                                    \
	static const int sLevel = 0;                                                                     \
	typedef base_unit_name base_unit_t;                                                              \
	static const char* getUnitLabel() { return unit_label; }                                         \
	template<typename T>                                                                             \
	static LLUnit<T, base_unit_name> fromValue(T value) { return LLUnit<T, base_unit_name>(value); } \
	template<typename STORAGE_T, typename UNIT_T>                                                    \
	static LLUnit<STORAGE_T, base_unit_name> fromValue(LLUnit<STORAGE_T, UNIT_T> value)              \
	{ return LLUnit<STORAGE_T, base_unit_name>(value); }                                             \
}


#define LL_DECLARE_DERIVED_UNIT(unit_name, unit_label, base_unit_name, conversion_operation)	 \
struct unit_name                                                                                 \
{                                                                                                \
	static const int sLevel = base_unit_name::sLevel + 1;                                        \
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
LL_FORCE_INLINE S2 ll_convert_units(LLUnit<S1, unit_name> in, LLUnit<S2, base_unit_name>& out)   \
{                                                                                                \
	typedef typename LLResultTypePromote<S1, S2>::type_t result_storage_t;                       \
	LLUnitInverseLinearOps<result_storage_t> result =                                            \
		LLUnitInverseLinearOps<result_storage_t>(in.value()) conversion_operation;               \
	out = LLUnit<S2, base_unit_name>((S2)result.mValue);	                                     \
	return result.mDivisor;                                                                      \
}                                                                                                \
                                                                                                 \
template<typename S1, typename S2>                                                               \
LL_FORCE_INLINE S2 ll_convert_units(LLUnit<S1, base_unit_name> in, LLUnit<S2, unit_name>& out)   \
{                                                                                                \
	typedef typename LLResultTypePromote<S1, S2>::type_t result_storage_t;                       \
	LLUnitLinearOps<result_storage_t> result =                                                   \
		LLUnitLinearOps<result_storage_t>(in.value()) conversion_operation;				         \
	out = LLUnit<S2, unit_name>((S2)result.mValue);                                              \
	return result.mDivisor;                                                                      \
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

#endif //LL_UNITTYPE_H
