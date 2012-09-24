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

typedef std::basic_ios<char,std::char_traits < char > > _Myios;

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

#if LL_WINDOWS

LLFILE *	LLFile::_Fiopen(const std::string& filename, 
		std::ios::openmode mode)
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

#endif /* LL_WINDOWS */

/************** llstdio file buffer ********************************/


//llstdio_filebuf* llstdio_filebuf::open(const char *_Filename,
//	ios_base::openmode _Mode)
//{
//#if LL_WINDOWS
//	_Filet *_File;
//	if (is_open() || (_File = LLFILE::_Fiopen(_Filename, _Mode)) == 0)
//		return (0);	// open failed
//
//	_Init(_File, _Openfl);
//	_Initcvt(&_USE(_Mysb::getloc(), _Cvt));
//	return (this);	// open succeeded
//#else
//	std::filebuf* _file = std::filebuf::open(_Filename, _Mode);
//	if (NULL == _file) return NULL;
//	return this;
//#endif
//}


// *TODO: Seek the underlying c stream for better cross-platform compatibility?
#if !LL_WINDOWS
llstdio_filebuf::int_type llstdio_filebuf::overflow(llstdio_filebuf::int_type __c)
{
	int_type __ret = traits_type::eof();
	const bool __testeof = traits_type::eq_int_type(__c, __ret);
	const bool __testout = _M_mode & ios_base::out;
	if (__testout && !_M_reading)
	{
		if (this->pbase() < this->pptr())
		{
			// If appropriate, append the overflow char.
			if (!__testeof)
			{
				*this->pptr() = traits_type::to_char_type(__c);
				this->pbump(1);
			}

			// Convert pending sequence to external representation,
			// and output.
			if (_convert_to_external(this->pbase(),
					 this->pptr() - this->pbase()))
			{
				_M_set_buffer(0);
				__ret = traits_type::not_eof(__c);
			}
		}
		else if (_M_buf_size > 1)
		{
			// Overflow in 'uncommitted' mode: set _M_writing, set
			// the buffer to the initial 'write' mode, and put __c
			// into the buffer.
			_M_set_buffer(0);
			_M_writing = true;
			if (!__testeof)
			{
				*this->pptr() = traits_type::to_char_type(__c);
				this->pbump(1);
			}
			__ret = traits_type::not_eof(__c);
		}
		else
		{
			// Unbuffered.
			char_type __conv = traits_type::to_char_type(__c);
			if (__testeof || _convert_to_external(&__conv, 1))
			{
				_M_writing = true;
				__ret = traits_type::not_eof(__c);
			}
		}
	}
	return __ret;
}

bool llstdio_filebuf::_convert_to_external(char_type* __ibuf,
						std::streamsize __ilen)
{
	// Sizes of external and pending output.
	streamsize __elen;
	streamsize __plen;
	if (__check_facet(_M_codecvt).always_noconv())
	{
		//__elen = _M_file.xsputn(reinterpret_cast<char*>(__ibuf), __ilen);
		__elen = fwrite(reinterpret_cast<void*>(__ibuf), 1,
						__ilen, _M_file.file());
		__plen = __ilen;
	}
	else
	{
		// Worst-case number of external bytes needed.
		// XXX Not done encoding() == -1.
		streamsize __blen = __ilen * _M_codecvt->max_length();
		char* __buf = static_cast<char*>(__builtin_alloca(__blen));

		char* __bend;
		const char_type* __iend;
		codecvt_base::result __r;
		__r = _M_codecvt->out(_M_state_cur, __ibuf, __ibuf + __ilen,
				__iend, __buf, __buf + __blen, __bend);

		if (__r == codecvt_base::ok || __r == codecvt_base::partial)
			__blen = __bend - __buf;
		else if (__r == codecvt_base::noconv)
		{
			// Same as the always_noconv case above.
			__buf = reinterpret_cast<char*>(__ibuf);
			__blen = __ilen;
		}
		else
			__throw_ios_failure(__N("llstdio_filebuf::_convert_to_external "
									"conversion error"));
  
		//__elen = _M_file.xsputn(__buf, __blen);
		__elen = fwrite(__buf, 1, __blen, _M_file.file());
		__plen = __blen;

		// Try once more for partial conversions.
		if (__r == codecvt_base::partial && __elen == __plen)
		{
			const char_type* __iresume = __iend;
			streamsize __rlen = this->pptr() - __iend;
			__r = _M_codecvt->out(_M_state_cur, __iresume,
					__iresume + __rlen, __iend, __buf,
					__buf + __blen, __bend);
			if (__r != codecvt_base::error)
			{
				__rlen = __bend - __buf;
				//__elen = _M_file.xsputn(__buf, __rlen);
				__elen = fwrite(__buf, 1, __rlen, _M_file.file());
				__plen = __rlen;
			}
			else
			{
				__throw_ios_failure(__N("llstdio_filebuf::_convert_to_external "
										"conversion error"));
			}
		}
	}
	return __elen == __plen;
}

llstdio_filebuf::int_type llstdio_filebuf::underflow()
{
	int_type __ret = traits_type::eof();
	const bool __testin = _M_mode & ios_base::in;
	if (__testin)
	{
		if (_M_writing)
		{
			if (overflow() == traits_type::eof())
			return __ret;
			//_M_set_buffer(-1);
			//_M_writing = false;
		}
		// Check for pback madness, and if so switch back to the
		// normal buffers and jet outta here before expensive
		// fileops happen...
		_M_destroy_pback();

		if (this->gptr() < this->egptr())
			return traits_type::to_int_type(*this->gptr());

		// Get and convert input sequence.
		const size_t __buflen = _M_buf_size > 1 ? _M_buf_size - 1 : 1;

		// Will be set to true if ::fread() returns 0 indicating EOF.
		bool __got_eof = false;
		// Number of internal characters produced.
		streamsize __ilen = 0;
		codecvt_base::result __r = codecvt_base::ok;
		if (__check_facet(_M_codecvt).always_noconv())
		{
			//__ilen = _M_file.xsgetn(reinterpret_cast<char*>(this->eback()),
			//			__buflen);
			__ilen = fread(reinterpret_cast<void*>(this->eback()), 1,
						__buflen, _M_file.file());
			if (__ilen == 0)
				__got_eof = true;
		}
		else
	    {
			// Worst-case number of external bytes.
			// XXX Not done encoding() == -1.
			const int __enc = _M_codecvt->encoding();
			streamsize __blen; // Minimum buffer size.
			streamsize __rlen; // Number of chars to read.
			if (__enc > 0)
				__blen = __rlen = __buflen * __enc;
			else
			{
				__blen = __buflen + _M_codecvt->max_length() - 1;
				__rlen = __buflen;
			}
			const streamsize __remainder = _M_ext_end - _M_ext_next;
			__rlen = __rlen > __remainder ? __rlen - __remainder : 0;

			// An imbue in 'read' mode implies first converting the external
			// chars already present.
			if (_M_reading && this->egptr() == this->eback() && __remainder)
				__rlen = 0;

			// Allocate buffer if necessary and move unconverted
			// bytes to front.
			if (_M_ext_buf_size < __blen)
			{
				char* __buf = new char[__blen];
				if (__remainder)
					__builtin_memcpy(__buf, _M_ext_next, __remainder);

				delete [] _M_ext_buf;
				_M_ext_buf = __buf;
				_M_ext_buf_size = __blen;
			}
			else if (__remainder)
				__builtin_memmove(_M_ext_buf, _M_ext_next, __remainder);

			_M_ext_next = _M_ext_buf;
			_M_ext_end = _M_ext_buf + __remainder;
			_M_state_last = _M_state_cur;

			do
			{
				if (__rlen > 0)
				{
					// Sanity check!
					// This may fail if the return value of
					// codecvt::max_length() is bogus.
					if (_M_ext_end - _M_ext_buf + __rlen > _M_ext_buf_size)
					{
						__throw_ios_failure(__N("llstdio_filebuf::underflow "
							"codecvt::max_length() "
							"is not valid"));
					}
					//streamsize __elen = _M_file.xsgetn(_M_ext_end, __rlen);
					streamsize __elen = fread(_M_ext_end, 1,
						__rlen, _M_file.file());
					if (__elen == 0)
						__got_eof = true;
					else if (__elen == -1)
					break;
					//_M_ext_end += __elen;
				}

				char_type* __iend = this->eback();
				if (_M_ext_next < _M_ext_end)
				{
					__r = _M_codecvt->in(_M_state_cur, _M_ext_next,
							_M_ext_end, _M_ext_next,
							this->eback(),
							this->eback() + __buflen, __iend);
				}
				if (__r == codecvt_base::noconv)
				{
					size_t __avail = _M_ext_end - _M_ext_buf;
					__ilen = std::min(__avail, __buflen);
					traits_type::copy(this->eback(),
						reinterpret_cast<char_type*>
						(_M_ext_buf), __ilen);
					_M_ext_next = _M_ext_buf + __ilen;
				}
				else
					__ilen = __iend - this->eback();

				// _M_codecvt->in may return error while __ilen > 0: this is
				// ok, and actually occurs in case of mixed encodings (e.g.,
				// XML files).
				if (__r == codecvt_base::error)
					break;

				__rlen = 1;
			} while (__ilen == 0 && !__got_eof);
		}

		if (__ilen > 0)
		{
			_M_set_buffer(__ilen);
			_M_reading = true;
			__ret = traits_type::to_int_type(*this->gptr());
		}
		else if (__got_eof)
		{
			// If the actual end of file is reached, set 'uncommitted'
			// mode, thus allowing an immediate write without an
			// intervening seek.
			_M_set_buffer(-1);
			_M_reading = false;
			// However, reaching it while looping on partial means that
			// the file has got an incomplete character.
			if (__r == codecvt_base::partial)
				__throw_ios_failure(__N("llstdio_filebuf::underflow "
					"incomplete character in file"));
		}
		else if (__r == codecvt_base::error)
			__throw_ios_failure(__N("llstdio_filebuf::underflow "
					"invalid byte sequence in file"));
		else
			__throw_ios_failure(__N("llstdio_filebuf::underflow "
					"error reading the file"));
	}
	return __ret;
}

std::streamsize llstdio_filebuf::xsgetn(char_type* __s, std::streamsize __n)
{
	// Clear out pback buffer before going on to the real deal...
	streamsize __ret = 0;
	if (_M_pback_init)
	{
		if (__n > 0 && this->gptr() == this->eback())
		{
			*__s++ = *this->gptr();
			this->gbump(1);
			__ret = 1;
			--__n;
		}
		_M_destroy_pback();
	}
       
	// Optimization in the always_noconv() case, to be generalized in the
	// future: when __n > __buflen we read directly instead of using the
	// buffer repeatedly.
	const bool __testin = _M_mode & ios_base::in;
	const streamsize __buflen = _M_buf_size > 1 ? _M_buf_size - 1 : 1;

	if (__n > __buflen && __check_facet(_M_codecvt).always_noconv()
		&& __testin && !_M_writing)
	{
		// First, copy the chars already present in the buffer.
		const streamsize __avail = this->egptr() - this->gptr();
		if (__avail != 0)
		{
			if (__avail == 1)
				*__s = *this->gptr();
			else
				traits_type::copy(__s, this->gptr(), __avail);
			__s += __avail;
			this->gbump(__avail);
			__ret += __avail;
			__n -= __avail;
		}

		// Need to loop in case of short reads (relatively common
		// with pipes).
		streamsize __len;
		for (;;)
		{
			//__len = _M_file.xsgetn(reinterpret_cast<char*>(__s), __n);
			__len = fread(reinterpret_cast<void*>(__s), 1, 
						__n, _M_file.file());
			if (__len == -1)
				__throw_ios_failure(__N("llstdio_filebuf::xsgetn "
										"error reading the file"));
			if (__len == 0)
				break;

			__n -= __len;
			__ret += __len;
			if (__n == 0)
				break;

			__s += __len;
		}

		if (__n == 0)
		{
			_M_set_buffer(0);
			_M_reading = true;
		}
		else if (__len == 0)
		{
			// If end of file is reached, set 'uncommitted'
			// mode, thus allowing an immediate write without
			// an intervening seek.
			_M_set_buffer(-1);
			_M_reading = false;
		}
	}
	else
		__ret += __streambuf_type::xsgetn(__s, __n);

	return __ret;
}

std::streamsize llstdio_filebuf::xsputn(char_type* __s, std::streamsize __n)
{
	// Optimization in the always_noconv() case, to be generalized in the
	// future: when __n is sufficiently large we write directly instead of
	// using the buffer.
	streamsize __ret = 0;
	const bool __testout = _M_mode & ios_base::out;
	if (__check_facet(_M_codecvt).always_noconv()
		&& __testout && !_M_reading)
	{
		// Measurement would reveal the best choice.
		const streamsize __chunk = 1ul << 10;
		streamsize __bufavail = this->epptr() - this->pptr();

		// Don't mistake 'uncommitted' mode buffered with unbuffered.
		if (!_M_writing && _M_buf_size > 1)
			__bufavail = _M_buf_size - 1;

		const streamsize __limit = std::min(__chunk, __bufavail);
		if (__n >= __limit)
		{
			const streamsize __buffill = this->pptr() - this->pbase();
			const char* __buf = reinterpret_cast<const char*>(this->pbase());
			//__ret = _M_file.xsputn_2(__buf, __buffill,
			//			reinterpret_cast<const char*>(__s), __n);
			if (__buffill)
			{
				__ret = fwrite(__buf, 1, __buffill, _M_file.file());
			}
			if (__ret == __buffill)
			{
				__ret += fwrite(reinterpret_cast<const char*>(__s), 1,
								__n, _M_file.file());
			}
			if (__ret == __buffill + __n)
			{
				_M_set_buffer(0);
				_M_writing = true;
			}
			if (__ret > __buffill)
				__ret -= __buffill;
			else
				__ret = 0;
		}
		else
			__ret = __streambuf_type::xsputn(__s, __n);
	}
	else
		__ret = __streambuf_type::xsputn(__s, __n);
    return __ret;
}

int llstdio_filebuf::sync()
{
	return (_M_file.sync() == 0 ? 0 : -1);
}
#endif

/************** input file stream ********************************/


llifstream::llifstream() : _M_filebuf(),
#if LL_WINDOWS
	std::istream(&_M_filebuf) {}
#else
	std::istream()
{
	this->init(&_M_filebuf);
}
#endif

// explicit
llifstream::llifstream(const std::string& _Filename, 
		ios_base::openmode _Mode) : _M_filebuf(),
#if LL_WINDOWS
	std::istream(&_M_filebuf)
{
	if (_M_filebuf.open(_Filename.c_str(), _Mode | ios_base::in) == 0)
	{
		_Myios::setstate(ios_base::failbit);
	}
}
#else
	std::istream()
{
	this->init(&_M_filebuf);
	this->open(_Filename.c_str(), _Mode | ios_base::in);
}
#endif

// explicit
llifstream::llifstream(const char* _Filename, 
		ios_base::openmode _Mode) : _M_filebuf(),
#if LL_WINDOWS
	std::istream(&_M_filebuf)
{
	if (_M_filebuf.open(_Filename, _Mode | ios_base::in) == 0)
	{
		_Myios::setstate(ios_base::failbit);
	}
}
#else
	std::istream()
{
	this->init(&_M_filebuf);
	this->open(_Filename, _Mode | ios_base::in);
}
#endif


// explicit
llifstream::llifstream(_Filet *_File,
		ios_base::openmode _Mode, size_t _Size) :
	_M_filebuf(_File, _Mode, _Size),
#if LL_WINDOWS
	std::istream(&_M_filebuf) {}
#else
	std::istream()
{
	this->init(&_M_filebuf);
}
#endif

#if !LL_WINDOWS
// explicit
llifstream::llifstream(int __fd,
		ios_base::openmode _Mode, size_t _Size) :
	_M_filebuf(__fd, _Mode, _Size),
	std::istream()
{
	this->init(&_M_filebuf);
}
#endif

bool llifstream::is_open() const
{	// test if C stream has been opened
	return _M_filebuf.is_open();
}

void llifstream::open(const char* _Filename, ios_base::openmode _Mode)
{	// open a C stream with specified mode
	if (_M_filebuf.open(_Filename, _Mode | ios_base::in) == 0)
#if LL_WINDOWS
	{
		_Myios::setstate(ios_base::failbit);
	}
	else
	{
		_Myios::clear();
	}
#else
	{
		this->setstate(ios_base::failbit);
	}
	else
	{
		this->clear();
	}
#endif
}

void llifstream::close()
{	// close the C stream
	if (_M_filebuf.close() == 0)
	{
#if LL_WINDOWS
		_Myios::setstate(ios_base::failbit);
#else
		this->setstate(ios_base::failbit);
#endif
	}
}


/************** output file stream ********************************/


llofstream::llofstream() : _M_filebuf(),
#if LL_WINDOWS
	std::ostream(&_M_filebuf) {}
#else
	std::ostream()
{
	this->init(&_M_filebuf);
}
#endif

// explicit
llofstream::llofstream(const std::string& _Filename,
		ios_base::openmode _Mode) : _M_filebuf(),
#if LL_WINDOWS
	std::ostream(&_M_filebuf)
{
	if (_M_filebuf.open(_Filename.c_str(), _Mode | ios_base::out) == 0)
	{
		_Myios::setstate(ios_base::failbit);
	}
}
#else
	std::ostream()
{
	this->init(&_M_filebuf);
	this->open(_Filename.c_str(), _Mode | ios_base::out);
}
#endif

// explicit
llofstream::llofstream(const char* _Filename,
		ios_base::openmode _Mode) : _M_filebuf(),
#if LL_WINDOWS
	std::ostream(&_M_filebuf)
{
	if (_M_filebuf.open(_Filename, _Mode | ios_base::out) == 0)
	{
		_Myios::setstate(ios_base::failbit);
	}
}
#else
	std::ostream()
{
	this->init(&_M_filebuf);
	this->open(_Filename, _Mode | ios_base::out);
}
#endif

// explicit
llofstream::llofstream(_Filet *_File,
			ios_base::openmode _Mode, size_t _Size) :
	_M_filebuf(_File, _Mode, _Size),
#if LL_WINDOWS
	std::ostream(&_M_filebuf) {}
#else
	std::ostream()
{
	this->init(&_M_filebuf);
}
#endif

#if !LL_WINDOWS
// explicit
llofstream::llofstream(int __fd,
			ios_base::openmode _Mode, size_t _Size) :
	_M_filebuf(__fd, _Mode, _Size),
	std::ostream()
{
	this->init(&_M_filebuf);
}
#endif

bool llofstream::is_open() const
{	// test if C stream has been opened
	return _M_filebuf.is_open();
}

void llofstream::open(const char* _Filename, ios_base::openmode _Mode)
{	// open a C stream with specified mode
	if (_M_filebuf.open(_Filename, _Mode | ios_base::out) == 0)
#if LL_WINDOWS
	{
		_Myios::setstate(ios_base::failbit);
	}
	else
	{
		_Myios::clear();
	}
#else
	{
		this->setstate(ios_base::failbit);
	}
	else
	{
		this->clear();
	}
#endif
}

void llofstream::close()
{	// close the C stream
	if (_M_filebuf.close() == 0)
	{
#if LL_WINDOWS
		_Myios::setstate(ios_base::failbit);
#else
		this->setstate(ios_base::failbit);
#endif
	}
}

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


