/** 
 * @file llcrashloggerlinux.cpp
 * @brief Linux crash logger implementation
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

#include "llcrashloggerlinux.h"

#include <iostream>

#include "linden_common.h"

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
				     "%s", dialog_text);
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
}

bool LLCrashLoggerLinux::mainLoop()
{
	bool send_logs = true;
	if(CRASH_BEHAVIOR_ASK == getCrashBehavior())
	{
		send_logs = do_ask_dialog();
	}
	else if(CRASH_BEHAVIOR_NEVER_SEND == getCrashBehavior())
	{
		send_logs = false;
	}

	if(send_logs)
	{
		sendCrashLogs();
	}
	return true;
}

bool LLCrashLoggerLinux::cleanup()
{
	commonCleanup();
	return true;
}

void LLCrashLoggerLinux::updateApplication(const std::string& message)
{
	LLCrashLogger::updateApplication(message);
}
