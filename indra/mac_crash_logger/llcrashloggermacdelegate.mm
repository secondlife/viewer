/** 
 * @file llcrashloggermacdelegate.mm
 * @brief Mac OSX crash logger implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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


/*
#import "llcrashloggermacdelegate.h"
#include <iostream>

extern std::string gUserNotes;
extern bool gSendReport;
extern bool gRememberChoice;

@implementation LLCrashLoggerMacDelegate

- (void)setWindow:(NSWindow *)window
{
    _window = window;
}

- (NSWindow *)window
{
    return _window;
}

- (void)dealloc
{
    [super dealloc];
}

std::string* NSToString( NSString *ns_str )
{
    return ( new std::string([ns_str UTF8String]) );
}

- (IBAction)remember:(id)sender
{
    gRememberChoice = [rememberCheck state];
}

- (IBAction)send:(id)sender
{
    std::string* user_input = NSToString([crashText stringValue]);
    gUserNotes = *user_input;
    gSendReport = true;
}

- (IBAction)cancel:(id)sender
{
    [ _window close];
}
@end
*/