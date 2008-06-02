/**
 * @file llappviewerlinux.cpp
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

#include "llappviewerlinux.h"

#include "llcommandlineparser.h"

#include "llmemtype.h"
#include "llviewernetwork.h"
#include "llviewercontrol.h"
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
	LLFILE* StraceFile = LLFile::fopen(strace_filename.c_str(), "w");
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
	void *array[MAX_STACK_TRACE_DEPTH];
	size_t size;
	char **strings;
	size_t i;
	BOOL success = FALSE;

	size = backtrace(array, MAX_STACK_TRACE_DEPTH);
	strings = backtrace_symbols(array, size);

	std::string strace_filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log");
	llinfos << "Opening stack trace file " << strace_filename << llendl;
	LLFILE* StraceFile = LLFile::fopen(strace_filename.c_str(), "w");		// Flawfinder: ignore
        if (!StraceFile)
	{
		llinfos << "Opening stack trace file " << strace_filename << " failed. Using stderr." << llendl;
		StraceFile = stderr;
	}

	if (size)
	{
		for (i = 0; i < size; i++)
			fputs((std::string(strings[i])+"\n").c_str(),
			      StraceFile);

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
	void *array[MAX_STACK_TRACE_DEPTH];
	size_t btsize;
	char **strings;
	BOOL success = FALSE;

	std::string appfilename = gDirUtilp->getExecutablePathAndName();

	std::string strace_filename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log");
	llinfos << "Opening stack trace file " << strace_filename << llendl;
	LLFILE* StraceFile = LLFile::fopen(strace_filename.c_str(), "w");		// Flawfinder: ignore
        if (!StraceFile)
	{
		llinfos << "Opening stack trace file " << strace_filename << " failed. Using stderr." << llendl;
		StraceFile = stderr;
	}

	// get backtrace address list and basic symbol info
	btsize = backtrace(array, MAX_STACK_TRACE_DEPTH);
	strings = backtrace_symbols(array, btsize);

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
		fprintf(StraceFile, "%d:\t", btpos);
		int symidx;
		for (symidx = 0; symidx < nSymNo; ++symidx)
		{
			if (ERR_ELFIO_NO_ERROR ==
			    pSymTbl->GetSymbol(symidx, name, value, ssize,
					       bind, type, section))
			{
				// check if trace address within symbol range
				if (uintptr_t(array[btpos]) >= value &&
				    uintptr_t(array[btpos]) < value+ssize)
				{
					char *demangled_str = NULL;
					int demangle_result = 1;
					demangled_str =
						abi::__cxa_demangle
						(name.c_str(), NULL, NULL,
						 &demangle_result);
					if (0 == demangle_result &&
					    NULL != demangled_str) {
						fprintf(StraceFile,
							"ELF(%s", demangled_str);
						free(demangled_str);
					}
					else // failed demangle; print it raw
					{
						fprintf(StraceFile,
							"ELF(%s", name.c_str());
					}
					// print offset from symbol start
					fprintf(StraceFile,
						"+0x%lx) [%p]\n",
						uintptr_t(array[btpos]) -
						value,
						array[btpos]);
					goto got_sym; // early escape
				}
			}
		}
		// Fallback:
		// Didn't find a suitable symbol in the binary - it's probably
		// a symbol in a DSO; use glibc's idea of what it should be.
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
	return LLAppViewer::init();
}

void LLAppViewerLinux::handleSyncCrashTrace()
{
	// This backtrace writes into stack_trace.log
#  if LL_ELFBIN
	do_elfio_glibc_backtrace(); // more useful backtrace
#  else
	do_basic_glibc_backtrace(); // only slightly useful backtrace
#  endif // LL_ELFBIN
}

void LLAppViewerLinux::handleCrashReporting()
{
	// Always generate the report, have the logger do the asking, and
	// don't wait for the logger before exiting (-> total cleanup).
	if (CRASH_BEHAVIOR_NEVER_SEND != LLAppViewer::instance()->getCrashBehavior())
	{	
		// launch the actual crash logger
		const char* ask_dialog = "-dialog";
		if (CRASH_BEHAVIOR_ASK != LLAppViewer::instance()->getCrashBehavior())
			ask_dialog = ""; // omit '-dialog' option
		std::string cmd =gDirUtilp->getAppRODataDir();
		cmd += gDirUtilp->getDirDelimiter();
		cmd += "linux-crash-logger.bin";
		const char * cmdargv[] =
			{cmd.c_str(),
			 ask_dialog,
			 "-user",
			 (char*)gGridName.c_str(),
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

bool LLAppViewerLinux::beingDebugged()
{
	static enum {unknown, no, yes} debugged = unknown;
	
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
}

bool LLAppViewerLinux::initLogging()
{
	// Remove the last stack trace, if any
	std::string old_stack_file =
		gDirUtilp->getExpandedFilename(LL_PATH_LOGS,"stack_trace.log");
	LLFile::remove(old_stack_file.c_str());

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
			LLControlVariable* c = gSavedSettings.getControl("SystemLanguage");
			if(c)
			{
				c->setValue(std::string(locale->lang), false);
			}
		}
		FL_FreeLocale(&locale);
	}

	return true;
}

std::string LLAppViewerLinux::generateSerialNumber()
{
	char serial_md5[MD5HEX_STR_SIZE];
	serial_md5[0] = 0;

	// TODO

	return serial_md5;
}
