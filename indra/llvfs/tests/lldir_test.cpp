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

#include "../lldir.h"

#include "../test/lltut.h"


namespace tut
{
	struct LLDirTest
        {
        };
        typedef test_group<LLDirTest> LLDirTest_t;
        typedef LLDirTest_t::object LLDirTest_object_t;
        tut::LLDirTest_t tut_LLDirTest("LLDir");

	template<> template<>
	void LLDirTest_object_t::test<1>()
		// getDirDelimiter
	{
		ensure("getDirDelimiter", !gDirUtilp->getDirDelimiter().empty());
	}

	template<> template<>
	void LLDirTest_object_t::test<2>()
		// getBaseFileName
	{
		std::string delim = gDirUtilp->getDirDelimiter();
		std::string rawFile = "foo";
		std::string rawFileExt = "foo.bAr";
		std::string rawFileNullExt = "foo.";
		std::string rawExt = ".bAr";
		std::string rawDot = ".";
		std::string pathNoExt = "aa" + delim + "bb" + delim + "cc" + delim + "dd" + delim + "ee";
		std::string pathExt = pathNoExt + ".eXt";
		std::string dottedPathNoExt = "aa" + delim + "bb" + delim + "cc.dd" + delim + "ee";
		std::string dottedPathExt = dottedPathNoExt + ".eXt";

		// foo[.bAr]

		ensure_equals("getBaseFileName/r-no-ext/no-strip-exten",
			      gDirUtilp->getBaseFileName(rawFile, false),
			      "foo");

		ensure_equals("getBaseFileName/r-no-ext/strip-exten",
			      gDirUtilp->getBaseFileName(rawFile, true),
			      "foo");

		ensure_equals("getBaseFileName/r-ext/no-strip-exten",
			      gDirUtilp->getBaseFileName(rawFileExt, false),
			      "foo.bAr");

		ensure_equals("getBaseFileName/r-ext/strip-exten",
			      gDirUtilp->getBaseFileName(rawFileExt, true),
			      "foo");

		// foo.

		ensure_equals("getBaseFileName/rn-no-ext/no-strip-exten",
			      gDirUtilp->getBaseFileName(rawFileNullExt, false),
			      "foo.");

		ensure_equals("getBaseFileName/rn-no-ext/strip-exten",
			      gDirUtilp->getBaseFileName(rawFileNullExt, true),
			      "foo");

		// .bAr
		// interesting case - with no basename, this IS the basename, not the extension.

		ensure_equals("getBaseFileName/e-ext/no-strip-exten",
			      gDirUtilp->getBaseFileName(rawExt, false),
			      ".bAr");

		ensure_equals("getBaseFileName/e-ext/strip-exten",
			      gDirUtilp->getBaseFileName(rawExt, true),
			      ".bAr");

		// .

		ensure_equals("getBaseFileName/d/no-strip-exten",
			      gDirUtilp->getBaseFileName(rawDot, false),
			      ".");

		ensure_equals("getBaseFileName/d/strip-exten",
			      gDirUtilp->getBaseFileName(rawDot, true),
			      ".");

		// aa/bb/cc/dd/ee[.eXt]

		ensure_equals("getBaseFileName/no-ext/no-strip-exten",
			      gDirUtilp->getBaseFileName(pathNoExt, false),
			      "ee");

		ensure_equals("getBaseFileName/no-ext/strip-exten",
			      gDirUtilp->getBaseFileName(pathNoExt, true),
			      "ee");

		ensure_equals("getBaseFileName/ext/no-strip-exten",
			      gDirUtilp->getBaseFileName(pathExt, false),
			      "ee.eXt");

		ensure_equals("getBaseFileName/ext/strip-exten",
			      gDirUtilp->getBaseFileName(pathExt, true),
			      "ee");

		// aa/bb/cc.dd/ee[.eXt]

		ensure_equals("getBaseFileName/d-no-ext/no-strip-exten",
			      gDirUtilp->getBaseFileName(dottedPathNoExt, false),
			      "ee");

		ensure_equals("getBaseFileName/d-no-ext/strip-exten",
			      gDirUtilp->getBaseFileName(dottedPathNoExt, true),
			      "ee");

		ensure_equals("getBaseFileName/d-ext/no-strip-exten",
			      gDirUtilp->getBaseFileName(dottedPathExt, false),
			      "ee.eXt");

		ensure_equals("getBaseFileName/d-ext/strip-exten",
			      gDirUtilp->getBaseFileName(dottedPathExt, true),
			      "ee");
	}

	template<> template<>
	void LLDirTest_object_t::test<3>()
		// getDirName
	{
		std::string delim = gDirUtilp->getDirDelimiter();
		std::string rawFile = "foo";
		std::string rawFileExt = "foo.bAr";
		std::string pathNoExt = "aa" + delim + "bb" + delim + "cc" + delim + "dd" + delim + "ee";
		std::string pathExt = pathNoExt + ".eXt";
		std::string dottedPathNoExt = "aa" + delim + "bb" + delim + "cc.dd" + delim + "ee";
		std::string dottedPathExt = dottedPathNoExt + ".eXt";

		// foo[.bAr]

		ensure_equals("getDirName/r-no-ext",
			      gDirUtilp->getDirName(rawFile),
			      "");

		ensure_equals("getDirName/r-ext",
			      gDirUtilp->getDirName(rawFileExt),
			      "");

		// aa/bb/cc/dd/ee[.eXt]

		ensure_equals("getDirName/no-ext",
			      gDirUtilp->getDirName(pathNoExt),
			      "aa" + delim + "bb" + delim + "cc" + delim + "dd");

		ensure_equals("getDirName/ext",
			      gDirUtilp->getDirName(pathExt),
			      "aa" + delim + "bb" + delim + "cc" + delim + "dd");

		// aa/bb/cc.dd/ee[.eXt]

		ensure_equals("getDirName/d-no-ext",
			      gDirUtilp->getDirName(dottedPathNoExt),
			      "aa" + delim + "bb" + delim + "cc.dd");

		ensure_equals("getDirName/d-ext",
			      gDirUtilp->getDirName(dottedPathExt),
			      "aa" + delim + "bb" + delim + "cc.dd");
	}

	template<> template<>
	void LLDirTest_object_t::test<4>()
		// getExtension
	{
		std::string delim = gDirUtilp->getDirDelimiter();
		std::string rawFile = "foo";
		std::string rawFileExt = "foo.bAr";
		std::string rawFileNullExt = "foo.";
		std::string rawExt = ".bAr";
		std::string rawDot = ".";
		std::string pathNoExt = "aa" + delim + "bb" + delim + "cc" + delim + "dd" + delim + "ee";
		std::string pathExt = pathNoExt + ".eXt";
		std::string dottedPathNoExt = "aa" + delim + "bb" + delim + "cc.dd" + delim + "ee";
		std::string dottedPathExt = dottedPathNoExt + ".eXt";

		// foo[.bAr]

		ensure_equals("getExtension/r-no-ext",
			      gDirUtilp->getExtension(rawFile),
			      "");

		ensure_equals("getExtension/r-ext",
			      gDirUtilp->getExtension(rawFileExt),
			      "bar");

		// foo.

		ensure_equals("getExtension/rn-no-ext",
			      gDirUtilp->getExtension(rawFileNullExt),
			      "");

		// .bAr
		// interesting case - with no basename, this IS the basename, not the extension.

		ensure_equals("getExtension/e-ext",
			      gDirUtilp->getExtension(rawExt),
			      "");

		// .

		ensure_equals("getExtension/d",
			      gDirUtilp->getExtension(rawDot),
			      "");

		// aa/bb/cc/dd/ee[.eXt]

		ensure_equals("getExtension/no-ext",
			      gDirUtilp->getExtension(pathNoExt),
			      "");

		ensure_equals("getExtension/ext",
			      gDirUtilp->getExtension(pathExt),
			      "ext");

		// aa/bb/cc.dd/ee[.eXt]

		ensure_equals("getExtension/d-no-ext",
			      gDirUtilp->getExtension(dottedPathNoExt),
			      "");

		ensure_equals("getExtension/d-ext",
			      gDirUtilp->getExtension(dottedPathExt),
			      "ext");
	}
}

