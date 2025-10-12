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

typedef FILE    LLFILE;

#include <fstream>
#include <sys/stat.h>

#if LL_WINDOWS
// windows version of stat function and stat data structure are called _statxx
typedef struct _stat64    llstat;
#else
typedef struct stat     llstat;
#include <sys/types.h>
#endif

#ifndef S_ISREG
# define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISDIR
# define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif

#include "llstring.h" // safe char* -> std::string conversion

/// LLFile is a class of static functions operating on paths
/// All the functions with a path string input take UTF8 path/filenames
class LL_COMMON_API LLFile
{
public:
    /// open a file with the specified access mode
    static  LLFILE* fopen(const std::string& filename, const char* accessmode);  /* Flawfinder: ignore */
    ///< 'accessmode' follows the rules of the Posix fopen() mode parameter
    /// "r" open the file for reading only and positions the stream at the beginning
    /// "r+" open the file for reading and writing and positions the stream at the beginning
    /// "w" open the file for reading and writing and truncate it to zero length
    /// "w+" open or create the file for reading and writing and truncate to zero length if it existed
    /// "a" open the file for reading and writing and position the stream at the end of the file
    /// "a+" open or create the file for reading and writing and position the stream at the end of the file
    ///
    /// in addition to these values, "b" can be appended to indicate binary stream access, but on Linux and Mac
    /// this is strictly for compatibility and has no effect. On Windows this makes the file functions not
    /// try to translate line endings. Windows also allows to append "t" to indicate text mode. If neither
    /// "b" or "t" is defined, Windows uses the value set by _fmode which by default is _O_TEXT.
    /// This means that it is always a good idea to append "b" specifically for binary file access to
    /// avoid corruption of the binary consistency of the data stream when reading or writing
    /// Other characters in 'accessmode' will usually cause an error as fopen will verify this parameter
    ///< @returns a valid LLFILE* pointer on success or NULL on failure

    static  int     close(LLFILE * file);

    /// retrieve the content of a file into a string
    static std::string getContents(const std::string& filename);
    ///< @returns the content of the file or an empty string on failure

    /// create a directory
    static  int     mkdir(const std::string& filename, int perms = 0700);
    ///< perms is a permissions mask like 0777 or 0700.  In most cases it will be
    ///  overridden by the user's umask. It is ignored on Windows.
    ///  mkdir() considers "directory already exists" to be not an error.
    ///  @returns 0 on success and -1 on failure.

    //// remove a directory
    static  int     rmdir(const std::string& filename, int supress_error = 0);
    ///< pass ENOENT in the optional 'supress_error' parameter
    ///  if you don't want a warning in the log when the directory does not exist
    ///  @returns 0 on success and -1 on failure.

    /// remove a file or directory
    static  int     remove(const std::string& filename, int supress_error = 0);
    ///< pass ENOENT in the optional 'supress_error' parameter
    ///  if you don't want a warning in the log when the directory does not exist
    ///  @returns 0 on success and -1 on failure.

    /// rename a file, 
    static  int     rename(const std::string& filename, const std::string& newname, int supress_error = 0);
    ///< it will silently overwrite newname if it exists without returning an error
    ///  @returns 0 on success and -1 on failure.


    // copy the contents of file from 'from' to 'to' filename
    static  bool    copy(const std::string& from, const std::string& to);
    ///< @returns true on success and false on failure.

    /// return the file stat structure for filename
    static  int     stat(const std::string& filename, llstat* file_status, int supress_error = ENOENT);
    ///< for compatibility with existing uses of LL_File::stat() we use ENOENT as default in the
    ///  optional 'supress_error' parameter to avoid spamming the log with warnings when the API
    ///  is used to detect if a file exists
    ///  @returns 0 on success and -1 on failure.

    /// get the file or directory attributes for filename
    static  int     getattr(const std::string& filename, unsigned short& st_mode, int supress_error = 0);
    ///< a more lightweight function on Windows to stat, that just returns the file attribute flags
    ///  pass ENOENT in the optional 'supress_error' parameter if you don't want a warning
    ///  in the log when the file or directory does not exist
    ///  @returns 0 on success and -1 on failure.

    /// check if filename is an existing directory
    static  bool    isdir(const std::string& filename);
    ///< @returns true if the path is for an existing directory
 
    /// check if filename is an existing file
    static  bool    isfile(const std::string& filename);
    ///< @returns true if the path is for an existing file

    /// return a path to the temporary directory on the system
    static  const char * tmpdir();
};

/// RAII class
class LLUniqueFile
{
public:
    // empty
    LLUniqueFile(): mFileHandle(nullptr) {}
    // wrap (e.g.) result of LLFile::fopen()
    LLUniqueFile(LLFILE* f): mFileHandle(f) {}
    // no copy
    LLUniqueFile(const LLUniqueFile&) = delete;
    // move construction
    LLUniqueFile(LLUniqueFile&& other) noexcept
    {
        mFileHandle = other.mFileHandle;
        other.mFileHandle = nullptr;
    }
    // The point of LLUniqueFile is to close on destruction.
    ~LLUniqueFile()
    {
        close();
    }

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
            LLFILE* h{nullptr};
            std::swap(h, mFileHandle);
            LLFile::close(h);
        }
    }

    // detect whether the wrapped LLFILE is open or not
    explicit operator bool() const { return bool(mFileHandle); }
    bool operator!() { return ! mFileHandle; }

    // LLUniqueFile should be usable for any operation that accepts LLFILE*
    // (or FILE* for that matter)
    operator LLFILE*() const { return mFileHandle; }

private:
    LLFILE* mFileHandle;
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
