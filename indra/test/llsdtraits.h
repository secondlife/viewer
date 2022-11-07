/**
 * @file llsdtraits.h
 * @brief Unit test helpers
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
LLSDTraits<const LLSD::Binary&>::LLSDTraits()
    : type(LLSD::TypeBinary), getter(&LLSD::asBinary)
{ }

#endif // LLSDTRAITS_H
