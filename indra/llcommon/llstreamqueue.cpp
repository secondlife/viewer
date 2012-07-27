/**
 * @file   llstreamqueue.cpp
 * @author Nat Goodspeed
 * @date   2012-01-05
 * @brief  Implementation for llstreamqueue.
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llstreamqueue.h"
// STL headers
// std headers
// external library headers
// other Linden headers

// As of this writing, llstreamqueue.h is entirely template-based, therefore
// we don't strictly need a corresponding .cpp file. However, our CMake test
// macro assumes one. Here it is.
bool llstreamqueue_cpp_ignored = true;
