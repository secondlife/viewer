/**
 * @file   lleventapi.cpp
 * @author Nat Goodspeed
 * @date   2009-11-10
 * @brief  Implementation for lleventapi.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lleventapi.h"
// STL headers
// std headers
// external library headers
// other Linden headers

LLEventAPI::LLEventAPI(const std::string& name, const std::string& desc, const std::string& field):
    lbase(name, field),
    ibase(name),
    mDesc(desc)
{
}

LLEventAPI::~LLEventAPI()
{
}
