/**
 * @file lldir_test.cpp
 * @date 2008-05
 * @brief LLDir test cases.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llstring.h"
#include "tests/StringVec.h"
#include "../lldir.h"
#include "../lldiriterator.h"

#include "../test/lldoctest.h"
#include "stringize.h"
#include <boost/assign/list_of.hpp>

using boost::assign::list_of;

// We use ensure_equals(..., vec(list_of(...))) not because it's functionally
// required, but because ensure_equals() knows how to format a StringVec.
// Turns out that when ensure_equals() displays a test failure with just
// list_of("string")("another"), you see 'stringanother' vs. '("string",
// "another")'.
StringVec vec(const StringVec& v)
{
    return v;
}

// For some tests, use a dummy LLDir that uses memory data instead of touching
// the filesystem
struct LLDir_Dummy: public LLDir
{
    /*----------------------------- LLDir API ------------------------------*/
    LLDir_Dummy()
    {
        // Initialize important LLDir data members based on the filesystem
        // data below.
        mDirDelimiter = "/";
        mExecutableDir = "install";
        mExecutableFilename = "test";
        mExecutablePathAndName = add(mExecutableDir, mExecutableFilename);
        mWorkingDir = mExecutableDir;
        mAppRODataDir = "install";
        mSkinBaseDir = add(mAppRODataDir, "skins");
        mOSUserDir = "user";
        mOSUserAppDir = mOSUserDir;
        mLindenUserDir = "";

        // Make the dummy filesystem look more or less like what we expect in
        // the real one.
        static const char* preload[] =
        {
            // We group these fixture-data pathnames by basename, rather than
            // sorting by full path as you might expect, because the outcome
            // of each test strongly depends on which skins/languages provide
            // a given basename.
            "install/skins/default/colors.xml",
            "install/skins/steam/colors.xml",
            "user/skins/default/colors.xml",
            "user/skins/steam/colors.xml",

            "install/skins/default/xui/en/strings.xml",
            "install/skins/default/xui/fr/strings.xml",
            "install/skins/steam/xui/en/strings.xml",
            "install/skins/steam/xui/fr/strings.xml",
            "user/skins/default/xui/en/strings.xml",
            "user/skins/default/xui/fr/strings.xml",
            "user/skins/steam/xui/en/strings.xml",
            "user/skins/steam/xui/fr/strings.xml",

            "install/skins/default/xui/en/floater.xml",
            "install/skins/default/xui/fr/floater.xml",
            "user/skins/default/xui/fr/floater.xml",

            "install/skins/default/xui/en/newfile.xml",
            "install/skins/default/xui/fr/newfile.xml",
            "user/skins/default/xui/en/newfile.xml",

            "install/skins/default/html/en-us/welcome.html",
            "install/skins/default/html/fr/welcome.html",

            "install/skins/default/textures/only_default.jpeg",
            "install/skins/steam/textures/only_steam.jpeg",
            "user/skins/default/textures/only_user_default.jpeg",
            "user/skins/steam/textures/only_user_steam.jpeg",

            "install/skins/default/future/somefile.txt"
        };
        for (const char* path : preload)
        {
            buildFilesystem(path);
        }
    }

    virtual ~LLDir_Dummy() {}

    virtual void initAppDirs(const std::string& app_name, const std::string& app_read_only_data_dir)
    {
        // Implement this when we write a test that needs it
    }

    virtual std::string getCurPath()
    {
        // Implement this when we write a test that needs it
        return "";
    }

    virtual U32 countFilesInDir(const std::string& dirname, const std::string& mask)
    {
        // Implement this when we write a test that needs it
        return 0;
    }

    virtual bool fileExists(const std::string& pathname) const
    {
        // Record fileExists() calls so we can check whether caching is
        // working right. Certain LLDir calls should be able to make decisions
        // without calling fileExists() again, having already checked existence.
        mChecked.insert(pathname);
        // For our simple flat set of strings, see whether the identical
        // pathname exists in our set.
        return (mFilesystem.find(pathname) != mFilesystem.end());
    }

    virtual std::string getLLPluginLauncher()
    {
        // Implement this when we write a test that needs it
        return "";
    }

    virtual std::string getLLPluginFilename(std::string base_name)
    {
        // Implement this when we write a test that needs it
        return "";
    }

    /*----------------------------- Dummy data -----------------------------*/
    void clearFilesystem() { mFilesystem.clear(); }
    void buildFilesystem(const std::string& path)
    {
        // Split the pathname on slashes, ignoring leading, trailing, doubles
        StringVec components;
        LLStringUtil::getTokens(path, components, "/");
        // Ensure we have an entry representing every level of this path
        std::string partial;
        for (std::string component : components)
        {
            append(partial, component);
            mFilesystem.insert(partial);
        }
    }

    void clear_checked() { mChecked.clear(); }
    void ensure_checked(const std::string& pathname) const
    {
        tut::ensure(STRINGIZE(pathname << " was not checked but should have been"),
                    mChecked.find(pathname) != mChecked.end());
    }
    void ensure_not_checked(const std::string& pathname) const
    {
        tut::ensure(STRINGIZE(pathname << " was checked but should not have been"),
                    mChecked.find(pathname) == mChecked.end());
    }

    std::set<std::string> mFilesystem;
    mutable std::set<std::string> mChecked;
};

TEST_SUITE("LLDir") {

TEST_CASE("test_6")
{

        set_test_name("findSkinnedFilenames()");
        LLDir_Dummy lldir;
        /*------------------------ "default", "en" -------------------------*/
        // Setting "default" means we shouldn't consider any "*/skins/steam"
        // directories; setting "en" means we shouldn't consider any "xui/fr"
        // directories.
        lldir.setSkinFolder("default", "en");
        ensure_equals(lldir.getSkinFolder(), "default");
        ensure_equals(lldir.getLanguage(), "en");

        // top-level directory of a skin isn't localized
        ensure_equals(lldir.findSkinnedFilenames(LLDir::SKINBASE, "colors.xml", LLDir::ALL_SKINS),
                      vec(list_of("install/skins/default/colors.xml")
                                 ("user/skins/default/colors.xml")));
        // We should not have needed to check for skins/default/en. We should
        // just "know" that SKINBASE is not localized.
        lldir.ensure_not_checked("install/skins/default/en");

        ensure_equals(lldir.findSkinnedFilenames(LLDir::TEXTURES, "only_default.jpeg"),
                      vec(list_of("install/skins/default/textures/only_default.jpeg")));
        // Nor should we have needed to check skins/default/textures/en
        // because textures is known not to be localized.
        lldir.ensure_not_checked("install/skins/default/textures/en");

        StringVec expected(vec(list_of("install/skins/default/xui/en/strings.xml")
                               ("user/skins/default/xui/en/strings.xml")));
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "strings.xml", LLDir::ALL_SKINS),
                      expected);
        // The first time, we had to probe to find out whether xui was localized.
        lldir.ensure_checked("install/skins/default/xui/en");
        lldir.clear_checked();
        // Now make the same call again -- should return same result --
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "strings.xml", LLDir::ALL_SKINS),
                      expected);
        // but this time it should remember that xui is localized.
        lldir.ensure_not_checked("install/skins/default/xui/en");

        // localized subdir with "en-us" instead of "en"
        ensure_equals(lldir.findSkinnedFilenames("html", "welcome.html"),
                      vec(list_of("install/skins/default/html/en-us/welcome.html")));
        lldir.ensure_checked("install/skins/default/html/en");
        lldir.ensure_checked("install/skins/default/html/en-us");
        lldir.clear_checked();
        ensure_equals(lldir.findSkinnedFilenames("html", "welcome.html"),
                      vec(list_of("install/skins/default/html/en-us/welcome.html")));
        lldir.ensure_not_checked("install/skins/default/html/en");
        lldir.ensure_not_checked("install/skins/default/html/en-us");

        ensure_equals(lldir.findSkinnedFilenames("future", "somefile.txt"),
                      vec(list_of("install/skins/default/future/somefile.txt")));
        // Test probing for an unrecognized unlocalized future subdir.
        lldir.ensure_checked("install/skins/default/future/en");
        lldir.clear_checked();
        ensure_equals(lldir.findSkinnedFilenames("future", "somefile.txt"),
                      vec(list_of("install/skins/default/future/somefile.txt")));
        // Second time it should remember that future is unlocalized.
        lldir.ensure_not_checked("install/skins/default/future/en");

        // When language is set to "en", requesting an html file pulls up the
        // "en-us" version -- not because it magically matches those strings,
        // but because there's no "en" localization and it falls back on the
        // default "en-us"! Note that it would probably still be better to
        // make the default localization be "en" and allow "en-gb" (or
        // whatever) localizations, which would work much more the way you'd
        // expect.
        ensure_equals(lldir.findSkinnedFilenames("html", "welcome.html"),
                      vec(list_of("install/skins/default/html/en-us/welcome.html")));

        /*------------------------ "default", "fr" -------------------------*/
        // We start being able to distinguish localized subdirs from
        // unlocalized when we ask for a non-English language.
        lldir.setSkinFolder("default", "fr");
        ensure_equals(lldir.getLanguage(), "fr");

        // pass merge=true to request this filename in all relevant skins
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "strings.xml", LLDir::ALL_SKINS),
                      vec(list_of
                          ("install/skins/default/xui/en/strings.xml")
                          ("install/skins/default/xui/fr/strings.xml")
                          ("user/skins/default/xui/en/strings.xml")
                          ("user/skins/default/xui/fr/strings.xml")));

        // pass (or default) merge=false to request only most specific skin
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "strings.xml"),
                      vec(list_of
                          ("user/skins/default/xui/en/strings.xml")
                          ("user/skins/default/xui/fr/strings.xml")));

        // Our dummy floater.xml has a user localization (for "fr") but no
        // English override. This is a case in which CURRENT_SKIN nonetheless
        // returns paths from two different skins.
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "floater.xml"),
                      vec(list_of
                          ("install/skins/default/xui/en/floater.xml")
                          ("user/skins/default/xui/fr/floater.xml")));

        // Our dummy newfile.xml has an English override but no user
        // localization. This is another case in which CURRENT_SKIN
        // nonetheless returns paths from two different skins.
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "newfile.xml"),
                      vec(list_of
                          ("user/skins/default/xui/en/newfile.xml")
                          ("install/skins/default/xui/fr/newfile.xml")));

        ensure_equals(lldir.findSkinnedFilenames("html", "welcome.html"),
                      vec(list_of
                          ("install/skins/default/html/en-us/welcome.html")
                          ("install/skins/default/html/fr/welcome.html")));

        /*------------------------ "default", "zh" -------------------------*/
        lldir.setSkinFolder("default", "zh");
        // Because strings.xml has only a "fr" override but no "zh" override
        // in any skin, the most localized version we can find is "en".
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "strings.xml"),
                      vec(list_of("user/skins/default/xui/en/strings.xml")));

        /*------------------------- "steam", "en" --------------------------*/
        lldir.setSkinFolder("steam", "en");

        ensure_equals(lldir.findSkinnedFilenames(LLDir::SKINBASE, "colors.xml", LLDir::ALL_SKINS),
                      vec(list_of
                          ("install/skins/default/colors.xml")
                          ("install/skins/steam/colors.xml")
                          ("user/skins/default/colors.xml")
                          ("user/skins/steam/colors.xml")));

        ensure_equals(lldir.findSkinnedFilenames(LLDir::TEXTURES, "only_default.jpeg"),
                      vec(list_of("install/skins/default/textures/only_default.jpeg")));

        ensure_equals(lldir.findSkinnedFilenames(LLDir::TEXTURES, "only_steam.jpeg"),
                      vec(list_of("install/skins/steam/textures/only_steam.jpeg")));

        ensure_equals(lldir.findSkinnedFilenames(LLDir::TEXTURES, "only_user_default.jpeg"),
                      vec(list_of("user/skins/default/textures/only_user_default.jpeg")));

        ensure_equals(lldir.findSkinnedFilenames(LLDir::TEXTURES, "only_user_steam.jpeg"),
                      vec(list_of("user/skins/steam/textures/only_user_steam.jpeg")));

        // CURRENT_SKIN
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "strings.xml"),
                      vec(list_of("user/skins/steam/xui/en/strings.xml")));

        // pass constraint=ALL_SKINS to request this filename in all relevant skins
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "strings.xml", LLDir::ALL_SKINS),
                      vec(list_of
                          ("install/skins/default/xui/en/strings.xml")
                          ("install/skins/steam/xui/en/strings.xml")
                          ("user/skins/default/xui/en/strings.xml")
                          ("user/skins/steam/xui/en/strings.xml")));

        /*------------------------- "steam", "fr" --------------------------*/
        lldir.setSkinFolder("steam", "fr");

        // pass CURRENT_SKIN to request only the most specialized files
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "strings.xml"),
                      vec(list_of
                          ("user/skins/steam/xui/en/strings.xml")
                          ("user/skins/steam/xui/fr/strings.xml")));

        // pass ALL_SKINS to request this filename in all relevant skins
        ensure_equals(lldir.findSkinnedFilenames(LLDir::XUI, "strings.xml", LLDir::ALL_SKINS),
                      vec(list_of
                          ("install/skins/default/xui/en/strings.xml")
                          ("install/skins/default/xui/fr/strings.xml")
                          ("install/skins/steam/xui/en/strings.xml")
                          ("install/skins/steam/xui/fr/strings.xml")
                          ("user/skins/default/xui/en/strings.xml")
                          ("user/skins/default/xui/fr/strings.xml")
                          ("user/skins/steam/xui/en/strings.xml")
                          ("user/skins/steam/xui/fr/strings.xml")));
    
}

TEST_CASE("test_7")
{

        set_test_name("add()");
        LLDir_Dummy lldir;
        CHECK_MESSAGE(lldir.add("" == "", "both empty"), "");
        CHECK_MESSAGE(lldir.add("" == "b", "path empty"), "b");
        CHECK_MESSAGE(lldir.add("a" == "", "name empty"), "a");
        CHECK_MESSAGE(lldir.add("a" == "b", "both simple"), "a/b");
        CHECK_MESSAGE(lldir.add("a" == "/b", "name leading slash"), "a/b");
        CHECK_MESSAGE(lldir.add("a/" == "b", "path trailing slash"), "a/b");
        CHECK_MESSAGE(lldir.add("a/" == "/b", "both bring slashes"), "a/b");
    
}

} // TEST_SUITE

