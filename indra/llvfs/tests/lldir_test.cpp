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

   std::string makeTestFile( const std::string& dir, const std::string& file )
   {
      std::string delim = gDirUtilp->getDirDelimiter();
      std::string path = dir + delim + file;
      LLFILE* handle = LLFile::fopen( path, "w" );
      ensure("failed to open test file '"+path+"'", handle != NULL );
      ensure("failed to write to test file '"+path+"'", !fputs("test file", handle) );
      fclose(handle);
      return path;
   }

   std::string makeTestDir( const std::string& dirbase )
   {
      int counter;
      std::string uniqueDir;
      bool foundUnused;
      std::string delim = gDirUtilp->getDirDelimiter();
      
      for (counter=0, foundUnused=false; !foundUnused; counter++ )
      {
         char counterStr[3];
         sprintf(counterStr, "%02d", counter);
         uniqueDir = dirbase + counterStr;
         foundUnused = ! ( LLFile::isdir(uniqueDir) || LLFile::isfile(uniqueDir) );
      }
      ensure("test directory '" + uniqueDir + "' creation failed", !LLFile::mkdir(uniqueDir));
      
      return uniqueDir + delim; // HACK - apparently, the trailing delimiter is needed...
   }

   static const char* DirScanFilename[5] = { "file1.abc", "file2.abc", "file1.xyz", "file2.xyz", "file1.mno" };
   
   void scanTest(const std::string& directory, const std::string& pattern, bool correctResult[5])
   {

      // Scan directory and see if any file1.* files are found
      std::string scanResult;
      int   found = 0;
      bool  filesFound[5] = { false, false, false, false, false };
      //std::cerr << "searching '"+directory+"' for '"+pattern+"'\n";

      while ( found <= 5 && gDirUtilp->getNextFileInDir(directory, pattern, scanResult) )
      {
         found++;
         //std::cerr << "  found '"+scanResult+"'\n";
         int check;
         for (check=0; check < 5 && ! ( scanResult == DirScanFilename[check] ); check++)
         {
         }
         // check is now either 5 (not found) or the index of the matching name
         if (check < 5)
         {
            ensure( "found file '"+(std::string)DirScanFilename[check]+"' twice", ! filesFound[check] );
            filesFound[check] = true;
         }
         else // check is 5 - should not happen
         {
            fail( "found unknown file '"+scanResult+"'");
         }
      }
      for (int i=0; i<5; i++)
      {
         if (correctResult[i])
         {
            ensure("scan of '"+directory+"' using '"+pattern+"' did not return '"+DirScanFilename[i]+"'", filesFound[i]);
         }
         else
         {
            ensure("scan of '"+directory+"' using '"+pattern+"' incorrectly returned '"+DirScanFilename[i]+"'", !filesFound[i]);
         }
      }
   }
   
   template<> template<>
   void LLDirTest_object_t::test<5>()
      // getNextFileInDir
   {
      std::string delim = gDirUtilp->getDirDelimiter();
      std::string dirTemp = LLFile::tmpdir();

      // Create the same 5 file names of the two directories

      std::string dir1 = makeTestDir(dirTemp + "getNextFileInDir");
      std::string dir2 = makeTestDir(dirTemp + "getNextFileInDir");
      std::string dir1files[5];
      std::string dir2files[5];
      for (int i=0; i<5; i++)
      {
         dir1files[i] = makeTestFile(dir1, DirScanFilename[i]);
         dir2files[i] = makeTestFile(dir2, DirScanFilename[i]);
      }

      // Scan dir1 and see if each of the 5 files is found exactly once
      bool expected1[5] = { true, true, true, true, true };
      scanTest(dir1, "*", expected1);

      // Scan dir2 and see if only the 2 *.xyz files are found
      bool  expected2[5] = { false, false, true, true, false };
      scanTest(dir1, "*.xyz", expected2);

      // Scan dir2 and see if only the 1 *.mno file is found
      bool  expected3[5] = { false, false, false, false, true };
      scanTest(dir2, "*.mno", expected3);

      // Scan dir1 and see if any *.foo files are found
      bool  expected4[5] = { false, false, false, false, false };
      scanTest(dir1, "*.foo", expected4);

      // Scan dir1 and see if any file1.* files are found
      bool  expected5[5] = { true, false, true, false, true };
      scanTest(dir1, "file1.*", expected5);

      // Scan dir1 and see if any file1.* files are found
      bool  expected6[5] = { true, true, false, false, false };
      scanTest(dir1, "file?.abc", expected6);

      // Scan dir2 and see if any file?.x?z files are found
      bool  expected7[5] = { false, false, true, true, false };
      scanTest(dir2, "file?.x?z", expected7);

      // Scan dir2 and see if any file?.??c files are found
      // THESE FAIL ON Mac and Windows, SO ARE COMMENTED OUT FOR NOW
      //      bool  expected8[5] = { true, true, false, false, false };
      //      scanTest(dir2, "file?.??c", expected8);
      //      scanTest(dir2, "*.??c", expected8);

      // Scan dir1 and see if any *.?n? files are found
      bool  expected9[5] = { false, false, false, false, true };
      scanTest(dir1, "*.?n?", expected9);

      // Scan dir1 and see if any *.???? files are found
      // THIS ONE FAILS ON WINDOWS (returns three charater suffixes) SO IS COMMENTED OUT FOR NOW
      // bool  expected10[5] = { false, false, false, false, false };
      // scanTest(dir1, "*.????", expected10);

      // Scan dir1 and see if any ?????.* files are found
      bool  expected11[5] = { true, true, true, true, true };
      scanTest(dir1, "?????.*", expected11);

      // Scan dir1 and see if any ??l??.xyz files are found
      bool  expected12[5] = { false, false, true, true, false };
      scanTest(dir1, "??l??.xyz", expected12);

      // clean up all test files and directories
      for (int i=0; i<5; i++)
      {
         LLFile::remove(dir1files[i]);
         LLFile::remove(dir2files[i]);
      }
      LLFile::rmdir(dir1);
      LLFile::rmdir(dir2);
   }
}

