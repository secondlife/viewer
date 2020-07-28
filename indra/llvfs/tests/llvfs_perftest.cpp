
#include "linden_common.h"

#include <iostream>
#include <string>

#include "llcommon.h"
#include "llfile.h"
#include "llvfile.h"
#include "llvfsthread.h"
#include "lldir.h"

#include "llsd.h"
#include "llthreadsafequeue.h"

void init()
{
	// needed for APR initialization
	std::cout << "LLCommon::initClass()" << std::endl;
	LLCommon::initClass();

	// WTF does this mean??
	static const bool enable_threads = true;
	std::cout << "LLCommon::LLVFSThread(" << (enable_threads && false) << ")" << std::endl;
	LLVFSThread::initClass(enable_threads && false);

	const unsigned int salt = 12345;
	const char* PERF_VFS_DATA_FILE_BASE = "perf.data.db2.x.";
	const char* PERF_VFS_INDEX_FILE_BASE = "perf.index.db2.x.";
	const std::string vfs_data_file = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, PERF_VFS_DATA_FILE_BASE) + llformat("%u", salt);
	const std::string vfs_index_file = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, PERF_VFS_INDEX_FILE_BASE) + llformat("%u", salt);
	const unsigned int vfs_size_u32 = 1024 * 1024 * 50;
	std::cout << "Creating VFS with size " << vfs_size_u32 << " bytes in " << LL_PATH_CACHE << std::endl;
	gVFS = LLVFS::createLLVFS(vfs_index_file, vfs_data_file, false, vfs_size_u32, false);
	if (gVFS)
	{
		std::cout << "    created LLVFS successfully" << std::endl;
	}
	else
	{
		std::cout << "    unable to create LLVFS" << std::endl;
	}

	BOOL valid_vfs = gVFS->isValid();
	if (valid_vfs)
	{
		std::cout << "LLVFS is valid" << std::endl;
	}
	else
	{
		std::cout << "LLVFS is NOT valid" << std::endl;
	}

	std::cout << "---------- begin VFS file list ----------" << std::endl;
	gVFS->listFiles();
	std::cout << "-----------------------------------------" << std::endl;

	// TODO: is order important here - does this need to happen before LLVFS is initialized?
	std::cout << "LLVFile::initClass()" << std::endl;
	LLVFile::initClass();
}

void cleanup()
{
	std::cout << "LLVFile::cleanupClass()" << std::endl;
	LLVFile::cleanupClass();

	std::cout << "LLVFSThread::cleanupClass()" << std::endl;
	LLVFSThread::cleanupClass();

	std::cout << "Deleting VFS" << std::endl;
	delete gVFS;
	gVFS = NULL;

	std::cout << "LLCommon::cleanupClass()" << std::endl;
	LLCommon::cleanupClass();
}

BOOL write_asset()
{
	LLUUID asset_id;
	asset_id.generate();

	static std::string script_data("2\n170\n0\n\n\n1\n0\nDance2\n928cae18-e31d-76fd-9cc9-2f55160ff818\n0");  // gesture

	LLVFile file(gVFS, asset_id, LLAssetType::AT_GESTURE, LLVFile::WRITE);

	S32 offset = 0;
	S32 size = script_data.length();

	////S32 file_size = file.getSize();
	//std::cout << "file.getSize() = " << file.getSize() << std::endl;
	//// TODO: what is this test for?
	////if (file.getSize() >= offset + size)
	////{
	////}

	std::cout << "Writing dummy LSL script with ID " << asset_id << "and size " << size << " bytes to VFS" << std::endl;

	file.seek(offset);
	BOOL result = file.write((U8*)script_data.c_str(), size);

	return result;
}

void main_loop()
{
	while (true)
	{
		S32 pending = LLVFSThread::updateClass(1);
		std::cout << "    waiting for pending (" << pending << ") to complete" << std::endl;
		if (!pending)
		{
			break;
		}
		Sleep(100);
	}
}

int main2(int argc, char* argv[])
{

	init();

	BOOL result = write_asset();

	main_loop();

	if (result)
	{
		std::cout << "Asset written successfully" << std::endl;
	}
	else
	{
		std::cout << "Unable to write asset" << std::endl;
	}

	cleanup();

	std::cout << "VFS Perf/stress test finished" << std::endl;

    return 1;
}

struct result
{
    U32 id;
    bool ok;
};

//typedef struct result;
typedef std::function<result()> callable; // internal to this impl
typedef std::function<void(void*, bool)> vfs_callback; // used in public VFS API
typedef void* vfs_callback_data;

struct request 
{
    vfs_callback cb;
    vfs_callback_data cbd;
};

typedef std::map<U32, request> request_map;



void worker_thread(LLThreadSafeQueue<callable>& in, LLThreadSafeQueue<result>& out)
{
    while( ! in.isClosed())
    {
        // consider API call to test as well as pop to avoid a second lock
        auto item = in.popBack();

        auto res = item();

        out.pushFront(res);
    }

    out.close();
}

//result read()
//{
//    std::cout << "Running on thread" << std::endl;
//
//    return result{
//        5674,
//        true
//    };
//}




void per_tick(request_map& rm, LLThreadSafeQueue<result>& out)
{
    result res;
    while(out.tryPopBack((res)))
    {
        // TODO: consider changing to inspect queue for empty or counter too high or LLTimer in this function too long

        std::cout << "Working: thread returned " << res.ok << " with id = " << res.id << std::endl;

        // note: no need to lock the map because it's only access on main thread
        auto found = rm.find(res.id);
        if (found != rm.end())
        {
            found->second.cb(found->second.cbd, res.ok);    
            rm.erase(found);
        }
        else
        {
            std::cout << "Working: result came back with unknown id" << std::endl;
        }
    }
}

// will be class vars
LLThreadSafeQueue<callable> in;
request_map req_map;

void add_read(std::string filename, vfs_callback cb, vfs_callback_data cbd)
{
    static U32 id = 0;

    ++id;

    req_map.emplace(id, request{ cb, cbd } );

    in.pushFront([filename]() {

        std::cout << "Running on thread - processing filename: " << filename << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        return result{
            id,
            true
        };
    });
}

void cb_func(void* data, bool ok)
{
    std::cout << "I got a callback - ok = " << ok << std::endl;
}

int main(int argc, char* argv[])
{
    LLThreadSafeQueue<result> out;

    std::cout << "About to start worker thread" << std::endl;

    // like JS, need this.
    std::thread worker([&out]() { 
        worker_thread(in, out); 
    });

    //in.pushFront(read);

    //in.pushFront([]() {

    //    std::cout << "Running on thread" << std::endl;

    //    return result{
    //        5674,
    //        true
    //    };
    //});

    add_read("foo.txt", cb_func, nullptr);

    // flush input queue and close it
    in.close();

    std::cout << "About to start main loop" << std::endl;

    //const F32 period = 1.0;
    //LLEventTimer::run_every(period, [&req_map, &out]() {
    //    per_tick(req_map, out);
    //});

    while( ! out.isClosed())
    {
        std::cout << ".";
        per_tick(req_map, out);

    }

    //    //in.pushFront(...)
    //    //in.close(); // on shutdown

    // wait and block until thread completes
    // goes in cleanup class eventually
    worker.join();
}

