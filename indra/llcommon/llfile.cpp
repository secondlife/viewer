/**
 * @file llfile.cpp
 * @author Michael Schlachter
 * @date 2006-03-23
 * @brief Implementation of cross-platform POSIX file buffer and c++
 * stream classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#if LL_WINDOWS
#include <windows.h>
#include <stdlib.h>                 // Windows errno
#else
#include <errno.h>
#endif

#include "linden_common.h"
#include "llfile.h"
#include "llstring.h"
#include "llerror.h"
#include "stringize.h"

using namespace std;

static std::string empty;

// Many of the methods below use OS-level functions that mess with errno. Wrap
// variants of strerror() to report errors.

#if LL_WINDOWS
// On Windows, use strerror_s().
std::string strerr(int errn)
{
	char buffer[256];
	strerror_s(buffer, errn);       // infers sizeof(buffer) -- love it!
	return buffer;
}

#else
// On Posix we want to call strerror_r(), but alarmingly, there are two
// different variants. The one that returns int always populates the passed
// buffer (except in case of error), whereas the other one always returns a
// valid char* but might or might not populate the passed buffer. How do we
// know which one we're getting? Define adapters for each and let the compiler
// select the applicable adapter.

// strerror_r() returns char*
std::string message_from(int /*orig_errno*/, const char* /*buffer*/, size_t /*bufflen*/,
						 const char* strerror_ret)
{
	return strerror_ret;
}

// strerror_r() returns int
std::string message_from(int orig_errno, const char* buffer, size_t bufflen,
						 int strerror_ret)
{
	if (strerror_ret == 0)
	{
		return buffer;
	}
	// Here strerror_r() has set errno. Since strerror_r() has already failed,
	// seems like a poor bet to call it again to diagnose its own error...
	int stre_errno = errno;
	if (stre_errno == ERANGE)
	{
		return STRINGIZE("strerror_r() can't explain errno " << orig_errno
						 << " (" << bufflen << "-byte buffer too small)");
	}
	if (stre_errno == EINVAL)
	{
		return STRINGIZE("unknown errno " << orig_errno);
	}
	// Here we don't even understand the errno from strerror_r()!
	return STRINGIZE("strerror_r() can't explain errno " << orig_errno
					 << " (error " << stre_errno << ')');
}

std::string strerr(int errn)
{
	char buffer[256];
	// Select message_from() function matching the strerror_r() we have on hand.
	return message_from(errn, buffer, sizeof(buffer),
						strerror_r(errn, buffer, sizeof(buffer)));
}
#endif	// ! LL_WINDOWS

// On either system, shorthand call just infers global 'errno'.
std::string strerr()
{
	return strerr(errno);
}

int warnif(const std::string& desc, const std::string& filename, int rc, int accept=0)
{
	if (rc < 0)
	{
		// Capture errno before we start emitting output
		int errn = errno;
		// For certain operations, a particular errno value might be
		// acceptable -- e.g. stat() could permit ENOENT, mkdir() could permit
		// EEXIST. Don't warn if caller explicitly says this errno is okay.
		if (errn != accept)
		{
			LL_WARNS("LLFile") << "Couldn't " << desc << " '" << filename
							   << "' (errno " << errn << "): " << strerr(errn) << LL_ENDL;
		}
#if 0 && LL_WINDOWS                 // turn on to debug file-locking problems
		// If the problem is "Permission denied," maybe it's because another
		// process has the file open. Try to find out.
		if (errn == EACCES)         // *not* EPERM
		{
			// Only do any of this stuff (before LL_ENDL) if it will be logged.
			LL_DEBUGS("LLFile") << empty;
			const char* TEMP = getenv("TEMP");
			if (! TEMP)
			{
				LL_CONT << "No $TEMP, not running 'handle'";
			}
			else
			{
				std::string tf(TEMP);
				tf += "\\handle.tmp";
				// http://technet.microsoft.com/en-us/sysinternals/bb896655
				std::string cmd(STRINGIZE("handle \"" << filename
										  // "openfiles /query /v | fgrep -i \"" << filename
										  << "\" > \"" << tf << '"'));
				LL_CONT << cmd;
				if (system(cmd.c_str()) != 0)
				{
					LL_CONT << "\nDownload 'handle.exe' from http://technet.microsoft.com/en-us/sysinternals/bb896655";
				}
				else
				{
					std::ifstream inf(tf);
					std::string line;
					while (std::getline(inf, line))
					{
						LL_CONT << '\n' << line;
					}
				}
				LLFile::remove(tf);
			}
			LL_CONT << LL_ENDL;
		}
#endif  // LL_WINDOWS hack to identify processes holding file open
	}
	return rc;
}

// static
int	LLFile::mkdir(const std::string& dirname, int perms)
{
#if LL_WINDOWS
	// permissions are ignored on Windows
	std::string utf8dirname = dirname;
	llutf16string utf16dirname = utf8str_to_utf16str(utf8dirname);
	int rc = _wmkdir(utf16dirname.c_str());
#else
	int rc = ::mkdir(dirname.c_str(), (mode_t)perms);
#endif
	// We often use mkdir() to ensure the existence of a directory that might
	// already exist. Don't spam the log if it does.
	return warnif("mkdir", dirname, rc, EEXIST);
}

// static
int	LLFile::rmdir(const std::string& dirname)
{
#if LL_WINDOWS
	// permissions are ignored on Windows
	std::string utf8dirname = dirname;
	llutf16string utf16dirname = utf8str_to_utf16str(utf8dirname);
	int rc = _wrmdir(utf16dirname.c_str());
#else
	int rc = ::rmdir(dirname.c_str());
#endif
	return warnif("rmdir", dirname, rc);
}

// static
LLFILE*	LLFile::fopen(const std::string& filename, const char* mode)	/* Flawfinder: ignore */
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	std::string utf8mode = std::string(mode);
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	llutf16string utf16mode = utf8str_to_utf16str(utf8mode);
	return _wfopen(utf16filename.c_str(),utf16mode.c_str());
#else
	return ::fopen(filename.c_str(),mode);	/* Flawfinder: ignore */
#endif
}

LLFILE*	LLFile::_fsopen(const std::string& filename, const char* mode, int sharingFlag)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	std::string utf8mode = std::string(mode);
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	llutf16string utf16mode = utf8str_to_utf16str(utf8mode);
	return _wfsopen(utf16filename.c_str(),utf16mode.c_str(),sharingFlag);
#else
	llassert(0);//No corresponding function on non-windows
	return NULL;
#endif
}

int	LLFile::close(LLFILE * file)
{
	int ret_value = 0;
	if (file)
	{
		ret_value = fclose(file);
	}
	return ret_value;
}


int	LLFile::remove(const std::string& filename)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	int rc = _wremove(utf16filename.c_str());
#else
	int rc = ::remove(filename.c_str());
#endif
	return warnif("remove", filename, rc);
}

int	LLFile::rename(const std::string& filename, const std::string& newname)
{
#if	LL_WINDOWS
	std::string utf8filename = filename;
	std::string utf8newname = newname;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	llutf16string utf16newname = utf8str_to_utf16str(utf8newname);
	int rc = _wrename(utf16filename.c_str(),utf16newname.c_str());
#else
	int rc = ::rename(filename.c_str(),newname.c_str());
#endif
	return warnif(STRINGIZE("rename to '" << newname << "' from"), filename, rc);
}

int	LLFile::stat(const std::string& filename, llstat* filestatus)
{
#if LL_WINDOWS
	std::string utf8filename = filename;
	llutf16string utf16filename = utf8str_to_utf16str(utf8filename);
	int rc = _wstat(utf16filename.c_str(),filestatus);
#else
	int rc = ::stat(filename.c_str(),filestatus);
#endif
	// We use stat() to determine existence (see isfile(), isdir()).
	// Don't spam the log if the subject pathname doesn't exist.
	return warnif("stat", filename, rc, ENOENT);
}

bool LLFile::isdir(const std::string& filename)
{
	llstat st;

	return stat(filename, &st) == 0 && S_ISDIR(st.st_mode);
}

bool LLFile::isfile(const std::string& filename)
{
	llstat st;

	return stat(filename, &st) == 0 && S_ISREG(st.st_mode);
}

const char *LLFile::tmpdir()
{
	static std::string utf8path;

	if (utf8path.empty())
	{
		char sep;
#if LL_WINDOWS
		sep = '\\';

		DWORD len = GetTempPathW(0, L"");
		llutf16string utf16path;
		utf16path.resize(len + 1);
		len = GetTempPathW(static_cast<DWORD>(utf16path.size()), &utf16path[0]);
		utf8path = utf16str_to_utf8str(utf16path);
#else
		sep = '/';

		char *env = getenv("TMPDIR");

		utf8path = env ? env : "/tmp/";
#endif
		if (utf8path[utf8path.size() - 1] != sep)
		{
			utf8path += sep;
		}
	}
	return utf8path.c_str();
}


/***************** Modified file stream created to overcome the incorrect behaviour of posix fopen in windows *******************/

#if USE_LLFILESTREAMS

LLFILE *	LLFile::_Fiopen(const std::string& filename, std::ios::openmode mode,int)	// protection currently unused
{	// open a file
	static const char *mods[] =
	{	// fopen mode strings corresponding to valid[i]
	"r", "w", "w", "a", "rb", "wb", "wb", "ab",
	"r+", "w+", "a+", "r+b", "w+b", "a+b",
	0};
	static const int valid[] =
	{	// valid combinations of open flags
		ios_base::in,
		ios_base::out,
		ios_base::out | ios_base::trunc,
		ios_base::out | ios_base::app,
		ios_base::in | ios_base::binary,
		ios_base::out | ios_base::binary,
		ios_base::out | ios_base::trunc | ios_base::binary,
		ios_base::out | ios_base::app | ios_base::binary,
		ios_base::in | ios_base::out,
		ios_base::in | ios_base::out | ios_base::trunc,
		ios_base::in | ios_base::out | ios_base::app,
		ios_base::in | ios_base::out | ios_base::binary,
		ios_base::in | ios_base::out | ios_base::trunc
			| ios_base::binary,
		ios_base::in | ios_base::out | ios_base::app
			| ios_base::binary,
	0};

	LLFILE *fp = 0;
	int n;
	ios_base::openmode atendflag = mode & ios_base::ate;
	ios_base::openmode norepflag = mode & ios_base::_Noreplace;

	if (mode & ios_base::_Nocreate)
		mode |= ios_base::in;	// file must exist
	mode &= ~(ios_base::ate | ios_base::_Nocreate | ios_base::_Noreplace);
	for (n = 0; valid[n] != 0 && valid[n] != mode; ++n)
		;	// look for a valid mode

	if (valid[n] == 0)
		return (0);	// no valid mode
	else if (norepflag && mode & (ios_base::out || ios_base::app)
		&& (fp = LLFile::fopen(filename, "r")) != 0)	/* Flawfinder: ignore */
		{	// file must not exist, close and fail
		fclose(fp);
		return (0);
		}
	else if (fp != 0 && fclose(fp) != 0)
		return (0);	// can't close after test open
// should open with protection here, if other than default
	else if ((fp = LLFile::fopen(filename, mods[n])) == 0)	/* Flawfinder: ignore */
		return (0);	// open failed

	if (!atendflag || fseek(fp, 0, SEEK_END) == 0)
		return (fp);	// no need to seek to end, or seek succeeded

	fclose(fp);	// can't position at end
	return (0);
}

/************** input file stream ********************************/

void llifstream::close()
{	// close the C stream
	if (_Filebuffer && _Filebuffer->close() == 0)
	{
		_Myios::setstate(ios_base::failbit);	/*Flawfinder: ignore*/
	}
}

void llifstream::open(const std::string& _Filename,	/* Flawfinder: ignore */
	ios_base::openmode _Mode,
	int _Prot)
{	// open a C stream with specified mode

	LLFILE* filep = LLFile::_Fiopen(_Filename,_Mode | ios_base::in, _Prot);
	if(filep == NULL)
	{
		_Myios::setstate(ios_base::failbit);	/*Flawfinder: ignore*/
		return;
	}
	llassert(_Filebuffer == NULL);
	_Filebuffer = new _Myfb(filep);
	_ShouldClose = true;
	_Myios::init(_Filebuffer);
}

bool llifstream::is_open() const
{	// test if C stream has been opened
	if(_Filebuffer)
		return (_Filebuffer->is_open());
	return false;
}
llifstream::~llifstream()
{
	if (_ShouldClose)
	{
		close();
	}
	delete _Filebuffer;
}

llifstream::llifstream(const std::string& _Filename,
	ios_base::openmode _Mode,
	int _Prot)
	: std::basic_istream< char , std::char_traits< char > >(NULL,true),_Filebuffer(NULL),_ShouldClose(false)

{	// construct with named file and specified mode
	open(_Filename, _Mode | ios_base::in, _Prot);	/* Flawfinder: ignore */
}


/************** output file stream ********************************/

bool llofstream::is_open() const
{	// test if C stream has been opened
	if(_Filebuffer)
		return (_Filebuffer->is_open());
	return false;
}

void llofstream::open(const std::string& _Filename,	/* Flawfinder: ignore */
	ios_base::openmode _Mode,
	int _Prot)
{	// open a C stream with specified mode

	LLFILE* filep = LLFile::_Fiopen(_Filename,_Mode | ios_base::out, _Prot);
	if(filep == NULL)
	{
		_Myios::setstate(ios_base::failbit);	/*Flawfinder: ignore*/
		return;
	}
	llassert(_Filebuffer==NULL);
	_Filebuffer = new _Myfb(filep);
	_ShouldClose = true;
	_Myios::init(_Filebuffer);
}

void llofstream::close()
{	// close the C stream
	if(is_open())
	{
		if (_Filebuffer->close() == 0)
		{
			_Myios::setstate(ios_base::failbit);	/*Flawfinder: ignore*/
		}
		delete _Filebuffer;
		_Filebuffer = NULL;
		_ShouldClose = false;
	}
}

llofstream::llofstream(const std::string& _Filename,
	std::ios_base::openmode _Mode,
	int _Prot)
		: std::basic_ostream<char,std::char_traits < char > >(NULL,true),_Filebuffer(NULL),_ShouldClose(false)
{	// construct with named file and specified mode
	open(_Filename, _Mode , _Prot);	/* Flawfinder: ignore */
}

llofstream::~llofstream()
{
	// destroy the object
	if (_ShouldClose)
	{
		close();
	}
	delete _Filebuffer;
}

#endif // #if USE_LLFILESTREAMS

/************** helper functions ********************************/

std::streamsize llifstream_size(llifstream& ifstr)
{
	if(!ifstr.is_open()) return 0;
	std::streampos pos_old = ifstr.tellg();
	ifstr.seekg(0, ios_base::beg);
	std::streampos pos_beg = ifstr.tellg();
	ifstr.seekg(0, ios_base::end);
	std::streampos pos_end = ifstr.tellg();
	ifstr.seekg(pos_old, ios_base::beg);
	return pos_end - pos_beg;
}

std::streamsize llofstream_size(llofstream& ofstr)
{
	if(!ofstr.is_open()) return 0;
	std::streampos pos_old = ofstr.tellp();
	ofstr.seekp(0, ios_base::beg);
	std::streampos pos_beg = ofstr.tellp();
	ofstr.seekp(0, ios_base::end);
	std::streampos pos_end = ofstr.tellp();
	ofstr.seekp(pos_old, ios_base::beg);
	return pos_end - pos_beg;
}


