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

#include "linden_common.h"
#include "llfile.h"
#include "llstring.h"
#include "llerror.h"
#include "stringize.h"

#if LL_WINDOWS
#include "llwin32headers.h"
#include <vector>
#else
#include <errno.h>
#endif

using namespace std;

static std::string empty;

// Many of the methods below use OS-level functions that mess with errno. Wrap
// variants of strerror() to report errors.

#if LL_WINDOWS
// For the situations where we directly call into Windows API functions we need to translate
// the Windows error codes into errno values
namespace
{
    struct errentry
    {
        unsigned long oserr; // Windows OS error value
        int errcode;         // System V error code
    };
}

// translation table between Windows OS error value and System V errno code
static errentry const errtable[]
{
    { ERROR_INVALID_FUNCTION,       EINVAL    },  //    1
    { ERROR_FILE_NOT_FOUND,         ENOENT    },  //    2
    { ERROR_PATH_NOT_FOUND,         ENOENT    },  //    3
    { ERROR_TOO_MANY_OPEN_FILES,    EMFILE    },  //    4
    { ERROR_ACCESS_DENIED,          EACCES    },  //    5
    { ERROR_INVALID_HANDLE,         EBADF     },  //    6
    { ERROR_ARENA_TRASHED,          ENOMEM    },  //    7
    { ERROR_NOT_ENOUGH_MEMORY,      ENOMEM    },  //    8
    { ERROR_INVALID_BLOCK,          ENOMEM    },  //    9
    { ERROR_BAD_ENVIRONMENT,        E2BIG     },  //   10
    { ERROR_BAD_FORMAT,             ENOEXEC   },  //   11
    { ERROR_INVALID_ACCESS,         EINVAL    },  //   12
    { ERROR_INVALID_DATA,           EINVAL    },  //   13
    { ERROR_INVALID_DRIVE,          ENOENT    },  //   15
    { ERROR_CURRENT_DIRECTORY,      EACCES    },  //   16
    { ERROR_NOT_SAME_DEVICE,        EXDEV     },  //   17
    { ERROR_NO_MORE_FILES,          ENOENT    },  //   18
    { ERROR_LOCK_VIOLATION,         EACCES    },  //   33
    { ERROR_BAD_NETPATH,            ENOENT    },  //   53
    { ERROR_NETWORK_ACCESS_DENIED,  EACCES    },  //   65
    { ERROR_BAD_NET_NAME,           ENOENT    },  //   67
    { ERROR_FILE_EXISTS,            EEXIST    },  //   80
    { ERROR_CANNOT_MAKE,            EACCES    },  //   82
    { ERROR_FAIL_I24,               EACCES    },  //   83
    { ERROR_INVALID_PARAMETER,      EINVAL    },  //   87
    { ERROR_NO_PROC_SLOTS,          EAGAIN    },  //   89
    { ERROR_DRIVE_LOCKED,           EACCES    },  //  108
    { ERROR_BROKEN_PIPE,            EPIPE     },  //  109
    { ERROR_DISK_FULL,              ENOSPC    },  //  112
    { ERROR_INVALID_TARGET_HANDLE,  EBADF     },  //  114
    { ERROR_WAIT_NO_CHILDREN,       ECHILD    },  //  128
    { ERROR_CHILD_NOT_COMPLETE,     ECHILD    },  //  129
    { ERROR_DIRECT_ACCESS_HANDLE,   EBADF     },  //  130
    { ERROR_NEGATIVE_SEEK,          EINVAL    },  //  131
    { ERROR_SEEK_ON_DEVICE,         EACCES    },  //  132
    { ERROR_DIR_NOT_EMPTY,          ENOTEMPTY },  //  145
    { ERROR_NOT_LOCKED,             EACCES    },  //  158
    { ERROR_BAD_PATHNAME,           ENOENT    },  //  161
    { ERROR_MAX_THRDS_REACHED,      EAGAIN    },  //  164
    { ERROR_LOCK_FAILED,            EACCES    },  //  167
    { ERROR_ALREADY_EXISTS,         EEXIST    },  //  183
    { ERROR_FILENAME_EXCED_RANGE,   ENOENT    },  //  206
    { ERROR_NESTING_NOT_ALLOWED,    EAGAIN    },  //  215
    { ERROR_NO_UNICODE_TRANSLATION, EILSEQ    },  // 1113
    { ERROR_NOT_ENOUGH_QUOTA,       ENOMEM    }   // 1816
};

static int set_errno_from_oserror(unsigned long oserr)
{
    if (!oserr)
        return 0;

    // Check the table for the Windows OS error code
    for (const struct errentry &entry : errtable)
    {
        if (oserr == entry.oserr)
        {
            _set_errno(entry.errcode);
            return -1;
        }
    }

    _set_errno(EINVAL);
    return -1;
}

// On Windows, use strerror_s().
std::string strerr(int errn)
{
    char buffer[256];
    strerror_s(buffer, errn);       // infers sizeof(buffer) -- love it!
    return buffer;
}

inline bool is_slash(wchar_t const c)
{
    return c == L'\\' || c == L'/';
}

static std::wstring utf8path_to_wstring(const std::string& utf8path)
{
    if (utf8path.size() >= MAX_PATH)
    {
        // By prepending "\\?\" to a path, Windows widechar file APIs will not fail on long path names
        std::wstring utf16path = L"\\\\?\\" + ll_convert<std::wstring>(utf8path);
        // We need to make sure that the path does not contain forward slashes as above
        // prefix does bypass the path normalization that replaces slashes with backslashes
        // before passing the path to kernel mode APIs
        std::replace(utf16path.begin(), utf16path.end(), L'/', L'\\');
        return utf16path;
    }
    return ll_convert<std::wstring>(utf8path);
}

static unsigned short get_fileattr(const std::wstring& utf16path, bool dontFollowSymLink = false)
{
    unsigned long  flags = FILE_FLAG_BACKUP_SEMANTICS;
    if (dontFollowSymLink)
    {
        flags |= FILE_FLAG_OPEN_REPARSE_POINT;
    }
    HANDLE file_handle = CreateFileW(utf16path.c_str(), FILE_READ_ATTRIBUTES,
                                     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     nullptr, OPEN_EXISTING, flags, nullptr);
    if (file_handle != INVALID_HANDLE_VALUE)
    {
        FILE_ATTRIBUTE_TAG_INFO attribute_info;
        if (GetFileInformationByHandleEx(file_handle, FileAttributeTagInfo, &attribute_info, sizeof(attribute_info)))
        {
            // A volume path alone (only drive letter) is not recognized as directory while it technically is
            bool is_directory = (attribute_info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                                (iswalpha(utf16path[0]) && utf16path[1] == ':' &&
                                 (!utf16path[2] || (is_slash(utf16path[2]) && !utf16path[3])));
            unsigned short st_mode = is_directory ? S_IFDIR :
                                     (attribute_info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ? S_IFLNK : S_IFREG);
            st_mode |= (attribute_info.FileAttributes & FILE_ATTRIBUTE_READONLY) ? S_IREAD : S_IREAD | S_IWRITE;
            // we do not try to guess executable flag

            // propagate user bits to group/other fields:
            st_mode |= (st_mode & 0700) >> 3;
            st_mode |= (st_mode & 0700) >> 6;

            CloseHandle(file_handle);
            return st_mode;
        }
    }
    // Retrieve last error and set errno before calling CloseHandle()
    set_errno_from_oserror(GetLastError());

    if (file_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file_handle);
    }
    return 0;
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
#endif  // ! LL_WINDOWS

static int warnif(const std::string& desc, const std::string& filename, int rc, int accept = 0)
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
            // would be nice to use LLDir for this, but dependency goes the
            // wrong way
            const char* TEMP = LLFile::tmpdir();
            if (! (TEMP && *TEMP))
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
int LLFile::mkdir(const std::string& dirname, int perms)
{
    // We often use mkdir() to ensure the existence of a directory that might
    // already exist. There is no known case in which we want to call out as
    // an error the requested directory already existing.
#if LL_WINDOWS
    // permissions are ignored on Windows
    int rc = 0;
    std::wstring utf16dirname = utf8path_to_wstring(dirname);
    if (!CreateDirectoryW(utf16dirname.c_str(), nullptr))
    {
        // Only treat other errors than an already existing file as a real error
        unsigned long oserr = GetLastError();
        if (oserr != ERROR_ALREADY_EXISTS)
        {
            rc = set_errno_from_oserror(oserr);
        }
    }
#else
    int rc = ::mkdir(dirname.c_str(), (mode_t)perms);
    if (rc < 0 && errno == EEXIST)
    {
        // this is not the error you want, move along
        return 0;
    }
#endif
    // anything else might be a problem
    return warnif("mkdir", dirname, rc);
}

// static
int LLFile::rmdir(const std::string& dirname, int suppress_error)
{
#if LL_WINDOWS
    std::wstring utf16dirname = utf8path_to_wstring(dirname);
    int rc = _wrmdir(utf16dirname.c_str());
#else
    int rc = ::rmdir(dirname.c_str());
#endif
    return warnif("rmdir", dirname, rc, suppress_error);
}

// static
LLFILE* LLFile::fopen(const std::string& filename, const char* mode)
{
#if LL_WINDOWS
    std::wstring utf16filename = utf8path_to_wstring(filename);
    std::wstring utf16mode = ll_convert<std::wstring>(std::string(mode));
    return _wfopen(utf16filename.c_str(), utf16mode.c_str());
#else
    return ::fopen(filename.c_str(),mode);
#endif
}

// static
int LLFile::close(LLFILE * file)
{
    int ret_value = 0;
    if (file)
    {
        ret_value = fclose(file);
    }
    return ret_value;
}

// static
std::string LLFile::getContents(const std::string& filename)
{
    LLFILE* fp = LLFile::fopen(filename, "rb");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        U32 length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        std::vector<char> buffer(length);
        size_t nread = fread(buffer.data(), 1, length, fp);
        fclose(fp);

        return std::string(buffer.data(), nread);
    }

    return LLStringUtil::null;
}

// static
int LLFile::remove(const std::string& filename, int suppress_error)
{
#if LL_WINDOWS
    // Posix remove() works on both files and directories although on Windows
    // remove() and its wide char variant _wremove() only removes files just
    // as its siblings unlink() and _wunlink().
    // If we really only want to support files we should instead use
    // unlink() in the non-Windows part below too
    int rc = -1;
    std::wstring utf16filename = utf8path_to_wstring(filename);
    unsigned short st_mode = get_fileattr(utf16filename);
    if (S_ISDIR(st_mode))
    {
        rc = _wrmdir(utf16filename.c_str());
    }
    else if (S_ISREG(st_mode))
    {
        rc = _wunlink(utf16filename.c_str());
    }
    else if (st_mode)
    {
        // it is something else than a file or directory
        // this should not really happen as long as we do not allow for symlink
        // detection in the optional parameter to get_fileattr()
        rc = set_errno_from_oserror(ERROR_INVALID_PARAMETER);
    }
    else
    {
        // get_filattr() failed and already set errno, preserve it for correct error reporting
    }
#else
    int rc = ::remove(filename.c_str());
#endif
    return warnif("remove", filename, rc, suppress_error);
}

// static
int LLFile::rename(const std::string& filename, const std::string& newname, int suppress_error)
{
#if LL_WINDOWS
    // Posix rename() will gladly overwrite a file at newname if it exists, the Windows
    // rename(), respectively _wrename(), will bark on that. Instead call directly the Windows
    // API MoveFileEx() and use its flags to specify that overwrite is allowed.
    std::wstring utf16filename = utf8path_to_wstring(filename);
    std::wstring utf16newname = utf8path_to_wstring(newname);
    int rc = 0;
    if (!MoveFileExW(utf16filename.c_str(), utf16newname.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
    {
        rc = set_errno_from_oserror(GetLastError());
    }
#else
    int rc = ::rename(filename.c_str(),newname.c_str());
#endif
    return warnif(STRINGIZE("rename to '" << newname << "' from"), filename, rc, suppress_error);
}

// Make this a define rather than using magic numbers multiple times in the code
#define LLFILE_COPY_BUFFER_SIZE 16384

// static
bool LLFile::copy(const std::string& from, const std::string& to)
{
    bool copied = false;
    LLFILE* in = LLFile::fopen(from, "rb");
    if (in)
    {
        LLFILE* out = LLFile::fopen(to, "wb");
        if (out)
        {
            char buf[LLFILE_COPY_BUFFER_SIZE];
            size_t readbytes;
            bool write_ok = true;
            while (write_ok && (readbytes = fread(buf, 1, LLFILE_COPY_BUFFER_SIZE, in)))
            {
                if (fwrite(buf, 1, readbytes, out) != readbytes)
                {
                    LL_WARNS("LLFile") << "Short write" << LL_ENDL;
                    write_ok = false;
                }
            }
            if ( write_ok )
            {
                copied = true;
            }
            fclose(out);
        }
        fclose(in);
    }
    return copied;
}

// static
int LLFile::stat(const std::string& filename, llstat* filestatus, int suppress_error)
{
#if LL_WINDOWS
    std::wstring utf16filename = utf8path_to_wstring(filename);
    int rc = _wstat64(utf16filename.c_str(), filestatus);
#else
    int rc = ::stat(filename.c_str(), filestatus);
#endif
    return warnif("stat", filename, rc, suppress_error);
}

// static
unsigned short LLFile::getattr(const std::string& filename, bool dontFollowSymLink, int suppress_error)
{
#if LL_WINDOWS
    // _wstat64() is a bit heavyweight on Windows, use a more lightweight API
    // to just get the attributes
    int            rc            = -1;
    std::wstring utf16filename = utf8path_to_wstring(filename);
    unsigned short st_mode = get_fileattr(utf16filename, dontFollowSymLink);
    if (st_mode)
    {
        return st_mode;
    }
#else
    llstat filestatus;
    int rc = dontFollowSymLink ? ::lstat(filename.c_str(), &filestatus) : ::stat(filename.c_str(), &filestatus);
    if (rc == 0)
    {
        return filestatus.st_mode;
    }
#endif
    warnif("getattr", filename, rc, suppress_error);
    return 0;
}

// static
bool LLFile::isdir(const std::string& filename)
{
    return S_ISDIR(getattr(filename));
}

// static
bool LLFile::isfile(const std::string& filename)
{
    return S_ISREG(getattr(filename));
}

// static
bool LLFile::islink(const std::string& filename)
{
    return S_ISLNK(getattr(filename, true));
}

// static
const char *LLFile::tmpdir()
{
    static std::string utf8path;

    if (utf8path.empty())
    {
        char sep;
#if LL_WINDOWS
        sep = '\\';

        std::vector<wchar_t> utf16path(MAX_PATH + 1);
        GetTempPathW(static_cast<DWORD>(utf16path.size()), &utf16path[0]);
        utf8path = ll_convert_wide_to_string(&utf16path[0]);
#else
        sep = '/';

        utf8path = LLStringUtil::getenv("TMPDIR", "/tmp/");
#endif
        if (utf8path[utf8path.size() - 1] != sep)
        {
            utf8path += sep;
        }
    }
    return utf8path.c_str();
}

#if LL_WINDOWS

/************** input file stream ********************************/

llifstream::llifstream() {}

// explicit
llifstream::llifstream(const std::string& _Filename, ios_base::openmode _Mode):
    std::ifstream(ll_convert<std::wstring>( _Filename ).c_str(),
                  _Mode | ios_base::in)
{
}

void llifstream::open(const std::string& _Filename, ios_base::openmode _Mode)
{
    std::ifstream::open(ll_convert<std::wstring>(_Filename).c_str(),
                        _Mode | ios_base::in);
}


/************** output file stream ********************************/


llofstream::llofstream() {}

// explicit
llofstream::llofstream(const std::string& _Filename, ios_base::openmode _Mode):
    std::ofstream(ll_convert<std::wstring>( _Filename ).c_str(),
                  _Mode | ios_base::out)
{
}

void llofstream::open(const std::string& _Filename, ios_base::openmode _Mode)
{
    std::ofstream::open(ll_convert<std::wstring>( _Filename ).c_str(),
                        _Mode | ios_base::out);
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

#endif  // LL_WINDOWS
