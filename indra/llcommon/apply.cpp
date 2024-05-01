/**
 * @file   apply.cpp
 * @author Nat Goodspeed
 * @date   2022-12-21
 * @brief  Implementation for apply.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "apply.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "stringize.h"

void LL::apply_validate_size(size_t size, size_t arity)
{
    if (size != arity)
    {
        LLTHROW(apply_error(stringize("LL::apply(func(", arity, " args), "
                                      "std::vector(", size, " elements))")));
    }
}
