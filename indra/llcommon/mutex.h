/**
 * @file   mutex.h
 * @author Nat Goodspeed
 * @date   2019-12-03
 * @brief  Wrap <mutex> in odious boilerplate
 * 
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable:4265)
#endif
// warning C4265: 'std::_Pad' : class has virtual functions, but destructor is not virtual

#include <mutex>

#if LL_WINDOWS
#pragma warning (pop)
#endif
