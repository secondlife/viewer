#!/usr/bin/env python
## 
## $LicenseInfo:firstyear=2011&license=viewerlgpl$
## Second Life Viewer Source Code
## Copyright (C) 2011, Linden Research, Inc.
## 
## This library is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License as published by the Free Software Foundation;
## version 2.1 of the License only.
## 
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Lesser General Public License for more details.
## 
## You should have received a copy of the GNU Lesser General Public
## License along with this library; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
## 
## Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
## $/LicenseInfo$

import sys
import os
import glob

def delete_file_types(path, filetypes):
    if os.path.exists(path):
        print 'Cleaning: ' + path
        orig_dir = os.getcwd();
        os.chdir(path)
        filelist = []
        for type in filetypes:
            filelist.extend(glob.glob(type))
        for file in filelist:
            os.remove(file)
        os.chdir(orig_dir)

def main():
    build_types = ['*.exp','*.exe','*.pdb','*.idb',
                 '*.ilk','*.lib','*.obj','*.ib_pdb_index']
    pch_types = ['*.pch']
    delete_file_types("build-vc80/newview/Release", build_types)
    delete_file_types("build-vc80/newview/secondlife-bin.dir/Release/", 
                      pch_types)
    delete_file_types("build-vc80/newview/RelWithDebInfo", build_types)
    delete_file_types("build-vc80/newview/secondlife-bin.dir/RelWithDebInfo/", 
                      pch_types)
    delete_file_types("build-vc80/newview/Debug", build_types)
    delete_file_types("build-vc80/newview/secondlife-bin.dir/Debug/", 
                      pch_types)


    delete_file_types("build-vc80/test/RelWithDebInfo", build_types)
    delete_file_types("build-vc80/test/test.dir/RelWithDebInfo/", 
                      pch_types)


if __name__ == "__main__":
    main()
