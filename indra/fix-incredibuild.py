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
