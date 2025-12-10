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
#include <fcntl.h>
#else
#include <errno.h>
#include <sys/file.h>
#endif

// Some of the methods below use OS-level functions that mess with errno. Wrap
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
    { ERROR_SHARING_VIOLATION,      EACCES    },  //   32
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

static int get_errno_from_oserror(int oserr)
{
    if (!oserr)
        return 0;

    // Check the table for the Windows OS error code
    for (const struct errentry& entry : errtable)
    {
        if (oserr == entry.oserr)
        {
            return entry.errcode;
        }
    }
    return EINVAL;
}

static int set_errno_from_oserror(unsigned long oserr)
{
    _set_errno(get_errno_from_oserror(oserr));
    return -1;
}

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

#endif  // ! LL_WINDOWS

#if LL_WINDOWS && 0 // turn on to debug file-locking problems
#define PROCESS_LOCKING_CHECK 1
static void find_locking_process(const std::string& filename)
{
    // Only do any of this stuff (before LL_ENDL) if it will be logged.
    LL_DEBUGS("LLFile") << "";
    // wrong way
    std::string TEMP = LLFile::tmpdir();
    if (TEMP.empty())
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
            std::string   line;
            while (std::getline(inf, line))
            {
                LL_CONT << '\n' << line;
            }
        }
        LLFile::remove(tf);
    }
    LL_CONT << LL_ENDL;
}
#endif // LL_WINDOWS hack to identify processes holding file open

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
            LL_WARNS("LLFile") << "Couldn't " << desc << " '" << filename << "' (errno " << errn << "): " << strerr(errn) << LL_ENDL;
        }
#if PROCESS_LOCKING_CHECK
        // If the problem is "Permission denied," maybe it's because another
        // process has the file open. Try to find out.
        if (errn == EACCES) // *not* EPERM
        {
            find_locking_process(filename);
        }
#endif
    }
    return rc;
}

static int warnif(const std::string& desc, const std::string& filename, std::error_code& ec, int accept = 0)
{
    if (ec)
    {
        // get Posix errno from the std::error_code so we can compare it to the accept parameter to see
        // when a caller wants us to not generate a warning for a particular error code
#if LL_WINDOWS
        int errn = get_errno_from_oserror(ec.value());
#else
        int errn = ec.value();
#endif
        // For certain operations, a particular errno value might be acceptable
        // Don't warn if caller explicitly says this errno is okay.
        if (errn != accept)
        {
            LL_WARNS("LLFile") << "Couldn't " << desc << " '" << filename << "' (errno " << errn << "): " << ec.message() << LL_ENDL;
        }
#if PROCESS_LOCKING_CHECK
        // Try to detect locked files by other processes
        if (ec.value() == ERROR_SHARING_VIOLATION || ec.value() == ERROR_LOCK_VIOLATION)
        {
            find_locking_process(filename);
        }
#endif
        return -1;
    }
    return 0;
}

#if LL_WINDOWS

inline int set_ec_from_system_error(std::error_code& ec, DWORD error)
{
    ec.assign(error, std::system_category());
    return -1;
}

static int set_ec_from_system_error(std::error_code& ec)
{
    return set_ec_from_system_error(ec, GetLastError());
}

inline int set_ec_to_parameter_error(std::error_code& ec)
{
    return set_ec_from_system_error(ec, ERROR_INVALID_PARAMETER);
}

inline int set_ec_to_outofmemory_error(std::error_code& ec)
{
    return set_ec_from_system_error(ec, ERROR_NOT_ENOUGH_MEMORY);
}

inline DWORD decode_access_mode(std::ios_base::openmode omode)
{
    switch (omode & (LLFile::in | LLFile::out))
    {
        case LLFile::in:
            return GENERIC_READ;
        case LLFile::out:
            return GENERIC_WRITE;
        case LLFile::in | LLFile::out:
            return GENERIC_READ | GENERIC_WRITE;
    }
    if (omode & LLFile::app)
    {
        return GENERIC_WRITE;
    }
    return 0;
}

inline DWORD decode_open_create_flags(std::ios_base::openmode omode)
{
    if (omode & LLFile::noreplace)
    {
        return CREATE_NEW; // create if it does not exist, otherwise fail
    }
    if (omode & LLFile::trunc)
    {
        if (!(omode & LLFile::out))
        {
            return TRUNCATE_EXISTING; // open and truncatte if it exists, otherwise fail
        }
        return CREATE_ALWAYS; // open and truncate if it exists, otherwise create it
    }
    if (!(omode & LLFile::out))
    {
        return OPEN_EXISTING; // open if exists, otherwise fail
    }
    // LLFile::app or (LLFile::out and (!LLFile::trunc or !LLFile::noreplace))
    return OPEN_ALWAYS; // open if it exists, otherwise create it
}

inline DWORD decode_share_mode(int omode)
{
    if (omode & LLFile::exclusive)
    {
        return 0; // allow no other access
    }
    if (omode & LLFile::shared)
    {
        return FILE_SHARE_READ; // allow read access
    }
    return FILE_SHARE_READ | FILE_SHARE_WRITE; // allow read and write access to others
}

inline DWORD decode_attributes(std::ios_base::openmode omode, int perm)
{
    return (perm & S_IWRITE) ? FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_READONLY;
}

// Under Windows the values for the std::ios_base::seekdir constants match the according FILE_BEGIN
// and other constants but we do a programmatic translation for now to be sure
static DWORD seek_mode_from_dir(std::ios_base::seekdir seekdir)
{
    switch (seekdir)
    {
        case LLFile::beg:
            return FILE_BEGIN;
        case LLFile::cur:
            return FILE_CURRENT;
        case LLFile::end:
            return FILE_END;
    }
    return FILE_BEGIN;
}

#else

inline int set_ec_from_system_error(std::error_code& ec, int error)
{
    ec.assign(error, std::system_category());
    return -1;
}

static int set_ec_from_system_error(std::error_code& ec)
{
    return set_ec_from_system_error(ec, errno);
}

inline int set_ec_to_parameter_error(std::error_code& ec)
{
    return set_ec_from_system_error(ec, EINVAL);
}

inline int set_ec_to_outofmemory_error(std::error_code& ec)
{
    return set_ec_from_system_error(ec, ENOMEM);
}

inline int decode_access_mode(std::ios_base::openmode omode)
{
    switch (omode & (LLFile::in | LLFile::out))
    {
        case LLFile::out:
            return O_WRONLY;
        case LLFile::in | LLFile::out:
            return O_RDWR;
    }
    return O_RDONLY;
}

inline int decode_open_mode(std::ios_base::openmode omode)
{
    int flags = O_CREAT | decode_access_mode(omode);
    if (omode & LLFile::app)
    {
        flags |= O_APPEND;
    }
    if (omode & LLFile::trunc)
    {
        flags |= O_TRUNC;
    }
    if (omode & LLFile::binary)
    {
        // Not a thing under *nix
    }
    if (omode & LLFile::noreplace)
    {
        flags |= O_EXCL;
    }
    return flags;
}

inline int decode_lock_mode(std::ios_base::openmode omode)
{
    int lmode = omode & LLFile::noblock ? LOCK_NB : 0;
    if (omode & LLFile::lock_mask)
    {
        if (omode & LLFile::exclusive)
        {
            return lmode | LOCK_EX;
        }
        return lmode | LOCK_SH;
    }
    return lmode | LOCK_UN;
}

// Under Linux and Mac the values for the std::ios_base::seekdir constants match the according SEEK_SET
// and other constants but we do a programmatic translation for now to be sure
inline int seek_mode_from_dir(std::ios_base::seekdir seekdir)
{
    switch (seekdir)
    {
        case LLFile::beg:
            return SEEK_SET;
        case LLFile::cur:
            return SEEK_CUR;
        case LLFile::end:
            return SEEK_END;
    }
    return SEEK_SET;
}

#endif

inline int clear_error(std::error_code& ec)
{
    ec.clear();
    return 0;
}

inline bool are_open_mode_flags_invalid(std::ios_base::openmode omode)
{
    // at least one of input or output needs to be specified
    if (!(omode & (LLFile::in | LLFile::out)))
    {
        return true;
    }
    // output must be possible for any of the extra options
    if (!(omode & LLFile::out) && (omode & (LLFile::trunc | LLFile::app | LLFile::noreplace)))
    {
        return true;
    }
    // invalid combination, mutually exclusive
    if ((omode & LLFile::app) && (omode & (LLFile::trunc | LLFile::noreplace)))
    {
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------
// class member functions
//----------------------------------------------------------------------------------------
int LLFile::open(const std::string& filename, std::ios_base::openmode omode, std::error_code& ec, int perm)
{
    close(ec);
    if (are_open_mode_flags_invalid(omode))
    {
        return set_ec_to_parameter_error(ec);
    }
#if LL_WINDOWS
    DWORD access = decode_access_mode(omode),
          share = decode_share_mode(omode),
          create = decode_open_create_flags(omode),
          attributes = decode_attributes(omode, perm);

    std::wstring file_path = utf8StringToWstring(filename);
    mHandle = CreateFileW(file_path.c_str(), access, share, nullptr, create, attributes, nullptr);
#else
    int oflags = decode_open_mode(omode);
    int lmode = LLFile::noblock | (omode & LLFile::lock_mask);
    mHandle = ::open(filename.c_str(), oflags, perm);
    if (mHandle != InvalidHandle && omode & LLFile::lock_mask &&
        lock(omode | LLFile::noblock, ec) != 0)
    {
        close();
        return -1;
    }
#endif
    if (mHandle == InvalidHandle)
    {
        return set_ec_from_system_error(ec);
    }

    if (omode & LLFile::ate && seek(0, LLFile::end, ec) != 0)
    {
        close();
        return -1;
    }
    mOpen = omode;
    return clear_error(ec);
}

S64 LLFile::size(std::error_code& ec)
{
#if LL_WINDOWS
    LARGE_INTEGER value = { 0 };
    if (GetFileSizeEx(mHandle, &value))
    {
        clear_error(ec);
        return value.QuadPart;
    }
#else
    struct stat statval;
    if (fstat(mHandle, &statval) == 0)
    {
        clear_error(ec);
        return statval.st_size;
    }
#endif
    return set_ec_from_system_error(ec);
}

S64 LLFile::tell(std::error_code& ec)
{
#if LL_WINDOWS
    LARGE_INTEGER value = { 0 };
    if (SetFilePointerEx(mHandle, value, &value, FILE_CURRENT))
    {
        clear_error(ec);
        return value.QuadPart;
    }
#else
    off_t offset = lseek(mHandle, 0, SEEK_CUR);
    if (offset != -1)
    {
        clear_error(ec);
        return offset;
    }
#endif
    return set_ec_from_system_error(ec);
}

int LLFile::seek(S64 pos, std::error_code& ec)
{
    return seek(pos, LLFile::beg, ec);
}

int LLFile::seek(S64 offset, std::ios_base::seekdir dir, std::error_code& ec)
{
    S64 newOffset = 0;
#if LL_WINDOWS
    DWORD seekdir = seek_mode_from_dir(dir);
    LARGE_INTEGER value;
    value.QuadPart = offset;
    if (SetFilePointerEx(mHandle, value, (PLARGE_INTEGER)&newOffset, seekdir))
#else
    newOffset = lseek(mHandle, offset, seek_mode_from_dir(dir));
    if (newOffset != -1)
#endif
    {
        return clear_error(ec);
    }
    return set_ec_from_system_error(ec);
}

#if LL_WINDOWS
inline DWORD next_buffer_size(S64 nbytes)
{
    return nbytes > 0x80000000 ? 0x80000000 : (DWORD)nbytes;
}
#endif

S64 LLFile::read(void* buffer, S64 nbytes, std::error_code& ec)
{
    if (nbytes == 0)
    {
        // Nothing to do
        clear_error(ec);
        return 0;
    }
    else if (!buffer || nbytes < 0)
    {
        return set_ec_to_parameter_error(ec);
    }
#if LL_WINDOWS
    S64 totalBytes = 0;
    char *ptr = (char*)buffer;
    DWORD bytesRead, bytesToRead = next_buffer_size(nbytes);

    // Read in chunks to support >4GB which the S64 nbytes value makes possible
    while (ReadFile(mHandle, ptr, bytesToRead, &bytesRead, nullptr))
    {
        totalBytes += bytesRead;
        if (nbytes <= totalBytes || // requested amount read
            bytesRead < bytesToRead) // ReadFile encountered eof
        {
            clear_error(ec);
            return totalBytes;
        }
        ptr += bytesRead;
        bytesToRead = next_buffer_size(nbytes - totalBytes);
    }
#else
    ssize_t bytesRead = ::read(mHandle, buffer, nbytes);
    if (bytesRead != -1)
    {
        clear_error(ec);
        return bytesRead;
    }
#endif
    return set_ec_from_system_error(ec);
}

S64 LLFile::write(const void* buffer, S64 nbytes, std::error_code& ec)
{
    if (nbytes == 0)
    {
        // Nothing to do here
        clear_error(ec);
        return 0;
    }
    else if (!buffer || nbytes < 0)
    {
        return set_ec_to_parameter_error(ec);
    }
#if LL_WINDOWS
    // If this was opened in append mode, we emulate it on Windows
    if (mOpen & LLFile::app && seek(0, LLFile::end, ec) != 0)
    {
        return -1;
    }

    S64 totalBytes = 0;
    char* ptr = (char*)buffer;
    DWORD bytesWritten, bytesToWrite = next_buffer_size(nbytes);

    // Write in chunks to support >4GB which the S64 nbytes value makes possible
    while (WriteFile(mHandle, ptr, bytesToWrite, &bytesWritten, nullptr))
    {
        totalBytes += bytesWritten;
        if (nbytes <= totalBytes)
        {
            clear_error(ec);
            return totalBytes;
        }
        ptr += bytesWritten;
        bytesToWrite = next_buffer_size(nbytes - totalBytes);
    }
#else
    ssize_t bytesWritten = ::write(mHandle, buffer, nbytes);
    if (bytesWritten != -1)
    {
        clear_error(ec);
        return bytesWritten;
    }
#endif
    return set_ec_from_system_error(ec);
}

S64 LLFile::printf(const char* fmt, ...)
{
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    int length = vsnprintf(NULL, 0, fmt, args1);
    va_end(args1);
    if (length < 0)
    {
        va_end(args2);
        return -1;
    }
    void* buffer = malloc(length + 1);
    if (!buffer)
    {
        va_end(args2);
        return -1;
    }
    length = vsnprintf((char*)buffer, length + 1, fmt, args2);
    va_end(args2);
    std::error_code ec;
    S64 written = write(buffer, length, ec);
    free(buffer);
    return written;
}

int LLFile::lock(int mode, std::error_code& ec)
{
#if LL_WINDOWS
    if (!(mode & LLFile::lock_mask))
    {
        if (UnlockFile(mHandle, 0, 0, MAXDWORD, MAXDWORD))
        {
            return clear_error(ec);
        }
    }
    else
    {
        OVERLAPPED overlapped = { 0 };
        DWORD flags = (mode & LLFile::noblock) ? LOCKFILE_FAIL_IMMEDIATELY : 0;
        if (mode & LLFile::exclusive)
        {
            flags |= LOCKFILE_EXCLUSIVE_LOCK;
        }
        // We lock the  maximum range, since flock only supports locking the entire file too
        if (LockFileEx(mHandle, flags, 0, MAXDWORD, MAXDWORD, &overlapped))
        {
            return clear_error(ec);
        }
    }
#else
    if (flock(mHandle, decode_lock_mode(mode)) == 0)
    {
        return clear_error(ec);
    }
#endif
    return set_ec_from_system_error(ec);
}

int LLFile::close(std::error_code& ec)
{
    if (mHandle != InvalidHandle)
    {
        llfile_handle_t handle = InvalidHandle;
        std::swap(handle, mHandle);
#if LL_WINDOWS
        if (!CloseHandle(handle))
#else
        if (::close(handle))
#endif
        {
            return set_ec_from_system_error(ec);
        }
    }
    return clear_error(ec);
}

int LLFile::close()
{
    std::error_code ec;
    return close(ec);
}

//----------------------------------------------------------------------------------------
// static member functions
//----------------------------------------------------------------------------------------

// static
LLFILE* LLFile::fopen(const std::string& filename, const char* mode, int lmode)
{
    LLFILE* file;
#if LL_WINDOWS
    int shflag = _SH_DENYNO;
    switch (lmode)
    {
        case LLFile::exclusive:
            shflag = _SH_DENYRW;
            break;
        case LLFile::shared:
            shflag = _SH_DENYWR;
            break;
    }
    std::wstring file_path = utf8StringToWstring(filename);
    std::wstring utf16mode = ll_convert<std::wstring>(std::string(mode));
    file = _wfsopen(file_path.c_str(), utf16mode.c_str(), shflag);
#else
    file = ::fopen(filename.c_str(), mode);
    if (file && (lmode & (LLFile::lock_mask)))
    {
        // Rather fail on a sharing conflict than block
        if (flock(fileno(file), decode_lock_mode(lmode | LLFile::noblock)))
        {
            LLFile::close(file);
            file = nullptr;
        }
    }
#endif
    return file;
}

// static
int LLFile::close(LLFILE* file)
{
    int ret_value = 0;
    if (file)
    {
        // Read the current errno and restore it if it was not 0
        int errn = errno;
        ret_value = ::fclose(file);
        if (errn)
        {
            errno = errn;
        }
    }
    return ret_value;
}

// static
std::string LLFile::getContents(const std::string& filename)
{
    std::error_code ec;
    return getContents(filename, ec);
}

// static
std::string LLFile::getContents(const std::string& filename, std::error_code& ec)
{
    std::string buffer;
    LLFile file(filename, LLFile::in | LLFile::binary, ec);
    if (file)
    {
        S64 length = file.size(ec);
        if (!ec && length > 0)
        {
            buffer = std::string(length, 0);
            file.read(&buffer[0], length, ec);
            if (ec)
            {
                buffer.clear();
            }
        }
    }
    return buffer;
}

// static
int LLFile::mkdir(const std::string& dirname)
{
    std::error_code ec;
    std::filesystem::path file_path = utf8StringToPath(dirname);
    // We often use mkdir() to ensure the existence of a directory that might
    // already exist. There is no known case in which we want to call out as
    // an error the requested directory already existing.
    std::filesystem::create_directory(file_path, ec);
    // The return value is only true if the directory was actually created.
    // But if it already existed, ec still indicates success.
    return warnif("mkdir", dirname, ec);
}

// static
int LLFile::remove(const std::string& filename, int suppress_error)
{
    std::error_code ec;
    std::filesystem::path file_path = utf8StringToPath(filename);
    std::filesystem::remove(file_path, ec);
    return warnif("remove", filename, ec, suppress_error);
}

// static
int LLFile::rename(const std::string& filename, const std::string& newname, int suppress_error)
{
    std::error_code ec;
    std::filesystem::path file_path = utf8StringToPath(filename);
    std::filesystem::path new_path = utf8StringToPath(newname);
    std::filesystem::rename(file_path, new_path, ec);
    return warnif(STRINGIZE("rename to '" << newname << "' from"), filename, ec, suppress_error);
}

// static
S64 LLFile::read(const std::string& filename, void* buf, S64 offset, S64 nbytes)
{
    std::error_code ec;
    return read(filename, buf, offset, nbytes, ec);
}

// static
S64 LLFile::read(const std::string& filename, void* buf, S64 offset, S64 nbytes, std::error_code& ec)
{
    // if number of bytes is 0 or less there is nothing to do here
    if (nbytes <= 0)
    {
        clear_error(ec);
        return 0;
    }

    std::ios_base::openmode omode = LLFile::in | LLFile::binary;

    LLFile file(filename, omode, ec);
    if (file)
    {
        S64 bytes_read = 0;
        if (offset > 0)
        {
            file.seek(offset, ec);
        }
        if (!ec)
        {
            bytes_read = file.read(buf, nbytes, ec);
            if (!ec)
            {
                return bytes_read;
            }
        }
    }
    return warnif("read from file failed", filename, ec);
}

// static
S64 LLFile::write(const std::string& filename, const void* buf, S64 offset, S64 nbytes)
{
    std::error_code ec;
    return write(filename, buf, offset, nbytes, ec);
}

// static
S64 LLFile::write(const std::string& filename, const void* buf, S64 offset, S64 nbytes, std::error_code& ec)
{
    // if number of bytes is 0 or less there is nothing to do here
    if (nbytes <= 0)
    {
        clear_error(ec);
        return 0;
    }

    std::ios_base::openmode omode = LLFile::out | LLFile::binary;
    if (offset < 0)
    {
        omode |= LLFile::app;
    }

    LLFile file(filename, omode, ec);
    if (file)
    {
        S64 bytes_written = 0;
        if (offset > 0)
        {
            file.seek(offset, ec);
        }
        if (!ec)
        {
            bytes_written = file.write(buf, nbytes, ec);
            if (!ec)
            {
                return bytes_written;
            }
        }
    }
    return warnif("write to file failed", filename, ec);
}

// static
bool LLFile::copy(const std::string& from, const std::string& to)
{
    std::error_code ec;
    std::filesystem::path from_path = utf8StringToPath(from);
    std::filesystem::path to_path   = utf8StringToPath(to);
    bool copied = std::filesystem::copy_file(from_path, to_path, ec);
    if (!copied)
    {
        LL_WARNS("LLFile") << "copy failed" << LL_ENDL;
    }
    return copied;
}

// static
int LLFile::stat(const std::string& filename, llstat* filestatus, const char *fname, int suppress_warning)
{
#if LL_WINDOWS
    std::wstring file_path = utf8StringToWstring(filename);
    int rc = _wstat64(file_path.c_str(), filestatus);
#else
    int rc = ::stat(filename.c_str(), filestatus);
#endif
    return warnif(fname ? fname : "stat", filename, rc, suppress_warning);
}

// static
std::time_t LLFile::getCreationTime(const std::string& filename, int suppress_warning)
{
    // As if C++20 there is no functionality in std::filesystem to retrieve this information
    llstat filestat;
    int rc = stat(filename, &filestat, "getCreationTime", suppress_warning);
    if (rc == 0)
    {
#if LL_DARWIN
        return filestat.st_birthtime;
#else
        // Linux stat() doesn't have a creation/birth time (st_ctime really is the last status
        // change or inode attributes change) unless we would use statx() instead. But that is
        // a major effort, which would require Linux specific changes to LLFile::stat() above
        // and possibly adaptions for other platforms that we leave for a later exercise if it
        // is ever desired.
        return filestat.st_ctime;
#endif
    }
    return 0;
}

// static
std::time_t LLFile::getModificationTime(const std::string& filename, int suppress_warning)
{
    // tried to use std::filesystem::last_write_time() but the whole std::chrono infrastructure is as of
    // C++20 still not fully implemented on all platforms. Specifically MacOS C++20 seems lacking here,
    // and Windows requires a roundabout through std::chrono::utc_clock to then get a
    // std::chrono::system_clock that can return a more useful time_t.
    // So we take the easy way out in a similar way as with getCreationTime().
    llstat filestat;
    int rc = stat(filename, &filestat, "getModificationTime", suppress_warning);
    if (rc == 0)
    {
        return filestat.st_mtime;
    }
    return 0;
}

// static
S64 LLFile::size(const std::string& filename, int suppress_warning)
{
    std::error_code ec;
    std::filesystem::path file_path = utf8StringToPath(filename);
    std::intmax_t size = (std::intmax_t)std::filesystem::file_size(file_path, ec);
    return warnif("size", filename, ec, suppress_warning) ? -1 : size;
}

// static
std::filesystem::file_status LLFile::getStatus(const std::string& filename, bool dontFollowSymLink, int suppress_warning)
{
    std::error_code ec;
    std::filesystem::path file_path = utf8StringToPath(filename);
    std::filesystem::file_status status;
    if (dontFollowSymLink)
    {
        status = std::filesystem::status(file_path, ec);
    }
    else
    {
        status = std::filesystem::symlink_status(file_path, ec);
    }
    warnif("getattr", filename, ec, suppress_warning);
    return status;
}

// static
bool LLFile::exists(const std::string& filename)
{
    std::filesystem::file_status status = getStatus(filename);
    return std::filesystem::exists(status);
}

// static
bool LLFile::isdir(const std::string& filename)
{
    std::filesystem::file_status status = getStatus(filename);
    return std::filesystem::is_directory(status);
}

// static
bool LLFile::isfile(const std::string& filename)
{
    std::filesystem::file_status status = getStatus(filename);
    return std::filesystem::is_regular_file(status);
}

// static
bool LLFile::islink(const std::string& filename)
{
    std::filesystem::file_status status = getStatus(filename, true);
    return std::filesystem::is_symlink(status);
}

// static
const std::string& LLFile::tmpdir()
{
    static std::string temppath;
    if (temppath.empty())
    {
        temppath = std::filesystem::temp_directory_path().string();
    }
    return temppath;
}

// static
std::filesystem::path LLFile::utf8StringToPath(const std::string& pathname)
{
#if LL_WINDOWS
    return ll_convert<std::wstring>(pathname);
#else
    return pathname;
#endif
}

#if LL_WINDOWS

// static
std::wstring LLFile::utf8StringToWstring(const std::string& pathname)
{
    std::wstring utf16string(ll_convert<std::wstring>(pathname));
    if (utf16string.size() >= MAX_PATH)
    {
        // By going through std::filesystem::path we get a lot of path sanitation done for us that
        // is needed when passing a path with a kernel object space prefix to Windows API functions
        // since this prefix disables the kernel32 path normalization
        std::filesystem::path utf16path(utf16string);

        // By prepending "\\?\" to a path, Windows widechar file APIs will not fail on long path names
        utf16string.assign(L"\\\\?\\").append(utf16path);

        /* remove trailing spaces and dots (yes, Windows really does that) */
        return utf16string.substr(0, utf16string.find_last_not_of(L" \t."));
    }
    return utf16string;
}

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
    ifstr.seekg(0, std::ios_base::beg);
    std::streampos pos_beg = ifstr.tellg();
    ifstr.seekg(0, std::ios_base::end);
    std::streampos pos_end = ifstr.tellg();
    ifstr.seekg(pos_old, std::ios_base::beg);
    return pos_end - pos_beg;
}

std::streamsize llofstream_size(llofstream& ofstr)
{
    if(!ofstr.is_open()) return 0;
    std::streampos pos_old = ofstr.tellp();
    ofstr.seekp(0, std::ios_base::beg);
    std::streampos pos_beg = ofstr.tellp();
    ofstr.seekp(0, std::ios_base::end);
    std::streampos pos_end = ofstr.tellp();
    ofstr.seekp(pos_old, std::ios_base::beg);
    return pos_end - pos_beg;
}

#endif  // LL_WINDOWS
