/** 
 * @file MacUpdaterAppDelegate.mm
 * @brief 
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#import "MacUpdaterAppDelegate.h"
#include "llvfs_objc.h"
#include <string.h>
#include <boost/filesystem.hpp>

@implementation MacUpdaterAppDelegate

MacUpdaterAppDelegate    *gWindow;
bool gCancelled = false;
bool gFailure =false;


//@synthesize window = _window;
- (void)setWindow:(NSWindow *)window
{
    _window = window;
}

- (NSWindow *)window
{
    return _window;
}

- (id)init
{
    self = [super init];
    if (self) {
        
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        mAnimated = false;
        mProgressPercentage = 0.0;
        NSArray *arguments = [[NSProcessInfo processInfo] arguments];
                
        [self parse_args:arguments];
        gWindow = self;

        mUpdater.doUpdate();
        [pool drain];
        [pool release];
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

std::string* NSToString( NSString *ns_str )
{
    return ( new std::string([ns_str UTF8String]) );
}


- (void) setProgress:(int)cur max:(int) max
{
    bool indeterminate = false;
    if (max==0)
    {
        indeterminate = true;
    }
    else 
    {
        double percentage = ((double)cur / (double)max) * 100.0;
        [mProgressBar setDoubleValue:percentage];
    }
    [mProgressBar setIndeterminate:indeterminate];
}

- (void) setProgressText:(const std::string& )str
{
    [mProgressText setStringValue:[NSString stringWithUTF8String:str.c_str()]];
}

void sendDone()
{
    [ [ (id) gWindow window ] close];
}

void sendStopAlert()
{
    [ gWindow stopAlert ];
}

void setProgress(int cur, int max)
{
    [ (id) gWindow setProgress:cur max:max];
}

void setProgressText(const std::string& str)
{
    [ (id) gWindow setProgressText:str];
}

void sendProgress(int cur, int max, const std::string str)
{
    setProgress(cur,max);
    setProgressText(str);
}

bool mkTempDir(boost::filesystem::path& temp_dir)
{    
    NSString * tempDir = NSTemporaryDirectory();
    if (tempDir == nil)
        tempDir = @"/tmp/";

    std::string* temp_str = NSToString(tempDir);
    *temp_str += std::string("SecondLifeUpdate_XXXXXX");
    
	char temp[PATH_MAX] = "";	/* Flawfinder: ignore */
    strncpy(temp, temp_str->c_str(), temp_str->length());

    if(mkdtemp(temp) == NULL)
    {
        return false;
    }
    
    temp_dir = boost::filesystem::path(temp);
    
    delete temp_str;
    
    return true;
}

bool copyDir(const std::string& src_dir, const std::string& dest_dir)
{
    NSString* file = [NSString stringWithCString:src_dir.c_str() 
                                         encoding:[NSString defaultCStringEncoding]];
    NSString* toParent = [NSString stringWithCString:dest_dir.c_str() 
                                             encoding:[NSString defaultCStringEncoding]];
    NSError* error = nil;

    bool result = [[NSFileManager defaultManager] copyItemAtPath: file toPath: toParent error:&error];
    
    if (!result) {
        NSLog(@"Error during copy: %@", [error localizedDescription]);
    }
    return result;
}

- (int) parse_args:(NSArray *) args
{
    int i;
    int argc = [args count];
    
    mUpdater.mApplicationPath = NSToString( [args objectAtIndex:0] );
        
    for( i = 1; i < argc; i++ )
    {
        NSString* ns_arg = [args objectAtIndex:i];
        const char *arg = [ns_arg UTF8String];
        
        if ((!strcmp(arg, "-url")) && (i < argc)) 
		{
			mUpdater.mUpdateURL = NSToString( [args objectAtIndex:(++i)] );
		}
		else if ((!strcmp(arg, "-name")) && (i < argc)) 
		{
			mUpdater.mProductName = NSToString( [args objectAtIndex:(++i)] );
		}
		else if ((!strcmp(arg, "-bundleid")) && (i < argc)) 
		{
			mUpdater.mBundleID = NSToString( [args objectAtIndex:(++i)] );
		}
		else if ((!strcmp(arg, "-dmg")) && (i < argc)) 
		{
			mUpdater.mDmgFile = NSToString( [args objectAtIndex:(++i)] );
		}
		else if ((!strcmp(arg, "-marker")) && (i < argc)) 
		{
			mUpdater.mMarkerPath = NSToString( [args objectAtIndex:(++i)] );
		}
    }
    return 0;
}

bool isDirWritable(const std::string& dir_name)
{
    
    NSString *fullPath = [NSString stringWithCString:dir_name.c_str() 
                                            encoding:[NSString defaultCStringEncoding]];

    NSFileManager *fm = [NSFileManager defaultManager];
    bool result = [fm isWritableFileAtPath:fullPath];
    
	return result;
}

std::string* getUserTrashFolder()
{
    NSString *trash_str=[NSHomeDirectory() stringByAppendingPathComponent:@".Trash"];
    return NSToString( trash_str );

}

bool isFSRefViewerBundle(const std::string& targetURL)
{
	bool result = false;
    NSString *fullPath = [NSString stringWithCString:targetURL.c_str() 
                                            encoding:[NSString defaultCStringEncoding]];
    NSBundle *targetBundle = [NSBundle bundleWithPath:fullPath];
    NSString *targetBundleStr = [targetBundle bundleIdentifier];
    NSString *sourceBundleStr = [NSString stringWithCString:mUpdater.mBundleID->c_str()
                                            encoding:[NSString defaultCStringEncoding]];
    
    result = [targetBundleStr isEqualToString:sourceBundleStr];
        
	if(!result)
    {
        std::cout << "Target bundle ID mismatch." << std::endl;
    }
    
	return result;
}


- (IBAction)cancel:(id)sender
{
    gCancelled = true;
    sendDone();
}

- (void)stopAlert
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setAlertStyle:NSInformationalAlertStyle];
    [alert setMessageText:@"Error"];
    [alert setInformativeText:@"An error occurred while updating Second Life.  Please download the latest version from www.secondlife.com."];
     
     [alert beginSheetModalForWindow:_window
                       modalDelegate:self
      
                      didEndSelector:@selector(stopAlertDidEnd:returnCode:
                                               contextInfo:)
                         contextInfo:nil];
 }
 
 - (void)stopAlertDidEnd:(NSAlert *)alert
               returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    [alert release];
}


@end
