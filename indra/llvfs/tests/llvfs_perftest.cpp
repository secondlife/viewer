
#include "linden_common.h"

#include <iostream>
#include <string>

#include "llfile.h"
#include "llvfile.h"
#include "llvfsthread.h"
#include "lldir.h"

int main(int argc, char* argv[])
{
	const unsigned int salt = 12345;
	const char* PERF_VFS_DATA_FILE_BASE = "perf.data.db2.x.";
	const char* PERF_VFS_INDEX_FILE_BASE = "perf.index.db2.x.";
	const std::string vfs_data_file = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, PERF_VFS_DATA_FILE_BASE) + llformat("%u", salt);
	const std::string vfs_index_file = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, PERF_VFS_INDEX_FILE_BASE) + llformat("%u", salt);
	const unsigned int vfs_size_u32 = 1024 * 1024 * 50;

	static const bool enable_threads = true;
	// WTF does this mean??
	LLVFSThread::initClass(enable_threads && false);

	std::cout << "Creating VFS with size " << vfs_size_u32 << " bytes in " << LL_PATH_CACHE << std::endl;
	gVFS = LLVFS::createLLVFS(vfs_index_file, vfs_data_file, false, vfs_size_u32, false);
	if (gVFS)
	{
		std::cout << "    created LLVFS successfully" << std::endl;

		LLUUID asset_id;
		asset_id.generate();

		static std::string script_data("This is a dummy LSL script");

		LLVFile file(gVFS, asset_id, LLAssetType::AT_LSL_TEXT, LLVFile::WRITE);

		S32 offset = 0;
		S32 size = script_data.length();

		if (file.getSize() >= offset + size)
		{
		}

		std::cout << "Writing dummy LSL script with ID " << asset_id << "and size " << size << " bytes to VFS" << std::endl;

		file.seek(offset);
		BOOL result = file.write((U8*)script_data.c_str(), size);

		while(true)
		{
			S32 pending = LLVFSThread::updateClass(1);
			std::cout << "    waiting for pending (" << pending << ") to complete" << std::endl;
			if (!pending)
			{
				break;
			}
			Sleep(100);
		}

		if (result)
		{
			std::cout << "    written successfully" << std::endl;
		}
		else
		{
			std::cout << "    unable to write file" << std::endl;
		}

		LLVFSThread::cleanupClass();
	}
	else
	{
		std::cout << "    unable to create LLVFS" << std::endl;
	}

	std::cout << "Deleting VFS" << std::endl;
	delete gVFS;
	gVFS = NULL;

	std::cout << "VFS Perf/stress test finished" << std::endl;
}
