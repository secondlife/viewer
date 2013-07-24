/**
 * @file   is_approx_equal_fraction.h
 * @author Nat Goodspeed
 * @date   2009-01-28
 * @brief  lltut.h uses is_approx_equal_fraction(). Moved to this header
 *         file in llcommon so we can use lltut.h for llcommon tests without
 *         making llcommon depend on llmath.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#if ! defined(LL_IS_APPROX_EQUAL_FRACTION_H)
#define LL_IS_APPROX_EQUAL_FRACTION_H

#include "lldefs.h"
#include <cmath>

/**
 * Originally llmath.h contained two complete implementations of
 * is_approx_equal_fraction(), with signatures as below, bodies identical save
 * where they specifically mentioned F32/F64. Unifying these into a template
 * makes sense -- but to preserve the compiler's overload-selection behavior,
 * we still wrap the template implementation with the specific overloaded
 * signatures.
 */
template <typename FTYPE>
inline BOOL is_approx_equal_fraction_impl(FTYPE x, FTYPE y, U32 frac_bits)
{
    BOOL ret = TRUE;
    FTYPE diff = (FTYPE) fabs(x - y);

    S32 diffInt = (S32) diff;
    S32 diffFracTolerance = (S32) ((diff - (FTYPE) diffInt) * (1 << frac_bits));

    // if integer portion is not equal, not enough bits were used for packing
    // so error out since either the use case is not correct OR there is
    // an issue with pack/unpack. should fail in either case.
    // for decimal portion, make sure that the delta is no more than 1
    // based on the number of bits used for packing decimal portion.
    if (diffInt != 0 || diffFracTolerance > 1)
    {
        ret = FALSE;
    }

    return ret;
}

/// F32 flavor
inline BOOL is_approx_equal_fraction(F32 x, F32 y, U32 frac_bits)
{
    return is_approx_equal_fraction_impl<F32>(x, y, frac_bits);
}

/// F64 flavor
inline BOOL is_approx_equal_fraction(F64 x, F64 y, U32 frac_bits)
{
    return is_approx_equal_fraction_impl<F64>(x, y, frac_bits);
}

#endif /* ! defined(LL_IS_APPROX_EQUAL_FRACTION_H) */
