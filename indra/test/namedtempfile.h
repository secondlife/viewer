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

#include "fsyspath.h"
#include "llerror.h"
#include "llstring.h"
#include "stringize.h"
#include <string>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string_view>
#include <random>

/**
 * Create a text file with specified content "somewhere in the
 * filesystem," cleaning up when it goes out of scope.
 */
class NamedTempFile
{
    LOG_CLASS(NamedTempFile);
public:
    NamedTempFile(const NamedTempFile&) = delete;
    NamedTempFile& operator=(const NamedTempFile&) = delete;

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
        std::filesystem::remove(mPath);
    }

    const std::filesystem::path& getPath() const { return mPath; }

    template <typename CALLABLE>
    void peep_via(CALLABLE&& callable) const
    {
        std::forward<CALLABLE>(callable)(stringize("File '", mPath, "' contains:"));
        std::ifstream reader(mPath, std::ios::binary);
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

    static std::filesystem::path temp_path(const std::string_view& pfx="",
                                             const std::string_view& sfx="")
    {
        // This variable is set by GitHub actions and is the recommended place
        // to put temp files belonging to an actions job.
        const char* RUNNER_TEMP = getenv("RUNNER_TEMP");
        std::filesystem::path tempdir{
            // if RUNNER_TEMP is set and not empty
            (RUNNER_TEMP && *RUNNER_TEMP)?
            fsyspath::path(RUNNER_TEMP) : // use RUNNER_TEMP if available
            std::filesystem::temp_directory_path()}; // else canonical temp dir

        static std::mt19937 random_generator{std::random_device{}()};
        static std::uniform_int_distribution<> distribution{0, std::numeric_limits<uint8_t>::max()};
        std::string tempname{};
        static constexpr auto num_bits = 128;
        for (auto i = 0; i < (num_bits / std::numeric_limits<uint8_t>::digits); ++i) {
            tempname += llformat("%02x", distribution(random_generator));
        }
        tempname = std::string(pfx) + tempname + std::string(sfx);
        return tempdir / tempname;
    }

protected:
    void createFile(const std::string_view& pfx,
                    const Streamer& func,
                    const std::string_view& sfx)
    {
        // Create file in a temporary place.
        mPath = temp_path(pfx, sfx);
        std::ofstream out{ mPath, std::ios::binary };
        // Write desired content.
        func(out);
    }

    std::filesystem::path mPath;
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
