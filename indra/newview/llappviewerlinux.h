/**
 * @file llappviewerlinux.h
 * @brief The LLAppViewerLinux class declaration
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLAPPVIEWERLINUX_H
#define LL_LLAPPVIEWERLINUX_H

extern "C" {
# include <glib.h>
}

#if LL_DBUS_ENABLED
extern "C" {
# include <glib-object.h>
# include <dbus/dbus-glib.h>
}
#endif

#ifndef LL_LLAPPVIEWER_H
#include "llappviewer.h"
#endif

class LLCommandLineParser;

class LLAppViewerLinux : public LLAppViewer
{
public:
	LLAppViewerLinux();
	virtual ~LLAppViewerLinux();

	//
	// Main application logic
	//
	virtual bool init();			// Override to do application initialization
	std::string generateSerialNumber();
	bool setupSLURLHandler();

protected:
	virtual bool beingDebugged();
	
	virtual bool restoreErrorTrap();
	virtual void handleCrashReporting(bool reportFreeze);

	virtual bool initLogging();
	virtual bool initParseCommandLine(LLCommandLineParser& clp);

	virtual bool initSLURLHandler();
	virtual bool sendURLToOtherInstance(const std::string& url);
};

#if LL_DBUS_ENABLED
typedef struct
{
        GObject parent;
        DBusGConnection *connection;
} ViewerAppAPI;

extern "C" {
	gboolean viewer_app_api_GoSLURL(ViewerAppAPI *obj, gchar *slurl, gboolean **success_rtn, GError **error);
}

#define VIEWERAPI_SERVICE "com.secondlife.ViewerAppAPIService"
#define VIEWERAPI_PATH "/com/secondlife/ViewerAppAPI"
#define VIEWERAPI_INTERFACE "com.secondlife.ViewerAppAPI"

#endif // LL_DBUS_ENABLED

#endif // LL_LLAPPVIEWERLINUX_H
