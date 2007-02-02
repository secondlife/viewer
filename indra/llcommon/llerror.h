/** 
 * @file llerror.h
 * @brief Constants, functions, and macros for logging and runtime errors.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLERROR_H
#define LL_LLERROR_H

#include <sstream>
#include <stdio.h>
#include <stdarg.h>

#include "llerrorstream.h"
#include "llerrorbuffer.h"

// Specific error codes
const S32 LL_ERR_NOERR = 0;
const S32 LL_ERR_ASSET_REQUEST_FAILED = -1;
//const S32 LL_ERR_ASSET_REQUEST_INVALID = -2;
const S32 LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE = -3;
const S32 LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE = -4;
const S32 LL_ERR_INSUFFICIENT_PERMISSIONS = -5;
const S32 LL_ERR_EOF = -39;
const S32 LL_ERR_CANNOT_OPEN_FILE = -42;
const S32 LL_ERR_FILE_NOT_FOUND = -43;
const S32 LL_ERR_FILE_EMPTY     = -44;
const S32 LL_ERR_TCP_TIMEOUT    = -23016;
const S32 LL_ERR_CIRCUIT_GONE   = -23017;

// Error types

#define LLERR_IMAGE				(1 << 1) // Image requests
#define LLERR_MESSAGE			(1 << 2) // Messaging
#define LLERR_PERF				(1 << 3) // Performance
#define LLERR_SQL				(1 << 4) // SQL statements
#define LLERR_DOUG				(1 << 5) // Doug's debugging
#define LLERR_USER_INPUT		(1 << 6) // Keyboard and mouse
#define LLERR_TIMING			(1 << 7) // Verbose time info
#define LLERR_TASK				(1 << 8) // Tracking tasks
#define LLERR_MSG_HANDLER		(1 << 9) //
#define LLERR_CIRCUIT_INFO		(1 << 10) // Message system circuit info
#define LLERR_PHYSICS			(1 << 11) // physics
#define LLERR_VFS				(1 << 12) // VFS
const U32 LLERR_ALL = 0xffff;
const U32 LLERR_NONE = 0x0;

// Define one of these for different error levels in release...
// #define RELEASE_SHOW_DEBUG // Define this if you want your release builds to show lldebug output.
#define RELEASE_SHOW_INFO // Define this if you want your release builds to show llinfo output
#define RELEASE_SHOW_WARN // Define this if you want your release builds to show llwarn output.


//////////////////////////////////////////
//
//  Implementation - ignore
//
//
#ifdef _DEBUG
#define SHOW_DEBUG
#define SHOW_WARN
#define SHOW_INFO
#define SHOW_ASSERT
#else // _DEBUG

#ifdef RELEASE_SHOW_DEBUG
#define SHOW_DEBUG
#endif

#ifdef RELEASE_SHOW_WARN
#define SHOW_WARN
#endif

#ifdef RELEASE_SHOW_INFO
#define SHOW_INFO
#endif

#ifdef RELEASE_SHOW_ASSERT
#define SHOW_ASSERT
#endif

#endif // _DEBUG


extern LLErrorStream gErrorStream;


// LL Error macros
//
// Usage:
//
// llerrs << "An error, oh my!" << variable << endl;
// llwarns << "Another error, fuck me!" << variable << endl;
// llwarnst(LLERR_IMAGE) << "Debug, mother fucker" << endl;
//
// NOTE: The output format of filename(lineno): is so that MS DevStudio
// can parse the output and automatically jump to that location

inline std::string llerrno_string(int errnum)
{
	std::stringstream res;
	res << "error(" << errnum << "):" << strerror(errnum) << " ";
	return res.str();
}

inline std::string llerror_file_line(const char* file, S32 line)
{
	std::stringstream res;
	res << file << "(" <<line << ")";
	return res.str();
}

// Used to throw an error which is always causes a system halt.
#define llerrs if (gErrorStream.isEnabledFor(LLErrorBuffer::FATAL)) \
	{	std::ostringstream llerror_oss; LLErrorBuffer::ELevel llerror_level = LLErrorBuffer::FATAL; \
		llerror_oss << llerror_file_line(__FILE__, __LINE__) << " : error\n"; \
		llerror_oss <<  "ERROR: " << llerror_file_line(__FILE__, __LINE__) << " "

// Used to show warnings
#define llwarns if (gErrorStream.isEnabledFor(LLErrorBuffer::WARN)) \
	{	std::ostringstream llerror_oss; LLErrorBuffer::ELevel llerror_level = LLErrorBuffer::WARN; \
		if (gErrorStream.getPrintLocation()) llerror_oss << llerror_file_line(__FILE__, __LINE__) << " : WARNING: "; \
		else llerror_oss << "WARNING: "; \
		llerror_oss

// Alerts are for serious non-fatal situations that are not supposed to happen and need to alert someone
#define llalerts if (gErrorStream.isEnabledFor(LLErrorBuffer::WARN)) \
	{	std::ostringstream llerror_oss; LLErrorBuffer::ELevel llerror_level = LLErrorBuffer::WARN; \
		if (gErrorStream.getPrintLocation()) llerror_oss << llerror_file_line(__FILE__, __LINE__) << " : ALERT: "; \
		else llerror_oss << "ALERT: ";  \
		llerror_oss

// Used to show informational messages that don't get disabled
#define llinfos if (gErrorStream.isEnabledFor(LLErrorBuffer::INFO)) \
	{	std::ostringstream llerror_oss; LLErrorBuffer::ELevel llerror_level = LLErrorBuffer::INFO; \
		if (gErrorStream.getPrintLocation()) llerror_oss << llerror_file_line(__FILE__, __LINE__) << " : INFO: "; \
		else llerror_oss << "INFO: ";  \
		llerror_oss

#define llinfost(type) if (gErrorStream.isEnabledFor(LLErrorBuffer::INFO, type)) \
	{	std::ostringstream llerror_oss; LLErrorBuffer::ELevel llerror_level = LLErrorBuffer::INFO; \
		if (gErrorStream.getPrintLocation()) llerror_oss << llerror_file_line(__FILE__, __LINE__) << " : INFO: "; \
		else llerror_oss << "INFO: [" << #type << "] ";  \
		llerror_oss
 
// Used for general debugging output
#define lldebugs if (gErrorStream.isEnabledFor(LLErrorBuffer::DEBUG)) \
	{	std::ostringstream llerror_oss; LLErrorBuffer::ELevel llerror_level = LLErrorBuffer::DEBUG; \
		if (gErrorStream.getPrintLocation()) llerror_oss << llerror_file_line(__FILE__, __LINE__) << " : DEBUG: "; \
		else llerror_oss << "DEBUG: ";  \
		llerror_oss

#define lldebugst(type) if (gErrorStream.isEnabledFor(LLErrorBuffer::DEBUG, type)) \
	{	std::ostringstream llerror_oss; LLErrorBuffer::ELevel llerror_level = LLErrorBuffer::DEBUG; \
		if (gErrorStream.getPrintLocation()) llerror_oss << llerror_file_line(__FILE__, __LINE__) << " : DEBUG: "; \
		else llerror_oss << "DEBUG: [" << #type << "] ";  \
		llerror_oss

#define llendl std::endl; gErrorStream.crashOnError(llerror_oss, llerror_level); }
#define llendflush std::endl << std::flush; gErrorStream.crashOnError(llerror_oss, llerror_level); }
#define llcont llerror_oss

#define llerror(msg, num)		llerrs << "Error # " << num << ": " << msg << llendl;

#define llwarning(msg, num)		llwarns << "Warning # " << num << ": " << msg << llendl;

#ifdef SHOW_ASSERT
#define llassert(func)			if (!(func)) llerrs << "ASSERT (" << #func << ")" << llendl;
#else
#define llassert(func)
#endif
#define llassert_always(func)	if (!(func)) llerrs << "ASSERT (" << #func << ")" << llendl;

#ifdef SHOW_ASSERT
#define llverify(func)			if (!(func)) llerrs << "ASSERT (" << #func << ")" << llendl;
#else
#define llverify(func)			(func); // get rid of warning C4189
#endif

// handy compile-time assert - enforce those template parameters! 
#define cassert(expn) typedef char __C_ASSERT__[(expn)?1:-1]	/* Flawfinder: ignore */
 
// Makes the app go down in flames, but on purpose!
void _llcrash_and_loop();

// Use as follows:
// llinfos << llformat("Test:%d (%.2f %.2f)", idx, x, y) << llendl;
//
// *NOTE: buffer limited to 1024, (but vsnprintf prevents overrun)
// should perhaps be replaced with boost::format.
inline std::string llformat(const char *fmt, ...)
{
	char tstr[1024];	/* Flawfinder: ignore */
	va_list va;
	va_start(va, fmt);
#if LL_WINDOWS
	_vsnprintf(tstr, 1024, fmt, va);
#else
	vsnprintf(tstr, 1024, fmt, va);	/* Flawfinder: ignore */
#endif
	va_end(va);
	return std::string(tstr);
}

// Helper class to temporarily change error level for the current scope.
class LLScopedErrorLevel
{
public:
	LLScopedErrorLevel(LLErrorBuffer::ELevel error_level);
	~LLScopedErrorLevel();

private:
	LLErrorBuffer::ELevel mOrigErrorLevel;
};
 
#endif // LL_LLERROR_H
