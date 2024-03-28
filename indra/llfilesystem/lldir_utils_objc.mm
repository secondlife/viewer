/** 
 * @file lldir_utils_objc.mm
 * @brief Cocoa implementation of directory utilities for macOS
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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
#if LL_DARWIN

//WARNING:  This file CANNOT use standard linden includes due to conflicts between definitions of BOOL

#include "lldir_utils_objc.h"
#import <Cocoa/Cocoa.h>

std::string getSystemTempFolder()
{
    std::string result;
    @autoreleasepool {
        NSString * tempDir = NSTemporaryDirectory();
        if (tempDir == nil)
            tempDir = @"/tmp";
        result = std::string([tempDir UTF8String]);
    }
    
    return result;
}

//findSystemDirectory scoped exclusively to this file. 
std::string findSystemDirectory(NSSearchPathDirectory searchPathDirectory,
                                   NSSearchPathDomainMask domainMask)
{
    std::string result;
    @autoreleasepool {
        NSString *path = nil;
        
        // Search for the path
        NSArray* paths = NSSearchPathForDirectoriesInDomains(searchPathDirectory,
                                                             domainMask,
                                                             YES);
        if ([paths count])
        {
            path = [paths objectAtIndex:0];
            //HACK:  Always attempt to create directory, ignore errors.
            NSError *error = nil;
            
            [[NSFileManager defaultManager] createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:&error];
            
            
            result = std::string([path UTF8String]);
        }
    }
    return result;
}

std::string getSystemExecutableFolder()
{
    std::string result;
    @autoreleasepool {
        NSString *bundlePath = [[NSBundle mainBundle] executablePath];
        result = std::string([bundlePath UTF8String]);
    }

    return result;
}

std::string getSystemResourceFolder()
{
    std::string result;
    @autoreleasepool {
        NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
        result = std::string([bundlePath UTF8String]);
    }
    
    return result;
}

std::string getSystemCacheFolder()
{
    return findSystemDirectory (NSCachesDirectory,
                                NSUserDomainMask);
}

std::string getSystemApplicationSupportFolder()
{
    return findSystemDirectory (NSApplicationSupportDirectory,
                                NSUserDomainMask);
    
}

#endif // LL_DARWIN
