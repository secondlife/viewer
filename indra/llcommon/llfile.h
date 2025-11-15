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

#include "llstring.h" // safe char* -> std::string conversion

/// All the functions with a path string input take UTF8 path/filenames
class LL_COMMON_API LLFile
{
public:
    //--------------------------------------------------------------------------------------
    // constants
    //--------------------------------------------------------------------------------------
    // These can be passed to the omode parameter of LLFile::open() and its constructor
    /*
    This is similar to the openmode flags for std:fstream but not exactly the same
    std::fstream open() does not allow to open a file for writing without either
    forcing the file to be truncated on open or all write operations being always
    appended to the end of the file.
    But to allow implementing the LLAPRFile::writeEx() functionality we need to be
    able to write at a random position to the file without truncating it on open.

    any other combinations than listed here are not allowed and will cause an error

    bin    in     out    trunc  app    norep  File exists       File does not exist
    -------------------------------------------------------------------------------
    -      +      -      -      -      -      Open at begin     Failure to open
    +      +      -      -      -      -        "                 "
    -      -      +      -      -      -        "               Create new
    +      -      +      -      -      -        "                 "
    -      +      +      -      -      -        "                 "
    +      +      +      -      -      -        "                 "
    -------------------------------------------------------------------------------
    -      -      +      +      -      -      Destroy contents  Create new
    +      -      +      +      -      -        "                 "
    -      +      +      +      -      -        "                 "
    +      +      +      +      -      -        "                 "
    -------------------------------------------------------------------------------
    -      -      +      x      -      +      Failure to open   Create new
    +      -      +      x      -      +        "                 "
    -      +      +      x      -      +        "                 "
    +      +      +      x      -      +        "                 "
    -------------------------------------------------------------------------------
    -      -      +      -      +      -      Write to end      Create new
    +      -      +      -      +      -        "                 "
    -      +      +      -      +      -        "                 "
    +      +      +      -      +      -        "                 "
    -------------------------------------------------------------------------------
    */
    static const std::ios_base::openmode app       = 1 << 1;    // append to end
    static const std::ios_base::openmode ate       = 1 << 2;    // initialize to end
    static const std::ios_base::openmode binary    = 1 << 3;    // binary mode
    static const std::ios_base::openmode in        = 1 << 4;    // for reading
    static const std::ios_base::openmode out       = 1 << 5;    // for writing
    static const std::ios_base::openmode trunc     = 1 << 6;    // truncate on open
    static const std::ios_base::openmode noreplace = 1 << 7;    // no replace if it exists

    // Additional optional omode flags to open() and lmode to fopen() or mode flags to lock()
    // to indicate which sort of lock if any to attempt to get
    static const std::ios_base::openmode exclusive = 1 << 16;
    static const std::ios_base::openmode shared    = 1 << 17;
    // When used either in omode for LLFile::open() or in lmode for LLFile::fopen() there is a
    // difference between platforms.
    // On Windows this lock is mandatory as it is part of the API to open a file and other processes
    // can not avoid it. If a file was opened denying other processes read and/or write access,
    // trying to open the same file with that access will fail.
    // On Mac and Linux it is only an advisory lock. This means that the other application
    // needs to also attempt to at least acquire a shared lock on the file in order to notice that
    // the file is actually already locked. It can therefore not be used to prevent random other
    // applications from accessing the file, but it works for other viewer processes when they use
    // either the LLFile::open() or LLFile::fopen() functions with the appropriate lock flags to
    // open a file.

    // Additional mode flag to indicate to rather fail instead of blocking when trying
    // to acquire a lock with LLFile::lock()
    static const std::ios_base::openmode noblock   = 1 << 18;

    static const std::ios_base::openmode lock_mask = exclusive | shared;

    // These can be passed to the dir parameter of LLFile::seek()
    static const std::ios_base::seekdir beg        = std::ios_base::beg;
    static const std::ios_base::seekdir cur        = std::ios_base::cur;
    static const std::ios_base::seekdir end        = std::ios_base::end;

    //--------------------------------------------------------------------------------------
    // constructor/deconstructor
    //--------------------------------------------------------------------------------------
    LLFile() : mHandle(InvalidHandle) {}

    // no copy
    LLFile(const LLFile&) = delete;

    // move construction
    LLFile(LLFile&& other) noexcept
    {
        mHandle = other.mHandle;
        other.mHandle = InvalidHandle;
    }

    // open a file
    LLFile(const std::string& filename, std::ios_base::openmode omode, std::error_code& ec, int perm = 0666) :
        mHandle(InvalidHandle)
    {
        open(filename, omode, ec, perm);
    }

    // Make it a RAII class
    ~LLFile() { close(); }

    //--------------------------------------------------------------------------------------
    // operators
    //--------------------------------------------------------------------------------------
    // copy assignment deleted
    LLFile& operator=(const LLFile&) = delete;

    // move assignment
    LLFile& operator=(LLFile&& other) noexcept
    {
        close();
        std::swap(mHandle, other.mHandle);
        return *this;
    }

    // detect whether the wrapped file descriptor/handle is open or not
    explicit operator bool() const { return (mHandle != InvalidHandle); }
    bool     operator!() { return (mHandle == InvalidHandle); }

    //--------------------------------------------------------------------------------------
    // class member methods
    //--------------------------------------------------------------------------------------

    /// All of these functions take as one of their parameters a std::error_code object which can be used to
    /// determine in more detail what error occurred if required

    /// Open a file with the specific open mode flags
    int open(const std::string& filename, std::ios_base::openmode omode, std::error_code& ec, int perm = 0666);
    ///< @returns 0 on success, -1 on failure

    /// Determine the size of the opened file
    S64 size(std::error_code& ec);
    ///< @returns the number of bytes in the file or -1 on failure

    /// Return the current file pointer into the file
    S64 tell(std::error_code& ec);
    ///< @returns the absolute offset of the file pointer in bytes relative to the start of the file or -1 on failure

    /// Move the file pointer to the specified absolute position relative to the start of the file
    int seek(S64 pos, std::error_code& ec);
    ///< @returns 0 on success, -1 on failure

    /// Move the file pointer to the specified position relative to dir
    int seek(S64 offset, std::ios_base::seekdir dir, std::error_code& ec);
    ///< @returns 0 on success, -1 on failure

    /// Read a number of bytes into the buffer starting at the current file pointer
    S64 read(void* buffer, S64 nbytes, std::error_code& ec);
    ///< If the file ends before the requested amount of bytes could be read, the function succeeds and
    /// returns the bytes up to the end of the file. The return value indicates the number of actually
    /// read bytes and can be therefore smaller than the requested amount.
    /// @returns the number of bytes read from the file or -1 on failure

    /// Write a number of bytes to the file starting at the current file pointer
    S64 write(const void* buffer, S64 nbytes, std::error_code& ec);
    /// @returns the number of bytes written to the file or -1 on failure

    /// Write into the file starting at the current file pointer using printf style format
    S64 printf(const char* fmt, ...);
    /// @returns the number of bytes written to the file or -1 on failure

    /// Attempt to acquire or release a lock on the file
    int lock(int mode, std::error_code& ec);
    ///< mode can be one of LLFile::exclusive or LLFile::shared to acquire the according lock
    /// or 0 to give up an earlier acquired lock. Adding LLFile::noblock together with one of
    /// the lock requests will cause the function to fail if the lock can not be acquired,
    /// otherwise the function will block until the lock can be acquired.
    /// @returns 0 on success, -1 on failure

    /// close the file explicitely
    int close(std::error_code& ec);
    ///< @returns 0 on success, -1 on failure

    /// Convinience function to close the file without parameters
    int close();
    ///< @returns 0 on success, -1 on failure

    //----------------------------------------------------------------------------------------
    // static member functions
    //
    // All filename parameters are UTF8 file paths
    //
    //----------------------------------------------------------------------------------------
    /// open a file with the specified access mode
    static LLFILE* fopen(const std::string& filename, const char* accessmode, int lmode = 0);
    ///< 'accessmode' follows the rules of the Posix fopen() mode parameter
    /// "r" open the file for reading only and positions the stream at the beginning
    /// "r+" open the file for reading and writing and positions the stream at the beginning
    /// "w" open the file for reading and writing and truncate it to zero length
    /// "w+" open or create the file for reading and writing and truncate to zero length if it existed
    /// "a" open the file for reading and writing and and before every write position the stream at the end of the file
    /// "a+" open or create the file for reading and writing and before every write position the stream at the end of the file
    ///
    /// in addition to these values, "b" can be appended to indicate binary stream access, but on Linux and Mac
    /// this is strictly for compatibility and has no effect. On Windows this makes the file functions not
    /// try to translate line endings. Windows also allows to append "t" to indicate text mode. If neither
    /// "b" or "t" is defined, Windows uses the value set by _fmode which by default is _O_TEXT.
    /// This means that it is always a good idea to append "b" specifically for binary file access to
    /// avoid corruption of the binary consistency of the data stream when reading or writing
    /// Other characters in 'accessmode', while possible on some platforms (Windows), will usually
    /// cause an error as fopen will verify this parameter
    ///
    /// lmode is optional and allows to lock the file for other processes either as a shared lock or an
    /// exclusive lock. If the requested lock conflicts with an already existing lock, the open fails.
    /// Pass either LLFIle::exclusive or LLFile::shared to this parameter if you want to prevent other
    /// processes from reading (exclusive lock) or writing (shared lock) to the file. It will always use
    /// LLFile::noblock, meaning the open will immediately fail if it conflicts with an existing lock on the
    /// file.
    ///
    /// @returns a valid LLFILE* pointer on success that can be passed to the fread() and fwrite() functions
    /// and some other f<something> functions in the Standard C library that accept a FILE* as parameter
    /// or NULL on failure

    /// Close a file handle opened with fopen() above
    static  int     close(LLFILE * file);
    ///  @returns 0 on success and -1 on failure.

    /// create a directory
    static  int     mkdir(const std::string& filename);
    ///< mkdir() considers "directory already exists" to be not an error.
    ///  @returns 0 on success and -1 on failure.

    /// remove a file or directory
    static  int     remove(const std::string& filename, int suppress_warning = 0);
    ///< pass ENOENT in the optional 'suppress_warning' parameter if you don't want
    ///  a warning in the log when the directory does not exist
    ///  @returns 0 on success and -1 on failure.

    /// rename a file
    static  int     rename(const std::string& filename, const std::string& newname, int suppress_warning = 0);
    ///< it will silently overwrite newname if it exists without returning an error
    ///  Posix guarantees that if newname already exists, then there will be no moment
    ///  in which for other processes newname does not exist. There is no such guarantee
    ///  under Windows at this time. It may do it in the same way but the used Windows API
    ///  does not make such guarantees.
    ///  @returns 0 on success and -1 on failure.

    /// copy the contents of the file from 'from' to 'to' filename
    static  bool    copy(const std::string& from, const std::string& to);
    ///< @returns true on success and false on failure.

    /// retrieve the content of a file into a string
    static std::string getContents(const std::string& filename);
    static std::string getContents(const std::string& filename, std::error_code& ec);
    ///< @returns the content of the file or an empty string on failure

    /// read nBytes from the file into the buffer, starting at offset in the file
    static S64      read(const std::string& filename, void* buf, S64 offset, S64 nbytes);
    static S64      read(const std::string& filename, void* buf, S64 offset, S64 nbytes, std::error_code& ec);
    ///< @returns bytes read on success, or -1 on failure

    /// write nBytes from the buffer into the file, starting at offset in the file
    static S64      write(const std::string& filename, const void* buf, S64 offset, S64 nbytes);
    static S64      write(const std::string& filename, const void* buf, S64 offset, S64 nbytes, std::error_code& ec);
    ///< A negative offset will append the data to the end of the file
    ///  @returns bytes written on success, or -1 on failure

    /// return the file stat structure for filename
    static  int     stat(const std::string& filename, llstat* file_status, const char *fname = nullptr, int suppress_warning = ENOENT);
    ///< for compatibility with existing uses of LL_File::stat() we use ENOENT as default in the
    ///  optional 'suppress_warning' parameter to avoid spamming the log with warnings when the API
    ///  is used to detect if a file exists
    ///  @returns 0 on success and -1 on failure.

    /// get the creation data and time of a file
    static std::time_t getCreationTime(const std::string& filename, int suppress_warning = 0);
    ///< @returns the creation time of the file or 0 on error

    /// get the last modification data and time of a file
    static std::time_t getModificationTime(const std::string& filename, int suppress_warning = 0);
    ///< @returns the modification time of the file or 0 on error

    /// get the file or directory attributes for filename
    static std::filesystem::file_status getStatus(const std::string& filename, bool dontFollowSymLink = false, int suppress_warning  = ENOENT);
    ///< dontFollowSymLinks set to true returns the attributes of the symlink if it is one, rather than resolving it
    ///  we pass by default ENOENT in the optional 'suppress_warning' parameter to not spam the log with
    ///  warnings when the file or directory does not exist
    ///  @returns a std::filesystem::file_status value that can be passed to the according std::filesystem::exists()
    ///  and other APIs accepting a file_status.

    /// get the size of a file in bytes
    static  S64     size(const std::string& filename, int suppress_warning = ENOENT);
    ///< we pass by default ENOENT in the optional 'suppress_warning' parameter to not spam
    ///  the log with warnings when the file does not exist
    ///  @returns the file size on success or -1 on failure.

    /// check if filename is an existing file or directory
    static bool     exists(const std::string& filename);
    ///< @returns true if the path is for an existing file or directory

    /// check if filename is an existing directory
    static bool     isdir(const std::string& filename);
    ///< @returns true if the path is for an existing directory

    /// check if filename is an existing file
    static bool     isfile(const std::string& filename);
    ///< @returns true if the path is for an existing file

    /// check if filename is a symlink
    static bool     islink(const std::string& filename);
    ///< @returns true if the path is pointing at a symlink

    /// return a path to the temporary directory on the system
    static const std::string& tmpdir();

    /// converts a string containing a path in utf8 encoding into an explicit filesystem path
    static std::filesystem::path utf8StringToPath(const std::string& pathname);
    ///< @returns the path as a std::filesystem::path

private:
#if LL_WINDOWS
    typedef HANDLE        llfile_handle_t;
    const llfile_handle_t InvalidHandle = INVALID_HANDLE_VALUE;
#else
    typedef int           llfile_handle_t;
    const llfile_handle_t InvalidHandle = -1;
#endif

    //----------------------------------------------------------------------------------------
    // static member functions
    //----------------------------------------------------------------------------------------
#if LL_WINDOWS
    /// convert a string containing a path in utf8 encoding into a Windows format std::wstring
    static std::wstring utf8StringToWstring(const std::string& pathname);
    ///< this will prepend the path with the Windows kernel object space prefix when the path is
    ///  equal or longer than MAX_PATH characters and do some sanitation on the path.
    ///  This allows the underlaying Windows APIs to process long path names. Do not pass such a path
    ///  to std::filesystem functions. These functions are not guaranteed to handle such paths properly.
    ///  It's only useful to pass the resulting string buffer to Microsoft Windows widechar APIs or
    ///  the Microsoft C runtime widechar file functions.
    ///
    ///  Example:
    ///
    ///  std::wstring file_path = utf8StringToWstring(filename);
    ///  HANDLE CreateFileW(file_path.c_str(), ......);
    ///
    ///  @returns the path as a std::wstring path
#endif
    llfile_handle_t mHandle;
    std::ios_base::openmode mOpen; // Used to emulate std::ios_base::app under Windows
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
class LL_COMMON_API llifstream : public std::ifstream
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
    llifstream();

    /**
     *  @brief  Create an input file stream.
     *  @param  Filename  String specifying the filename.
     *  @param  Mode  Open file in specified mode (see std::ios_base).
     *
     *  @c ios_base::in is automatically included in @a mode.
     */
    explicit llifstream(const std::string& _Filename,
                        ios_base::openmode _Mode = ios_base::in);

    /**
     *  @brief  Opens an external file.
     *  @param  Filename  The name of the file.
     *  @param  Node  The open mode flags.
     *
     *  Calls @c llstdio_filebuf::open(s,mode|in).  If that function
     *  fails, @c failbit is set in the stream's error state.
     */
    void open(const std::string& _Filename,
              ios_base::openmode _Mode = ios_base::in);
};


/**
 *  @brief  Controlling output for files.
 *
 *  This class supports writing to named files, using the inherited functions
 *  from std::ofstream. The only added value is that our constructor Does The
 *  Right Thing when passed a non-ASCII pathname. Sadly, that isn't true of
 *  Microsoft's std::ofstream.
*/
class LL_COMMON_API llofstream : public std::ofstream
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
    llofstream();

    /**
     *  @brief  Create an output file stream.
     *  @param  Filename  String specifying the filename.
     *  @param  Mode  Open file in specified mode (see std::ios_base).
     *
     *  @c ios_base::out is automatically included in @a mode.
     */
    explicit llofstream(const std::string& _Filename,
                        ios_base::openmode _Mode = ios_base::out|ios_base::trunc);

    /**
     *  @brief  Opens an external file.
     *  @param  Filename  The name of the file.
     *  @param  Node  The open mode flags.
     *
     *  @c ios_base::out is automatically included in @a mode.
     */
    void open(const std::string& _Filename,
              ios_base::openmode _Mode = ios_base::out|ios_base::trunc);
};


/**
 * @brief filesize helpers.
 *
 * The file size helpers are not considered particularly efficient,
 * and should only be used for config files and the like -- not in a
 * loop.
 */
std::streamsize LL_COMMON_API llifstream_size(llifstream& fstr);
std::streamsize LL_COMMON_API llofstream_size(llofstream& fstr);

#else // ! LL_WINDOWS

// on non-windows, llifstream and llofstream are just mapped directly to the std:: equivalents
typedef std::ifstream llifstream;
typedef std::ofstream llofstream;

#endif // LL_WINDOWS or ! LL_WINDOWS

#endif // not LL_LLFILE_H
