/** 
 * @file linux_crash_logger.cpp
 * @brief Linux crash logger implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#include "linden_common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curl/curl.h>

#if LL_GTK
# include "gtk/gtk.h"
#endif // LL_GTK

#include "indra_constants.h"	// CRASH_BEHAVIOR_ASK
#include "llerror.h"
#include "lltimer.h"
#include "lldir.h"

#include "llstring.h"


// These need to be localized.
static const char dialog_text[] =
"Second Life appears to have crashed.\n"
"This crash reporter collects information about your computer's hardware, operating system, and some Second Life logs, which are used for debugging purposes only.\n"
"Sending crash reports is the best way to help us improve the quality of Second Life.\n"
"If you continue to experience this problem, please try:\n"
"- Contacting support by visiting http://www.secondlife.com/support\n"
"\n"
"Send crash report?";

static const char dialog_title[] =
"Second Life Crash Logger";


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

LLString gServerResponse;
BOOL gSendReport = FALSE;
LLString gUserserver;
LLString gUserText;
BOOL gCrashInPreviousExec = FALSE;
time_t gLaunchTime;

static size_t curl_download_callback(void *data, size_t size, size_t nmemb,
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

#if LL_GTK
static void response_callback (GtkDialog *dialog,
			       gint       arg1,
			       gpointer   user_data)
{
	gint *response = (gint*)user_data;
	*response = arg1;
	gtk_widget_destroy(GTK_WIDGET(dialog));
	gtk_main_quit();
}
#endif // LL_GTK

static BOOL do_ask_dialog(void)
{
#if LL_GTK
	gtk_disable_setlocale();
	if (!gtk_init_check(NULL, NULL)) {
		llinfos << "Could not initialize GTK for 'ask to send crash report' dialog; not sending report." << llendl;
		return FALSE;
	}
	
	GtkWidget *win = NULL;
	GtkDialogFlags flags = GTK_DIALOG_MODAL;
	GtkMessageType messagetype = GTK_MESSAGE_QUESTION;
	GtkButtonsType buttons = GTK_BUTTONS_YES_NO;
	gint response = GTK_RESPONSE_NONE;

	win = gtk_message_dialog_new(NULL,
				     flags, messagetype, buttons,
				     dialog_text);
	gtk_window_set_type_hint(GTK_WINDOW(win),
				 GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title(GTK_WINDOW(win), dialog_title);
	g_signal_connect (win,
			  "response", 
			  G_CALLBACK (response_callback),
			  &response);
	gtk_widget_show_all (win);
	gtk_main();

	return (GTK_RESPONSE_OK == response ||
		GTK_RESPONSE_YES == response ||
		GTK_RESPONSE_APPLY == response);
#else
	return FALSE;
#endif // LL_GTK
}


int main(int argc, char **argv)
{
	const S32 BT_MAX_SIZE = 100000;			// Maximum size to transmit of the backtrace file
	const S32 SL_MAX_SIZE = 100000;			// Maximum size of the Second Life log file.
	int i;
	S32 crash_behavior = CRASH_BEHAVIOR_ALWAYS_SEND;
	
	time(&gLaunchTime);
	
	llinfos << "Starting Second Life Viewer Crash Reporter" << llendl;
	
	for(i=1; i<argc; i++)
	{
		if(!strcmp(argv[i], "-dialog"))
		{
			llinfos << "Show the user dialog" << llendl;
			crash_behavior = CRASH_BEHAVIOR_ASK;
		}
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
		// Wait a while to let the crashed client finish exiting,
		// freeing up the screen/etc.
		sleep(5);
	}

	// *FIX: do some dialog stuff here?
	if (CRASH_BEHAVIOR_ALWAYS_SEND == crash_behavior)
	{
		gSendReport = TRUE;
	}
	else if (CRASH_BEHAVIOR_ASK == crash_behavior)
	{
		gSendReport = do_ask_dialog();
	}
	
	if(!gSendReport)
	{
		// Only send the report if the user agreed to it.
		llinfos << "User cancelled, not sending report" << llendl;

		return(0);
	}

	// We assume that all the logs we're looking for reside on the current drive
	gDirUtilp->initAppDirs("SecondLife");

	// Lots of silly variable, replicated for each log file.
	LLString db_file_name;
	LLString sl_file_name;
	LLString bt_file_name; // stack_trace.log file
	LLString st_file_name; // stats.log file
	LLString si_file_name; // settings.xml file

	LLFileEncoder *db_filep = NULL;
	LLFileEncoder *sl_filep = NULL;
	LLFileEncoder *st_filep = NULL;
	LLFileEncoder *bt_filep = NULL;
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
	// *NOTE: These buffer sizes are hardcoded into a scanf() below.
	char tmp_sl_name[LL_MAX_PATH];
	tmp_sl_name[0] = '\0';
	char tmp_space[256];
	tmp_space[0] = '\0';

	// Look for it in the debug_info.log file
	if (db_filep->isValid())
	{
		// This was originally scanning for "SL Log: %[^\r\n]", which happily skipped to the next line
		// on debug logs (which don't have anything after "SL Log:" and tried to open a nonsensical filename.
		sscanf(db_filep->mBuf.c_str(), "SL Log:%255[ ]%1023[^\r\n]", tmp_space, tmp_sl_name);
	}
	else
	{
		delete db_filep;
		db_filep = NULL;
	}

	// If we actually have a legitimate file name, use it.
	if (gCrashInPreviousExec)
	{
		// If we froze, the crash log this time around isn't useful.
		// Use the old one.
		sl_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"SecondLife.old");
	}
	else if (tmp_sl_name[0])
	{
		sl_file_name = tmp_sl_name;
		llinfos << "Using log file from debug log: " << sl_file_name << llendl;
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

	si_file_name = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,"settings.xml").c_str();
	si_filep = new LLFileEncoder("SI", si_file_name.c_str());
	if (!si_filep->isValid())
	{
		delete si_filep;
		si_filep = NULL;
	}

	// encode this as if it were a 'Dr Watson' plain-text backtrace
	bt_file_name = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log").c_str();
	bt_filep = new LLFileEncoder("DW", bt_file_name.c_str());
	if (!bt_filep->isValid())
	{
		delete bt_filep;
		bt_filep = NULL;
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

	if (bt_filep)
	{
		post_data.append("&");
		tmp_url_buf = bt_filep->encodeURL(BT_MAX_SIZE);
		post_data += tmp_url_buf;
		llinfos << "Sending crash log file" << llendl;
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
		llinfos << "Not sending settings.xml file" << llendl;
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
	delete bt_filep;
	bt_filep = NULL;

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
			double age = difftime(gLaunchTime, stat_data.st_mtim.tv_sec);

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
	size_t nread = fread(buf, 1, buf_size, fp);
	fclose(fp);
	buf[nread] = 0;

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
		if ((S32)mBuf.size() > max_length)
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
