/** 
 * @file llunits.h
 * @brief Unit definitions
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

#ifndef LL_LLUNITTYPE_H
#define LL_LLUNITTYPE_H

#include "stdtypes.h"
#include "llunittype.h"

//
// Unit declarations
//

namespace LLUnits
{
LL_DECLARE_BASE_UNIT(Bytes, "B");
// technically, these are kibibytes, mibibytes, etc. but we should stick with commonly accepted terminology
LL_DECLARE_DERIVED_UNIT(Kilobytes, "KB", Bytes, / 1024);
LL_DECLARE_DERIVED_UNIT(Megabytes, "MB", Kilobytes, / 1024);
LL_DECLARE_DERIVED_UNIT(Gigabytes, "GB", Megabytes, / 1024);
}

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Bytes);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Kilobytes);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Megabytes);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Gigabytes);

namespace LLUnits
{
// technically, these are kibibits, mibibits, etc. but we should stick with commonly accepted terminology
LL_DECLARE_DERIVED_UNIT(Bits,       "b",    Bytes, * 8  );
LL_DECLARE_DERIVED_UNIT(Kilobits,   "Kb",   Bits, / 1024);
LL_DECLARE_DERIVED_UNIT(Megabits,   "Mb",   Kilobits, / 1024);  
LL_DECLARE_DERIVED_UNIT(Gigabits,   "Gb",   Megabits, / 1024);
}

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Bits);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Kilobits);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Megabits);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Gigabits);

namespace LLUnits
{
LL_DECLARE_BASE_UNIT(Seconds, "s");
LL_DECLARE_DERIVED_UNIT(Minutes,        "min",          Seconds, / 60);
LL_DECLARE_DERIVED_UNIT(Hours,          "h",            Minutes, / 60);
LL_DECLARE_DERIVED_UNIT(Days,           "d",            Hours, / 24);
LL_DECLARE_DERIVED_UNIT(Milliseconds,   "ms",           Seconds, * 1000);
LL_DECLARE_DERIVED_UNIT(Microseconds,   "\x09\x3cs",    Milliseconds, * 1000);
LL_DECLARE_DERIVED_UNIT(Nanoseconds,    "ns",           Microseconds, * 1000);
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
LL_DECLARE_DERIVED_UNIT(Kilometers,     "km",   Meters, / 1000);
LL_DECLARE_DERIVED_UNIT(Centimeters,    "cm",   Meters, * 100);
LL_DECLARE_DERIVED_UNIT(Millimeters,    "mm",   Meters, * 1000);
}

LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Meters);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Kilometers);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Centimeters);
LL_DECLARE_UNIT_TYPEDEFS(LLUnits, Millimeters);

namespace LLUnits
{
// rare units
LL_DECLARE_BASE_UNIT(Hertz, "Hz");
LL_DECLARE_DERIVED_UNIT(Kilohertz, "KHz", Hertz, / 1000);
LL_DECLARE_DERIVED_UNIT(Megahertz, "MHz", Kilohertz, / 1000);
LL_DECLARE_DERIVED_UNIT(Gigahertz, "GHz", Megahertz, / 1000);

LL_DECLARE_BASE_UNIT(Radians, "rad");
LL_DECLARE_DERIVED_UNIT(Degrees, "deg", Radians, * 57.29578f);

LL_DECLARE_BASE_UNIT(Percent, "%");
LL_DECLARE_DERIVED_UNIT(Ratio, "x", Percent, / 100);

LL_DECLARE_BASE_UNIT(Triangles, "tris");
LL_DECLARE_DERIVED_UNIT(Kilotriangles, "ktris", Triangles, / 1000);

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


#endif // LL_LLUNITTYPE_H
