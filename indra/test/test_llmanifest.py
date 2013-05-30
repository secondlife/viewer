#!/usr/bin/env python
"""
@file test_llmanifest.py
@author Ryan Williams
@brief Test cases for LLManifest library.

$LicenseInfo:firstyear=2006&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2010, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""

from indra.util import llmanifest
import os.path
import os
import unittest

class DemoManifest(llmanifest.LLManifest):
    def construct(self):
        super(DemoManifest, self).construct()
        if self.prefix("dir_1"):
            self.path("test_a")
            self.path(src="test_b", dst="test_dst_b")
            self.path("*.test")
            self.path("*.tex", "*.jpg")
            if self.prefix("nested", dst=""):
                self.path("deep")
                self.end_prefix()
            self.end_prefix("dir_1")


class Demo_ArchManifest(llmanifest.LLManifest):
        pass

class TestLLManifest(unittest.TestCase):
    mode='static'
    def setUp(self):
        self.m = llmanifest.LLManifest({'source':'src', 'dest':'dst', 'grid':'default', 'platform':'darwin', 'version':(1,2,3,4),
                                        'artwork':'art', 'build':'build'})

    def testproperwindowspath(self):
        self.assertEqual(llmanifest.proper_windows_path("C:\Program Files", "cygwin"),"/cygdrive/c/Program Files")
        self.assertEqual(llmanifest.proper_windows_path("C:\Program Files", "windows"), "C:\Program Files")
        self.assertEqual(llmanifest.proper_windows_path("/cygdrive/c/Program Files/NSIS", "windows"), "C:\Program Files\NSIS")
        self.assertEqual(llmanifest.proper_windows_path("/cygdrive/c/Program Files/NSIS", "cygwin"), "/cygdrive/c/Program Files/NSIS")

    def testpathancestors(self):
        self.assertEqual(["dir"], [p for p in llmanifest.path_ancestors("dir")])
        self.assertEqual(["dir/sub", "dir"], [p for p in llmanifest.path_ancestors("dir/sub")])
        self.assertEqual(["dir/sub", "dir"], [p for p in llmanifest.path_ancestors("dir/sub/")])
        self.assertEqual(["dir/sub/two", "dir/sub", "dir"], [p for p in llmanifest.path_ancestors("dir/sub/two")])


    def testforplatform(self):
        self.assertEqual(llmanifest.LLManifest.for_platform('demo'), DemoManifest)
        def tmp_test():
            return llmanifest.LLManifest.for_platform('extant')
        self.assertRaises(KeyError, tmp_test)
        ExtantManifest = llmanifest.LLManifestRegistry('ExtantManifest', (llmanifest.LLManifest,), {})
        self.assertEqual(llmanifest.LLManifest.for_platform('extant'), ExtantManifest)
        self.assertEqual(llmanifest.LLManifest.for_platform('demo', 'Arch'), Demo_ArchManifest)


    def testprefix(self):
        self.assertEqual(self.m.get_src_prefix(), "src")
        self.assertEqual(self.m.get_dst_prefix(), "dst")
        self.m.prefix("level1")
        self.assertEqual(self.m.get_src_prefix(), "src/level1")
        self.assertEqual(self.m.get_dst_prefix(), "dst/level1")
        self.m.end_prefix()
        self.m.prefix(src="src", dst="dst")
        self.assertEqual(self.m.get_src_prefix(), "src/src")
        self.assertEqual(self.m.get_dst_prefix(), "dst/dst")
        self.m.end_prefix()

    def testendprefix(self):
        self.assertEqual(self.m.get_src_prefix(), "src")
        self.assertEqual(self.m.get_dst_prefix(), "dst")
        self.m.prefix("levela")
        self.m.end_prefix()
        self.assertEqual(self.m.get_src_prefix(), "src")
        self.assertEqual(self.m.get_dst_prefix(), "dst")
        self.m.prefix("level1")
        self.m.end_prefix("level1")
        self.assertEqual(self.m.get_src_prefix(), "src")
        self.assertEqual(self.m.get_dst_prefix(), "dst")
        self.m.prefix("level1")
        def tmp_test():
            self.m.end_prefix("mismatch")
        self.assertRaises(ValueError, tmp_test)

    def testruncommand(self):
        self.assertEqual("Hello\n", self.m.run_command("echo Hello"))
        def exit_1_test():
            self.m.run_command("exit 1")
        self.assertRaises(RuntimeError, exit_1_test)
        def not_found_test():
            self.m.run_command("test_command_that_should_not_be_found")
        self.assertRaises(RuntimeError, not_found_test)


    def testpathof(self):
        self.assertEqual(self.m.src_path_of("a"), "src/a")
        self.assertEqual(self.m.dst_path_of("a"), "dst/a")
        self.m.prefix("tmp")
        self.assertEqual(self.m.src_path_of("b/c"), "src/tmp/b/c")
        self.assertEqual(self.m.dst_path_of("b/c"), "dst/tmp/b/c")

    def testcmakedirs(self):
        self.m.cmakedirs("test_dir_DELETE/nested/dir")
        self.assert_(os.path.exists("test_dir_DELETE/nested/dir"))
        self.assert_(os.path.isdir("test_dir_DELETE"))
        self.assert_(os.path.isdir("test_dir_DELETE/nested"))
        self.assert_(os.path.isdir("test_dir_DELETE/nested/dir"))
        os.removedirs("test_dir_DELETE/nested/dir")

if __name__ == '__main__':
    unittest.main()
