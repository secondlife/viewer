'''
@file shutil2.py
@brief a better shutil.copytree replacement

$LicenseInfo:firstyear=2007&license=mit$

Copyright (c) 2007, Linden Research, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
$/LicenseInfo$
'''

#
# shutil2.py
# Taken from http://www.scons.org/wiki/AccumulateBuilder
# the stock copytree sucks because it insists that the
# target dir not exist
#

import os.path
import shutil

def copytree(src, dest, symlinks=False):
    """My own copyTree which does not fail if the directory exists.
    
    Recursively copy a directory tree using copy2().

    If the optional symlinks flag is true, symbolic links in the
    source tree result in symbolic links in the destination tree; if
    it is false, the contents of the files pointed to by symbolic
    links are copied.
    
    Behavior is meant to be identical to GNU 'cp -R'.    
    """
    def copyItems(src, dest, symlinks=False):
        """Function that does all the work.
        
        It is necessary to handle the two 'cp' cases:
        - destination does exist
        - destination does not exist
        
        See 'cp -R' documentation for more details
        """
        for item in os.listdir(src):
           srcPath = os.path.join(src, item)
           if os.path.isdir(srcPath):
               srcBasename = os.path.basename(srcPath)
               destDirPath = os.path.join(dest, srcBasename)
               if not os.path.exists(destDirPath):
                   os.makedirs(destDirPath)
               copyItems(srcPath, destDirPath)
           elif os.path.islink(item) and symlinks:
               linkto = os.readlink(item)
               os.symlink(linkto, dest)
           else:
               shutil.copy2(srcPath, dest)

    # case 'cp -R src/ dest/' where dest/ already exists
    if os.path.exists(dest):
       destPath = os.path.join(dest, os.path.basename(src))
       if not os.path.exists(destPath):
           os.makedirs(destPath)
    # case 'cp -R src/ dest/' where dest/ does not exist
    else:
       os.makedirs(dest)
       destPath = dest
    # actually copy the files
    copyItems(src, destPath)
