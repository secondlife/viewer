/** 
 * @file mac_updater.cpp
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

#include "linden_common.h"

#include <boost/format.hpp>

#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curl/curl.h>
#include <pthread.h>

#include "llerror.h"
#include "lltimer.h"
#include "lldir.h"
#include "llfile.h"

#include "llstring.h"

#include <Carbon/Carbon.h>

#include "llerrorcontrol.h"

enum
{
	kEventClassCustom = 'Cust',
	kEventCustomProgress = 'Prog',
	kEventParamCustomCurValue = 'Cur ',
	kEventParamCustomMaxValue = 'Max ',
	kEventParamCustomText = 'Text',
	kEventCustomDone = 'Done',
};

WindowRef gWindow = NULL;
EventHandlerRef gEventHandler = NULL;
OSStatus gFailure = noErr;
Boolean gCancelled = false;

const char *gUpdateURL;
const char *gProductName;
const char *gBundleID;
const char *gDmgFile;
const char *gMarkerPath;

void *updatethreadproc(void*);

pthread_t updatethread;

OSStatus setProgress(int cur, int max)
{
	OSStatus err;
	ControlRef progressBar = NULL;
	ControlID id;

	id.signature = 'prog';
	id.id = 0;

	err = GetControlByID(gWindow, &id, &progressBar);
	if(err == noErr)
	{
		Boolean indeterminate;
		
		if(max == 0)
		{
			indeterminate = true;
			err = SetControlData(progressBar, kControlEntireControl, kControlProgressBarIndeterminateTag, sizeof(Boolean), (Ptr)&indeterminate);
		}
		else
		{
			double percentage = (double)cur / (double)max;
			SetControlMinimum(progressBar, 0);
			SetControlMaximum(progressBar, 100);
			SetControlValue(progressBar, (SInt16)(percentage * 100));

			indeterminate = false;
			err = SetControlData(progressBar, kControlEntireControl, kControlProgressBarIndeterminateTag, sizeof(Boolean), (Ptr)&indeterminate);

			Draw1Control(progressBar);
		}
	}

	return(err);
}

OSStatus setProgressText(CFStringRef text)
{
	OSStatus err;
	ControlRef progressText = NULL;
	ControlID id;

	id.signature = 'what';
	id.id = 0;

	err = GetControlByID(gWindow, &id, &progressText);
	if(err == noErr)
	{
		err = SetControlData(progressText, kControlEntireControl, kControlStaticTextCFStringTag, sizeof(CFStringRef), (Ptr)&text);
		Draw1Control(progressText);
	}

	return(err);
}

OSStatus sendProgress(long cur, long max, CFStringRef text = NULL)
{
	OSStatus result;
	EventRef evt;
	
	result = CreateEvent( 
			NULL,
			kEventClassCustom, 
			kEventCustomProgress,
			0, 
			kEventAttributeNone, 
			&evt);
	
	// This event needs to be targeted at the window so it goes to the window's handler.
	if(result == noErr)
	{
		EventTargetRef target = GetWindowEventTarget(gWindow);
		result = SetEventParameter (
			evt,
			kEventParamPostTarget,
			typeEventTargetRef,
			sizeof(target),
			&target);
	}

	if(result == noErr)
	{
		result = SetEventParameter (
			evt,
			kEventParamCustomCurValue,
			typeLongInteger,
			sizeof(cur),
			&cur);
	}

	if(result == noErr)
	{
		result = SetEventParameter (
			evt,
			kEventParamCustomMaxValue,
			typeLongInteger,
			sizeof(max),
			&max);
	}
	
	if(result == noErr)
	{
		if(text != NULL)
		{
			result = SetEventParameter (
				evt,
				kEventParamCustomText,
				typeCFStringRef,
				sizeof(text),
				&text);
		}
	}
	
	if(result == noErr)
	{
		// Send the event
		PostEventToQueue(
			GetMainEventQueue(),
			evt,
			kEventPriorityStandard);

	}
	
	return(result);
}

OSStatus sendDone(void)
{
	OSStatus result;
	EventRef evt;
	
	result = CreateEvent( 
			NULL,
			kEventClassCustom, 
			kEventCustomDone,
			0, 
			kEventAttributeNone, 
			&evt);
	
	// This event needs to be targeted at the window so it goes to the window's handler.
	if(result == noErr)
	{
		EventTargetRef target = GetWindowEventTarget(gWindow);
		result = SetEventParameter (
			evt,
			kEventParamPostTarget,
			typeEventTargetRef,
			sizeof(target),
			&target);
	}

	if(result == noErr)
	{
		// Send the event
		PostEventToQueue(
			GetMainEventQueue(),
			evt,
			kEventPriorityStandard);

	}
	
	return(result);
}

OSStatus dialogHandler(EventHandlerCallRef handler, EventRef event, void *userdata)
{
	OSStatus result = eventNotHandledErr;
	OSStatus err;
	UInt32 evtClass = GetEventClass(event);
	UInt32 evtKind = GetEventKind(event);
	
	if((evtClass == kEventClassCommand) && (evtKind == kEventCommandProcess))
	{
		HICommand cmd;
		err = GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(cmd), NULL, &cmd);
		
		if(err == noErr)
		{
			switch(cmd.commandID)
			{				
				case kHICommandCancel:
					gCancelled = true;
//					QuitAppModalLoopForWindow(gWindow);
					result = noErr;
				break;
			}
		}
	}
	else if((evtClass == kEventClassCustom) && (evtKind == kEventCustomProgress))
	{
		// Request to update the progress dialog
		long cur = 0;
		long max = 0;
		CFStringRef text = NULL;
		(void) GetEventParameter(event, kEventParamCustomCurValue, typeLongInteger, NULL, sizeof(cur), NULL, &cur);
		(void) GetEventParameter(event, kEventParamCustomMaxValue, typeLongInteger, NULL, sizeof(max), NULL, &max);
		(void) GetEventParameter(event, kEventParamCustomText, typeCFStringRef, NULL, sizeof(text), NULL, &text);
		
		err = setProgress(cur, max);
		if(err == noErr)
		{
			if(text != NULL)
			{
				setProgressText(text);
			}
		}
		
		result = noErr;
	}
	else if((evtClass == kEventClassCustom) && (evtKind == kEventCustomDone))
	{
		// We're done.  Exit the modal loop.
		QuitAppModalLoopForWindow(gWindow);
		result = noErr;
	}
	
	return(result);
}

#if 0
size_t curl_download_callback(void *data, size_t size, size_t nmemb,
										  void *user_data)
{
	S32 bytes = size * nmemb;
	char *cdata = (char *) data;
	for (int i =0; i < bytes; i += 1)
	{
		gServerResponse.append(cdata[i]);
	}
	return bytes;
}
#endif

int curl_progress_callback_func(void *clientp,
							  double dltotal,
							  double dlnow,
							  double ultotal,
							  double ulnow)
{
	int max = (int)(dltotal / 1024.0);
	int cur = (int)(dlnow / 1024.0);
	sendProgress(cur, max);
	
	if(gCancelled)
		return(1);

	return(0);
}

int parse_args(int argc, char **argv)
{
	int j;

	for (j = 1; j < argc; j++) 
	{
		if ((!strcmp(argv[j], "-url")) && (++j < argc)) 
		{
			gUpdateURL = argv[j];
		}
		else if ((!strcmp(argv[j], "-name")) && (++j < argc)) 
		{
			gProductName = argv[j];
		}
		else if ((!strcmp(argv[j], "-bundleid")) && (++j < argc)) 
		{
			gBundleID = argv[j];
		}
		else if ((!strcmp(argv[j], "-dmg")) && (++j < argc)) 
		{
			gDmgFile = argv[j];
		}
		else if ((!strcmp(argv[j], "-marker")) && (++j < argc)) 
		{
			gMarkerPath = argv[j];;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	// We assume that all the logs we're looking for reside on the current drive
	gDirUtilp->initAppDirs("SecondLife");

	LLError::initForApplication( gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));

	// Rename current log file to ".old"
	std::string old_log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "updater.log.old");
	std::string log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "updater.log");
	LLFile::rename(log_file.c_str(), old_log_file.c_str());

	// Set the log file to updater.log
	LLError::logToFile(log_file);

	/////////////////////////////////////////
	//
	// Process command line arguments
	//
	gUpdateURL  = NULL;
	gProductName = NULL;
	gBundleID = NULL;
	gDmgFile = NULL;
	gMarkerPath = NULL;
	parse_args(argc, argv);
	if ((gUpdateURL == NULL) && (gDmgFile == NULL))
	{
		llinfos << "Usage: mac_updater -url <url> | -dmg <dmg file> [-name <product_name>] [-program <program_name>]" << llendl;
		exit(1);
	}
	else
	{
		llinfos << "Update url is: " << gUpdateURL << llendl;
		if (gProductName)
		{
			llinfos << "Product name is: " << gProductName << llendl;
		}
		else
		{
			gProductName = "Second Life";
		}
		if (gBundleID)
		{
			llinfos << "Bundle ID is: " << gBundleID << llendl;
		}
		else
		{
			gBundleID = "com.secondlife.indra.viewer";
		}
	}
	
	llinfos << "Starting " << gProductName << " Updater" << llendl;

	// Real UI...
	OSStatus err;
	IBNibRef nib = NULL;
	
	err = CreateNibReference(CFSTR("AutoUpdater"), &nib);

	char windowTitle[MAX_PATH];		/* Flawfinder: ignore */
	snprintf(windowTitle, sizeof(windowTitle), "%s Updater", gProductName);		
	CFStringRef windowTitleRef = NULL;
	windowTitleRef = CFStringCreateWithCString(NULL, windowTitle, kCFStringEncodingUTF8);
	
	if(err == noErr)
	{
		err = CreateWindowFromNib(nib, CFSTR("Updater"), &gWindow);
	}

	if (err == noErr)
	{
		err = SetWindowTitleWithCFString(gWindow, windowTitleRef);	
	}
	CFRelease(windowTitleRef);

	if(err == noErr)
	{
		// Set up an event handler for the window.
		EventTypeSpec handlerEvents[] = 
		{
			{ kEventClassCommand, kEventCommandProcess },
			{ kEventClassCustom, kEventCustomProgress },
			{ kEventClassCustom, kEventCustomDone }
		};
		InstallStandardEventHandler(GetWindowEventTarget(gWindow));
		InstallWindowEventHandler(
				gWindow, 
				NewEventHandlerUPP(dialogHandler), 
				GetEventTypeCount (handlerEvents), 
				handlerEvents, 
				0, 
				&gEventHandler);
	}
	
	if(err == noErr)
	{
		ShowWindow(gWindow);
		SelectWindow(gWindow);
	}
		
	if(err == noErr)
	{
		pthread_create(&updatethread, 
                         NULL,
                         &updatethreadproc, 
                         NULL);
						 
	}
	
	if(err == noErr)
	{
		RunAppModalLoopForWindow(gWindow);
	}

	void *threadresult;

	pthread_join(updatethread, &threadresult);

	if(!gCancelled && (gFailure != noErr))
	{
		// Something went wrong.  Since we always just tell the user to download a new version, we don't really care what.
		AlertStdCFStringAlertParamRec params;
		SInt16 retval_mac = 1;
		DialogRef alert = NULL;
		OSStatus err;

		params.version = kStdCFStringAlertVersionOne;
		params.movable = false;
		params.helpButton = false;
		params.defaultText = (CFStringRef)kAlertDefaultOKText;
		params.cancelText = 0;
		params.otherText = 0;
		params.defaultButton = 1;
		params.cancelButton = 0;
		params.position = kWindowDefaultPosition;
		params.flags = 0;

		err = CreateStandardAlert(
				kAlertStopAlert,
				CFSTR("Error"),
				CFSTR("An error occurred while updating Second Life.  Please download the latest version from www.secondlife.com."),
				&params,
				&alert);
		
		if(err == noErr)
		{
			err = RunStandardAlert(
					alert,
					NULL,
					&retval_mac);
		}
		
		if(gMarkerPath != 0)
		{
			// Create a install fail marker that can be used by the viewer to
			// detect install problems.
			std::ofstream stream(gMarkerPath);
			if(stream) stream << -1;
		}
		exit(-1);
	} else {
		exit(0);
	}

	if(gWindow != NULL)
	{
		DisposeWindow(gWindow);
	}
	
	if(nib != NULL)
	{
		DisposeNibReference(nib);
	}
	
	return 0;
}

bool isDirWritable(FSRef &dir)
{
	bool result = false;
	
	// Test for a writable directory by creating a directory, then deleting it again.
	// This is kinda lame, but will pretty much always give the right answer.
	
	OSStatus err = noErr;
	char temp[PATH_MAX] = "";		/* Flawfinder: ignore */

	err = FSRefMakePath(&dir, (UInt8*)temp, sizeof(temp));

	if(err == noErr)
	{
		strncat(temp, "/.test_XXXXXX", (sizeof(temp) - strlen(temp)) - 1);
		
		if(mkdtemp(temp) != NULL)
		{
			// We were able to make the directory.  This means the directory is writable.
			result = true;
			
			// Clean up.
			rmdir(temp);
		}
	}

#if 0
	// This seemed like a good idea, but won't tell us if we're on a volume mounted read-only.
	UInt8 perm;
	err = FSGetUserPrivilegesPermissions(&targetParentRef, &perm, NULL);
	if(err == noErr)
	{
		if(perm & kioACUserNoMakeChangesMask)
		{
			// Parent directory isn't writable.
			llinfos << "Target parent directory not writable." << llendl;
			err = -1;
			replacingTarget = false;
		}
	}
#endif

	return result;
}

static std::string HFSUniStr255_to_utf8str(const HFSUniStr255* src)
{
	llutf16string string16((U16*)&(src->unicode), src->length);
	std::string result = utf16str_to_utf8str(string16);
	return result;
}

int restoreObject(const char* aside, const char* target, const char* path, const char* object)
{
	char source[PATH_MAX] = "";		/* Flawfinder: ignore */
	char dest[PATH_MAX] = "";		/* Flawfinder: ignore */
	snprintf(source, sizeof(source), "%s/%s/%s", aside, path, object);		
	snprintf(dest, sizeof(dest), "%s/%s", target, path);		
	FSRef sourceRef;
	FSRef destRef;
	OSStatus err;
	err = FSPathMakeRef((UInt8 *)source, &sourceRef, NULL);
	if(err != noErr) return false;
	err = FSPathMakeRef((UInt8 *)dest, &destRef, NULL);
	if(err != noErr) return false;

	llinfos << "Copying " << source << " to " << dest << llendl;

	err = FSCopyObjectSync(
			&sourceRef,
			&destRef,
			NULL,
			NULL,
			kFSFileOperationOverwrite);

	if(err != noErr) return false;
	return true;
}

// Replace any mention of "Second Life" with the product name.
void filterFile(const char* filename)
{
	char temp[PATH_MAX] = "";		/* Flawfinder: ignore */
	// First copy the target's version, so we can run it through sed.
	snprintf(temp, sizeof(temp), "cp '%s' '%s.tmp'", filename, filename);		
	system(temp);		/* Flawfinder: ignore */

	// Now run it through sed.
	snprintf(temp, sizeof(temp), 		
			"sed 's/Second Life/%s/g' '%s.tmp' > '%s'", gProductName, filename, filename);
	system(temp);		/* Flawfinder: ignore */
}

static bool isFSRefViewerBundle(FSRef *targetRef)
{
	bool result = false;
	CFURLRef targetURL = NULL;
	CFBundleRef targetBundle = NULL;
	CFStringRef targetBundleID = NULL;
	CFStringRef sourceBundleID = NULL;

	targetURL = CFURLCreateFromFSRef(NULL, targetRef);

	if(targetURL == NULL)
	{
		llinfos << "Error creating target URL." << llendl;
	}
	else
	{
		targetBundle = CFBundleCreate(NULL, targetURL);
	}
	
	if(targetBundle == NULL)
	{
		llinfos << "Failed to create target bundle." << llendl;
	}
	else
	{
		targetBundleID = CFBundleGetIdentifier(targetBundle);
	}
	
	if(targetBundleID == NULL)
	{
		llinfos << "Couldn't retrieve target bundle ID." << llendl;
	}
	else
	{
		sourceBundleID = CFStringCreateWithCString(NULL, gBundleID, kCFStringEncodingUTF8);
		if(CFStringCompare(sourceBundleID, targetBundleID, 0) == kCFCompareEqualTo)
		{
			// This is the bundle we're looking for.
			result = true;
		}
		else
		{
			llinfos << "Target bundle ID mismatch." << llendl;
		}
	}
	
	// Don't release targetBundleID -- since we don't retain it, it's released when targetBundle is released.
	if(targetURL != NULL)
		CFRelease(targetURL);
	if(targetBundle != NULL)
		CFRelease(targetBundle);
	
	return result;
}

// Search through the directory specified by 'parent' for an item that appears to be a Second Life viewer.
static OSErr findAppBundleOnDiskImage(FSRef *parent, FSRef *app)
{
	FSIterator		iterator;
	bool			found = false;

	OSErr err = FSOpenIterator( parent, kFSIterateFlat, &iterator );
	if(!err)
	{
		do
		{
			ItemCount actualObjects = 0;
			Boolean containerChanged = false;
			FSCatalogInfo info;
			FSRef ref;
			HFSUniStr255 unicodeName;
			err = FSGetCatalogInfoBulk( 
					iterator, 
					1, 
					&actualObjects, 
					&containerChanged,
					kFSCatInfoNodeFlags, 
					&info, 
					&ref,
					NULL, 
					&unicodeName );
			
			if(actualObjects == 0)
				break;
				
			if(!err)
			{
				// Call succeeded and not done with the iteration.
				std::string name = HFSUniStr255_to_utf8str(&unicodeName);

				llinfos << "Considering \"" << name << "\"" << llendl;

				if(info.nodeFlags & kFSNodeIsDirectoryMask)
				{
					// This is a directory.  See if it's a .app
					if(name.find(".app") != std::string::npos)
					{
						// Looks promising.  Check to see if it has the right bundle identifier.
						if(isFSRefViewerBundle(&ref))
						{
							llinfos << name << " is the one" << llendl;
							// This is the one.  Return it.
							*app = ref;
							found = true;
							break;
						} else {
							llinfos << name << " is not the bundle we are looking for; move along" << llendl;
						}

					}
				}
			}
		}
		while(!err);
		
		llinfos << "closing the iterator" << llendl;
		
		FSCloseIterator(iterator);
		
		llinfos << "closed" << llendl;
	}
	
	if(!err && !found)
		err = fnfErr;
		
	return err;
}

void *updatethreadproc(void*)
{
	char tempDir[PATH_MAX] = "";		/* Flawfinder: ignore */
	FSRef tempDirRef;
	char temp[PATH_MAX] = "";	/* Flawfinder: ignore */
	// *NOTE: This buffer length is used in a scanf() below.
	char deviceNode[1024] = "";	/* Flawfinder: ignore */
	LLFILE *downloadFile = NULL;
	OSStatus err;
	ProcessSerialNumber psn;
	char target[PATH_MAX] = "";		/* Flawfinder: ignore */
	FSRef targetRef;
	FSRef targetParentRef;
	FSVolumeRefNum targetVol;
	FSRef trashFolderRef;
	Boolean replacingTarget = false;

	memset(&tempDirRef, 0, sizeof(tempDirRef));
	memset(&targetRef, 0, sizeof(targetRef));
	memset(&targetParentRef, 0, sizeof(targetParentRef));
	
	try
	{
		// Attempt to get a reference to the Second Life application bundle containing this updater.
		// Any failures during this process will cause us to default to updating /Applications/Second Life.app
		{
			FSRef myBundle;

			err = GetCurrentProcess(&psn);
			if(err == noErr)
			{
				err = GetProcessBundleLocation(&psn, &myBundle);
			}

			if(err == noErr)
			{
				// Sanity check:  Make sure the name of the item referenced by targetRef is "Second Life.app".
				FSRefMakePath(&myBundle, (UInt8*)target, sizeof(target));
				
				llinfos << "Updater bundle location: " << target << llendl;
			}
			
			// Our bundle should be in Second Life.app/Contents/Resources/AutoUpdater.app
			// so we need to go up 3 levels to get the path to the main application bundle.
			if(err == noErr)
			{
				err = FSGetCatalogInfo(&myBundle, kFSCatInfoNone, NULL, NULL, NULL, &targetRef);
			}
			if(err == noErr)
			{
				err = FSGetCatalogInfo(&targetRef, kFSCatInfoNone, NULL, NULL, NULL, &targetRef);
			}
			if(err == noErr)
			{
				err = FSGetCatalogInfo(&targetRef, kFSCatInfoNone, NULL, NULL, NULL, &targetRef);
			}
			
			// And once more to get the parent of the target
			if(err == noErr)
			{
				err = FSGetCatalogInfo(&targetRef, kFSCatInfoNone, NULL, NULL, NULL, &targetParentRef);
			}
			
			if(err == noErr)
			{
				FSRefMakePath(&targetRef, (UInt8*)target, sizeof(target));
				llinfos << "Path to target: " << target << llendl;
			}
			
			// Sanity check: make sure the target is a bundle with the right identifier
			if(err == noErr)
			{
				// Assume the worst...
				err = -1;

				if(isFSRefViewerBundle(&targetRef))
				{
					// This is the bundle we're looking for.
					err = noErr;
					replacingTarget = true;
				}
			}
			
			// Make sure the target's parent directory is writable.
			if(err == noErr)
			{
				if(!isDirWritable(targetParentRef))
				{
					// Parent directory isn't writable.
					llinfos << "Target parent directory not writable." << llendl;
					err = -1;
					replacingTarget = false;
				}
			}

			if(err != noErr)
			{
				Boolean isDirectory;
				llinfos << "Target search failed, defaulting to /Applications/" << gProductName << ".app." << llendl;
				
				// Set up the parent directory
				err = FSPathMakeRef((UInt8*)"/Applications", &targetParentRef, &isDirectory);
				if((err != noErr) || (!isDirectory))
				{
					// We're so hosed.
					llinfos << "Applications directory not found, giving up." << llendl;
					throw 0;
				}
				
				snprintf(target, sizeof(target), "/Applications/%s.app", gProductName);		

				memset(&targetRef, 0, sizeof(targetRef));
				err = FSPathMakeRef((UInt8*)target, &targetRef, NULL);
				if(err == fnfErr)
				{
					// This is fine, just means we're not replacing anything.
					err = noErr;
					replacingTarget = false;
				}
				else
				{
					replacingTarget = true;
				}

				// Make sure the target's parent directory is writable.
				if(err == noErr)
				{
					if(!isDirWritable(targetParentRef))
					{
						// Parent directory isn't writable.
						llinfos << "Target parent directory not writable." << llendl;
						err = -1;
						replacingTarget = false;
					}
				}

			}
			
			// If we haven't fixed all problems by this point, just bail.
			if(err != noErr)
			{
				llinfos << "Unable to pick a target, giving up." << llendl;
				throw 0;
			}
		}
		
		// Find the volID of the volume the target resides on
		{
			FSCatalogInfo info;
			err = FSGetCatalogInfo(
				&targetParentRef,
				kFSCatInfoVolume,
				&info,
				NULL, 
				NULL,  
				NULL);
				
			if(err != noErr)
				throw 0;
			
			targetVol = info.volume;
		}

		// Find the temporary items and trash folders on that volume.
		err = FSFindFolder(
			targetVol,
			kTrashFolderType,
			true,
			&trashFolderRef);

		if(err != noErr)
			throw 0;

#if 0 // *HACK for DEV-11935 see below for details.

		FSRef tempFolderRef;

		err = FSFindFolder(
			targetVol,
			kTemporaryFolderType,
			true,
			&tempFolderRef);
		
		if(err != noErr)
			throw 0;
		
		err = FSRefMakePath(&tempFolderRef, (UInt8*)temp, sizeof(temp));

		if(err != noErr)
			throw 0;

#else		

		// *HACK for DEV-11935  the above kTemporaryFolderType query was giving
		// back results with path names that seem to be too long to be used as
		// mount points.  I suspect this incompatibility was introduced in the
		// Leopard 10.5.2 update, but I have not verified this. 
		char const HARDCODED_TMP[] = "/tmp";
		strncpy(temp, HARDCODED_TMP, sizeof(HARDCODED_TMP));

#endif // 0 *HACK for DEV-11935
		
		// Skip downloading the file if the dmg was passed on the command line.
		std::string dmgName;
		if(gDmgFile != NULL) {
			dmgName = basename((char *)gDmgFile);
			char * dmgDir = dirname((char *)gDmgFile);
			strncpy(tempDir, dmgDir, sizeof(tempDir));
			err = FSPathMakeRef((UInt8*)tempDir, &tempDirRef, NULL);
			if(err != noErr) throw 0;
			chdir(tempDir);
			goto begin_install;
		} else {
			// Continue on to download file.
			dmgName = "SecondLife.dmg";
		}

		
		strncat(temp, "/SecondLifeUpdate_XXXXXX", (sizeof(temp) - strlen(temp)) - 1);
		if(mkdtemp(temp) == NULL)
		{
			throw 0;
		}
		
		strncpy(tempDir, temp, sizeof(tempDir));
		temp[sizeof(tempDir) - 1] = '\0';
		
		llinfos << "tempDir is " << tempDir << llendl;

		err = FSPathMakeRef((UInt8*)tempDir, &tempDirRef, NULL);

		if(err != noErr)
			throw 0;
				
		chdir(tempDir);
		
		snprintf(temp, sizeof(temp), "SecondLife.dmg");		
		
		downloadFile = LLFile::fopen(temp, "wb");		/* Flawfinder: ignore */
		if(downloadFile == NULL)
		{
			throw 0;
		}

		{
			CURL *curl = curl_easy_init();

			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	//		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_download_callback);
			curl_easy_setopt(curl, CURLOPT_FILE, downloadFile);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &curl_progress_callback_func);
			curl_easy_setopt(curl, CURLOPT_URL,	gUpdateURL);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
			
			sendProgress(0, 1, CFSTR("Downloading..."));
			
			CURLcode result = curl_easy_perform(curl);
			
			curl_easy_cleanup(curl);
			
			if(gCancelled)
			{
				llinfos << "User cancel, bailing out."<< llendl;
				throw 0;
			}
			
			if(result != CURLE_OK)
			{
				llinfos << "Error " << result << " while downloading disk image."<< llendl;
				throw 0;
			}
			
			fclose(downloadFile);
			downloadFile = NULL;
		}

	begin_install:
		sendProgress(0, 0, CFSTR("Mounting image..."));
		LLFile::mkdir("mnt", 0700);
		
		// NOTE: we could add -private at the end of this command line to keep the image from showing up in the Finder,
		//		but if our cleanup fails, this makes it much harder for the user to unmount the image.
		std::string mountOutput;
		boost::format cmdFormat("hdiutil attach %s -mountpoint mnt");
		cmdFormat % dmgName;
		FILE* mounter = popen(cmdFormat.str().c_str(), "r");		/* Flawfinder: ignore */
		
		if(mounter == NULL)
		{
			llinfos << "Failed to mount disk image, exiting."<< llendl;
			throw 0;
		}
		
		// We need to scan the output from hdiutil to find the device node it uses to attach the disk image.
		// If we don't have this information, we can't detach it later.
		while(mounter != NULL)
		{
			size_t len = fread(temp, 1, sizeof(temp)-1, mounter);
			temp[len] = 0;
			mountOutput.append(temp);
			if(len < sizeof(temp)-1)
			{
				// End of file or error.
				int result = pclose(mounter);
				if(result != 0)
				{
					// NOTE: We used to abort here, but pclose() started returning 
					// -1, possibly when the size of the DMG passed a certain point 
					llinfos << "Unexpected result closing pipe: " << result << llendl; 
				}
				mounter = NULL;
			}
		}
		
		if(!mountOutput.empty())
		{
			const char *s = mountOutput.c_str();
			const char *prefix = "/dev/";
			char *sub = strstr(s, prefix);
			
			if(sub != NULL)
			{
				sub += strlen(prefix);	/* Flawfinder: ignore */
				sscanf(sub, "%1023s", deviceNode);	/* Flawfinder: ignore */
			}
		}
		
		if(deviceNode[0] != 0)
		{
			llinfos << "Disk image attached on /dev/" << deviceNode << llendl;
		}
		else
		{
			llinfos << "Disk image device node not found!" << llendl;
			throw 0; 
		}
		
		// Get an FSRef to the new application on the disk image
		FSRef sourceRef;
		FSRef mountRef;
		snprintf(temp, sizeof(temp), "%s/mnt", tempDir);		

		llinfos << "Disk image mount point is: " << temp << llendl;

		err = FSPathMakeRef((UInt8 *)temp, &mountRef, NULL);
		if(err != noErr)
		{
			llinfos << "Couldn't make FSRef to disk image mount point." << llendl;
			throw 0;
		}

		sendProgress(0, 0, CFSTR("Searching for the app bundle..."));
		err = findAppBundleOnDiskImage(&mountRef, &sourceRef);
		if(err != noErr)
		{
			llinfos << "Couldn't find application bundle on mounted disk image." << llendl;
			throw 0;
		}
		else
		{
			llinfos << "found the bundle." << llendl;
		}

		sendProgress(0, 0, CFSTR("Preparing to copy files..."));
		
		FSRef asideRef;
		char aside[MAX_PATH];		/* Flawfinder: ignore */
		
		// this will hold the name of the destination target
		CFStringRef appNameRef;

		if(replacingTarget)
		{
			// Get the name of the target we're replacing
			HFSUniStr255 appNameUniStr;
			err = FSGetCatalogInfo(&targetRef, 0, NULL, &appNameUniStr, NULL, NULL);
			if(err != noErr)
				throw 0;
			appNameRef = FSCreateStringFromHFSUniStr(NULL, &appNameUniStr);
			
			// Move aside old version (into work directory)
			err = FSMoveObject(&targetRef, &tempDirRef, &asideRef);
			if(err != noErr)
			{
				llwarns << "failed to move aside old version (error code " << 
					err << ")" << llendl;
				throw 0;
			}

			// Grab the path for later use.
			err = FSRefMakePath(&asideRef, (UInt8*)aside, sizeof(aside));
		}
		else
		{
			// Construct the name of the target based on the product name
			char appName[MAX_PATH];		/* Flawfinder: ignore */
			snprintf(appName, sizeof(appName), "%s.app", gProductName);		
			appNameRef = CFStringCreateWithCString(NULL, appName, kCFStringEncodingUTF8);
		}
		
		sendProgress(0, 0, CFSTR("Copying files..."));
		
		llinfos << "Starting copy..." << llendl;

		// Copy the new version from the disk image to the target location.
		err = FSCopyObjectSync(
				&sourceRef,
				&targetParentRef,
				appNameRef,
				&targetRef,
				kFSFileOperationDefaultOptions);
		
		// Grab the path for later use.
		err = FSRefMakePath(&targetRef, (UInt8*)target, sizeof(target));
		if(err != noErr)
			throw 0;

		llinfos << "Copy complete. Target = " << target << llendl;

		if(err != noErr)
		{
			// Something went wrong during the copy.  Attempt to put the old version back and bail.
			(void)FSDeleteObject(&targetRef);
			if(replacingTarget)
			{
				(void)FSMoveObject(&asideRef, &targetParentRef, NULL);
			}
			throw 0;
		}
		else
		{
			// The update has succeeded.  Clear the cache directory.

			sendProgress(0, 0, CFSTR("Clearing cache..."));
	
			llinfos << "Clearing cache..." << llendl;
			
			gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,""), "*.*");
			
			llinfos << "Clear complete." << llendl;

		}
	}
	catch(...)
	{
		if(!gCancelled)
			if(gFailure == noErr)
				gFailure = -1;
	}

	// Failures from here on out are all non-fatal and not reported.
	sendProgress(0, 3, CFSTR("Cleaning up..."));

	// Close disk image file if necessary
	if(downloadFile != NULL)
	{
		llinfos << "Closing download file." << llendl;

		fclose(downloadFile);
		downloadFile = NULL;
	}

	sendProgress(1, 3);
	// Unmount image
	if(deviceNode[0] != 0)
	{
		llinfos << "Detaching disk image." << llendl;

		snprintf(temp, sizeof(temp), "hdiutil detach '%s'", deviceNode);		
		system(temp);		/* Flawfinder: ignore */
	}

	sendProgress(2, 3);

	// Move work directory to the trash
	if(tempDir[0] != 0)
	{
		llinfos << "Moving work directory to the trash." << llendl;

		FSRef trashRef;
		OSStatus err = FSMoveObjectToTrashSync(&tempDirRef, &trashRef, 0); 
		if(err != noErr) {
			llwarns << "failed to move files to trash, (error code " <<
				err << ")" << llendl;
		}
	}
	
	if(!gCancelled  && !gFailure && (target[0] != 0))
	{
		llinfos << "Touching application bundle." << llendl;

		snprintf(temp, sizeof(temp), "touch '%s'", target);		
		system(temp);		/* Flawfinder: ignore */

		llinfos << "Launching updated application." << llendl;

		snprintf(temp, sizeof(temp), "open '%s'", target);		
		system(temp);		/* Flawfinder: ignore */
	}

	sendDone();
	
	return(NULL);
}
