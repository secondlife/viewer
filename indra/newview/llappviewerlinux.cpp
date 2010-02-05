/**
 * @file llappviewerlinux.cpp
 * @brief The LLAppViewerLinux class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llappviewerlinux.h"

#include "llcommandlineparser.h"

#include "llmemtype.h"
#include "llurldispatcher.h"		// SLURL from other app instance
#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llwindowsdl.h"
#include "llmd5.h"
#include "llfindlocale.h"

#include <exception>

#if LL_LINUX
# include <dlfcn.h>		// RTLD_LAZY
# include <execinfo.h>  // backtrace - glibc only
#elif LL_SOLARIS
# include <sys/types.h>
# include <unistd.h>
# include <fcntl.h>
# include <ucontext.h>
#endif

#ifdef LL_ELFBIN
# ifdef __GNUC__
#  include <cxxabi.h>			// for symbol demangling
# endif
# include "ELFIO/ELFIO.h"		// for better backtraces
#endif

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
	LLAppViewer::handleSyncViewerCrash();
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
	viewer_app_ptr->setSyncErrorHandler(LLAppViewer::handleSyncViewerCrash);

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

#ifdef LL_SOLARIS
static inline BOOL do_basic_glibc_backtrace()
{
	BOOL success = FALSE;

	std::string strace_filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log");
	llinfos << "Opening stack trace file " << strace_filename << llendl;
	LLFILE* StraceFile = LLFile::fopen(strace_filename, "w");
	if (!StraceFile)
	{
		llinfos << "Opening stack trace file " << strace_filename << " failed. Using stderr." << llendl;
		StraceFile = stderr;
	}

	printstack(fileno(StraceFile));

	if (StraceFile != stderr)
		fclose(StraceFile);

	return success;
}
#else
#define MAX_STACK_TRACE_DEPTH 40
// This uses glibc's basic built-in stack-trace functions for a not very
// amazing backtrace.
static inline BOOL do_basic_glibc_backtrace()
{
	void *stackarray[MAX_STACK_TRACE_DEPTH];
	size_t size;
	char **strings;
	size_t i;
	BOOL success = FALSE;

	size = backtrace(stackarray, MAX_STACK_TRACE_DEPTH);
	strings = backtrace_symbols(stackarray, size);

	std::string strace_filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log");
	llinfos << "Opening stack trace file " << strace_filename << llendl;
	LLFILE* StraceFile = LLFile::fopen(strace_filename, "w");		// Flawfinder: ignore
        if (!StraceFile)
	{
		llinfos << "Opening stack trace file " << strace_filename << " failed. Using stderr." << llendl;
		StraceFile = stderr;
	}

	if (size)
	{
		for (i = 0; i < size; i++)
		{
			// the format of the StraceFile is very specific, to allow (kludgy) machine-parsing
			fprintf(StraceFile, "%-3lu ", (unsigned long)i);
			fprintf(StraceFile, "%-32s\t", "unknown");
			fprintf(StraceFile, "%p ", stackarray[i]);
			fprintf(StraceFile, "%s\n", strings[i]);
		}

		success = TRUE;
	}
	
	if (StraceFile != stderr)
		fclose(StraceFile);

	free (strings);
	return success;
}

#if LL_ELFBIN
// This uses glibc's basic built-in stack-trace functions together with
// ELFIO's ability to parse the .symtab ELF section for better symbol
// extraction without exporting symbols (which'd cause subtle, fatal bugs).
static inline BOOL do_elfio_glibc_backtrace()
{
	void *stackarray[MAX_STACK_TRACE_DEPTH];
	size_t btsize;
	char **strings;
	BOOL success = FALSE;

	std::string appfilename = gDirUtilp->getExecutablePathAndName();

	std::string strace_filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log");
	llinfos << "Opening stack trace file " << strace_filename << llendl;
	LLFILE* StraceFile = LLFile::fopen(strace_filename, "w");		// Flawfinder: ignore
        if (!StraceFile)
	{
		llinfos << "Opening stack trace file " << strace_filename << " failed. Using stderr." << llendl;
		StraceFile = stderr;
	}

	// get backtrace address list and basic symbol info
	btsize = backtrace(stackarray, MAX_STACK_TRACE_DEPTH);
	strings = backtrace_symbols(stackarray, btsize);

	// create ELF reader for our app binary
	IELFI* pReader;
	const IELFISection* pSec = NULL;
	IELFISymbolTable* pSymTbl = 0;
	if (ERR_ELFIO_NO_ERROR != ELFIO::GetInstance()->CreateELFI(&pReader) ||
	    ERR_ELFIO_NO_ERROR != pReader->Load(appfilename.c_str()) ||
	    // find symbol table, create reader-object
	    NULL == (pSec = pReader->GetSection( ".symtab" )) ||
	    ERR_ELFIO_NO_ERROR != pReader->CreateSectionReader(IELFI::ELFI_SYMBOL, pSec, (void**)&pSymTbl) )
	{
		// Failed to open our binary and read its symbol table somehow
		llinfos << "Could not initialize ELF symbol reading - doing basic backtrace." << llendl;
		if (StraceFile != stderr)
			fclose(StraceFile);
		// note that we may be leaking some of the above ELFIO
		// objects now, but it's expected that we'll be dead soon
		// and we want to tread delicately until we get *some* kind
		// of useful backtrace.
		return do_basic_glibc_backtrace();
	}

	// iterate over trace and symtab, looking for plausible symbols
	std::string   name;
	Elf32_Addr    value;
	Elf32_Word    ssize;
	unsigned char bind;
	unsigned char type;
	Elf32_Half    section;
	int nSymNo = pSymTbl->GetSymbolNum();
	size_t btpos;
	for (btpos = 0; btpos < btsize; ++btpos)
	{
		// the format of the StraceFile is very specific, to allow (kludgy) machine-parsing
		fprintf(StraceFile, "%-3ld ", (long)btpos);
		int symidx;
		for (symidx = 0; symidx < nSymNo; ++symidx)
		{
			if (ERR_ELFIO_NO_ERROR ==
			    pSymTbl->GetSymbol(symidx, name, value, ssize,
					       bind, type, section))
			{
				// check if trace address within symbol range
				if (uintptr_t(stackarray[btpos]) >= value &&
				    uintptr_t(stackarray[btpos]) < value+ssize)
				{
					// symbol is inside viewer
					fprintf(StraceFile, "%-32s\t", "com.secondlife.indra.viewer");
					fprintf(StraceFile, "%p ", stackarray[btpos]);

					char *demangled_str = NULL;
					int demangle_result = 1;
					demangled_str =
						abi::__cxa_demangle
						(name.c_str(), NULL, NULL,
						 &demangle_result);
					if (0 == demangle_result &&
					    NULL != demangled_str) {
						fprintf(StraceFile,
							"%s", demangled_str);
						free(demangled_str);
					}
					else // failed demangle; print it raw
					{
						fprintf(StraceFile,
							"%s", name.c_str());
					}
					// print offset from symbol start
					fprintf(StraceFile,
						" + %lu\n",
						uintptr_t(stackarray[btpos]) -
						value);
					goto got_sym; // early escape
				}
			}
		}
		// Fallback:
		// Didn't find a suitable symbol in the binary - it's probably
		// a symbol in a DSO; use glibc's idea of what it should be.
		fprintf(StraceFile, "%-32s\t", "unknown");
		fprintf(StraceFile, "%p ", stackarray[btpos]);
		fprintf(StraceFile, "%s\n", strings[btpos]);
	got_sym:;
	}
	
	if (StraceFile != stderr)
		fclose(StraceFile);

	pSymTbl->Release();
	pSec->Release();
	pReader->Release();

	free(strings);

	llinfos << "Finished generating stack trace." << llendl;

	success = TRUE;
	return success;
}
#endif // LL_ELFBIN

#endif // LL_SOLARIS


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
	if (LLURLDispatcher::dispatch(url, web, trusted_browser))
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

void LLAppViewerLinux::handleSyncCrashTrace()
{
	// This backtrace writes into stack_trace.log
#  if LL_ELFBIN
	do_elfio_glibc_backtrace(); // more useful backtrace
#  else
	do_basic_glibc_backtrace(); // only slightly useful backtrace
#  endif // LL_ELFBIN
}

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
		const S32 cb = gCrashSettings.getS32(CRASH_BEHAVIOR_SETTING);

		// Always generate the report, have the logger do the asking, and
		// don't wait for the logger before exiting (-> total cleanup).
		if (CRASH_BEHAVIOR_NEVER_SEND != cb)
		{	
			// launch the actual crash logger
			const char* ask_dialog = "-dialog";
			if (CRASH_BEHAVIOR_ASK != cb)
				ask_dialog = ""; // omit '-dialog' option
			const char * cmdargv[] =
				{cmd.c_str(),
				 ask_dialog,
				 "-user",
				 (char*)LLGridManager::getInstance()->getGridLabel().c_str(),
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
	BOOL wrap = FALSE;
	while (gDirUtilp->getNextFileInDir(uuiddir, "*", this_name, wrap))
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
