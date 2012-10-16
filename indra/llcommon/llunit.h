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

template<typename STORAGE_TYPE, typename BASE_UNIT, typename DERIVED_UNIT = BASE_UNIT>
struct LLUnitType : public BASE_UNIT
{
	typedef DERIVED_UNIT unit_t;
	typedef typename BASE_UNIT::value_t value_t;
	typedef void is_unit_t;

	LLUnitType()
	{}

	explicit LLUnitType(value_t value)
	:	BASE_UNIT(convertToBase(value))
	{}

	operator unit_t& ()
	{
		return static_cast<unit_t&>(*this);
	}

	value_t value() const
	{
		return convertToDerived(mBaseValue);
	}

	template<typename CONVERTED_TYPE>
	value_t value() const
	{
		return CONVERTED_TYPE(*this).value();
	}

	static value_t convertToBase(value_t derived_value)
	{
		return (value_t)((F32)derived_value * unit_t::conversionToBaseFactor());
	}

	static value_t convertToDerived(value_t base_value)
	{
		return (value_t)((F32)base_value / unit_t::conversionToBaseFactor());
	}

	unit_t operator + (const unit_t other) const
	{
		return unit_t(mBaseValue + other.mBaseValue);
	}

	unit_t operator - (const unit_t other) const
	{
		return unit_t(mBaseValue - other.mBaseValue);
	}

	unit_t operator * (value_t multiplicand) const
	{
		return unit_t(mBaseValue * multiplicand);
	}

	unit_t operator / (value_t divisor) const
	{
		return unit_t(mBaseValue / divisor);
	}

};

template<typename STORAGE_TYPE, typename T>
struct LLUnitType<STORAGE_TYPE, T, T>
{
	typedef T unit_t;
	typedef typename STORAGE_TYPE value_t;
	typedef void is_unit_t;

	LLUnitType()
	:	mBaseValue()
	{}

	explicit LLUnitType(value_t value)
	:	mBaseValue(value)
	{}

	unit_t& operator=(value_t value)
	{
		setBaseValue(value);
		return *this;
	}

	operator unit_t& ()
	{
		return static_cast<unit_t&>(*this);
	}

	value_t value() const { return mBaseValue; }

	static value_t convertToBase(value_t derived_value)
	{
		return (value_t)derived_value;
	}

	static value_t convertToDerived(value_t base_value)
	{
		return (value_t)base_value;
	}

	unit_t operator + (const unit_t other) const
	{
		return unit_t(mBaseValue + other.mBaseValue);
	}

	void operator += (const unit_t other)
	{
		mBaseValue += other.mBaseValue;
	}

	unit_t operator - (const unit_t other) const
	{
		return unit_t(mBaseValue - other.mBaseValue);
	}

	void operator -= (const unit_t other)
	{
		mBaseValue -= other.mBaseValue;
	}

	unit_t operator * (value_t multiplicand) const
	{
		return unit_t(mBaseValue * multiplicand);
	}

	void operator *= (value_t multiplicand)
	{
		mBaseValue *= multiplicand;
	}

	unit_t operator / (value_t divisor) const
	{
		return unit_t(mBaseValue / divisor);
	}

	void operator /= (value_t divisor)
	{
		mBaseValue /= divisor;
	}

protected:
	void setBaseValue(value_t value)
	{
		mBaseValue = value;
	}

	value_t mBaseValue;
};

#define LL_DECLARE_BASE_UNIT(unit_name)                                                     \
	struct unit_name : public LLUnitType<F64, unit_name>									\
	{                                                                                       \
		typedef unit_name base_unit_t;                                                      \
		typedef LLUnitType<F64, unit_name> unit_t;						                    \
		typedef F64			storage_t;			                                            \
	                                                                                        \
		unit_name(F64 value)                                                                \
		:	LLUnitType(value)                                                               \
		{}                                                                                  \
		                                                                                    \
		unit_name()                                                                         \
		{}                                                                                  \
		                                                                                    \
		template <typename SOURCE_STORAGE_TYPE, typename SOURCE_TYPE>                       \
		unit_name(const LLUnitType<SOURCE_STORAGE_TYPE, unit_name, SOURCE_TYPE>& source)    \
		{                                                                                   \
			setBaseValue((F64)source.unit_name::unit_t::value());				            \
		}                                                                                   \
		                                                                                    \
		using LLUnitType::operator +;	                                                    \
		using LLUnitType::operator +=;														\
		using LLUnitType::operator -;														\
		using LLUnitType::operator -=;														\
		using LLUnitType::operator *;														\
		using LLUnitType::operator *=;														\
		using LLUnitType::operator /;														\
		using LLUnitType::operator /=;														\
	};

#define LL_DECLARE_DERIVED_UNIT(base_unit, derived_unit, conversion_factor)                 \
	struct derived_unit : public LLUnitType<F64, base_unit, derived_unit>                   \
	{                                                                                       \
		typedef base_unit base_unit_t;                                                      \
		typedef LLUnitType<F64, base_unit, derived_unit> unit_t;				            \
		typedef F64			storage_t;							                            \
		                                                                                    \
		derived_unit(value_t value)                                                         \
		:	LLUnitType(value)                                                               \
		{}                                                                                  \
		                                                                                    \
		derived_unit()                                                                      \
		{}                                                                                  \
		                                                                                    \
		template <typename SOURCE_STORAGE_TYPE, typename SOURCE_TYPE>                       \
		derived_unit(const LLUnitType<SOURCE_STORAGE_TYPE, base_unit, SOURCE_TYPE>& source) \
		{                                                                                   \
			setBaseValue((F64)source.base_unit::unit_t::value());						    \
		}                                                                                   \
		                                                                                    \
		static F32 conversionToBaseFactor() { return (F32)(conversion_factor); }            \
		                                                                                    \
		using LLUnitType::operator +;	                                                    \
		using LLUnitType::operator +=;                                                      \
		using LLUnitType::operator -;                                                       \
		using LLUnitType::operator -=;                                                      \
		using LLUnitType::operator *;                                                       \
		using LLUnitType::operator *=;                                                      \
		using LLUnitType::operator /;                                                       \
		using LLUnitType::operator /=;                                                      \
	};

namespace LLUnits
{
	LL_DECLARE_BASE_UNIT(Bytes);
	LL_DECLARE_DERIVED_UNIT(Bytes, Kilobytes, 1024);
	LL_DECLARE_DERIVED_UNIT(Bytes, Megabytes, 1024 * 1024);
	LL_DECLARE_DERIVED_UNIT(Bytes, Gigabytes, 1024 * 1024 * 1024);
	LL_DECLARE_DERIVED_UNIT(Bytes, Bits,	  (1.f / 8.f));
	LL_DECLARE_DERIVED_UNIT(Bytes, Kilobits,  (1024 / 8));
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

	LL_DECLARE_BASE_UNIT(Meters);
	LL_DECLARE_DERIVED_UNIT(Meters, Kilometers, 1000);
	LL_DECLARE_DERIVED_UNIT(Meters, Centimeters, 1 / 100);
	LL_DECLARE_DERIVED_UNIT(Meters, Millimeters, 1 / 1000);
}

#endif // LL_LLUNIT_H
