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

   template<> template<>
   void LLDirTest_object_t::test<5>()
      // getNextFileInDir
   {
      std::string delim = gDirUtilp->getDirDelimiter();
      std::string dirTemp = LLFile::tmpdir();

      // Create the same 5 file names of the two directories
      const char* filenames[5] = { "file1.abc", "file2.abc", "file1.xyz", "file2.xyz", "file1.mno" };
      std::string dir1 = makeTestDir(dirTemp + "getNextFileInDir");
      std::string dir2 = makeTestDir(dirTemp + "getNextFileInDir");
      std::string dir1files[5];
      std::string dir2files[5];
      for (int i=0; i<5; i++)
      {
         dir1files[i] = makeTestFile(dir1, filenames[i]);
         dir2files[i] = makeTestFile(dir2, filenames[i]);
      }

      // Scan dir1 and see if each of the 5 files is found exactly once
      std::string scan1result;
      int   found1 = 0;
      bool  filesFound1[5] = { false, false, false, false, false };
      // std::cerr << "searching '"+dir1+"' for *\n";
      while ( found1 <= 5 && gDirUtilp->getNextFileInDir(dir1, "*", scan1result, false) )
      {
         found1++;
         // std::cerr << "  found '"+scan1result+"'\n";
         int check;
         for (check=0; check < 5 && ! ( scan1result == filenames[check] ); check++)
         {
         }
         // check is now either 5 (not found) or the index of the matching name
         if (check < 5)
         {
            ensure( "found file '"+(std::string)filenames[check]+"' twice", ! filesFound1[check] );
            filesFound1[check] = true;
         }
         else
         {
            ensure( "found unknown file '"+(std::string)filenames[check]+"'", false);
         }
      }
      ensure("wrong number of files found in '"+dir1+"'", found1 == 5);

      // Scan dir2 and see if only the 2 *.xyz files are found
      std::string scan2result;
      int   found2 = 0;
      bool  filesFound2[5] = { false, false, false, false, false };
      // std::cerr << "searching '"+dir2+"' for *.xyz\n";

      while ( found2 <= 5 && gDirUtilp->getNextFileInDir(dir2, "*.xyz", scan2result, false) )
      {
         found2++;
         // std::cerr << "  found '"+scan2result+"'\n";
         int check;
         for (check=0; check < 5 && ! ( scan2result == filenames[check] ); check++)
         {
         }
         // check is now either 5 (not found) or the index of the matching name
         if (check < 5)
         {
            ensure( "found file '"+(std::string)filenames[check]+"' twice", ! filesFound2[check] );
            filesFound2[check] = true;
         }
         else // check is 5 - should not happen
         {
            ensure( "found unknown file '"+(std::string)filenames[check]+"'", false);
         }
      }
      ensure("wrong files found in '"+dir2+"'",
             !filesFound2[0] && !filesFound2[1] && filesFound2[2] && filesFound2[3] && !filesFound2[4] );


      // Scan dir2 and see if only the 1 *.mno file is found
      std::string scan3result;
      int   found3 = 0;
      bool  filesFound3[5] = { false, false, false, false, false };
      // std::cerr << "searching '"+dir2+"' for *.mno\n";

      while ( found3 <= 5 && gDirUtilp->getNextFileInDir(dir2, "*.mno", scan3result, false) )
      {
         found3++;
         // std::cerr << "  found '"+scan3result+"'\n";
         int check;
         for (check=0; check < 5 && ! ( scan3result == filenames[check] ); check++)
         {
         }
         // check is now either 5 (not found) or the index of the matching name
         if (check < 5)
         {
            ensure( "found file '"+(std::string)filenames[check]+"' twice", ! filesFound3[check] );
            filesFound3[check] = true;
         }
         else // check is 5 - should not happen
         {
            ensure( "found unknown file '"+(std::string)filenames[check]+"'", false);
         }
      }
      ensure("wrong files found in '"+dir2+"'",
             !filesFound3[0] && !filesFound3[1] && !filesFound3[2] && !filesFound3[3] && filesFound3[4] );


      // Scan dir1 and see if any *.foo files are found
      std::string scan4result;
      int   found4 = 0;
      bool  filesFound4[5] = { false, false, false, false, false };
      // std::cerr << "searching '"+dir1+"' for *.foo\n";

      while ( found4 <= 5 && gDirUtilp->getNextFileInDir(dir1, "*.foo", scan4result, false) )
      {
         found4++;
         // std::cerr << "  found '"+scan4result+"'\n";
         int check;
         for (check=0; check < 5 && ! ( scan4result == filenames[check] ); check++)
         {
         }
         // check is now either 5 (not found) or the index of the matching name
         if (check < 5)
         {
            ensure( "found file '"+(std::string)filenames[check]+"' twice", ! filesFound4[check] );
            filesFound4[check] = true;
         }
         else // check is 5 - should not happen
         {
            ensure( "found unknown file '"+(std::string)filenames[check]+"'", false);
         }
      }
      ensure("wrong files found in '"+dir1+"'",
             !filesFound4[0] && !filesFound4[1] && !filesFound4[2] && !filesFound4[3] && !filesFound4[4] );

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

