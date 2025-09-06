/**
 * @file llappviewermacosx-objc.mm
 * @brief Functions related to LLAppViewerMacOSX that must be expressed in obj-c
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

#if !defined LL_DARWIN
	#error "Use only with macOS"
#endif

#import <Cocoa/Cocoa.h>
#include <iostream>

#include "llappviewermacosx-objc.h"

void force_ns_sxeption()
{
    NSException *exception = [NSException exceptionWithName:@"Forced NSException" reason:nullptr userInfo:nullptr];
    @throw exception;
}
