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

template<typename BASE_UNIT, typename DERIVED_UNIT = BASE_UNIT>
struct LLUnit : public BASE_UNIT
{
	typedef LLUnit<BASE_UNIT, DERIVED_UNIT> unit_t;
	typedef typename BASE_UNIT::value_t value_t;
	typedef void is_unit_t;

	LLUnit()
	{}

	explicit LLUnit(value_t value)
	:	BASE_UNIT(convertToBase(value))
	{}

	operator value_t() { return value(); }

	value_t value() const
	{
		return convertToDerived(mValue);
	}

	template<typename CONVERTED_TYPE>
	value_t value() const
	{
		return CONVERTED_TYPE(*this).value();
	}

	static value_t convertToBase(value_t derived_value)
	{
		return (value_t)((F32)derived_value * DERIVED_UNIT::conversionToBaseFactor());
	}

	static value_t convertToDerived(value_t base_value)
	{
		return (value_t)((F32)base_value / DERIVED_UNIT::conversionToBaseFactor());
	}

};

template<typename T>
struct LLUnit<T, T>
{
	typedef LLUnit<T, T> unit_t;
	typedef T value_t;
	typedef void is_unit_t;

	LLUnit()
	:	mValue()
	{}

	explicit LLUnit(T value)
	:	mValue(value)
	{}

	unit_t& operator=(T value)
	{
		setBaseValue(value);
		return *this;
	}

	value_t value() { return mValue; }

	static value_t convertToBase(value_t derived_value)
	{
		return (value_t)1;
	}

	static value_t convertToDerived(value_t base_value)
	{
		return (value_t)1;
	}

	unit_t operator + (const unit_t other) const
	{
		return unit_t(mValue + other.mValue);
	}

	void operator += (const unit_t other)
	{
		mValue += other.mValue;
	}

	unit_t operator - (const unit_t other) const
	{
		return unit_t(mValue - other.mValue);
	}

	void operator -= (const unit_t other)
	{
		mValue -= other.mValue;
	}

	unit_t operator * (value_t multiplicand) const
	{
		return unit_t(mValue * multiplicand);
	}

	void operator *= (value_t multiplicand)
	{
		mValue *= multiplicand;
	}

	unit_t operator / (value_t divisor) const
	{
		return unit_t(mValue / divisor);
	}

	void operator /= (value_t divisor)
	{
		mValue /= divisor;
	}

protected:
	void setBaseValue(T value)
	{
		mValue = value;
	}

	T mValue;
};

#define LL_DECLARE_BASE_UNIT(unit_name)                 \
	template<typename STORAGE_TYPE>                     \
	struct unit_name : public LLUnit<STORAGE_TYPE>      \
	{                                                   \
		unit_name(STORAGE_TYPE value)                   \
		:	LLUnit(value)                               \
		{}                                              \
		                                                \
		unit_name()                                     \
		{}                                              \
		                                                \
		template <typename T>                           \
		unit_name(const LLUnit<unit_name, T>& other)    \
		{                                               \
			setBaseValue(other.unit_name::get());       \
		}                                               \
		                                                \
		using LLUnit<STORAGE_TYPE>::operator +;	        \
		using LLUnit<STORAGE_TYPE>::operator +=;        \
		using LLUnit<STORAGE_TYPE>::operator -;         \
		using LLUnit<STORAGE_TYPE>::operator -=;        \
		using LLUnit<STORAGE_TYPE>::operator *;         \
		using LLUnit<STORAGE_TYPE>::operator *=;        \
		using LLUnit<STORAGE_TYPE>::operator /;         \
		using LLUnit<STORAGE_TYPE>::operator /=;        \
	};

#define LL_DECLARE_DERIVED_UNIT(base_unit, derived_unit, conversion_factor)                   \
	template<typename STORAGE_TYPE>                                                           \
	struct derived_unit : public LLUnit<base_unit<STORAGE_TYPE>, derived_unit<STORAGE_TYPE> > \
	{                                                                                         \
		derived_unit(value_t value)                                                           \
		:	LLUnit(value)                                                                     \
		{}                                                                                    \
		                                                                                      \
		derived_unit()                                                                        \
		{}                                                                                    \
		                                                                                      \
		template <typename T>                                                                 \
		derived_unit(const LLUnit<base_unit<STORAGE_TYPE>, T>& other)                         \
		{                                                                                     \
			setBaseValue(other.base_unit<STORAGE_TYPE>::get());                               \
		}                                                                                     \
		                                                                                      \
		static F32 conversionToBaseFactor() { return (F32)(conversion_factor); }              \
		                                                                                      \
	using LLUnit<STORAGE_TYPE>::operator +;	                                                  \
	using LLUnit<STORAGE_TYPE>::operator +=;                                                  \
	using LLUnit<STORAGE_TYPE>::operator -;                                                   \
	using LLUnit<STORAGE_TYPE>::operator -=;                                                  \
	using LLUnit<STORAGE_TYPE>::operator *;                                                   \
	using LLUnit<STORAGE_TYPE>::operator *=;                                                  \
	using LLUnit<STORAGE_TYPE>::operator /;                                                   \
	using LLUnit<STORAGE_TYPE>::operator /=;                                                  \
	};

namespace LLUnits
{
	LL_DECLARE_BASE_UNIT(Bytes);
	LL_DECLARE_DERIVED_UNIT(Bytes, Kilobytes, 1024);
	LL_DECLARE_DERIVED_UNIT(Bytes, Megabytes, 1024 * 1024);
	LL_DECLARE_DERIVED_UNIT(Bytes, Gigabytes, 1024 * 1024 * 1024);
	LL_DECLARE_DERIVED_UNIT(Bytes, Bits,	  (1.f / 8.f));
	LL_DECLARE_DERIVED_UNIT(Bytes, Kilobit,   (1024 / 8));
	LL_DECLARE_DERIVED_UNIT(Bytes, Megabits,  (1024 / 8));
	LL_DECLARE_DERIVED_UNIT(Bytes, Gigabits,  (1024 * 1024 * 1024 / 8));

	LL_DECLARE_BASE_UNIT(Seconds);
	LL_DECLARE_DERIVED_UNIT(Seconds, Minutes,		60);
	LL_DECLARE_DERIVED_UNIT(Seconds, Hours,			60 * 60);
	LL_DECLARE_DERIVED_UNIT(Seconds, Days,			60 * 60 * 24);
	LL_DECLARE_DERIVED_UNIT(Seconds, Weeks,			60 * 60 * 24 * 7);
	LL_DECLARE_DERIVED_UNIT(Seconds, Milliseconds,	(1.f / 1000.f));
	LL_DECLARE_DERIVED_UNIT(Seconds, Microseconds,	(1.f / (1000000.f)));
	LL_DECLARE_DERIVED_UNIT(Seconds, Nanoseconds,	(1.f / (1000000000.f)));

}

#endif // LL_LLUNIT_H
