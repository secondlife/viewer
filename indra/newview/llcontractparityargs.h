/**
 * @file llcontractparityargs.h
 * @brief Early command-line selection for isolated renderer parity diagnostics.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#ifndef LL_LLCONTRACTPARITYARGS_H
#define LL_LLCONTRACTPARITYARGS_H

struct LLContractParitySelection
{
    bool mTonemap       = false;
    bool mMaterial      = false;
    bool mTextureUpload = false;
};

LLContractParitySelection getRawContractParitySelection(int argc, char* const* argv);

#endif
