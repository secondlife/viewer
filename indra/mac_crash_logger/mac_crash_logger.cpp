/** 
 * @file mac_crash_logger.cpp
 * @brief Mac OSX crash logger implementation
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include <stdio.h>
#include <stdlib.h>
//#include <direct.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curl/curl.h>

#include "llerror.h"
#include "lltimer.h"
#include "lldir.h"

#include "llstring.h"

class LLFileEncoder
{
public:
	LLFileEncoder(const char *formname, const char *filename, bool isCrashLog = false);

	BOOL isValid() const { return mIsValid; }
	LLString encodeURL(const S32 max_length = 0);
public:
	BOOL mIsValid;
	LLString mFilename;
	LLString mFormname;
	LLString mBuf;
};

LLString encode_string(const char *formname, const LLString &str);

#include <Carbon/Carbon.h>

LLString gServerResponse;
BOOL gSendReport = FALSE;
LLString gUserserver;
LLString gUserText;
WindowRef gWindow = NULL;
EventHandlerRef gEventHandler = NULL;
BOOL gCrashInPreviousExec = FALSE;
time_t gLaunchTime;

size_t curl_download_callback(void *data, size_t size, size_t nmemb,
										  void *user_data)
{
	S32 bytes = size * nmemb;
	char *cdata = (char *) data;
	for (int i =0; i < bytes; i += 1)
	{
		gServerResponse += (cdata[i]);
	}
	return bytes;
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
				case kHICommandOK:
				{
					char buffer[65535];
					Size size = sizeof(buffer) - 1;
					ControlRef textField = NULL;
					ControlID id;

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
						gUserText = buffer;
						llinfos << buffer << llendl;
					}
					
					// Send the report.
					gSendReport = TRUE;

					QuitAppModalLoopForWindow(gWindow);
					result = noErr;
				}
				break;
				
				case kHICommandCancel:
					QuitAppModalLoopForWindow(gWindow);
					result = noErr;
				break;
			}
		}
	}
	
	return(result);
}

int main(int argc, char **argv)
{
	const S32 DW_MAX_SIZE = 100000;			// Maximum size to transmit of the Dr. Watson log file
	const S32 SL_MAX_SIZE = 100000;			// Maximum size of the Second Life log file.
	int i;
	
	time(&gLaunchTime);
	
	llinfos << "Starting Second Life Viewer Crash Reporter" << llendl;
	
	for(i=1; i<argc; i++)
	{
		if(!strcmp(argv[i], "-previous"))
		{
			gCrashInPreviousExec = TRUE;
		}
		if(!strcmp(argv[i], "-user"))
		{
			if ((i + 1) < argc)
			{
				i++;
				gUserserver = argv[i];
				llinfos << "Got userserver " << gUserserver << llendl;
			}
		}
	}
	
	if( gCrashInPreviousExec )
	{
		llinfos << "Previous execution did not remove SecondLife.exec_marker" << llendl;
	}
	
	if(!gCrashInPreviousExec)
	{
		// Delay five seconds to let CrashReporter do its thing.
		sleep(5);
	}
		
#if 1
	// Real UI...
	OSStatus err;
	IBNibRef nib = NULL;
	
	err = CreateNibReference(CFSTR("CrashReporter"), &nib);
	
	if(err == noErr)
	{
		if(gCrashInPreviousExec)
		{
			err = CreateWindowFromNib(nib, CFSTR("CrashReporterDelayed"), &gWindow);
		}
		else
		{
			err = CreateWindowFromNib(nib, CFSTR("CrashReporter"), &gWindow);
		}
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
			
	if(err == noErr)
	{
		RunAppModalLoopForWindow(gWindow);
	}
			
	if(gWindow != NULL)
	{
		DisposeWindow(gWindow);
	}
	
	if(nib != NULL)
	{
		DisposeNibReference(nib);
	}
#else
	// Cheap version -- just use the standard system alert.
	SInt16 itemHit = 0;
	AlertStdCFStringAlertParamRec params;
	OSStatus err = noErr;
	DialogRef alert = NULL;

	params.version = kStdCFStringAlertVersionOne;
	params.movable = false;
	params.helpButton = false;
	params.defaultText = CFSTR("Send Report");
	params.cancelText = CFSTR("Don't Send Report");
	params.otherText = 0;
	params.defaultButton = kAlertStdAlertOKButton;
	params.cancelButton = kAlertStdAlertCancelButton;
	params.position = kWindowDefaultPosition;
	params.flags = 0;

	err = CreateStandardAlert(
			kAlertCautionAlert,
			CFSTR("Second Life appears to have crashed."),
			CFSTR(
			"To help us debug the problem, you can send a crash report to Linden Lab.  "
			"The report contains information about your microprocessor type, graphics card, "
			"memory, things that happened during the last run of the program, and the Crash Log "
			"of where the crash occurred.\r"
			"\r"
			"You may also report crashes in the forums, or by sending e-mail to peter@lindenlab.com"),
			&params,
			&alert);
	
	if(err == noErr)
	{
		err = RunStandardAlert(
				alert,
				NULL,
				&itemHit);
  	}
	
	if(itemHit == kAlertStdAlertOKButton)
		gSendReport = TRUE;
#endif
	
	if(!gSendReport)
	{
		// Only send the report if the user agreed to it.
		llinfos << "User cancelled, not sending report" << llendl;

		return(0);
	}

	// We assume that all the logs we're looking for reside on the current drive
	gDirUtilp->initAppDirs("SecondLife");

	int res;

	// Lots of silly variable, replicated for each log file.
	LLString db_file_name;
	LLString sl_file_name;
	LLString dw_file_name; // DW file name is a hack for now...
	LLString st_file_name; // stats.log file
	LLString si_file_name; // settings.ini file

	LLFileEncoder *db_filep = NULL;
	LLFileEncoder *sl_filep = NULL;
	LLFileEncoder *st_filep = NULL;
	LLFileEncoder *dw_filep = NULL;
	LLFileEncoder *si_filep = NULL;

	///////////////////////////////////
	//
	// We do the parsing for the debug_info file first, as that will
	// give us the location of the SecondLife.log file.
	//

	// Figure out the filename of the debug log
	db_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"debug_info.log").c_str();
	db_filep = new LLFileEncoder("DB", db_file_name.c_str());

	// Get the filename of the SecondLife.log file

	// *NOTE: changing the size of either of these buffers will
	// require changing the sscanf() format string to correctly
	// account for it.
	char tmp_sl_name[LL_MAX_PATH];
	tmp_sl_name[0] = '\0';
	char tmp_space[MAX_STRING];
	tmp_space[0] = '\0';

	// Look for it in the debug_info.log file
	if (db_filep->isValid())
	{
		// This was originally scanning for "SL Log: %[^\r\n]", which happily skipped to the next line
		// on debug logs (which don't have anything after "SL Log:" and tried to open a nonsensical filename.
		sscanf(
			db_filep->mBuf.c_str(),
			"SL Log:%254[ ]%1023[^\r\n]",
			tmp_space,
			tmp_sl_name);
	}
	else
	{
		delete db_filep;
		db_filep = NULL;
	}

	// If we actually have a legitimate file name, use it.
	if (tmp_sl_name[0])
	{
		sl_file_name = tmp_sl_name;
		llinfos << "Using log file from debug log " << sl_file_name << llendl;
	}
	else
	{
		// Figure out the filename of the second life log
		sl_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.log").c_str();
	}

	// Now we get the SecondLife.log file if it's there, and recent enough...
	sl_filep = new LLFileEncoder("SL", sl_file_name.c_str());
	if (!sl_filep->isValid())
	{
		delete sl_filep;
		sl_filep = NULL;
	}

	st_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stats.log").c_str();
	st_filep = new LLFileEncoder("ST", st_file_name.c_str());
	if (!st_filep->isValid())
	{
		delete st_filep;
		st_filep = NULL;
	}

	si_file_name = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,"settings.ini").c_str();
	si_filep = new LLFileEncoder("SI", si_file_name.c_str());
	if (!si_filep->isValid())
	{
		delete si_filep;
		si_filep = NULL;
	}

	// MBW -- This needs to go find "~/Library/Logs/CrashReporter/Second Life.crash.log" on 10.3
	// or "~/Library/Logs/Second Life.crash.log" on 10.2.
	{
		char path[MAX_PATH];
		FSRef folder;
		
		if(FSFindFolder(kUserDomain, kLogsFolderType, false, &folder) == noErr)
		{
			// folder is an FSRef to ~/Library/Logs/
			if(FSRefMakePath(&folder, (UInt8*)&path, sizeof(path)) == noErr)
			{
				struct stat dw_stat;
//				printf("path is %s\n", path);
				
				// Try the 10.3 path first...
				dw_file_name = LLString(path) + LLString("/CrashReporter/Second Life.crash.log");
				res = stat(dw_file_name.c_str(), &dw_stat);

				if (res)
				{
					// Try the 10.2 one next...
					dw_file_name = LLString(path) + LLString("/Second Life.crash.log");
					res = stat(dw_file_name.c_str(), &dw_stat);
				}
				
				if (!res)
				{
					dw_filep = new LLFileEncoder("DW", dw_file_name.c_str(), true);
					if (!dw_filep->isValid())
					{
						delete dw_filep;
						dw_filep = NULL;
					}
				}
				else
				{
					llwarns << "Couldn't find any CrashReporter files..." << llendl;
				}
			}
		}
	}

	LLString post_data;
	LLString tmp_url_buf;

	// Append the userserver
	tmp_url_buf = encode_string("USER", gUserserver);
	post_data += tmp_url_buf;
	llinfos << "PostData:" << post_data << llendl;

	if (gCrashInPreviousExec)
	{
		post_data.append("&");
		tmp_url_buf = encode_string("EF", "Y");
		post_data += tmp_url_buf;
	}

	if (db_filep)
	{
		post_data.append("&");
		tmp_url_buf = db_filep->encodeURL();
		post_data += tmp_url_buf;
		llinfos << "Sending DB log file" << llendl;
	}
	else
	{
		llinfos << "Not sending DB log file" << llendl;
	}

	if (sl_filep)
	{
		post_data.append("&");
		tmp_url_buf = sl_filep->encodeURL(SL_MAX_SIZE);
		post_data += tmp_url_buf;
		llinfos << "Sending SL log file" << llendl;
	}
	else
	{
		llinfos << "Not sending SL log file" << llendl;
	}

	if (st_filep)
	{
		post_data.append("&");
		tmp_url_buf = st_filep->encodeURL(SL_MAX_SIZE);
		post_data += tmp_url_buf;
		llinfos << "Sending stats log file" << llendl;
	}
	else
	{
		llinfos << "Not sending stats log file" << llendl;
	}

	if (dw_filep)
	{
		post_data.append("&");
		tmp_url_buf = dw_filep->encodeURL(DW_MAX_SIZE);
		post_data += tmp_url_buf;
	}
	else
	{
		llinfos << "Not sending crash log file" << llendl;
	}

	if (si_filep)
	{
		post_data.append("&");
		tmp_url_buf = si_filep->encodeURL();
		post_data += tmp_url_buf;
		llinfos << "Sending settings log file" << llendl;
	}
	else
	{
		llinfos << "Not sending settings.ini file" << llendl;
	}

	if (gUserText.size())
	{
		post_data.append("&");
		tmp_url_buf = encode_string("UN", gUserText);
		post_data += tmp_url_buf;
	}

	delete db_filep;
	db_filep = NULL;
	delete sl_filep;
	sl_filep = NULL;
	delete dw_filep;
	dw_filep = NULL;

	// Debugging spam
#if 0
	printf("Crash report post data:\n--------\n");
	printf("%s", post_data.getString());
	printf("\n--------\n");
#endif
	
	// Send the report.  Yes, it's this easy.
	{
		CURL *curl = curl_easy_init();

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_download_callback);
		curl_easy_setopt(curl, CURLOPT_POST, 1); 
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str()); 
		curl_easy_setopt(curl, CURLOPT_URL,	"http://secondlife.com/cgi-bin/viewer_crash_reporter2");
		
		llinfos << "Connecting to crash report server" << llendl;
		CURLcode result = curl_easy_perform(curl);
		
		curl_easy_cleanup(curl);
		
		if(result != CURLE_OK)
		{
			llinfos << "Couldn't talk to crash report server" << llendl;
		}
		else
		{
			llinfos << "Response from crash report server:" << llendl;
			llinfos << gServerResponse << llendl;			
		}
	}
	
	return 0;
}

LLFileEncoder::LLFileEncoder(const char *form_name, const char *filename, bool isCrashLog)
{
	mFormname = form_name;
	mFilename = filename;
	mIsValid = FALSE;

	int res;
	
	struct stat stat_data;
	res = stat(mFilename.c_str(), &stat_data);
	if (res)
	{
		llwarns << "File " << mFilename << " is missing!" << llendl;
		return;
	}
	else
	{
		// Debugging spam
//		llinfos << "File " << mFilename << " is present..." << llendl;

		if(!gCrashInPreviousExec && isCrashLog)
		{
			// Make sure the file isn't too old.
			double age = difftime(gLaunchTime, stat_data.st_mtimespec.tv_sec);

//			llinfos << "age is " << age << llendl;

			if(age > 60.0)
			{
				// The file was last modified more than 60 seconds before the crash reporter was launched.  Assume it's stale.
				llwarns << "File " << mFilename << " is too old!" << llendl;
				return;
			}
		}

	}

	S32 buf_size = stat_data.st_size;
	FILE *fp = fopen(mFilename.c_str(), "rb");
	U8 *buf = new U8[buf_size + 1];
	fread(buf, 1, buf_size, fp);
	fclose(fp);
	buf[buf_size] = 0;

	mBuf = (char *)buf;
	
	if(isCrashLog)
	{
		// Crash logs consist of a number of entries, one per crash.
		// Each entry is preceeded by "**********" on a line by itself.
		// We want only the most recent (i.e. last) one.
		const char *sep = "**********";
		const char *start = mBuf.c_str();
		const char *cur = start;
		const char *temp = strstr(cur, sep);
		
		while(temp != NULL)
		{
			// Skip past the marker we just found
			cur = temp + strlen(sep);
			
			// and try to find another
			temp = strstr(cur, sep);
		}
		
		// If there's more than one entry in the log file, strip all but the last one.
		if(cur != start)
		{
			mBuf.erase(0, cur - start);
		}
	}

	mIsValid = TRUE;
	delete[] buf;
}

LLString LLFileEncoder::encodeURL(const S32 max_length)
{
	LLString result = mFormname;
	result.append("=");

	S32 i = 0;

	if (max_length)
	{
		if (mBuf.size() > max_length)
		{
			i = mBuf.size() - max_length;
		}
	}

#if 0
	// Plain text version for debugging
	result.append(mBuf);
#else
	// Not using LLString because of bad performance issues
	S32 buf_size = mBuf.size();
	S32 url_buf_size = 3*mBuf.size() + 1;
	char *url_buf = new char[url_buf_size];

	S32 cur_pos = 0;
	for (; i < buf_size; i++)
	{
		sprintf(url_buf + cur_pos, "%%%02x", mBuf[i]);
		cur_pos += 3;
	}
	url_buf[i*3] = 0;

	result.append(url_buf);
	delete[] url_buf;
#endif
	return result;
}

LLString encode_string(const char *formname, const LLString &str)
{
	LLString result = formname;
	result.append("=");
	// Not using LLString because of bad performance issues
	S32 buf_size = str.size();
	S32 url_buf_size = 3*str.size() + 1;
	char *url_buf = new char[url_buf_size];

	S32 cur_pos = 0;
	S32 i;
	for (i = 0; i < buf_size; i++)
	{
		sprintf(url_buf + cur_pos, "%%%02x", str[i]);
		cur_pos += 3;
	}
	url_buf[i*3] = 0;

	result.append(url_buf);
	delete[] url_buf;
	return result;
}
