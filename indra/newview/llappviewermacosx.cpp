/**
 * @file llappviewermacosx.cpp
 * @brief The LLAppViewerWin32 class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llviewerprecompiledheaders.h"

#if !defined LL_DARWIN
	#error "Use only with Mac OS X"
#endif

#include "llappviewermacosx.h"
#include "llcommandlineparser.h"

#include "llmemtype.h"

#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llmd5.h"
#include "llurlsimstring.h"
#include "llfloaterworldmap.h"
#include "llurldispatcher.h"
#include <Carbon/Carbon.h>
#include "lldir.h"
namespace 
{
	// The command line args stored.
	// They are not used immediately by the app.
	int gArgC;
	char** gArgV;
}

int main( int argc, char **argv ) 
{
	LLMemType mt1(LLMemType::MTYPE_STARTUP);

#if LL_SOLARIS && defined(__sparc)
	asm ("ta\t6");		 // NOTE:  Make sure memory alignment is enforced on SPARC
#endif

	// Set the working dir to <bundle>/Contents/Resources
	if (chdir(gDirUtilp->getAppRODataDir().c_str()) == -1)
	{
		llwarns << "Could not change directory to "
				<< gDirUtilp->getAppRODataDir() << ": " << strerror(errno)
				<< llendl;
	}

	LLAppViewerMacOSX* viewer_app_ptr = new LLAppViewerMacOSX();

	viewer_app_ptr->setErrorHandler(LLAppViewer::handleViewerCrash);

	// Store off the command line args for use later.
	gArgC = argc;
	gArgV = argv;
	
	bool ok = viewer_app_ptr->init();
	if(!ok)
	{
		llwarns << "Application init failed." << llendl;
		return -1;
	}

		// Run the application main loop
	if(!LLApp::isQuitting()) 
	{
		viewer_app_ptr->mainLoop();
	}

	if (!LLApp::isError())
	{
		//
		// We don't want to do cleanup here if the error handler got called -
		// the assumption is that the error handler is responsible for doing
		// app cleanup if there was a problem.
		//
		viewer_app_ptr->cleanup();
	}
	delete viewer_app_ptr;
	viewer_app_ptr = NULL;
	return 0;
}

LLAppViewerMacOSX::LLAppViewerMacOSX()
{
}

LLAppViewerMacOSX::~LLAppViewerMacOSX()
{
}

bool LLAppViewerMacOSX::init()
{
	return LLAppViewer::init();
}

// MacOSX may add and addition command line arguement for the process serial number.
// The option takes a form like '-psn_0_12345'. The following method should be able to recognize
// and either ignore or return a pair of values for the option.
// look for this method to be added to the parser in parseAndStoreResults.
std::pair<std::string, std::string> parse_psn(const std::string& s)
{
    if (s.find("-psn_") == 0) 
	{
		// *FIX:Mani Not sure that the value makes sense.
		// fix it once the actual -psn_XXX syntax is known.
		return std::make_pair("psn", s.substr(5));
    }
	else 
	{
        return std::make_pair(std::string(), std::string());
    }
}

bool LLAppViewerMacOSX::initParseCommandLine(LLCommandLineParser& clp)
{
	// The next two lines add the support for parsing the mac -psn_XXX arg.
	clp.addOptionDesc("psn", NULL, 1, "MacOSX process serial number");
	clp.setCustomParser(parse_psn);
	
	// First parse the command line, not often used on the mac.
	if(clp.parseCommandLine(gArgC, gArgV) == false)
	{
		return false;
	}
    
    // Now read in the args from arguments txt.
    // Succesive calls to clp.parse... will NOT override earlier 
    // options. 
    const char* filename = "arguments.txt";
	llifstream ifs(filename, llifstream::binary);
	if (!ifs.is_open())
	{
		llwarns << "Unable to open file" << filename << llendl;
		return false;
	}
	
	if(clp.parseCommandLineFile(ifs) == false)
	{
		return false;
	}
	
	// Get the user's preferred language string based on the Mac OS localization mechanism.
	// To add a new localization:
		// go to the "Resources" section of the project
		// get info on "language.txt"
		// in the "General" tab, click the "Add Localization" button
		// create a new localization for the language you're adding
		// set the contents of the new localization of the file to the string corresponding to our localization
		//   (i.e. "en-us", "ja", etc.  Use the existing ones as a guide.)
	CFURLRef url = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("language"), CFSTR("txt"), NULL);
	char path[MAX_PATH];
	if(CFURLGetFileSystemRepresentation(url, false, (UInt8 *)path, sizeof(path)))
	{
		LLString lang;
		if(_read_file_into_string(lang, path))		/* Flawfinder: ignore*/
		{
            LLControlVariable* c = gSavedSettings.getControl("SystemLanguage");
            if(c)
            {
                c->setValue(lang, false);
            }
		}
	}
	CFRelease(url);
	
    return true;
}

void LLAppViewerMacOSX::handleSyncCrashTrace()
{
	// do nothing
}

void LLAppViewerMacOSX::handleCrashReporting()
{
	// Macintosh
	LLString command_str;
	command_str += "open crashreporter.app";	
	
	clear_signals();
	llinfos << "Launching crash reporter using: '" << command_str << "'" << llendl;
	system(command_str.c_str());		/* Flawfinder: ignore */
	llinfos << "returned from crash reporter... dying" << llendl;	
	_exit(1);
}

std::string LLAppViewerMacOSX::generateSerialNumber()
{
	char serial_md5[MD5HEX_STR_SIZE];		// Flawfinder: ignore
	serial_md5[0] = 0;

	// JC: Sample code from http://developer.apple.com/technotes/tn/tn1103.html
	CFStringRef serialNumber = NULL;
	io_service_t    platformExpert = IOServiceGetMatchingService(kIOMasterPortDefault,
																 IOServiceMatching("IOPlatformExpertDevice"));
	if (platformExpert) {
		serialNumber = (CFStringRef) IORegistryEntryCreateCFProperty(platformExpert,
																	 CFSTR(kIOPlatformSerialNumberKey),
																	 kCFAllocatorDefault, 0);		
		IOObjectRelease(platformExpert);
	}
	
	if (serialNumber)
	{
		char buffer[MAX_STRING];		// Flawfinder: ignore
		if (CFStringGetCString(serialNumber, buffer, MAX_STRING, kCFStringEncodingASCII))
		{
			LLMD5 md5( (unsigned char*)buffer );
			md5.hex_digest(serial_md5);
		}
		CFRelease(serialNumber);
	}

	return serial_md5;
}

OSErr AEGURLHandler(const AppleEvent *messagein, AppleEvent *reply, long refIn)
{
	OSErr result = noErr;
	DescType actualType;
	char buffer[1024];		// Flawfinder: ignore
	Size size;
	
	result = AEGetParamPtr (
		messagein,
		keyDirectObject,
		typeCString,
		&actualType,
		(Ptr)buffer,
		sizeof(buffer),
		&size);	
	
	if(result == noErr)
	{
		std::string url = buffer;
		const bool from_external_browser = true;
		LLURLDispatcher::dispatch(url, from_external_browser);
	}
	
	return(result);
}

OSErr AEQuitHandler(const AppleEvent *messagein, AppleEvent *reply, long refIn)
{
	OSErr result = noErr;
	
	LLAppViewer::instance()->userQuit();
	
	return(result);
}

OSStatus simpleDialogHandler(EventHandlerCallRef handler, EventRef event, void *userdata)
{
	OSStatus result = eventNotHandledErr;
	OSStatus err;
	UInt32 evtClass = GetEventClass(event);
	UInt32 evtKind = GetEventKind(event);
	WindowRef window = (WindowRef)userdata;
	
	if((evtClass == kEventClassCommand) && (evtKind == kEventCommandProcess))
	{
		HICommand cmd;
		err = GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(cmd), NULL, &cmd);
		
		if(err == noErr)
		{
			switch(cmd.commandID)
			{
				case kHICommandOK:
					QuitAppModalLoopForWindow(window);
					result = noErr;
				break;
				
				case kHICommandCancel:
					QuitAppModalLoopForWindow(window);
					result = userCanceledErr;
				break;
			}
		}
	}
	
	return(result);
}

OSStatus DisplayReleaseNotes(void)
{
	OSStatus err;
	IBNibRef nib = NULL;
	WindowRef window = NULL;
	
	err = CreateNibReference(CFSTR("SecondLife"), &nib);
	
	if(err == noErr)
	{
		CreateWindowFromNib(nib, CFSTR("Release Notes"), &window);
	}
		
	if(err == noErr)
	{
		// Get the text view control
		HIViewRef textView;
		ControlID id;

		id.signature = 'text';
		id.id = 0;

		LLString releaseNotesText;
		
		_read_file_into_string(releaseNotesText, "releasenotes.txt");		// Flawfinder: ignore

		err = HIViewFindByID(HIViewGetRoot(window), id, &textView);
		
		if(err == noErr)
		{
			// Convert from the encoding used in the release notes.
			CFStringRef str = CFStringCreateWithBytes(
				NULL, 
				(const UInt8*)releaseNotesText.c_str(), 
				releaseNotesText.size(), 
				kCFStringEncodingWindowsLatin1, 		// This matches the way the Windows version displays the release notes.
				FALSE);
			
			if(str != NULL)
			{
				int size = CFStringGetLength(str);

				if(size > 0)
				{
					UniChar *chars = new UniChar[size + 1];
					CFStringGetCharacters(str, CFRangeMake(0, size), chars);
				
					err = TXNSetData(HITextViewGetTXNObject(textView), kTXNUnicodeTextData, chars, size * sizeof(UniChar), kTXNStartOffset, kTXNStartOffset);
					
					delete[] chars;
				}
				
				CFRelease(str);
			}
			else
			{
				// Creating the string failed.  Probably an encoding problem.  Display SOMETHING...
				err = TXNSetData(HITextViewGetTXNObject(textView), kTXNTextData, releaseNotesText.c_str(), releaseNotesText.size(), kTXNStartOffset, kTXNStartOffset);
			}
		}
		
		// Set the selection to the beginning of the text and scroll it into view.
		if(err == noErr)
		{
			err = TXNSetSelection(HITextViewGetTXNObject(textView), kTXNStartOffset, kTXNStartOffset);
		}
		
		if(err == noErr)
		{
			// This function returns void.
			TXNShowSelection(HITextViewGetTXNObject(textView), false);
		}
	}

	if(err == noErr)
	{
		ShowWindow(window);
	}

	if(err == noErr)
	{
		// Set up an event handler for the window.
		EventHandlerRef handler = NULL;
		EventTypeSpec handlerEvents[] = 
		{
			{ kEventClassCommand, kEventCommandProcess }
		};

		InstallWindowEventHandler(
				window, 
				NewEventHandlerUPP(simpleDialogHandler), 
				GetEventTypeCount (handlerEvents), 
				handlerEvents, 
				(void*)window, 
				&handler);
	}
			
	if(err == noErr)
	{
		RunAppModalLoopForWindow(window);
	}
			
	if(window != NULL)
	{
		DisposeWindow(window);
	}
	
	if(nib != NULL)
	{
		DisposeNibReference(nib);
	}

	return(err);
}

void init_apple_menu(const char* product)
{
	// Load up a proper menu bar.
	{
		OSStatus err;
		IBNibRef nib = NULL;
		// NOTE: DO NOT translate or brand this string.  It's an internal name in the .nib file, and MUST match exactly.
		err = CreateNibReference(CFSTR("SecondLife"), &nib);
		
		if(err == noErr)
		{
			// NOTE: DO NOT translate or brand this string.  It's an internal name in the .nib file, and MUST match exactly.
			SetMenuBarFromNib(nib, CFSTR("MenuBar"));
		}

		if(nib != NULL)
		{
			DisposeNibReference(nib);
		}
	}
	
	// Install a handler for 'gurl' AppleEvents.  This is how secondlife:// URLs get passed to the viewer.
	
	if(AEInstallEventHandler('GURL', 'GURL', NewAEEventHandlerUPP(AEGURLHandler),0, false) != noErr)
	{
		// Couldn't install AppleEvent handler.  This error shouldn't be fatal.
		llinfos << "Couldn't install 'GURL' AppleEvent handler.  Continuing..." << llendl;
	}

	// Install a handler for 'quit' AppleEvents.  This makes quitting the application from the dock work.
	if(AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP(AEQuitHandler),0, false) != noErr)
	{
		// Couldn't install AppleEvent handler.  This error shouldn't be fatal.
		llinfos << "Couldn't install Quit AppleEvent handler.  Continuing..." << llendl;
	}
}
