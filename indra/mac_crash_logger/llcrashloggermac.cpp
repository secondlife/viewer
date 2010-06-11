/** 
 * @file llcrashloggermac.cpp
 * @brief Mac OSX crash logger implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */


#include "llcrashloggermac.h"

#include <Carbon/Carbon.h>
#include <iostream>
#include <sstream>

#include "boost/tokenizer.hpp"

#include "indra_constants.h"	// CRASH_BEHAVIOR_ASK, CRASH_SETTING_NAME
#include "llerror.h"
#include "llfile.h"
#include "lltimer.h"
#include "llstring.h"
#include "lldir.h"
#include "llsdserialize.h"

#define MAX_LOADSTRING 100
const char* const SETTINGS_FILE_HEADER = "version";
const S32 SETTINGS_FILE_VERSION = 101;

// Windows Message Handlers

BOOL gFirstDialog = TRUE;	// Are we currently handling the Send/Don't Send dialog?
LLFILE *gDebugFile = NULL;

WindowRef gWindow = NULL;
EventHandlerRef gEventHandler = NULL;
std::string gUserNotes = "";
bool gSendReport = false;
bool gRememberChoice = false;
IBNibRef nib = NULL;

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
			//Get the value of the checkbox
			ControlID id;
			ControlRef checkBox = NULL;
			id.signature = 'remb';
			id.id = 0;
			err = GetControlByID(gWindow, &id, &checkBox);
			
			if(err == noErr)
			{
				if(GetControl32BitValue(checkBox) == kControlCheckBoxCheckedValue)
				{
					gRememberChoice = true;
				}
				else
				{
					gRememberChoice = false;
				}
			}	
			switch(cmd.commandID)
			{
				case kHICommandOK:
				{
					char buffer[65535];		/* Flawfinder: ignore */
					Size size = sizeof(buffer) - 1;
					ControlRef textField = NULL;

					id.signature = 'text';
					id.id = 0;

					err = GetControlByID(gWindow, &id, &textField);
					if(err == noErr)
					{
						// Get the user response text
						err = GetControlData(textField, kControlNoPart, kControlEditTextTextTag, size, (Ptr)buffer, &size);
					}
					if(err == noErr)
					{
						// Make sure the string is terminated.
						buffer[size] = 0;
						gUserNotes = buffer;

						llinfos << buffer << llendl;
					}
					
					// Send the report.

					QuitAppModalLoopForWindow(gWindow);
					gSendReport = true;
					result = noErr;
				}
				break;
				
				case kHICommandCancel:
					QuitAppModalLoopForWindow(gWindow);
					result = noErr;
				break;
				default:
					result = eventNotHandledErr;
			}
		}
	}

	return(result);
}


LLCrashLoggerMac::LLCrashLoggerMac(void)
{
}

LLCrashLoggerMac::~LLCrashLoggerMac(void)
{
}

bool LLCrashLoggerMac::init(void)
{	
	bool ok = LLCrashLogger::init();
	if(!ok) return false;
	if(mCrashBehavior != CRASH_BEHAVIOR_ASK) return true;
	
	// Real UI...
	OSStatus err;
	
	err = CreateNibReference(CFSTR("CrashReporter"), &nib);
	
	if(err == noErr)
	{
		err = CreateWindowFromNib(nib, CFSTR("CrashReporter"), &gWindow);
	}

	if(err == noErr)
	{
		// Set focus to the edit text area
		ControlRef textField = NULL;
		ControlID id;

		id.signature = 'text';
		id.id = 0;
		
		// Don't set err if any of this fails, since it's non-critical.
		if(GetControlByID(gWindow, &id, &textField) == noErr)
		{
			SetKeyboardFocus(gWindow, textField, kControlFocusNextPart);
		}
	}
	
	if(err == noErr)
	{
		ShowWindow(gWindow);
	}
	
	if(err == noErr)
	{
		// Set up an event handler for the window.
		EventTypeSpec handlerEvents[] = 
		{
			{ kEventClassCommand, kEventCommandProcess }
		};

		InstallWindowEventHandler(
				gWindow, 
				NewEventHandlerUPP(dialogHandler), 
				GetEventTypeCount (handlerEvents), 
				handlerEvents, 
				0, 
				&gEventHandler);
	}
	return true;
}

void LLCrashLoggerMac::gatherPlatformSpecificFiles()
{
	updateApplication("Gathering hardware information...");
}

bool LLCrashLoggerMac::mainLoop()
{
	OSStatus err = noErr;
				
	if(err == noErr && mCrashBehavior == CRASH_BEHAVIOR_ASK)
	{
		RunAppModalLoopForWindow(gWindow);
	}
	else if (mCrashBehavior == CRASH_BEHAVIOR_ALWAYS_SEND)
	{
		gSendReport = true;
	}
	
	if(gRememberChoice)
	{
		if(gSendReport) saveCrashBehaviorSetting(CRASH_BEHAVIOR_ALWAYS_SEND);
		else saveCrashBehaviorSetting(CRASH_BEHAVIOR_NEVER_SEND);
	}
	
	if(gSendReport)
	{
		setUserText(gUserNotes);
		sendCrashLogs();
	}		
	
	if(gWindow != NULL)
	{
		DisposeWindow(gWindow);
	}
	
	if(nib != NULL)
	{
		DisposeNibReference(nib);
	}
	
	return true;
}

void LLCrashLoggerMac::updateApplication(const std::string& message)
{
	LLCrashLogger::updateApplication();
}

bool LLCrashLoggerMac::cleanup()
{
	return true;
}
