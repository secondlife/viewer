/**
 * @file   namedtempfile.h
 * @author Nat Goodspeed
 * @date   2012-01-13
 * @brief  NamedTempFile class for tests that need disk files as fixtures.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_NAMEDTEMPFILE_H)
#define LL_NAMEDTEMPFILE_H

#include "llerror.h"
#include "llstring.h"
#include "stringize.h"
#include <string>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/noncopyable.hpp>
#include <functional>
#include <iostream>
#include <sstream>
#include <string_view>

/**
 * Create a text file with specified content "somewhere in the
 * filesystem," cleaning up when it goes out of scope.
 */
class NamedTempFile: public boost::noncopyable
{
    LOG_CLASS(NamedTempFile);
public:
    NamedTempFile(const std::string_view& pfx,
                  const std::string_view& content,
                  const std::string_view& sfx=std::string_view(""))
    {
        createFile(pfx, [&content](std::ostream& out){ out << content; }, sfx);
    }

    // Disambiguate when passing string literal -- unclear why a string
    // literal should be ambiguous wrt std::string_view and Streamer
    NamedTempFile(const std::string_view& pfx,
                  const char* content,
                  const std::string_view& sfx=std::string_view(""))
    {
        createFile(pfx, [&content](std::ostream& out){ out << content; }, sfx);
    }

    // Function that accepts an ostream ref and (presumably) writes stuff to
    // it, e.g.:
    // (boost::phoenix::placeholders::arg1 << "the value is " << 17 << '\n')
    typedef std::function<void(std::ostream&)> Streamer;

    NamedTempFile(const std::string_view& pfx,
                  const Streamer& func,
                  const std::string_view& sfx=std::string_view(""))
    {
        createFile(pfx, func, sfx);
    }

    virtual ~NamedTempFile()
    {
        boost::filesystem::remove(mPath);
    }

    std::string getName() const { return mPath.string(); }

    template <typename CALLABLE>
    void peep_via(CALLABLE&& callable) const
    {
        std::forward<CALLABLE>(callable)(stringize("File '", mPath, "' contains:"));
        boost::filesystem::ifstream reader(mPath, std::ios::binary);
        std::string line;
        while (std::getline(reader, line))
            std::forward<CALLABLE>(callable)(line);
        std::forward<CALLABLE>(callable)("---");
    }

    void peep_log() const
    {
        peep_via([](const std::string& line){ LL_DEBUGS() << line << LL_ENDL; });
    }

    void peep(std::ostream& out=std::cout) const
    {
        peep_via([&out](const std::string& line){ out << line << '\n'; });
    }

    friend std::ostream& operator<<(std::ostream& out, const NamedTempFile& self)
    {
        self.peep(out);
        return out;
    }

    static boost::filesystem::path temp_path(const std::string_view& pfx="",
                                             const std::string_view& sfx="")
    {
        // This variable is set by GitHub actions and is the recommended place
        // to put temp files belonging to an actions job.
        const char* RUNNER_TEMP = getenv("RUNNER_TEMP");
        boost::filesystem::path tempdir{
            // if RUNNER_TEMP is set and not empty
            (RUNNER_TEMP && *RUNNER_TEMP)?
            boost::filesystem::path(RUNNER_TEMP) : // use RUNNER_TEMP if available
            boost::filesystem::temp_directory_path()}; // else canonical temp dir
        boost::filesystem::path tempname{
            // use filename template recommended by unique_path() doc, but
            // with underscores instead of hyphens: some use cases involve
            // temporary Python scripts
            tempdir / stringize(pfx, "%%%%_%%%%_%%%%_%%%%", sfx) };
        return boost::filesystem::unique_path(tempname);
    }

protected:
    void createFile(const std::string_view& pfx,
                    const Streamer& func,
                    const std::string_view& sfx)
    {
        // Create file in a temporary place.
        mPath = temp_path(pfx, sfx);
        boost::filesystem::ofstream out{ mPath, std::ios::binary };
        // Write desired content.
        func(out);
    }

    boost::filesystem::path mPath;
};

/**
 * Create a NamedTempFile with a specified filename extension. This is useful
 * when, for instance, you must be able to use the file in a Python import
 * statement.
 */
class NamedExtTempFile: public NamedTempFile
{
    LOG_CLASS(NamedExtTempFile);
public:
    NamedExtTempFile(const std::string& ext, const std::string_view& content):
        NamedTempFile(remove_dot(ext), content, ensure_dot(ext))
    {}

    // Disambiguate when passing string literal
    NamedExtTempFile(const std::string& ext, const char* content):
        NamedTempFile(remove_dot(ext), content, ensure_dot(ext))
    {}

    NamedExtTempFile(const std::string& ext, const Streamer& func):
        NamedTempFile(remove_dot(ext), func, ensure_dot(ext))
    {}

    static std::string ensure_dot(const std::string& ext)
    {
        if (ext.empty())
        {
            // What SHOULD we do when the caller makes a point of using
            // NamedExtTempFile to generate a file with a particular
            // extension, then passes an empty extension? Use just "."? That
            // sounds like a Bad Idea, especially on Windows. Treat that as a
            // coding error.
            LL_ERRS("NamedExtTempFile") << "passed empty extension" << LL_ENDL;
        }
        if (ext[0] == '.')
        {
            return ext;
        }
        return "." + ext;
    }

    static std::string remove_dot(const std::string& ext)
    {
        std::string::size_type found = ext.find_first_not_of(".");
        if (found == std::string::npos)
        {
            return ext;
        }
        return ext.substr(found);
    }
};

#endif /* ! defined(LL_NAMEDTEMPFILE_H) */
