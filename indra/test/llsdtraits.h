/**
 * @file llsdtraits.h
 * @brief Unit test helpers
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LLSDTRAITS_H
#define LLSDTRAITS_H

#include "llsd.h"
#include "llstring.h"

template<class T>
class LLSDTraits
{
 protected:
	typedef T (LLSD::*Getter)() const;
	
	LLSD::Type type;
	Getter getter;
	
 public:
	LLSDTraits();
	
	T get(const LLSD& actual)
		{
			return (actual.*getter)();
		}
	
	bool checkType(const LLSD& actual)
		{
			return actual.type() == type;
		}
};

template<> inline
LLSDTraits<LLSD::Boolean>::LLSDTraits()
	: type(LLSD::TypeBoolean), getter(&LLSD::asBoolean)
{ }

template<> inline
LLSDTraits<LLSD::Integer>::LLSDTraits()
	: type(LLSD::TypeInteger), getter(&LLSD::asInteger)
{ }

template<> inline
LLSDTraits<LLSD::Real>::LLSDTraits()
	: type(LLSD::TypeReal), getter(&LLSD::asReal)
{ }

template<> inline
LLSDTraits<LLSD::UUID>::LLSDTraits()
	: type(LLSD::TypeUUID), getter(&LLSD::asUUID)
{ }

template<> inline
LLSDTraits<LLSD::String>::LLSDTraits()
	: type(LLSD::TypeString), getter(&LLSD::asString)
{ }

template<>
class LLSDTraits<LLString> : public LLSDTraits<LLSD::String>
{ };

template<>
class LLSDTraits<const char*> : public LLSDTraits<LLSD::String>
{ };

template<> inline
LLSDTraits<LLSD::Date>::LLSDTraits()
	: type(LLSD::TypeDate), getter(&LLSD::asDate)
{ }

template<> inline
LLSDTraits<LLSD::URI>::LLSDTraits()
	: type(LLSD::TypeURI), getter(&LLSD::asURI)
{ }

template<> inline
LLSDTraits<LLSD::Binary>::LLSDTraits()
	: type(LLSD::TypeBinary), getter(&LLSD::asBinary)
{ }

#endif // LLSDTRAITS_H
