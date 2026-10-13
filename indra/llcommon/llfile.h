/**
 * @file llfile.h
 * @author Michael Schlachter
 * @date 2006-03-23
 * @brief Declaration of cross-platform POSIX file buffer and c++
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

#ifndef LL_LLFILE_H
#define LL_LLFILE_H

/**
 * This class provides a cross platform interface to the filesystem.
 * Attempts to mostly mirror the POSIX style IO functions.
 */

#include "stdtypes.h"

#include <fstream>
#include <filesystem>
#include <sys/stat.h>

#if LL_WINDOWS
#include <windows.h>
// The Windows version of stat function and stat data structure are called _stat64
// We use _stat64 here to support 64-bit st_size and time_t values
typedef struct _stat64 llstat;
#else
#include <sys/types.h>
typedef struct stat llstat;
#endif

typedef FILE LLFILE;

#include "fsyspath.h"
#include "llstring.h" // safe char* -> std::string conversion

#if LL_WINDOWS
#define LLFILE_MODE(flags) L##flags
#else
#define LLFILE_MODE(flags) flags
#endif

/// This class provides a selection of functions to operate on files through names and
/// a class implementation to represent a file for reading and writing to it
/// All the functions with a path string input take UTF8 path/filenames
///
/// @nosubgrouping
///
class LLFile
{
public:
    // ================================================================================
    /// @name Constants
    ///
    ///@{
    /** These can be passed to the omode parameter of LLFile::open() and its constructor

    This is similar to the openmode flags for std:fstream but not exactly the same
    std::fstream open() does not allow to open a file for writing without either
    forcing the file to be truncated on open or all write operations being always
    appended to the end of the file or failing to open when the file does not exist.
    But to allow implementing the LLAPRFile::writeEx() functionality we need to be
    able to write at a random position to the file without truncating it on open.

    any other combinations than listed here are not allowed and will cause an error

    bin    in     out    trunc  app    noreplace  File exists       File doesn't exist
    ----------------------------------------------------------------------------------
    -      +      -      -      -      -          Open at begin     Failure to open
    +      +      -      -      -      -            "                 "
    -      -      +      -      -      -            "               Create new
    +      -      +      -      -      -            "                 "
    -      +      +      -      -      -            "                 "
    +      +      +      -      -      -            "                 "
    ----------------------------------------------------------------------------------
    -      -      +      +      -      -          Destroy contents  Create new
    +      -      +      +      -      -            "                 "
    -      +      +      +      -      -            "                 "
    +      +      +      +      -      -            "                 "
    ----------------------------------------------------------------------------------
    -      -      +      x      -      +          Failure to open   Create new
    +      -      +      x      -      +            "                 "
    -      +      +      x      -      +            "                 "
    +      +      +      x      -      +            "                 "
    ----------------------------------------------------------------------------------
    -      -      +      -      +      -          Write to end      Create new
    +      -      +      -      +      -            "                 "
    -      +      +      -      +      -            "                 "
    +      +      +      -      +      -            "                 "
    ----------------------------------------------------------------------------------
    */
    static const std::ios_base::openmode app       = static_cast<std::ios_base::openmode>(1 << 1);    // append to end
    static const std::ios_base::openmode ate       = static_cast<std::ios_base::openmode>(1 << 2);    // initialize to end
    static const std::ios_base::openmode binary    = static_cast<std::ios_base::openmode>(1 << 3);    // binary mode
    static const std::ios_base::openmode in        = static_cast<std::ios_base::openmode>(1 << 4);    // for reading
    static const std::ios_base::openmode out       = static_cast<std::ios_base::openmode>(1 << 5);    // for writing          // for writing and reading
    static const std::ios_base::openmode trunc     = static_cast<std::ios_base::openmode>(1 << 6);    // truncate on open
    static const std::ios_base::openmode noreplace = static_cast<std::ios_base::openmode>(1 << 7);    // no replace if it exists

    /// Additional optional flags to omode in open() and lmode in fopen() or lock()
    /// to indicate which sort of lock if any to attempt to get
    ///
    /// NOTE: there is a fundamental difference between platforms.
    /// On Windows this lock is mandatory as it is part of the API to open a file handle and other
    /// processes can not avoid it. If a file was opened denying other processes read and/or write
    /// access, trying to open the same file in another process with that access will fail.
    /// On Mac and Linux it is only an advisory lock implemented through the flock() system call.
    /// This means that any other application needs to also attempt to at least acquire a shared
    /// lock on the file in order to notice that the file is actually already locked. It can
    /// therefore not be used to prevent random other applications from accessing the file, but it
    /// works for other viewer processes when they use either the LLFile::open() or LLFile::fopen()
    /// functions with the appropriate lock flags to open a file.
    static const std::ios_base::openmode exclusive = static_cast<std::ios_base::openmode>(1 << 16);
    static const std::ios_base::openmode shared    = static_cast<std::ios_base::openmode>(1 << 17);

    /// Additional lmode flag to indicate to rather fail instead of blocking when trying
    /// to acquire a lock with LLFile::lock()
    static const std::ios_base::openmode noblock   = static_cast<std::ios_base::openmode>(1 << 18);

    /// The mask value for the lock mask bits
    static const std::ios_base::openmode lock_mask = static_cast<std::ios_base::openmode>(exclusive | shared);

    /// One of these can be passed to the dir parameter of LLFile::seek()
    static const std::ios_base::seekdir beg        = std::ios_base::beg;
    static const std::ios_base::seekdir cur        = std::ios_base::cur;
    static const std::ios_base::seekdir end        = std::ios_base::end;
    ///@}

    // ================================================================================
    /// @name constructor/deconstructor
    ///
    ///@{
    ///  default constructor
    LLFile() : mHandle(InvalidHandle) {}

    /// no copy constructor
    LLFile(const LLFile&) = delete;

    /// move constructor
    LLFile(LLFile&& other) noexcept
    {
        mHandle = other.mHandle;
        other.mHandle = InvalidHandle;
    }

    /// constructor opening the file
    explicit LLFile(const char* filename, std::ios_base::openmode omode, std::error_code& ec, int perm = 0666) :
        mHandle(InvalidHandle)
    {
        open(filename, omode, ec, perm);
    }

    explicit LLFile(const std::string& filename, std::ios_base::openmode omode, std::error_code& ec, int perm = 0666) :
        mHandle(InvalidHandle)
    {
        open(filename, omode, ec, perm);
    }

    explicit LLFile(const std::filesystem::path& file_path, std::ios_base::openmode omode, std::error_code& ec, int perm = 0666) :
        mHandle(InvalidHandle)
    {
        open(file_path, omode, ec, perm);
    }

    /// destructor always attempts to close the file
    ~LLFile() { close(); }
    ///@}

    // ================================================================================
    /// @name operators
    ///
    ///@{
    /// copy assignment deleted
    LLFile& operator=(const LLFile&) = delete;

    /// move assignment
    LLFile& operator=(LLFile&& other) noexcept
    {
        close();
        std::swap(mHandle, other.mHandle);
        return *this;
    }

    // detect whether the wrapped file descriptor/handle is open or not
    explicit operator bool() const { return (mHandle != InvalidHandle); }
    bool     operator!() { return (mHandle == InvalidHandle); }
    ///@}

    /// ================================================================================
    /// @name  class member methods
    ///
    /// These methods provide read and write support as well as additional functionality to query the size of
    /// the file, change the position of the current file pointer or query it.
    ///
    /// Most of these functions take as one of their parameters a std::error_code object which can be used to
    /// determine in more detail what error occurred if required
    ///@{

    /// Open a file with the specific open mode flags
    inline int open(const char* filename, std::ios_base::openmode omode, std::error_code& ec, int perm = 0666)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return open(file_path, omode, ec, perm);
    }
    inline int open(const std::string& filename, std::ios_base::openmode omode, std::error_code& ec, int perm = 0666)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return open(file_path, omode, ec, perm);
    }
    int open(const std::filesystem::path& filename, std::ios_base::openmode omode, std::error_code& ec, int perm = 0666);
    ///< @returns 0 on success, -1 on failure

    /// Determine the size of the opened file
    S64 size(std::error_code& ec);
    ///< @returns the number of bytes in the file or 0 on failure

    /// Query the position of the current file pointer in the file
    S64 tell(std::error_code& ec);
    ///< @returns the absolute offset of the file pointer in bytes relative to the start of the file or -1 on failure

    /// Move the file pointer to the specified absolute position relative to the start of the file
    int seek(S64 pos, std::error_code& ec);
    ///< @returns 0 on success, -1 on failure

    /// Move the file pointer to the specified position relative to dir
    int seek(S64 offset, std::ios_base::seekdir dir, std::error_code& ec);
    ///< @returns 0 on success, -1 on failure

    /// Read the specified number of bytes into the buffer starting at the current file pointer
    S64 read(void* buffer, S64 nbytes, std::error_code& ec);
    ///< If the file ends before the requested amount of bytes could be read, the function succeeds and
    ///  returns the bytes up to the end of the file. The return value indicates the number of actually
    ///  read bytes and can be therefore smaller than the requested amount.
    ///  @returns the number of bytes read from the file or -1 on failure

    /// Write the specified number of bytes to the file starting at the current file pointer
    S64 write(const void* buffer, S64 nbytes, std::error_code& ec);
    ///< @returns the number of bytes written to the file or -1 on failure

    /// Write into the file starting at the current file pointer using printf style format and
    /// additional optional parameters as specified in the fmt string
    S64 printf(const char* fmt, ...);
    ///< @returns the number of bytes written to the file or -1 on failure

    /// Attempt to acquire or release a lock on the file
    int lock(int lmode, std::error_code& ec);
    ///< lmode can be one of LLFile::exclusive or LLFile::shared to acquire the according lock
    ///  or 0 to give up an earlier acquired lock. Adding LLFile::noblock together with one of
    ///  the lock requests will cause the function to fail if the lock can not be acquired,
    ///  otherwise the function will block until the lock can be acquired.
    ///  @returns 0 on success, -1 on failure

    /// close the file explicitly
    int close(std::error_code& ec);
    ///< @returns 0 on success, -1 on failure

    /// Convenience function to close the file without additional parameters
    int close();
    ///< @returns 0 on success, -1 on failure
    ///@}

    /// ================================================================================
    /// @name  static member functions
    ///
    /// These functions are static and operate with UTF8 filenames as one of their parameters.
    ///
    ///@{
    /// open a file with the specified access mode
    ///
#if LL_WINDOWS
    using fopen_flags_t = wchar_t;
#else
    using fopen_flags_t = char;
#endif
    inline static LLFILE* fopen(const char* filename, const fopen_flags_t* accessmode, int lmode = 0)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return LLFile::fopen(file_path, accessmode, lmode);
    }
    inline static LLFILE* fopen(const std::string& filename, const fopen_flags_t* accessmode, int lmode = 0)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return LLFile::fopen(file_path, accessmode, lmode);
    }
    static LLFILE* fopen(const std::filesystem::path& file_path, const fopen_flags_t* accessmode, int lmode = 0);
    ///< 'accessmode' follows the rules of the Posix fopen() mode parameter
    ///  "r" open the file for reading only and positions the stream at the beginning
    ///  "r+" open the file for reading and writing and positions the stream at the beginning
    ///  "w" open the file for reading and writing and truncate it to zero length
    ///  "w+" open or create the file for reading and writing and truncate to zero length if it existed
    ///  "a" open the file for reading and writing and before every write position the stream at the end of the file
    ///  "a+" open or create the file for reading and writing and before every write position the stream at the end of the file
    ///
    ///  in addition to these values, "b" can be appended to indicate binary stream access, but on Linux and Mac
    ///  this is strictly for compatibility and has no effect. On Windows this makes the file functions not
    ///  try to translate line endings. Windows also allows to append "t" to indicate text mode. If neither
    ///  "b" or "t" is defined, Windows uses the value set by _fmode which by default is _O_TEXT.
    ///  This means that it is always a good idea to append "b" specifically for binary file access to
    ///  avoid corruption of the binary consistency of the data stream when reading or writing
    ///  Other characters in 'accessmode', while possible on some platforms (Windows), will usually
    ///  cause an error on other platforms as fopen will verify this parameter
    ///
    ///  lmode is optional and allows to lock the file for other processes either as a shared lock or an
    ///  exclusive lock. If the requested lock conflicts with an already existing lock, the open fails.
    ///  Pass either LLFIle::exclusive or LLFile::shared to this parameter if you want to prevent other
    ///  processes from reading (exclusive lock) or writing (shared lock) to the file. It will always use
    ///  LLFile::noblock, meaning the open will immediately fail if it conflicts with an existing lock on the
    ///  file.
    ///
    ///  @returns a valid LLFILE* pointer on success that can be passed to the fread() and fwrite() functions
    ///  and some other f<something> functions in the Standard C library that accept a FILE* as parameter
    ///  or NULL on failure

    /// Close a file handle opened with fopen() above
    static  int     close(LLFILE * file);
    ///< @returns 0 on success and -1 on failure.

    /// create a directory
    inline static int mkdir(const char* dirname)
    {
        std::filesystem::path dir_path = fsyspath(dirname);
        return mkdir(dir_path);
    }
    inline static int mkdir(const std::string& dirname)
    {
        std::filesystem::path dir_path = fsyspath(dirname);
        return mkdir(dir_path);
    }
    static int mkdir(const std::filesystem::path& dirname);
    ///< mkdir() considers "directory already exists" to be not an error.
    ///  @returns 0 on success and -1 on failure.

    /// remove a file or directory
    inline static int remove(const char* filename, int suppress_warning = 0)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return remove(file_path, suppress_warning);
    }
    inline static int remove(const std::string& filename, int suppress_warning = 0)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return remove(file_path, suppress_warning);
    }
    static int remove(const std::filesystem::path& file_path, int suppress_warning = 0);
    ///< pass an errno value (e.g., ENOENT) in the optional 'suppress_warning' parameter if you want to
    ///  suppress a warning in the log when the failure matches that errno (e.g., suppress warning if
    ///  the file or directory does not exist)
    ///  @returns 0 on success and -1 on failure.

    /// rename a file
    inline static int rename(const char* filename, const char* newname, int suppress_warning = 0)
    {
        std::filesystem::path file_path = fsyspath(filename);
        std::filesystem::path new_path  = fsyspath(newname);
        return rename(file_path, new_path, suppress_warning);
    }
    inline static int rename(const std::string& filename, const char* newname, int suppress_warning = 0)
    {
        std::filesystem::path file_path = fsyspath(filename);
        std::filesystem::path new_path  = fsyspath(newname);
        return rename(file_path, new_path, suppress_warning);
    }
    inline static int rename(const char* filename, const std::string& newname, int suppress_warning = 0)
    {
        std::filesystem::path file_path = fsyspath(filename);
        std::filesystem::path new_path  = fsyspath(newname);
        return rename(file_path, new_path, suppress_warning);
    }
    inline static int rename(const std::string& filename, const std::string& newname, int suppress_warning = 0)
    {
        std::filesystem::path file_path = fsyspath(filename);
        std::filesystem::path new_path  = fsyspath(newname);
        return rename(file_path, new_path, suppress_warning);
    }
    static int rename(const std::filesystem::path& file_path, const std::filesystem::path& new_path, int suppress_warning = 0);
    ///< it will silently overwrite newname if it exists without returning an error
    ///  Posix guarantees that if newname already exists, then there will be no moment
    ///  in which for other processes newname does not exist. There is no such guarantee
    ///  under Windows at this time. It may do it in the same way but the used Windows
    ///  APIs do not make such guarantees.
    ///  @returns 0 on success and -1 on failure.

    /// copy the contents of the file from 'source' to 'target'
    inline static bool copy(const char* source, const char* target)
    {
        std::error_code ec;
        return copy(source, target, std::filesystem::copy_options::overwrite_existing, ec);
    }
    inline static bool copy(const char* source, const std::string& target)
    {
        std::error_code ec;
        return copy(source, target, std::filesystem::copy_options::overwrite_existing, ec);
    }
    inline static bool copy(const std::string& source, const char* target)
    {
        std::error_code ec;
        return copy(source, target, std::filesystem::copy_options::overwrite_existing, ec);
    }
    inline static bool copy(const std::string& source, const std::string& target)
    {
        std::error_code ec;
        return copy(source, target, std::filesystem::copy_options::overwrite_existing, ec);
    }
    inline static bool copy(const std::filesystem::path& source_path, const std::filesystem::path& target_path)
    {
        std::error_code ec;
        return copy(source_path, target_path, std::filesystem::copy_options::overwrite_existing, ec);
    }
    ///< Copies the contents of the file 'source' to the file 'target', overwriting 'target' if it already
    ///  existed.
    ///  This is a convenience function that implements the previous behavior of silently overwriting an
    ///  already existing target file. Consider using the function below if you desire a different
    ///  behavior when the target file already exists
    ///  @returns true on success and false on failure.

    /// copy the contents of the file from 'from' to 'to'
    inline static bool copy(const char* source, const char* target, std::filesystem::copy_options options, std::error_code& ec)
    {
        std::filesystem::path source_path = fsyspath(source);
        std::filesystem::path target_path = fsyspath(target);
        return copy(source_path, target_path, options, ec);
    }
    inline static bool copy(const char* source, const std::string& target, std::filesystem::copy_options options, std::error_code& ec)
    {
        std::filesystem::path source_path = fsyspath(source);
        std::filesystem::path target_path = fsyspath(target);
        return copy(source_path, target_path, options, ec);
    }
    inline static bool copy(const std::string& source, const char* target, std::filesystem::copy_options options, std::error_code& ec)
    {
        std::filesystem::path source_path = fsyspath(source);
        std::filesystem::path target_path = fsyspath(target);
        return copy(source_path, target_path, options, ec);
    }
    inline static bool copy(const std::string& source, const std::string& target, std::filesystem::copy_options options, std::error_code& ec)
    {
        std::filesystem::path source_path = fsyspath(source);
        std::filesystem::path target_path = fsyspath(target);
        return copy(source_path, target_path, options, ec);
    }
    static bool copy(const std::filesystem::path& source_path, const std::filesystem::path& target_path, std::filesystem::copy_options options, std::error_code& ec);
    ///< Copies the contents of the file 'source' to the file 'target'. The options parameter allows to
    ///  specify what should happen if the "target" file already exists:
    ///   std::filesystem::copy_options::none - return an error in ec and fail
    ///   std::filesystem::copy_options::skip_existing - skip the operation and do not overwrite file
    ///   std::filesystem::copy_options::overwrite_existing - overwrite the file
    ///   std::filesystem::copy_options::update_existing - overwrite the file only if it is older than the file being copied
    ///  @returns true on success and false on failure.

    /// retrieve the content of a file into a string
    inline static std::string getContents(const char* filename)
    {
        std::error_code ec;
        return getContents(filename, ec);
    }
    inline static std::string getContents(const std::string& filename)
    {
        std::error_code ec;
        return getContents(filename, ec);
    }
    inline static std::string getContents(const std::filesystem::path& file_path)
    {
        std::error_code ec;
        return getContents(file_path, ec);
    }
    inline static std::string getContents(const char* filename, std::error_code& ec)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return getContents(file_path, ec);
    }
    inline static std::string getContents(const std::string& filename, std::error_code& ec)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return getContents(file_path, ec);
    }
    static std::string getContents(const std::filesystem::path& file_path, std::error_code& ec);
    ///< @returns the entire content of the file as std::string or an empty string on failure

    /// read nBytes from the file into the buffer, starting at offset in the file
    inline static S64 read(const char* filename, void* buf, S64 offset, S64 nbytes)
    {
        std::error_code ec;
        return read(filename, buf, offset, nbytes, ec);
    }
    inline static S64 read(const std::string& filename, void* buf, S64 offset, S64 nbytes)
    {
        std::error_code ec;
        return read(filename, buf, offset, nbytes, ec);
    }
    inline static S64 read(const std::filesystem::path& file_path, void* buf, S64 offset, S64 nbytes)
    {
        std::error_code ec;
        return read(file_path, buf, offset, nbytes, ec);
    }
    inline static S64 read(const char* filename, void* buf, S64 offset, S64 nbytes, std::error_code& ec)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return read(file_path, buf, offset, nbytes, ec);
    }
    inline static S64 read(const std::string& filename, void* buf, S64 offset, S64 nbytes, std::error_code& ec)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return read(file_path, buf, offset, nbytes, ec);
    }
    static S64 read(const std::filesystem::path& filename, void* buf, S64 offset, S64 nbytes, std::error_code& ec);
    ///< @returns bytes read on success, or -1 on failure

    /// write nBytes from the buffer into the file, starting at offset in the file
    inline static S64 write(const std::string& filename, const void* buf, S64 offset, S64 nbytes)
    {
        std::error_code ec;
        std::filesystem::path file_path = fsyspath(filename);
        return write(file_path, buf, offset, nbytes, ec);
    }
    inline static S64 write(const std::filesystem::path& filename, const void* buf, S64 offset, S64 nbytes)
    {
        std::error_code ec;
        std::filesystem::path file_path = fsyspath(filename);
        return write(file_path, buf, offset, nbytes, ec);
    }
    inline static S64 write(const std::string& filename, const void* buf, S64 offset, S64 nbytes, std::error_code& ec)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return write(file_path, buf, offset, nbytes, ec);
    }
    static S64 write(const std::filesystem::path& filename, const void* buf, S64 offset, S64 nbytes, std::error_code& ec);
    ///< If a negative offset is provided, the file is opened in append mode and the
    ///  write will be appended to the end of the file.
    ///  @returns bytes written on success, or -1 on failure

    /// return the file stat structure for filename
    inline static int stat(const char* filename, llstat* file_status, const char* operation = nullptr, int suppress_warning = ENOENT)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return stat(file_path, file_status, operation, suppress_warning);
    }
    inline static int stat(const std::string& filename, llstat* file_status, const char* operation = nullptr, int suppress_warning = ENOENT)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return stat(file_path, file_status, operation, suppress_warning);
    }
    static int stat(const std::filesystem::path& file_path, llstat* file_status, const char *operation = nullptr, int suppress_warning = ENOENT);
    ///< for compatibility with existing uses of LL_File::stat() we use ENOENT as default in the
    ///  optional 'suppress_warning' parameter to avoid spamming the log with warnings when the API
    ///  is used to detect if a file exists
    ///  @returns 0 on success and -1 on failure.

    /// get the std::filesystem::file_status for filename
    inline static std::filesystem::file_status getStatus(const char* filename, bool dontFollowSymLink = false, int suppress_warning = ENOENT)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return getStatus(file_path, dontFollowSymLink, suppress_warning);
    }
    inline static std::filesystem::file_status getStatus(const std::string& filename, bool dontFollowSymLink = false, int suppress_warning = ENOENT)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return getStatus(file_path, dontFollowSymLink, suppress_warning);
    }
    static std::filesystem::file_status getStatus(const std::filesystem::path& file_path, bool dontFollowSymLink = false, int suppress_warning = ENOENT);
    ///< dontFollowSymLinks set to true returns the std::filesystem::file_status of the symlink if it
    ///  is one, rather than resolving it. We pass by default ENOENT in the optional 'suppress_warning'
    ///  parameter to not spam the log with warnings when the file or directory does not exist
    ///  @returns a std::filesystem::file_status value that can be passed to the appropriate std::filesystem::exists()
    ///  and other APIs accepting a file_status.

    /// get the size of a file in bytes
    inline static S64 size(const char* filename, int suppress_warning = ENOENT)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return size(file_path, suppress_warning);
    }
    inline static S64 size(const std::string& filename, int suppress_warning = ENOENT)
    {
        std::filesystem::path file_path = fsyspath(filename);
        return size(file_path, suppress_warning);
    }
    static S64 size(const std::filesystem::path& file_path, int suppress_warning = ENOENT);
    ///< we pass by default ENOENT in the optional 'suppress_warning' parameter to not spam
    ///  the log with warnings when the file does not exist
    ///  @returns the file size on success or 0 on failure.

    /// check if filename is an existing file or directory
    inline static bool exists(const char* filename)
    {
        std::filesystem::file_status status = getStatus(filename);
        return std::filesystem::exists(status);
    }
    inline static bool exists(const std::string& filename)
    {
        std::filesystem::file_status status = getStatus(filename);
        return std::filesystem::exists(status);
    }
    inline static bool exists(const std::filesystem::path& file_path)
    {
        std::filesystem::file_status status = getStatus(file_path);
        return std::filesystem::exists(status);
    }
    ///< @returns true if the path is for an existing file or directory

    /// check if filename is an existing directory
    inline static bool isdir(const char* filename)
    {
        std::filesystem::file_status status = getStatus(filename);
        return std::filesystem::is_directory(status);
    }
    inline static bool isdir(const std::string& filename)
    {
        std::filesystem::file_status status = getStatus(filename);
        return std::filesystem::is_directory(status);
    }
    inline static bool isdir(const std::filesystem::path& file_path)
    {
        std::filesystem::file_status status = getStatus(file_path);
        return std::filesystem::is_directory(status);
    }
    ///< @returns true if the path is for an existing directory

    /// check if filename is an existing file
    inline static bool isfile(const char* filename)
    {
        std::filesystem::file_status status = getStatus(filename);
        return std::filesystem::is_regular_file(status);
    }
    inline static bool isfile(const std::string& filename)
    {
        std::filesystem::file_status status = getStatus(filename);
        return std::filesystem::is_regular_file(status);
    }
    inline static bool isfile(const std::filesystem::path& file_path)
    {
        std::filesystem::file_status status = getStatus(file_path);
        return std::filesystem::is_regular_file(status);
    }
    ///< @returns true if the path is for an existing file

    /// check if filename is a symlink
    inline static bool islink(const char* filename)
    {
        std::filesystem::file_status status = getStatus(filename, true);
        return std::filesystem::is_symlink(status);
    }
    inline static bool islink(const std::string& filename)
    {
        std::filesystem::file_status status = getStatus(filename, true);
        return std::filesystem::is_symlink(status);
    }
    inline static bool islink(const std::filesystem::path& file_path)
    {
        std::filesystem::file_status status = getStatus(file_path, true);
        return std::filesystem::is_symlink(status);
    }
    ///< @returns true if the path is pointing at a symlink

    /// return a path to the temporary directory on the system
    static const std::string& tmpdir();

private:
#if LL_WINDOWS
    typedef HANDLE        llfile_handle_t;
    const llfile_handle_t InvalidHandle = INVALID_HANDLE_VALUE;
    llfile_handle_t       mHandle       = INVALID_HANDLE_VALUE; // The file handle/descriptor
#else
    typedef int           llfile_handle_t;
    const llfile_handle_t InvalidHandle = -1;
    llfile_handle_t       mHandle       = -1; // The file handle/descriptor
#endif

    std::ios_base::openmode mOpen{}; // Used to emulate std::ios_base::app under Windows
};

#if LL_WINDOWS
/**
 *  @brief  Controlling input for files.
 *
 *  This class supports reading from named files, using the inherited
 *  functions from std::ifstream. The only added value is that our constructor
 *  Does The Right Thing when passed a non-ASCII pathname. Sadly, that isn't
 *  true of Microsoft's std::ifstream.
 */
class llifstream : public std::ifstream
{
    // input stream associated with a C stream
  public:
    // Constructors:
    /**
     *  @brief  Default constructor.
     *
     *  Initializes @c sb using its default constructor, and passes
     *  @c &sb to the base class initializer.  Does not open any files
     *  (you haven't given it a filename to open).
     */
    llifstream() = default;

    /**
     *  @brief  Create an input file stream.
     *  @param  Filename  String specifying the filename.
     *  @param  Mode  Open file in specified mode (see std::ios_base).
     *
     *  @c ios_base::in is automatically included in @a mode.
     */
    explicit llifstream(const char* _Filename,
                        ios_base::openmode _Mode = ios_base::in);
    explicit llifstream(const std::string& _Filename,
                        ios_base::openmode _Mode = ios_base::in);
    explicit llifstream(const std::filesystem::path& _Filepath,
                        ios_base::openmode _Mode = ios_base::in);

    /**
     *  @brief  Opens an external file.
     *  @param  Filename  The name of the file.
     *  @param  Node  The open mode flags.
     *
     *  Calls @c llstdio_filebuf::open(s,mode|in).  If that function
     *  fails, @c failbit is set in the stream's error state.
     */
    void open(const char* _Filename,
              ios_base::openmode _Mode = ios_base::in);
    void open(const std::string& _Filename,
              ios_base::openmode _Mode = ios_base::in);
    void open(const std::filesystem::path& _Filepath,
              ios_base::openmode _Mode = ios_base::in);
};

/// RAII class
class LLUniqueFile
{
public:
    // empty
    LLUniqueFile() = default;
    // wrap (e.g.) result of LLFile::fopen()
    LLUniqueFile(LLFILE* f) : mFileHandle(f) {}
    // no copy
    LLUniqueFile(const LLUniqueFile&) = delete;
    // move construction
    LLUniqueFile(LLUniqueFile&& other) noexcept
    {
        mFileHandle       = other.mFileHandle;
        other.mFileHandle = nullptr;
    }
    // The point of LLUniqueFile is to close on destruction.
    ~LLUniqueFile() { close(); }

    // simple assignment
    LLUniqueFile& operator=(LLFILE* f)
    {
        close();
        mFileHandle = f;
        return *this;
    }
    // copy assignment deleted
    LLUniqueFile& operator=(const LLUniqueFile&) = delete;
    // move assignment
    LLUniqueFile& operator=(LLUniqueFile&& other) noexcept
    {
        close();
        std::swap(mFileHandle, other.mFileHandle);
        return *this;
    }

    // explicit close operation
    void close()
    {
        if (mFileHandle)
        {
            // in case close() throws, set mFileHandle null FIRST
            LLFILE* h{ nullptr };
            std::swap(h, mFileHandle);
            LLFile::close(h);
        }
    }

    // detect whether the wrapped LLFILE is open or not
    explicit operator bool() const { return bool(mFileHandle); }
    bool     operator!() { return !mFileHandle; }

    // LLUniqueFile should be usable for any operation that accepts LLFILE*
    // (or FILE* for that matter)
    operator LLFILE*() const { return mFileHandle; }

private:
    LLFILE* mFileHandle = nullptr;
};

/**
 *  @brief  Controlling output for files.
 *
 *  This class supports writing to named files, using the inherited functions
 *  from std::ofstream. The only added value is that our constructor Does The
 *  Right Thing when passed a non-ASCII pathname. Sadly, that isn't true of
 *  Microsoft's std::ofstream.
*/
class llofstream : public std::ofstream
{
  public:
    // Constructors:
    /**
     *  @brief  Default constructor.
     *
     *  Initializes @c sb using its default constructor, and passes
     *  @c &sb to the base class initializer.  Does not open any files
     *  (you haven't given it a filename to open).
     */
    llofstream() = default;

    /**
     *  @brief  Create an output file stream.
     *  @param  Filename  String specifying the filename.
     *  @param  Mode  Open file in specified mode (see std::ios_base).
     *
     *  @c ios_base::out is automatically included in @a mode.
     */
    explicit llofstream(const char* _Filename,
                        ios_base::openmode _Mode = ios_base::out|ios_base::trunc);
    explicit llofstream(const std::string& _Filename,
                        ios_base::openmode _Mode = ios_base::out|ios_base::trunc);
    explicit llofstream(const std::filesystem::path& _Filepath,
                        ios_base::openmode _Mode = ios_base::out|ios_base::trunc);

    /**
     *  @brief  Opens an external file.
     *  @param  Filename  The name of the file.
     *  @param  Node  The open mode flags.
     *
     *  @c ios_base::out is automatically included in @a mode.
     */
    void open(const char* _Filename,
              ios_base::openmode _Mode = ios_base::out|ios_base::trunc);
    void open(const std::string& _Filename,
              ios_base::openmode _Mode = ios_base::out|ios_base::trunc);
    void open(const std::filesystem::path& _Filepath,
              ios_base::openmode _Mode = ios_base::out|ios_base::trunc);
};

#else // ! LL_WINDOWS

// on non-windows, llifstream and llofstream are just mapped directly to the std:: equivalents
typedef std::ifstream llifstream;
typedef std::ofstream llofstream;

#endif // LL_WINDOWS or ! LL_WINDOWS

#endif // not LL_LLFILE_H
