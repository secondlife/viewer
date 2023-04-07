/** 
 * @file llfilepicker_mac.cpp
 * @brief OS-specific file picker
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifdef LL_DARWIN
#import <Cocoa/Cocoa.h>
#include <iostream>
#include "llfilepicker_mac.h"

std::unique_ptr<std::vector<std::string>> doLoadDialog(const std::vector<std::string>* allowed_types,
                 unsigned int flags)
{
    std::unique_ptr<std::vector<std::string>> outfiles;

    @autoreleasepool {
        int i, result;
        //Aura TODO:  We could init a small window and release it at the end of this routine
        //for a modeless interface.
        
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        //NSString *fileName = nil;
        NSMutableArray *fileTypes = nil;
        
        if ( allowed_types && !allowed_types->empty())
        {
            fileTypes = [[[NSMutableArray alloc] init] autorelease];
            
            for (i=0;i<allowed_types->size();++i)
            {
                [fileTypes addObject:
                 [NSString stringWithCString:(*allowed_types)[i].c_str()
                                    encoding:[NSString defaultCStringEncoding]]];
            }
        }
        
        //[panel setMessage:@"Import one or more files or directories."];
        [panel setAllowsMultipleSelection: ( (flags & F_MULTIPLE)?true:false ) ];
        [panel setCanChooseDirectories: ( (flags & F_DIRECTORY)?true:false ) ];
        [panel setCanCreateDirectories: true];
        [panel setResolvesAliases: true];
        [panel setCanChooseFiles: ( (flags & F_FILE)?true:false )];
        [panel setTreatsFilePackagesAsDirectories: ( flags & F_NAV_SUPPORT ) ];
        
        if (fileTypes)
        {
            [panel setAllowedFileTypes:fileTypes];
            result = [panel runModal];
        }
        else
        {
            // I suggest it's better to open the last path and let this default to home dir as necessary
            // for consistency with other OS X apps
            //
            //[panel setDirectoryURL: fileURLWithPath(NSHomeDirectory()) ];
            result = [panel runModal];
        }
        
        if (result == NSOKButton)
        {
            NSArray *filesToOpen = [panel URLs];
            int i, count = [filesToOpen count];
            
            if (count > 0)
            {
                outfiles.reset(new std::vector<std::string>);
            }
            
            for (i=0; i<count; i++) {
                NSString *aFile = [[filesToOpen objectAtIndex:i] path];
                std::string afilestr = std::string([aFile UTF8String]);
                outfiles->push_back(afilestr);
            }
        }
    }

    return outfiles;
}


std::unique_ptr<std::string> doSaveDialog(const std::string* file,
                  const std::string* type,
                  const std::string* creator,
                  const std::string* extension,
                  unsigned int flags)
{
    std::unique_ptr<std::string> outfile;
    @autoreleasepool {
        NSSavePanel *panel = [NSSavePanel savePanel];
        
        NSString *extensionns = [NSString stringWithCString:extension->c_str() encoding:[NSString defaultCStringEncoding]];
        NSArray *fileType = [extensionns componentsSeparatedByString:@","];
        
        //[panel setMessage:@"Save Image File"];
        [panel setTreatsFilePackagesAsDirectories: ( flags & F_NAV_SUPPORT ) ];
        [panel setCanSelectHiddenExtension:true];
        [panel setAllowedFileTypes:fileType];
        NSString *fileName = [NSString stringWithCString:file->c_str() encoding:[NSString defaultCStringEncoding]];
        
        NSURL* url = [NSURL fileURLWithPath:fileName];
        [panel setNameFieldStringValue: fileName];
        [panel setDirectoryURL: url];
        if([panel runModal] ==
           NSFileHandlingPanelOKButton)
        {
            NSURL* url = [panel URL];
            NSString* p = [url path];
            outfile.reset(new std::string([p UTF8String]));
            // write the file
        }
    }
    return outfile;
}

#endif
