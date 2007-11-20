/** 
 * @file llcrashloggerlinux.cpp
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

#include "llcrashloggerlinux.h"

#include <iostream>

#include "linden_common.h"

#include "boost/tokenizer.hpp"

#include "indra_constants.h"	// CRASH_BEHAVIOR_ASK, CRASH_SETTING_NAME
#include "llerror.h"
#include "llfile.h"
#include "lltimer.h"
#include "llstring.h"
#include "lldir.h"
#include "llsdserialize.h"

#if LL_GTK
# include "gtk/gtk.h"
#endif // LL_GTK

#define MAX_LOADSTRING 100

// These need to be localized.
static const char dialog_text[] =
"Second Life appears to have crashed or frozen last time it ran.\n"
"This crash reporter collects information about your computer's hardware, operating system, and some Second Life logs, all of which are used for debugging purposes only.\n"
"In the space below, please briefly describe what you were doing or trying to do just prior to the crash. Thank you for your help!\n"
"This report is NOT read by Customer Support. If you have billing or other questions, contact support by visiting http://www.secondlife.com/support\n"
"\n"
"Send crash report?";

static const char dialog_title[] =
"Second Life Crash Logger";

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

LLCrashLoggerLinux::LLCrashLoggerLinux(void)
{
}

LLCrashLoggerLinux::~LLCrashLoggerLinux(void)
{
}

void LLCrashLoggerLinux::gatherPlatformSpecificFiles()
{
	mFileMap["CrashLog"] = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log").c_str();
}

bool LLCrashLoggerLinux::mainLoop()
{
	if(!do_ask_dialog())
	{
		return true;
	}
	sendCrashLogs();
	return true;
}

void LLCrashLoggerLinux::updateApplication(LLString message)
{
	LLCrashLogger::updateApplication(message);
}
