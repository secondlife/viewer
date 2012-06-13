/**
 * @file llappviewerlinux.cpp
 * @brief The LLAppViewerLinux class definitions
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

#include "llviewerprecompiledheaders.h"

#include "llappviewerlinux.h"

#include "llcommandlineparser.h"

#include "lldiriterator.h"
#include "llmemtype.h"
#include "llurldispatcher.h"		// SLURL from other app instance
#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llwindowsdl.h"
#include "llmd5.h"
#include "llfindlocale.h"

#include <exception>

#if LL_DBUS_ENABLED
# include "llappviewerlinux_api_dbus.h"

// regrettable hacks to give us better runtime compatibility with older systems inside llappviewerlinux_api.h:
#define llg_return_if_fail(COND) do{if (!(COND)) return;}while(0)
#undef g_return_if_fail
#define g_return_if_fail(COND) llg_return_if_fail(COND)
// The generated API
# include "llappviewerlinux_api.h"
#endif

namespace
{
	int gArgC = 0;
	char **gArgV = NULL;
	void (*gOldTerminateHandler)() = NULL;
}

static void exceptionTerminateHandler()
{
	// reinstall default terminate() handler in case we re-terminate.
	if (gOldTerminateHandler) std::set_terminate(gOldTerminateHandler);
	// treat this like a regular viewer crash, with nice stacktrace etc.
	LLAppViewer::handleViewerCrash();
	// we've probably been killed-off before now, but...
	gOldTerminateHandler(); // call old terminate() handler
}

int main( int argc, char **argv ) 
{
	LLMemType mt1(LLMemType::MTYPE_STARTUP);

#if LL_SOLARIS && defined(__sparc)
	asm ("ta\t6");		 // NOTE:  Make sure memory alignment is enforced on SPARC
#endif

	gArgC = argc;
	gArgV = argv;

	LLAppViewer* viewer_app_ptr = new LLAppViewerLinux();

	// install unexpected exception handler
	gOldTerminateHandler = std::set_terminate(exceptionTerminateHandler);
	// install crash handlers
	viewer_app_ptr->setErrorHandler(LLAppViewer::handleViewerCrash);

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

LLAppViewerLinux::LLAppViewerLinux()
{
}

LLAppViewerLinux::~LLAppViewerLinux()
{
}

bool LLAppViewerLinux::init()
{
	// g_thread_init() must be called before *any* use of glib, *and*
	// before any mutexes are held, *and* some of our third-party
	// libraries likes to use glib functions; in short, do this here
	// really early in app startup!
	if (!g_thread_supported ()) g_thread_init (NULL);
	
	return LLAppViewer::init();
}

bool LLAppViewerLinux::restoreErrorTrap()
{
	// *NOTE:Mani there is a case for implementing this on the mac.
	// Linux doesn't need it to my knowledge.
	return true;
}

/////////////////////////////////////////
#if LL_DBUS_ENABLED

typedef struct
{
        GObjectClass parent_class;
} ViewerAppAPIClass;

static void viewerappapi_init(ViewerAppAPI *server);
static void viewerappapi_class_init(ViewerAppAPIClass *klass);

///

// regrettable hacks to give us better runtime compatibility with older systems in general
static GType llg_type_register_static_simple_ONCE(GType parent_type,
						  const gchar *type_name,
						  guint class_size,
						  GClassInitFunc class_init,
						  guint instance_size,
						  GInstanceInitFunc instance_init,
						  GTypeFlags flags)
{
	static GTypeInfo type_info;
	memset(&type_info, 0, sizeof(type_info));

	type_info.class_size = class_size;
	type_info.class_init = class_init;
	type_info.instance_size = instance_size;
	type_info.instance_init = instance_init;

	return g_type_register_static(parent_type, type_name, &type_info, flags);
}
#define llg_intern_static_string(S) (S)
#define g_intern_static_string(S) llg_intern_static_string(S)
#define g_type_register_static_simple(parent_type, type_name, class_size, class_init, instance_size, instance_init, flags) llg_type_register_static_simple_ONCE(parent_type, type_name, class_size, class_init, instance_size, instance_init, flags)

G_DEFINE_TYPE(ViewerAppAPI, viewerappapi, G_TYPE_OBJECT);

void viewerappapi_class_init(ViewerAppAPIClass *klass)
{
}

static bool dbus_server_init = false;

void viewerappapi_init(ViewerAppAPI *server)
{
	// Connect to the default DBUS, register our service/API.

	if (!dbus_server_init)
	{
		GError *error = NULL;
		
		server->connection = lldbus_g_bus_get(DBUS_BUS_SESSION, &error);
		if (server->connection)
		{
			lldbus_g_object_type_install_info(viewerappapi_get_type(), &dbus_glib_viewerapp_object_info);
			
			lldbus_g_connection_register_g_object(server->connection, VIEWERAPI_PATH, G_OBJECT(server));
			
			DBusGProxy *serverproxy = lldbus_g_proxy_new_for_name(server->connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

			guint request_name_ret_unused;
			// akin to org_freedesktop_DBus_request_name
			if (lldbus_g_proxy_call(serverproxy, "RequestName", &error, G_TYPE_STRING, VIEWERAPI_SERVICE, G_TYPE_UINT, 0, G_TYPE_INVALID, G_TYPE_UINT, &request_name_ret_unused, G_TYPE_INVALID))
			{
				// total success.
				dbus_server_init = true;
			}
			else 
			{
				llwarns << "Unable to register service name: " << error->message << llendl;
			}
	
			g_object_unref(serverproxy);
		}
		else
		{
			g_warning("Unable to connect to dbus: %s", error->message);
		}

		if (error)
			g_error_free(error);
	}
}

gboolean viewer_app_api_GoSLURL(ViewerAppAPI *obj, gchar *slurl, gboolean **success_rtn, GError **error)
{
	bool success = false;

	llinfos << "Was asked to go to slurl: " << slurl << llendl;

	std::string url = slurl;
	LLMediaCtrl* web = NULL;
	const bool trusted_browser = false;
	if (LLURLDispatcher::dispatch(url, "", web, trusted_browser))
	{
		// bring window to foreground, as it has just been "launched" from a URL
		// todo: hmm, how to get there from here?
		//xxx->mWindow->bringToFront();
		success = true;
	}		

	*success_rtn = g_new (gboolean, 1);
	(*success_rtn)[0] = (gboolean)success;

	return TRUE; // the invokation succeeded, even if the actual dispatch didn't.
}

///

//virtual
bool LLAppViewerLinux::initSLURLHandler()
{
	if (!grab_dbus_syms(DBUSGLIB_DYLIB_DEFAULT_NAME))
	{
		return false; // failed
	}

	g_type_init();

	//ViewerAppAPI *api_server = (ViewerAppAPI*)
	g_object_new(viewerappapi_get_type(), NULL);

	return true;
}

//virtual
bool LLAppViewerLinux::sendURLToOtherInstance(const std::string& url)
{
	if (!grab_dbus_syms(DBUSGLIB_DYLIB_DEFAULT_NAME))
	{
		return false; // failed
	}

	bool success = false;
	DBusGConnection *bus;
	GError *error = NULL;

	g_type_init();
	
	bus = lldbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (bus)
	{
		gboolean rtn = FALSE;
		DBusGProxy *remote_object =
			lldbus_g_proxy_new_for_name(bus, VIEWERAPI_SERVICE, VIEWERAPI_PATH, VIEWERAPI_INTERFACE);

		if (lldbus_g_proxy_call(remote_object, "GoSLURL", &error,
					G_TYPE_STRING, url.c_str(), G_TYPE_INVALID,
				       G_TYPE_BOOLEAN, &rtn, G_TYPE_INVALID))
		{
			success = rtn;
		}
		else
		{
			llinfos << "Call-out to other instance failed (perhaps not running): " << error->message << llendl;
		}

		g_object_unref(G_OBJECT(remote_object));
	}
	else
	{
		llwarns << "Couldn't connect to session bus: " << error->message << llendl;
	}

	if (error)
		g_error_free(error);
	
	return success;
}

#else // LL_DBUS_ENABLED
bool LLAppViewerLinux::initSLURLHandler()
{
	return false; // not implemented without dbus
}
bool LLAppViewerLinux::sendURLToOtherInstance(const std::string& url)
{
	return false; // not implemented without dbus
}
#endif // LL_DBUS_ENABLED

void LLAppViewerLinux::handleCrashReporting(bool reportFreeze)
{
	std::string cmd =gDirUtilp->getExecutableDir();
	cmd += gDirUtilp->getDirDelimiter();
#if LL_LINUX
	cmd += "linux-crash-logger.bin";
#elif LL_SOLARIS
	cmd += "solaris-crash-logger";
#else
# error Unknown platform
#endif

	if(reportFreeze)
	{
		char* const cmdargv[] =
			{(char*)cmd.c_str(),
			 (char*)"-previous",
			 NULL};

		fflush(NULL); // flush all buffers before the child inherits them
		pid_t pid = fork();
		if (pid == 0)
		{ // child
			execv(cmd.c_str(), cmdargv);		/* Flawfinder: Ignore */
			llwarns << "execv failure when trying to start " << cmd << llendl;
			_exit(1); // avoid atexit()
		} else {
			if (pid > 0)
			{
				// wait for child proc to die
				int childExitStatus;
				waitpid(pid, &childExitStatus, 0);
			} else {
				llwarns << "fork failure." << llendl;
			}
		}
	}
	else
	{
		// launch the actual crash logger
		const char * cmdargv[] =
			{cmd.c_str(),
			 "-user",
			 (char*)LLGridManager::getInstance()->getGridId().c_str(),
			 "-name",
			 LLAppViewer::instance()->getSecondLifeTitle().c_str(),
			 NULL};
		fflush(NULL);
		pid_t pid = fork();
		if (pid == 0)
		{ // child
			execv(cmd.c_str(), (char* const*) cmdargv);		/* Flawfinder: ignore */
			llwarns << "execv failure when trying to start " << cmd << llendl;
			_exit(1); // avoid atexit()
		} 
		else
		{
			if (pid > 0)
			{
				// DO NOT wait for child proc to die; we want
				// the logger to outlive us while we quit to
				// free up the screen/keyboard/etc.
				////int childExitStatus;
				////waitpid(pid, &childExitStatus, 0);
			} 
			else
			{
				llwarns << "fork failure." << llendl;
			}
		}
		// Sometimes signals don't seem to quit the viewer.  Also, we may
		// have been called explicitly instead of from a signal handler.
		// Make sure we exit so as to not totally confuse the user.
		_exit(1); // avoid atexit(), else we may re-crash in dtors.
	}
}

bool LLAppViewerLinux::beingDebugged()
{
	static enum {unknown, no, yes} debugged = unknown;

#if LL_SOLARIS
	return debugged == no;	// BUG: fix this for Solaris
#else
	if (debugged == unknown)
	{
		pid_t ppid = getppid();
		char *name;
		int ret;

		ret = asprintf(&name, "/proc/%d/exe", ppid);
		if (ret != -1)
		{
			char buf[1024];
			ssize_t n;
			
			n = readlink(name, buf, sizeof(buf) - 1);
			if (n != -1)
			{
				char *base = strrchr(buf, '/');
				buf[n + 1] = '\0';
				if (base == NULL)
				{
					base = buf;
				} else {
					base += 1;
				}
				
				if (strcmp(base, "gdb") == 0)
				{
					debugged = yes;
				}
			}
			free(name);
		}
	}

	return debugged == yes;
#endif
}

bool LLAppViewerLinux::initLogging()
{
	// Remove the last stack trace, if any
	// This file is no longer created, since the move to Google Breakpad
	// The code is left here to clean out any old state in the log dir
	std::string old_stack_file =
		gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log");
	LLFile::remove(old_stack_file);

	return LLAppViewer::initLogging();
}

bool LLAppViewerLinux::initParseCommandLine(LLCommandLineParser& clp)
{
	if (!clp.parseCommandLine(gArgC, gArgV))
	{
		return false;
	}

	// Find the system language.
	FL_Locale *locale = NULL;
	FL_Success success = FL_FindLocale(&locale, FL_MESSAGES);
	if (success != 0)
	{
		if (success >= 2 && locale->lang) // confident!
		{
			LL_INFOS("AppInit") << "Language " << ll_safe_string(locale->lang) << LL_ENDL;
			LL_INFOS("AppInit") << "Location " << ll_safe_string(locale->country) << LL_ENDL;
			LL_INFOS("AppInit") << "Variant " << ll_safe_string(locale->variant) << LL_ENDL;

			LLControlVariable* c = gSavedSettings.getControl("SystemLanguage");
			if(c)
			{
				c->setValue(std::string(locale->lang), false);
			}
		}
	}
	FL_FreeLocale(&locale);

	return true;
}

std::string LLAppViewerLinux::generateSerialNumber()
{
	char serial_md5[MD5HEX_STR_SIZE];
	serial_md5[0] = 0;
	std::string best;
	std::string uuiddir("/dev/disk/by-uuid/");

	// trawl /dev/disk/by-uuid looking for a good-looking UUID to grab
	std::string this_name;

	LLDirIterator iter(uuiddir, "*");
	while (iter.next(this_name))
	{
		if (this_name.length() > best.length() ||
		    (this_name.length() == best.length() &&
		     this_name > best))
		{
			// longest (and secondarily alphabetically last) so far
			best = this_name;
		}
	}

	// we don't return the actual serial number, just a hash of it.
	LLMD5 md5( reinterpret_cast<const unsigned char*>(best.c_str()) );
	md5.hex_digest(serial_md5);

	return serial_md5;
}
