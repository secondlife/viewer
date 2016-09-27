/**
 * @file llmeshrepository.cpp
 * @brief Mesh repository implementation.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010-2014, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "apr_pools.h"
#include "apr_dso.h"
#include "llhttpconstants.h"
#include "llapr.h"
#include "llmeshrepository.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llbufferstream.h"
#include "llcallbacklist.h"
#include "lldatapacker.h"
#include "lldeadmantimer.h"
#include "llfloatermodelpreview.h"
#include "llfloaterperms.h"
#include "lleconomy.h"
#include "llimagej2c.h"
#include "llhost.h"
#include "llmath.h"
#include "llnotificationsutil.h"
#include "llsd.h"
#include "llsdutil_math.h"
#include "llsdserialize.h"
#include "llthread.h"
#include "llvfile.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewermenufile.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llvolume.h"
#include "llvolumemgr.h"
#include "llvovolume.h"
#include "llworld.h"
#include "material_codes.h"
#include "pipeline.h"
#include "llinventorymodel.h"
#include "llfoldertype.h"
#include "llviewerparcelmgr.h"
#include "lluploadfloaterobservers.h"
#include "bufferarray.h"
#include "bufferstream.h"
#include "llfasttimer.h"
#include "llcorehttputil.h"
#include "lltrans.h"
#include "llstatusbar.h"
#include "llinventorypanel.h"
#include "lluploaddialog.h"
#include "llfloaterreg.h"

#include "boost/lexical_cast.hpp"

#ifndef LL_WINDOWS
#include "netdb.h"
#endif


// Purpose
//
//   The purpose of this module is to provide access between the viewer
//   and the asset system as regards to mesh objects.
//
//   * High-throughput download of mesh assets from servers while
//     following best industry practices for network profile.
//   * Reliable expensing and upload of new mesh assets.
//   * Recovery and retry from errors when appropriate.
//   * Decomposition of mesh assets for preview and uploads.
//   * And most important:  all of the above without exposing the
//     main thread to stalls due to deep processing or thread
//     locking actions.  In particular, the following operations
//     on LLMeshRepository are very averse to any stalls:
//     * loadMesh
//     * getMeshHeader (For structural details, see:
//       http://wiki.secondlife.com/wiki/Mesh/Mesh_Asset_Format)
//     * notifyLoadedMeshes
//     * getSkinInfo
//
// Threads
//
//   main     Main rendering thread, very sensitive to locking and other stalls
//   repo     Overseeing worker thread associated with the LLMeshRepoThread class
//   decom    Worker thread for mesh decomposition requests
//   core     HTTP worker thread:  does the work but doesn't intrude here
//   uploadN  0-N temporary mesh upload threads (0-1 in practice)
//
// Sequence of Operations
//
//   What follows is a description of the retrieval of one LOD for
//   a new mesh object.  Work is performed by a series of short, quick
//   actions distributed over a number of threads.  Each is meant
//   to proceed without stalling and the whole forms a deep request
//   pipeline to achieve throughput.  Ellipsis indicates a return
//   or break in processing which is resumed elsewhere.
//
//         main thread         repo thread (run() method)
//
//         loadMesh() invoked to request LOD
//           append LODRequest to mPendingRequests
//         ...
//         other mesh requests may be made
//         ...
//         notifyLoadedMeshes() invoked to stage work
//           append HeaderRequest to mHeaderReqQ
//         ...
//                             scan mHeaderReqQ
//                             issue 4096-byte GET for header
//                             ...
//                             onCompleted() invoked for GET
//                               data copied
//                               headerReceived() invoked
//                                 LLSD parsed
//                                 mMeshHeader, mMeshHeaderSize updated
//                                 scan mPendingLOD for LOD request
//                                 push LODRequest to mLODReqQ
//                             ...
//                             scan mLODReqQ
//                             fetchMeshLOD() invoked
//                               issue Byte-Range GET for LOD
//                             ...
//                             onCompleted() invoked for GET
//                               data copied
//                               lodReceived() invoked
//                                 unpack data into LLVolume
//                                 append LoadedMesh to mLoadedQ
//                             ...
//         notifyLoadedMeshes() invoked again
//           scan mLoadedQ
//           notifyMeshLoaded() for LOD
//             setMeshAssetLoaded() invoked for system volume
//             notifyMeshLoaded() invoked for each interested object
//         ...
//
// Mutexes
//
//   LLMeshRepository::mMeshMutex
//   LLMeshRepoThread::mMutex
//   LLMeshRepoThread::mHeaderMutex
//   LLMeshRepoThread::mSignal (LLCondition)
//   LLPhysicsDecomp::mSignal (LLCondition)
//   LLPhysicsDecomp::mMutex
//   LLMeshUploadThread::mMutex
//
// Mutex Order Rules
//
//   1.  LLMeshRepoThread::mMutex before LLMeshRepoThread::mHeaderMutex
//   2.  LLMeshRepository::mMeshMutex before LLMeshRepoThread::mMutex
//   (There are more rules, haven't been extracted.)
//
// Data Member Access/Locking
//
//   Description of how shared access to static and instance data
//   members is performed.  Each member is followed by the name of
//   the mutex, if any, covering the data and then a list of data
//   access models each of which is a triplet of the following form:
//
//     {ro, wo, rw}.{main, repo, any}.{mutex, none}
//     Type of access:  read-only, write-only, read-write.
//     Accessing thread or 'any'
//     Relevant mutex held during access (several may be held) or 'none'
//
//   A careful eye will notice some unsafe operations.  Many of these
//   have an alibi of some form.  Several types of alibi are identified
//   and listed here:
//
//     [0]  No alibi.  Probably unsafe.
//     [1]  Single-writer, self-consistent readers.  Old data must
//          be tolerated by any reader but data will come true eventually.
//     [2]  Like [1] but provides a hint about thread state.  These
//          may be unsafe.
//     [3]  empty() check outside of lock.  Can me made safish when
//          done in double-check lock style.  But this depends on
//          std:: implementation and memory model.
//     [4]  Appears to be covered by a mutex but doesn't need one.
//     [5]  Read of a double-checked lock.
//
//   So, in addition to documentation, take this as a to-do/review
//   list and see if you can improve things.  For porters to non-x86
//   architectures, the weaker memory models will make these platforms
//   probabilistically more susceptible to hitting race conditions.
//   True here and in other multi-thread code such as texture fetching.
//   (Strong memory models make weak programmers.  Weak memory models
//   make strong programmers.  Ref:  arm, ppc, mips, alpha)
//
//   LLMeshRepository:
//
//     sBytesReceived                  none            rw.repo.none, ro.main.none [1]
//     sMeshRequestCount               "
//     sHTTPRequestCount               "
//     sHTTPLargeRequestCount          "
//     sHTTPRetryCount                 "
//     sHTTPErrorCount                 "
//     sLODPending                     mMeshMutex [4]  rw.main.mMeshMutex
//     sLODProcessing                  Repo::mMutex    rw.any.Repo::mMutex
//     sCacheBytesRead                 none            rw.repo.none, ro.main.none [1]
//     sCacheBytesWritten              "
//     sCacheReads                     "
//     sCacheWrites                    "
//     mLoadingMeshes                  mMeshMutex [4]  rw.main.none, rw.any.mMeshMutex
//     mSkinMap                        none            rw.main.none
//     mDecompositionMap               none            rw.main.none
//     mPendingRequests                mMeshMutex [4]  rw.main.mMeshMutex
//     mLoadingSkins                   mMeshMutex [4]  rw.main.mMeshMutex
//     mPendingSkinRequests            mMeshMutex [4]  rw.main.mMeshMutex
//     mLoadingDecompositions          mMeshMutex [4]  rw.main.mMeshMutex
//     mPendingDecompositionRequests   mMeshMutex [4]  rw.main.mMeshMutex
//     mLoadingPhysicsShapes           mMeshMutex [4]  rw.main.mMeshMutex
//     mPendingPhysicsShapeRequests    mMeshMutex [4]  rw.main.mMeshMutex
//     mUploads                        none            rw.main.none (upload thread accessing objects)
//     mUploadWaitList                 none            rw.main.none (upload thread accessing objects)
//     mInventoryQ                     mMeshMutex [4]  rw.main.mMeshMutex, ro.main.none [5]
//     mUploadErrorQ                   mMeshMutex      rw.main.mMeshMutex, rw.any.mMeshMutex
//     mGetMeshVersion                 none            rw.main.none
//
//   LLMeshRepoThread:
//
//     sActiveHeaderRequests    mMutex        rw.any.mMutex, ro.repo.none [1]
//     sActiveLODRequests       mMutex        rw.any.mMutex, ro.repo.none [1]
//     sMaxConcurrentRequests   mMutex        wo.main.none, ro.repo.none, ro.main.mMutex
//     mMeshHeader              mHeaderMutex  rw.repo.mHeaderMutex, ro.main.mHeaderMutex, ro.main.none [0]
//     mMeshHeaderSize          mHeaderMutex  rw.repo.mHeaderMutex
//     mSkinRequests            mMutex        rw.repo.mMutex, ro.repo.none [5]
//     mSkinInfoQ               mMutex        rw.repo.mMutex, rw.main.mMutex [5] (was:  [0])
//     mDecompositionRequests   mMutex        rw.repo.mMutex, ro.repo.none [5]
//     mPhysicsShapeRequests    mMutex        rw.repo.mMutex, ro.repo.none [5]
//     mDecompositionQ          mMutex        rw.repo.mMutex, rw.main.mMutex [5] (was:  [0])
//     mHeaderReqQ              mMutex        ro.repo.none [5], rw.repo.mMutex, rw.any.mMutex
//     mLODReqQ                 mMutex        ro.repo.none [5], rw.repo.mMutex, rw.any.mMutex
//     mUnavailableQ            mMutex        rw.repo.none [0], ro.main.none [5], rw.main.mMutex
//     mLoadedQ                 mMutex        rw.repo.mMutex, ro.main.none [5], rw.main.mMutex
//     mPendingLOD              mMutex        rw.repo.mMutex, rw.any.mMutex
//     mGetMeshCapability       mMutex        rw.main.mMutex, ro.repo.mMutex (was:  [0])
//     mGetMesh2Capability      mMutex        rw.main.mMutex, ro.repo.mMutex (was:  [0])
//     mGetMeshVersion          mMutex        rw.main.mMutex, ro.repo.mMutex
//     mHttp*                   none          rw.repo.none
//
//   LLMeshUploadThread:
//
//     mDiscarded               mMutex        rw.main.mMutex, ro.uploadN.none [1]
//     ... more ...
//
// QA/Development Testing
//
//   Debug variable 'MeshUploadFakeErrors' takes a mask of bits that will
//   simulate an error on fee query or upload.  Defined bits are:
//
//   0x01            Simulate application error on fee check reading
//                   response body from file "fake_upload_error.xml"
//   0x02            Same as 0x01 but for actual upload attempt.
//   0x04            Simulate a transport problem on fee check with a
//                   locally-generated 500 status.
//   0x08            As with 0x04 but for the upload operation.
//
//   For major changes, see the LL_MESH_FASTTIMER_ENABLE below and
//   instructions for looking for frame stalls using fast timers.
//
// *TODO:  Work list for followup actions:
//   * Review anything marked as unsafe above, verify if there are real issues.
//   * See if we can put ::run() into a hard sleep.  May not actually perform better
//     than the current scheme so be prepared for disappointment.  You'll likely
//     need to introduce a condition variable class that references a mutex in
//     methods rather than derives from mutex which isn't correct.
//   * On upload failures, make more information available to the alerting
//     dialog.  Get the structured information going into the log into a
//     tree there.
//   * Header parse failures come without much explanation.  Elaborate.
//   * Work queue for uploads?  Any need for this or is the current scheme good
//     enough?
//   * Various temp buffers used in VFS I/O might be allocated once or even
//     statically.  Look for some wins here.
//   * Move data structures holding mesh data used by main thread into main-
//     thread-only access so that no locking is needed.  May require duplication
//     of some data so that worker thread has a minimal data set to guide
//     operations.
//
// --------------------------------------------------------------------------
//                    Development/Debug/QA Tools
//
// Enable here or in build environment to get fasttimer data on mesh fetches.
//
// Typically, this is used to perform A/B testing using the
// fasttimer console (shift-ctrl-9).  This is done by looking
// for stalls due to lock contention between the main thread
// and the repository and HTTP code.  In a release viewer,
// these appear as ping-time or worse spikes in frame time.
// With this instrumentation enabled, a stall will appear
// under the 'Mesh Fetch' timer which will be either top-level
// or under 'Render' time.

static LLFastTimer::DeclareTimer FTM_MESH_FETCH("Mesh Fetch");

// Random failure testing for development/QA.
//
// Set the MESH_*_FAILED macros to either 'false' or to
// an invocation of MESH_RANDOM_NTH_TRUE() with some
// suitable number.  In production, all must be false.
//
// Example:
// #define	MESH_HTTP_RESPONSE_FAILED				MESH_RANDOM_NTH_TRUE(9)

// 1-in-N calls will test true
#define	MESH_RANDOM_NTH_TRUE(_N)				( ll_rand(S32(_N)) == 0 )

#define	MESH_HTTP_RESPONSE_FAILED				false
#define	MESH_HEADER_PROCESS_FAILED				false
#define	MESH_LOD_PROCESS_FAILED					false
#define	MESH_SKIN_INFO_PROCESS_FAILED			false
#define	MESH_DECOMP_PROCESS_FAILED				false
#define MESH_PHYS_SHAPE_PROCESS_FAILED			false

// --------------------------------------------------------------------------


LLMeshRepository gMeshRepo;

const S32 MESH_HEADER_SIZE = 4096;                      // Important:  assumption is that headers fit in this space

const S32 REQUEST_HIGH_WATER_MIN = 32;					// Limits for GetMesh regions
const S32 REQUEST_HIGH_WATER_MAX = 150;					// Should remain under 2X throttle
const S32 REQUEST_LOW_WATER_MIN = 16;
const S32 REQUEST_LOW_WATER_MAX = 75;

const S32 REQUEST2_HIGH_WATER_MIN = 32;					// Limits for GetMesh2 regions
const S32 REQUEST2_HIGH_WATER_MAX = 100;
const S32 REQUEST2_LOW_WATER_MIN = 16;
const S32 REQUEST2_LOW_WATER_MAX = 50;

const U32 LARGE_MESH_FETCH_THRESHOLD = 1U << 21;		// Size at which requests goes to narrow/slow queue
const long SMALL_MESH_XFER_TIMEOUT = 120L;				// Seconds to complete xfer, small mesh downloads
const long LARGE_MESH_XFER_TIMEOUT = 600L;				// Seconds to complete xfer, large downloads

// Would normally like to retry on uploads as some
// retryable failures would be recoverable.  Unfortunately,
// the mesh service is using 500 (retryable) rather than
// 400/bad request (permanent) for a bad payload and
// retrying that just leads to revocation of the one-shot
// cap which then produces a 404 on retry destroying some
// (occasionally) useful error information.  We'll leave
// upload retries to the user as in the past.  SH-4667.
const long UPLOAD_RETRY_LIMIT = 0L;

// Maximum mesh version to support.  Three least significant digits are reserved for the minor version, 
// with major version changes indicating a format change that is not backwards compatible and should not
// be parsed by viewers that don't specifically support that version. For example, if the integer "1" is 
// present, the version is 0.001. A viewer that can parse version 0.001 can also parse versions up to 0.999, 
// but not 1.0 (integer 1000).
// See wiki at https://wiki.secondlife.com/wiki/Mesh/Mesh_Asset_Format
const S32 MAX_MESH_VERSION = 999;

U32 LLMeshRepository::sBytesReceived = 0;
U32 LLMeshRepository::sMeshRequestCount = 0;
U32 LLMeshRepository::sHTTPRequestCount = 0;
U32 LLMeshRepository::sHTTPLargeRequestCount = 0;
U32 LLMeshRepository::sHTTPRetryCount = 0;
U32 LLMeshRepository::sHTTPErrorCount = 0;
U32 LLMeshRepository::sLODProcessing = 0;
U32 LLMeshRepository::sLODPending = 0;

U32 LLMeshRepository::sCacheBytesRead = 0;
U32 LLMeshRepository::sCacheBytesWritten = 0;
U32 LLMeshRepository::sCacheReads = 0;
U32 LLMeshRepository::sCacheWrites = 0;
U32 LLMeshRepository::sMaxLockHoldoffs = 0;
	
LLDeadmanTimer LLMeshRepository::sQuiescentTimer(15.0, false);	// true -> gather cpu metrics

namespace {
    // The NoOpDeletor is used when passing certain objects (generally the LLMeshUploadThread) 
    // in a smart pointer below for passage into the LLCore::Http libararies.  
    // When the smart pointer is destroyed,  no action will be taken since we 
    // do not in these cases want the object to be destroyed at the end of the call.
    // 
    // *NOTE$: Yes! It is "Deletor" 
    // http://english.stackexchange.com/questions/4733/what-s-the-rule-for-adding-er-vs-or-when-nouning-a-verb
    // "delete" derives from Latin "deletus"

    void NoOpDeletor(LLCore::HttpHandler *)
    { /*NoOp*/ }
}

static S32 dump_num = 0;
std::string make_dump_name(std::string prefix, S32 num)
{
	return prefix + boost::lexical_cast<std::string>(num) + std::string(".xml");
}
void dump_llsd_to_file(const LLSD& content, std::string filename);
LLSD llsd_from_file(std::string filename);

const std::string header_lod[] = 
{
	"lowest_lod",
	"low_lod",
	"medium_lod",
	"high_lod"
};
const char * const LOG_MESH = "Mesh";

// Static data and functions to measure mesh load
// time metrics for a new region scene.
static unsigned int metrics_teleport_start_count = 0;
boost::signals2::connection metrics_teleport_started_signal;
static void teleport_started();

void on_new_single_inventory_upload_complete(
    LLAssetType::EType asset_type,
    LLInventoryType::EType inventory_type,
    const std::string inventory_type_string,
    const LLUUID& item_folder_id,
    const std::string& item_name,
    const std::string& item_description,
    const LLSD& server_response,
    S32 upload_price);


//get the number of bytes resident in memory for given volume
U32 get_volume_memory_size(const LLVolume* volume)
{
	U32 indices = 0;
	U32 vertices = 0;

	for (U32 i = 0; i < volume->getNumVolumeFaces(); ++i)
	{
		const LLVolumeFace& face = volume->getVolumeFace(i);
		indices += face.mNumIndices;
		vertices += face.mNumVertices;
	}


	return indices*2+vertices*11+sizeof(LLVolume)+sizeof(LLVolumeFace)*volume->getNumVolumeFaces();
}

void get_vertex_buffer_from_mesh(LLCDMeshData& mesh, LLModel::PhysicsMesh& res, F32 scale = 1.f)
{
	res.mPositions.clear();
	res.mNormals.clear();
	
	const F32* v = mesh.mVertexBase;

	if (mesh.mIndexType == LLCDMeshData::INT_16)
	{
		U16* idx = (U16*) mesh.mIndexBase;
		for (S32 j = 0; j < mesh.mNumTriangles; ++j)
		{ 
			F32* mp0 = (F32*) ((U8*)v+idx[0]*mesh.mVertexStrideBytes);
			F32* mp1 = (F32*) ((U8*)v+idx[1]*mesh.mVertexStrideBytes);
			F32* mp2 = (F32*) ((U8*)v+idx[2]*mesh.mVertexStrideBytes);

			idx = (U16*) (((U8*)idx)+mesh.mIndexStrideBytes);
			
			LLVector3 v0(mp0);
			LLVector3 v1(mp1);
			LLVector3 v2(mp2);

			LLVector3 n = (v1-v0)%(v2-v0);
			n.normalize();

			res.mPositions.push_back(v0*scale);
			res.mPositions.push_back(v1*scale);
			res.mPositions.push_back(v2*scale);

			res.mNormals.push_back(n);
			res.mNormals.push_back(n);
			res.mNormals.push_back(n);			
		}
	}
	else
	{
		U32* idx = (U32*) mesh.mIndexBase;
		for (S32 j = 0; j < mesh.mNumTriangles; ++j)
		{ 
			F32* mp0 = (F32*) ((U8*)v+idx[0]*mesh.mVertexStrideBytes);
			F32* mp1 = (F32*) ((U8*)v+idx[1]*mesh.mVertexStrideBytes);
			F32* mp2 = (F32*) ((U8*)v+idx[2]*mesh.mVertexStrideBytes);

			idx = (U32*) (((U8*)idx)+mesh.mIndexStrideBytes);
			
			LLVector3 v0(mp0);
			LLVector3 v1(mp1);
			LLVector3 v2(mp2);

			LLVector3 n = (v1-v0)%(v2-v0);
			n.normalize();

			res.mPositions.push_back(v0*scale);
			res.mPositions.push_back(v1*scale);
			res.mPositions.push_back(v2*scale);

			res.mNormals.push_back(n);
			res.mNormals.push_back(n);
			res.mNormals.push_back(n);			
		}
	}
}

LLViewerFetchedTexture* LLMeshUploadThread::FindViewerTexture(const LLImportMaterial& material)
{
	LLPointer< LLViewerFetchedTexture > * ppTex = static_cast< LLPointer< LLViewerFetchedTexture > * >(material.mOpaqueData);
	return ppTex ? (*ppTex).get() : NULL;
}

volatile S32 LLMeshRepoThread::sActiveHeaderRequests = 0;
volatile S32 LLMeshRepoThread::sActiveLODRequests = 0;
U32	LLMeshRepoThread::sMaxConcurrentRequests = 1;
S32 LLMeshRepoThread::sRequestLowWater = REQUEST2_LOW_WATER_MIN;
S32 LLMeshRepoThread::sRequestHighWater = REQUEST2_HIGH_WATER_MIN;
S32 LLMeshRepoThread::sRequestWaterLevel = 0;

// Base handler class for all mesh users of llcorehttp.
// This is roughly equivalent to a Responder class in
// traditional LL code.  The base is going to perform
// common response/data handling in the inherited
// onCompleted() method.  Derived classes, one for each
// type of HTTP action, define processData() and
// processFailure() methods to customize handling and
// error messages.
//
// LLCore::HttpHandler
//   LLMeshHandlerBase
//     LLMeshHeaderHandler
//     LLMeshLODHandler
//     LLMeshSkinInfoHandler
//     LLMeshDecompositionHandler
//     LLMeshPhysicsShapeHandler
//   LLMeshUploadThread

class LLMeshHandlerBase : public LLCore::HttpHandler,
    public boost::enable_shared_from_this<LLMeshHandlerBase>
{
public:
    typedef boost::shared_ptr<LLMeshHandlerBase> ptr_t;

	LOG_CLASS(LLMeshHandlerBase);
	LLMeshHandlerBase(U32 offset, U32 requested_bytes)
		: LLCore::HttpHandler(),
		  mMeshParams(),
		  mProcessed(false),
		  mHttpHandle(LLCORE_HTTP_HANDLE_INVALID),
		  mOffset(offset),
		  mRequestedBytes(requested_bytes)
		{}

	virtual ~LLMeshHandlerBase()
		{}

protected:
	LLMeshHandlerBase(const LLMeshHandlerBase &);				// Not defined
	void operator=(const LLMeshHandlerBase &);					// Not defined
	
public:
	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);
	virtual void processData(LLCore::BufferArray * body, S32 body_offset, U8 * data, S32 data_size) = 0;
	virtual void processFailure(LLCore::HttpStatus status) = 0;
	
public:
	LLVolumeParams mMeshParams;
	bool mProcessed;
	LLCore::HttpHandle mHttpHandle;
	U32 mOffset;
	U32 mRequestedBytes;
};


// Subclass for header fetches.
//
// Thread:  repo
class LLMeshHeaderHandler : public LLMeshHandlerBase
{
public:
	LOG_CLASS(LLMeshHeaderHandler);
	LLMeshHeaderHandler(const LLVolumeParams & mesh_params, U32 offset, U32 requested_bytes)
		: LLMeshHandlerBase(offset, requested_bytes)
	{
		mMeshParams = mesh_params;
		LLMeshRepoThread::incActiveHeaderRequests();
	}
	virtual ~LLMeshHeaderHandler();

protected:
	LLMeshHeaderHandler(const LLMeshHeaderHandler &);			// Not defined
	void operator=(const LLMeshHeaderHandler &);				// Not defined
	
public:
	virtual void processData(LLCore::BufferArray * body, S32 body_offset, U8 * data, S32 data_size);
	virtual void processFailure(LLCore::HttpStatus status);
};


// Subclass for LOD fetches.
//
// Thread:  repo
class LLMeshLODHandler : public LLMeshHandlerBase
{
public:
	LOG_CLASS(LLMeshLODHandler);
	LLMeshLODHandler(const LLVolumeParams & mesh_params, S32 lod, U32 offset, U32 requested_bytes)
		: LLMeshHandlerBase(offset, requested_bytes),
		  mLOD(lod)
	{
			mMeshParams = mesh_params;
			LLMeshRepoThread::incActiveLODRequests();
		}
	virtual ~LLMeshLODHandler();
	
protected:
	LLMeshLODHandler(const LLMeshLODHandler &);					// Not defined
	void operator=(const LLMeshLODHandler &);					// Not defined
	
public:
	virtual void processData(LLCore::BufferArray * body, S32 body_offset, U8 * data, S32 data_size);
	virtual void processFailure(LLCore::HttpStatus status);

public:
	S32 mLOD;
};


// Subclass for skin info fetches.
//
// Thread:  repo
class LLMeshSkinInfoHandler : public LLMeshHandlerBase
{
public:
	LOG_CLASS(LLMeshSkinInfoHandler);
	LLMeshSkinInfoHandler(const LLUUID& id, U32 offset, U32 requested_bytes)
		: LLMeshHandlerBase(offset, requested_bytes),
		  mMeshID(id)
	{}
	virtual ~LLMeshSkinInfoHandler();

protected:
	LLMeshSkinInfoHandler(const LLMeshSkinInfoHandler &);		// Not defined
	void operator=(const LLMeshSkinInfoHandler &);				// Not defined

public:
	virtual void processData(LLCore::BufferArray * body, S32 body_offset, U8 * data, S32 data_size);
	virtual void processFailure(LLCore::HttpStatus status);

public:
	LLUUID mMeshID;
};


// Subclass for decomposition fetches.
//
// Thread:  repo
class LLMeshDecompositionHandler : public LLMeshHandlerBase
{
public:
	LOG_CLASS(LLMeshDecompositionHandler);
	LLMeshDecompositionHandler(const LLUUID& id, U32 offset, U32 requested_bytes)
		: LLMeshHandlerBase(offset, requested_bytes),
		  mMeshID(id)
	{}
	virtual ~LLMeshDecompositionHandler();

protected:
	LLMeshDecompositionHandler(const LLMeshDecompositionHandler &);		// Not defined
	void operator=(const LLMeshDecompositionHandler &);					// Not defined

public:
	virtual void processData(LLCore::BufferArray * body, S32 body_offset, U8 * data, S32 data_size);
	virtual void processFailure(LLCore::HttpStatus status);

public:
	LLUUID mMeshID;
};


// Subclass for physics shape fetches.
//
// Thread:  repo
class LLMeshPhysicsShapeHandler : public LLMeshHandlerBase
{
public:
	LOG_CLASS(LLMeshPhysicsShapeHandler);
	LLMeshPhysicsShapeHandler(const LLUUID& id, U32 offset, U32 requested_bytes)
		: LLMeshHandlerBase(offset, requested_bytes),
		  mMeshID(id)
	{}
	virtual ~LLMeshPhysicsShapeHandler();

protected:
	LLMeshPhysicsShapeHandler(const LLMeshPhysicsShapeHandler &);	// Not defined
	void operator=(const LLMeshPhysicsShapeHandler &);				// Not defined

public:
	virtual void processData(LLCore::BufferArray * body, S32 body_offset, U8 * data, S32 data_size);
	virtual void processFailure(LLCore::HttpStatus status);

public:
	LLUUID mMeshID;
};


void log_upload_error(LLCore::HttpStatus status, const LLSD& content,
					  const char * const stage, const std::string & model_name)
{
	// Add notification popup.
	LLSD args;
	std::string message = content["error"]["message"].asString();
	std::string identifier = content["error"]["identifier"].asString();
	args["MESSAGE"] = message;
	args["IDENTIFIER"] = identifier;
	args["LABEL"] = model_name;

	// Log details.
	LL_WARNS(LOG_MESH) << "Error in stage:  " << stage
					   << ", Reason:  " << status.toString()
					   << " (" << status.toTerseString() << ")" << LL_ENDL;

	std::ostringstream details;
	typedef std::set<std::string> mav_errors_set_t;
	mav_errors_set_t mav_errors;

	if (content.has("error"))
	{
		const LLSD& err = content["error"];
		LL_WARNS(LOG_MESH) << "error: " << err << LL_ENDL;
		LL_WARNS(LOG_MESH) << "  mesh upload failed, stage '" << stage
						   << "', error '" << err["error"].asString()
						   << "', message '" << err["message"].asString()
						   << "', id '" << err["identifier"].asString()
						   << "'" << LL_ENDL;

		if (err.has("errors"))
		{
			details << std::endl << std::endl;

			S32 error_num = 0;
			const LLSD& err_list = err["errors"];
			for (LLSD::array_const_iterator it = err_list.beginArray();
				 it != err_list.endArray();
				 ++it)
			{
				const LLSD& err_entry = *it;
				std::string message = err_entry["message"];

				if (message.length() > 0)
				{
					mav_errors.insert(message);
				}

				LL_WARNS(LOG_MESH) << "  error[" << error_num << "]:" << LL_ENDL;
				for (LLSD::map_const_iterator map_it = err_entry.beginMap();
					 map_it != err_entry.endMap();
					 ++map_it)
				{
					LL_WARNS(LOG_MESH) << "    " << map_it->first << ":  "
									   << map_it->second << LL_ENDL;
				}
				error_num++;
			}
		}
	}
	else
	{
		LL_WARNS(LOG_MESH) << "Bad response to mesh request, no additional error information available." << LL_ENDL;
	}
	
	mav_errors_set_t::iterator mav_errors_it = mav_errors.begin();
	for (; mav_errors_it != mav_errors.end(); ++mav_errors_it)
	{
		std::string mav_details = "Mav_Details_" + *mav_errors_it;
		details << "Message: '" << *mav_errors_it << "': " << LLTrans::getString(mav_details) << std::endl << std::endl;
	}

	std::string details_str = details.str();
	if (details_str.length() > 0)
	{
		args["DETAILS"] = details_str;
	}

	gMeshRepo.uploadError(args);
}

LLMeshRepoThread::LLMeshRepoThread()
: LLThread("mesh repo"),
  mHttpRequest(NULL),
  mHttpOptions(),
  mHttpLargeOptions(),
  mHttpHeaders(),
  mHttpPolicyClass(LLCore::HttpRequest::DEFAULT_POLICY_ID),
  mHttpLegacyPolicyClass(LLCore::HttpRequest::DEFAULT_POLICY_ID),
  mHttpLargePolicyClass(LLCore::HttpRequest::DEFAULT_POLICY_ID),
  mHttpPriority(0),
  mGetMeshVersion(2)
{
	LLAppCoreHttp & app_core_http(LLAppViewer::instance()->getAppCoreHttp());

	mMutex = new LLMutex(NULL);
	mHeaderMutex = new LLMutex(NULL);
	mSignal = new LLCondition(NULL);
	mHttpRequest = new LLCore::HttpRequest;
	mHttpOptions = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);
	mHttpOptions->setTransferTimeout(SMALL_MESH_XFER_TIMEOUT);
	mHttpOptions->setUseRetryAfter(gSavedSettings.getBOOL("MeshUseHttpRetryAfter"));
	mHttpLargeOptions = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);
	mHttpLargeOptions->setTransferTimeout(LARGE_MESH_XFER_TIMEOUT);
	mHttpLargeOptions->setUseRetryAfter(gSavedSettings.getBOOL("MeshUseHttpRetryAfter"));
	mHttpHeaders = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders);
	mHttpHeaders->append(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_VND_LL_MESH);
	mHttpPolicyClass = app_core_http.getPolicy(LLAppCoreHttp::AP_MESH2);
	mHttpLegacyPolicyClass = app_core_http.getPolicy(LLAppCoreHttp::AP_MESH1);
	mHttpLargePolicyClass = app_core_http.getPolicy(LLAppCoreHttp::AP_LARGE_MESH);
}


LLMeshRepoThread::~LLMeshRepoThread()
{
	LL_INFOS(LOG_MESH) << "Small GETs issued:  " << LLMeshRepository::sHTTPRequestCount
					   << ", Large GETs issued:  " << LLMeshRepository::sHTTPLargeRequestCount
					   << ", Max Lock Holdoffs:  " << LLMeshRepository::sMaxLockHoldoffs
					   << LL_ENDL;

	mHttpRequestSet.clear();
    mHttpHeaders.reset();

    delete mHttpRequest;
	mHttpRequest = NULL;
	delete mMutex;
	mMutex = NULL;
	delete mHeaderMutex;
	mHeaderMutex = NULL;
	delete mSignal;
	mSignal = NULL;
}

void LLMeshRepoThread::run()
{
	LLCDResult res = LLConvexDecomposition::initThread();
	if (res != LLCD_OK)
	{
		LL_WARNS(LOG_MESH) << "Convex decomposition unable to be loaded.  Expect severe problems." << LL_ENDL;
	}

	while (!LLApp::isQuitting())
	{
		// *TODO:  Revise sleep/wake strategy and try to move away
		// from polling operations in this thread.  We can sleep
		// this thread hard when:
		// * All Http requests are serviced
		// * LOD request queue empty
		// * Header request queue empty
		// * Skin info request queue empty
		// * Decomposition request queue empty
		// * Physics shape request queue empty
		// We wake the thread when any of the above become untrue.
		// Will likely need a correctly-implemented condition variable to do this.
		// On the other hand, this may actually be an effective and efficient scheme...
		
		mSignal->wait();

		if (LLApp::isQuitting())
		{
			break;
		}
		
		if (! mHttpRequestSet.empty())
		{
			// Dispatch all HttpHandler notifications
			mHttpRequest->update(0L);
		}
		sRequestWaterLevel = mHttpRequestSet.size();			// Stats data update
			
		// NOTE: order of queue processing intentionally favors LOD requests over header requests

		while (!mLODReqQ.empty() && mHttpRequestSet.size() < sRequestHighWater)
		{
			if (! mMutex)
			{
				break;
			}
			mMutex->lock();
			LODRequest req = mLODReqQ.front();
			mLODReqQ.pop();
			LLMeshRepository::sLODProcessing--;
			mMutex->unlock();

			if (!fetchMeshLOD(req.mMeshParams, req.mLOD))		// failed, resubmit
			{
				mMutex->lock();
				mLODReqQ.push(req); 
				++LLMeshRepository::sLODProcessing;
				mMutex->unlock();
			}
		}

		while (!mHeaderReqQ.empty() && mHttpRequestSet.size() < sRequestHighWater)
		{
			if (! mMutex)
			{
				break;
			}
			mMutex->lock();
			HeaderRequest req = mHeaderReqQ.front();
			mHeaderReqQ.pop();
			mMutex->unlock();
			if (!fetchMeshHeader(req.mMeshParams))//failed, resubmit
			{
				mMutex->lock();
				mHeaderReqQ.push(req) ;
				mMutex->unlock();
			}
		}

		// For the final three request lists, similar goal to above but
		// slightly different queue structures.  Stay off the mutex when
		// performing long-duration actions.

		if (mHttpRequestSet.size() < sRequestHighWater
			&& (! mSkinRequests.empty()
				|| ! mDecompositionRequests.empty()
				|| ! mPhysicsShapeRequests.empty()))
		{
			// Something to do probably, lock and double-check.  We don't want
			// to hold the lock long here.  That will stall main thread activities
			// so we bounce it.

			mMutex->lock();
			if (! mSkinRequests.empty() && mHttpRequestSet.size() < sRequestHighWater)
			{
				std::set<LLUUID> incomplete;
				std::set<LLUUID>::iterator iter(mSkinRequests.begin());
				while (iter != mSkinRequests.end() && mHttpRequestSet.size() < sRequestHighWater)
				{
					LLUUID mesh_id = *iter;
					mSkinRequests.erase(iter);
					mMutex->unlock();

					if (! fetchMeshSkinInfo(mesh_id))
					{
						incomplete.insert(mesh_id);
					}

					mMutex->lock();
					iter = mSkinRequests.begin();
				}

				if (! incomplete.empty())
				{
					mSkinRequests.insert(incomplete.begin(), incomplete.end());
				}
			}

			// holding lock, try next list
			// *TODO:  For UI/debug-oriented lists, we might drop the fine-
			// grained locking as there's a lowered expectation of smoothness
			// in these cases.
			if (! mDecompositionRequests.empty() && mHttpRequestSet.size() < sRequestHighWater)
			{
				std::set<LLUUID> incomplete;
				std::set<LLUUID>::iterator iter(mDecompositionRequests.begin());
				while (iter != mDecompositionRequests.end() && mHttpRequestSet.size() < sRequestHighWater)
				{
					LLUUID mesh_id = *iter;
					mDecompositionRequests.erase(iter);
					mMutex->unlock();
					
					if (! fetchMeshDecomposition(mesh_id))
					{
						incomplete.insert(mesh_id);
					}

					mMutex->lock();
					iter = mDecompositionRequests.begin();
				}

				if (! incomplete.empty())
				{
					mDecompositionRequests.insert(incomplete.begin(), incomplete.end());
				}
			}

			// holding lock, final list
			if (! mPhysicsShapeRequests.empty() && mHttpRequestSet.size() < sRequestHighWater)
			{
				std::set<LLUUID> incomplete;
				std::set<LLUUID>::iterator iter(mPhysicsShapeRequests.begin());
				while (iter != mPhysicsShapeRequests.end() && mHttpRequestSet.size() < sRequestHighWater)
				{
					LLUUID mesh_id = *iter;
					mPhysicsShapeRequests.erase(iter);
					mMutex->unlock();
					
					if (! fetchMeshPhysicsShape(mesh_id))
					{
						incomplete.insert(mesh_id);
					}

					mMutex->lock();
					iter = mPhysicsShapeRequests.begin();
				}

				if (! incomplete.empty())
				{
					mPhysicsShapeRequests.insert(incomplete.begin(), incomplete.end());
				}
			}
			mMutex->unlock();
		}

		// For dev purposes only.  A dynamic change could make this false
		// and that shouldn't assert.
		// llassert_always(mHttpRequestSet.size() <= sRequestHighWater);
	}
	
	if (mSignal->isLocked())
	{ //make sure to let go of the mutex associated with the given signal before shutting down
		mSignal->unlock();
	}

	res = LLConvexDecomposition::quitThread();
	if (res != LLCD_OK)
	{
		LL_WARNS(LOG_MESH) << "Convex decomposition unable to be quit." << LL_ENDL;
	}
}

// Mutex:  LLMeshRepoThread::mMutex must be held on entry
void LLMeshRepoThread::loadMeshSkinInfo(const LLUUID& mesh_id)
{
	mSkinRequests.insert(mesh_id);
}

// Mutex:  LLMeshRepoThread::mMutex must be held on entry
void LLMeshRepoThread::loadMeshDecomposition(const LLUUID& mesh_id)
{
	mDecompositionRequests.insert(mesh_id);
}

// Mutex:  LLMeshRepoThread::mMutex must be held on entry
void LLMeshRepoThread::loadMeshPhysicsShape(const LLUUID& mesh_id)
{
	mPhysicsShapeRequests.insert(mesh_id);
}

void LLMeshRepoThread::lockAndLoadMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{
	if (!LLAppViewer::isQuitting())
	{
		loadMeshLOD(mesh_params, lod);
	}
}


void LLMeshRepoThread::loadMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{ //could be called from any thread
	LLMutexLock lock(mMutex);
	mesh_header_map::iterator iter = mMeshHeader.find(mesh_params.getSculptID());
	if (iter != mMeshHeader.end())
	{ //if we have the header, request LOD byte range
		LODRequest req(mesh_params, lod);
		{
			mLODReqQ.push(req);
			LLMeshRepository::sLODProcessing++;
		}
	}
	else
	{ 
		HeaderRequest req(mesh_params);
		
		pending_lod_map::iterator pending = mPendingLOD.find(mesh_params);

		if (pending != mPendingLOD.end())
		{ //append this lod request to existing header request
			pending->second.push_back(lod);
			llassert(pending->second.size() <= LLModel::NUM_LODS);
		}
		else
		{ //if no header request is pending, fetch header
			mHeaderReqQ.push(req);
			mPendingLOD[mesh_params].push_back(lod);
		}
	}
}

// Mutex:  must be holding mMutex when called
void LLMeshRepoThread::setGetMeshCaps(const std::string & get_mesh1,
									  const std::string & get_mesh2,
									  int pref_version)
{
	mGetMeshCapability = get_mesh1;
	mGetMesh2Capability = get_mesh2;
	mGetMeshVersion = pref_version;
}


// Constructs a Cap URL for the mesh.  Prefers a GetMesh2 cap
// over a GetMesh cap.
//
// Mutex:  acquires mMutex
void LLMeshRepoThread::constructUrl(LLUUID mesh_id, std::string * url, int * version)
{
	std::string res_url;
	int res_version(2);
	
	if (gAgent.getRegion())
	{
		LLMutexLock lock(mMutex);

		// Get a consistent pair of (cap string, version).  The
		// locking could be eliminated here without loss of safety
		// by using a set of staging values in setGetMeshCaps().
		
		if (! mGetMesh2Capability.empty() && mGetMeshVersion > 1)
		{
			res_url = mGetMesh2Capability;
			res_version = 2;
		}
		else
		{
			res_url = mGetMeshCapability;
			res_version = 1;
		}
	}

	if (! res_url.empty())
	{
		res_url += "/?mesh_id=";
		res_url += mesh_id.asString().c_str();
	}
	else
	{
		LL_WARNS_ONCE(LOG_MESH) << "Current region does not have GetMesh capability!  Cannot load "
								<< mesh_id << ".mesh" << LL_ENDL;
	}

	*url = res_url;
	*version = res_version;
}

// Issue an HTTP GET request with byte range using the right
// policy class.  Large requests go to the large request class.
// If the current region supports GetMesh2, we prefer that for
// smaller requests otherwise we try to use the traditional
// GetMesh capability and connection concurrency.
//
// @return		Valid handle or LLCORE_HTTP_HANDLE_INVALID.
//				If the latter, actual status is found in
//				mHttpStatus member which is valid until the
//				next call to this method.
//
// Thread:  repo
LLCore::HttpHandle LLMeshRepoThread::getByteRange(const std::string & url, int cap_version,
												  size_t offset, size_t len,
												  const LLCore::HttpHandler::ptr_t &handler)
{
	// Also used in lltexturefetch.cpp
	static LLCachedControl<bool> disable_range_req(gSavedSettings, "HttpRangeRequestsDisable", false);
	
	LLCore::HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);
	
	if (len < LARGE_MESH_FETCH_THRESHOLD)
	{
		handle = mHttpRequest->requestGetByteRange((2 == cap_version
													? mHttpPolicyClass
													: mHttpLegacyPolicyClass),
												   mHttpPriority,
												   url,
												   (disable_range_req ? size_t(0) : offset),
												   (disable_range_req ? size_t(0) : len),
												   mHttpOptions,
												   mHttpHeaders,
												   handler);
		if (LLCORE_HTTP_HANDLE_INVALID != handle)
		{
			++LLMeshRepository::sHTTPRequestCount;
		}
	}
	else
	{
		handle = mHttpRequest->requestGetByteRange(mHttpLargePolicyClass,
												   mHttpPriority,
												   url,
												   (disable_range_req ? size_t(0) : offset),
												   (disable_range_req ? size_t(0) : len),
												   mHttpLargeOptions,
												   mHttpHeaders,
												   handler);
		if (LLCORE_HTTP_HANDLE_INVALID != handle)
		{
			++LLMeshRepository::sHTTPLargeRequestCount;
		}
	}
	if (LLCORE_HTTP_HANDLE_INVALID == handle)
	{
		// Something went wrong, capture the error code for caller.
		mHttpStatus = mHttpRequest->getStatus();
	}
	return handle;
}


bool LLMeshRepoThread::fetchMeshSkinInfo(const LLUUID& mesh_id)
{
	
	if (!mHeaderMutex)
	{
		return false;
	}

	mHeaderMutex->lock();

	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{ //we have no header info for this mesh, do nothing
		mHeaderMutex->unlock();
		return false;
	}

	++LLMeshRepository::sMeshRequestCount;
	bool ret = true;
	U32 header_size = mMeshHeaderSize[mesh_id];
	
	if (header_size > 0)
	{
		S32 version = mMeshHeader[mesh_id]["version"].asInteger();
		S32 offset = header_size + mMeshHeader[mesh_id]["skin"]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id]["skin"]["size"].asInteger();

		mHeaderMutex->unlock();

		if (version <= MAX_MESH_VERSION && offset >= 0 && size > 0)
		{
			//check VFS for mesh skin info
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{				
				LLMeshRepository::sCacheBytesRead += size;
				++LLMeshRepository::sCacheReads;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's by checking the first 1KB (reserved block but not written)
				bool zero = true;
				for (S32 i = 0; i < llmin(size, 1024) && zero; ++i)
				{
					zero = buffer[i] > 0 ? false : true;
				}

				if (!zero)
				{ //attempt to parse
					if (skinInfoReceived(mesh_id, buffer, size))
					{						
						delete[] buffer;
						return true;
					}
				}

				delete[] buffer;
			}

			//reading from VFS failed for whatever reason, fetch from sim
			int cap_version(2);
			std::string http_url;
			constructUrl(mesh_id, &http_url, &cap_version);

			if (!http_url.empty())
			{
                LLMeshHandlerBase::ptr_t handler(new LLMeshSkinInfoHandler(mesh_id, offset, size));
				LLCore::HttpHandle handle = getByteRange(http_url, cap_version, offset, size, handler);
				if (LLCORE_HTTP_HANDLE_INVALID == handle)
				{
					LL_WARNS(LOG_MESH) << "HTTP GET request failed for skin info on mesh " << mID
									   << ".  Reason:  " << mHttpStatus.toString()
									   << " (" << mHttpStatus.toTerseString() << ")"
									   << LL_ENDL;
					ret = false;
				}
				else
				{
					handler->mHttpHandle = handle;
					mHttpRequestSet.insert(handler);
				}
			}
		}
	}
	else
	{	
		mHeaderMutex->unlock();
	}

	//early out was not hit, effectively fetched
	return ret;
}

bool LLMeshRepoThread::fetchMeshDecomposition(const LLUUID& mesh_id)
{
	if (!mHeaderMutex)
	{
		return false;
	}

	mHeaderMutex->lock();

	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{ //we have no header info for this mesh, do nothing
		mHeaderMutex->unlock();
		return false;
	}

	++LLMeshRepository::sMeshRequestCount;
	U32 header_size = mMeshHeaderSize[mesh_id];
	bool ret = true;
	
	if (header_size > 0)
	{
		S32 version = mMeshHeader[mesh_id]["version"].asInteger();
		S32 offset = header_size + mMeshHeader[mesh_id]["physics_convex"]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id]["physics_convex"]["size"].asInteger();

		mHeaderMutex->unlock();

		if (version <= MAX_MESH_VERSION && offset >= 0 && size > 0)
		{
			//check VFS for mesh skin info
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;
				++LLMeshRepository::sCacheReads;

				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's by checking the first 1KB (reserved block but not written)
				bool zero = true;
				for (S32 i = 0; i < llmin(size, 1024) && zero; ++i)
				{
					zero = buffer[i] > 0 ? false : true;
				}

				if (!zero)
				{ //attempt to parse
					if (decompositionReceived(mesh_id, buffer, size))
					{
						delete[] buffer;
						return true;
					}
				}

				delete[] buffer;
			}

			//reading from VFS failed for whatever reason, fetch from sim
			int cap_version(2);
			std::string http_url;
			constructUrl(mesh_id, &http_url, &cap_version);
			
			if (!http_url.empty())
			{
                LLMeshHandlerBase::ptr_t handler(new LLMeshDecompositionHandler(mesh_id, offset, size));
				LLCore::HttpHandle handle = getByteRange(http_url, cap_version, offset, size, handler);
				if (LLCORE_HTTP_HANDLE_INVALID == handle)
				{
					LL_WARNS(LOG_MESH) << "HTTP GET request failed for decomposition mesh " << mID
									   << ".  Reason:  " << mHttpStatus.toString()
									   << " (" << mHttpStatus.toTerseString() << ")"
									   << LL_ENDL;
					ret = false;
				}
				else
				{
					handler->mHttpHandle = handle;
					mHttpRequestSet.insert(handler);
				}
			}
		}
	}
	else
	{	
		mHeaderMutex->unlock();
	}

	//early out was not hit, effectively fetched
	return ret;
}

bool LLMeshRepoThread::fetchMeshPhysicsShape(const LLUUID& mesh_id)
{
	if (!mHeaderMutex)
	{
		return false;
	}

	mHeaderMutex->lock();

	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{ //we have no header info for this mesh, do nothing
		mHeaderMutex->unlock();
		return false;
	}

	++LLMeshRepository::sMeshRequestCount;
	U32 header_size = mMeshHeaderSize[mesh_id];
	bool ret = true;

	if (header_size > 0)
	{
		S32 version = mMeshHeader[mesh_id]["version"].asInteger();
		S32 offset = header_size + mMeshHeader[mesh_id]["physics_mesh"]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id]["physics_mesh"]["size"].asInteger();

		mHeaderMutex->unlock();

		if (version <= MAX_MESH_VERSION && offset >= 0 && size > 0)
		{
			//check VFS for mesh physics shape info
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;
				++LLMeshRepository::sCacheReads;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's by checking the first 1KB (reserved block but not written)
				bool zero = true;
				for (S32 i = 0; i < llmin(size, 1024) && zero; ++i)
				{
					zero = buffer[i] > 0 ? false : true;
				}

				if (!zero)
				{ //attempt to parse
					if (physicsShapeReceived(mesh_id, buffer, size))
					{
						delete[] buffer;
						return true;
					}
				}

				delete[] buffer;
			}

			//reading from VFS failed for whatever reason, fetch from sim
			int cap_version(2);
			std::string http_url;
			constructUrl(mesh_id, &http_url, &cap_version);
			
			if (!http_url.empty())
			{
                LLMeshHandlerBase::ptr_t handler(new LLMeshPhysicsShapeHandler(mesh_id, offset, size));
				LLCore::HttpHandle handle = getByteRange(http_url, cap_version, offset, size, handler);
				if (LLCORE_HTTP_HANDLE_INVALID == handle)
				{
					LL_WARNS(LOG_MESH) << "HTTP GET request failed for physics shape on mesh " << mID
									   << ".  Reason:  " << mHttpStatus.toString()
									   << " (" << mHttpStatus.toTerseString() << ")"
									   << LL_ENDL;
					ret = false;
				}
				else
				{
					handler->mHttpHandle = handle;
					mHttpRequestSet.insert(handler);
				}
			}
		}
		else
		{ //no physics shape whatsoever, report back NULL
			physicsShapeReceived(mesh_id, NULL, 0);
		}
	}
	else
	{	
		mHeaderMutex->unlock();
	}

	//early out was not hit, effectively fetched
	return ret;
}

//static
void LLMeshRepoThread::incActiveLODRequests()
{
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	++LLMeshRepoThread::sActiveLODRequests;
}

//static
void LLMeshRepoThread::decActiveLODRequests()
{
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	--LLMeshRepoThread::sActiveLODRequests;
}

//static
void LLMeshRepoThread::incActiveHeaderRequests()
{
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	++LLMeshRepoThread::sActiveHeaderRequests;
}

//static
void LLMeshRepoThread::decActiveHeaderRequests()
{
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	--LLMeshRepoThread::sActiveHeaderRequests;
}

//return false if failed to get header
bool LLMeshRepoThread::fetchMeshHeader(const LLVolumeParams& mesh_params)
{
	++LLMeshRepository::sMeshRequestCount;

	{
		//look for mesh in asset in vfs
		LLVFile file(gVFS, mesh_params.getSculptID(), LLAssetType::AT_MESH);
			
		S32 size = file.getSize();

		if (size > 0)
		{
			// *NOTE:  if the header size is ever more than 4KB, this will break
			U8 buffer[MESH_HEADER_SIZE];
			S32 bytes = llmin(size, MESH_HEADER_SIZE);
			LLMeshRepository::sCacheBytesRead += bytes;	
			++LLMeshRepository::sCacheReads;
			file.read(buffer, bytes);
			if (headerReceived(mesh_params, buffer, bytes))
			{
				// Found mesh in VFS cache
				return true;
			}
		}
	}

	//either cache entry doesn't exist or is corrupt, request header from simulator	
	bool retval = true;
	int cap_version(2);
	std::string http_url;
	constructUrl(mesh_params.getSculptID(), &http_url, &cap_version);
	
	if (!http_url.empty())
	{
		//grab first 4KB if we're going to bother with a fetch.  Cache will prevent future fetches if a full mesh fits
		//within the first 4KB
		//NOTE -- this will break of headers ever exceed 4KB		

        LLMeshHandlerBase::ptr_t handler(new LLMeshHeaderHandler(mesh_params, 0, MESH_HEADER_SIZE));
		LLCore::HttpHandle handle = getByteRange(http_url, cap_version, 0, MESH_HEADER_SIZE, handler);
		if (LLCORE_HTTP_HANDLE_INVALID == handle)
		{
			LL_WARNS(LOG_MESH) << "HTTP GET request failed for mesh header " << mID
							   << ".  Reason:  " << mHttpStatus.toString()
							   << " (" << mHttpStatus.toTerseString() << ")"
							   << LL_ENDL;
			retval = false;
		}
		else
		{
			handler->mHttpHandle = handle;
			mHttpRequestSet.insert(handler);
		}
	}

	return retval;
}

//return false if failed to get mesh lod.
bool LLMeshRepoThread::fetchMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{
	if (!mHeaderMutex)
	{
		return false;
	}

	mHeaderMutex->lock();

	++LLMeshRepository::sMeshRequestCount;
	bool retval = true;

	LLUUID mesh_id = mesh_params.getSculptID();
	
	U32 header_size = mMeshHeaderSize[mesh_id];

	if (header_size > 0)
	{
		S32 version = mMeshHeader[mesh_id]["version"].asInteger();
		S32 offset = header_size + mMeshHeader[mesh_id][header_lod[lod]]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id][header_lod[lod]]["size"].asInteger();
		mHeaderMutex->unlock();
				
		if (version <= MAX_MESH_VERSION && offset >= 0 && size > 0)
		{

			//check VFS for mesh asset
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;
				++LLMeshRepository::sCacheReads;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's by checking the first 1KB (reserved block but not written)
				bool zero = true;
				for (S32 i = 0; i < llmin(size, 1024) && zero; ++i)
				{
					zero = buffer[i] > 0 ? false : true;
				}

				if (!zero)
				{ //attempt to parse
					if (lodReceived(mesh_params, lod, buffer, size))
					{
						delete[] buffer;
						return true;
					}
				}

				delete[] buffer;
			}

			//reading from VFS failed for whatever reason, fetch from sim
			int cap_version(2);
			std::string http_url;
			constructUrl(mesh_id, &http_url, &cap_version);
			
			if (!http_url.empty())
			{
                LLMeshHandlerBase::ptr_t handler(new LLMeshLODHandler(mesh_params, lod, offset, size));
				LLCore::HttpHandle handle = getByteRange(http_url, cap_version, offset, size, handler);
				if (LLCORE_HTTP_HANDLE_INVALID == handle)
				{
					LL_WARNS(LOG_MESH) << "HTTP GET request failed for LOD on mesh " << mID
									   << ".  Reason:  " << mHttpStatus.toString()
									   << " (" << mHttpStatus.toTerseString() << ")"
									   << LL_ENDL;
					retval = false;
				}
				else
				{
					handler->mHttpHandle = handle;
					mHttpRequestSet.insert(handler);
					// *NOTE:  Allowing a re-request, not marking as unavailable.  Is that correct?
				}
			}
			else
			{
				mUnavailableQ.push(LODRequest(mesh_params, lod));
			}
		}
		else
		{
			mUnavailableQ.push(LODRequest(mesh_params, lod));
		}
	}
	else
	{
		mHeaderMutex->unlock();
	}

	return retval;
}

bool LLMeshRepoThread::headerReceived(const LLVolumeParams& mesh_params, U8* data, S32 data_size)
{
	const LLUUID mesh_id = mesh_params.getSculptID();
	LLSD header;
	
	U32 header_size = 0;
	if (data_size > 0)
	{
		std::string res_str((char*) data, data_size);

		std::string deprecated_header("<? LLSD/Binary ?>");

		if (res_str.substr(0, deprecated_header.size()) == deprecated_header)
		{
			res_str = res_str.substr(deprecated_header.size()+1, data_size);
			header_size = deprecated_header.size()+1;
		}
		data_size = res_str.size();

		std::istringstream stream(res_str);

		if (!LLSDSerialize::fromBinary(header, stream, data_size))
		{
			LL_WARNS(LOG_MESH) << "Mesh header parse error.  Not a valid mesh asset!  ID:  " << mesh_id
							   << LL_ENDL;
			return false;
		}

		header_size += stream.tellg();
	}
	else
	{
		LL_INFOS(LOG_MESH) << "Non-positive data size.  Marking header as non-existent, will not retry.  ID:  " << mesh_id
						   << LL_ENDL;
		header["404"] = 1;
	}

	{
		
		{
			LLMutexLock lock(mHeaderMutex);
			mMeshHeaderSize[mesh_id] = header_size;
			mMeshHeader[mesh_id] = header;
		}

		
		LLMutexLock lock(mMutex); // make sure only one thread access mPendingLOD at the same time.

		//check for pending requests
		pending_lod_map::iterator iter = mPendingLOD.find(mesh_params);
		if (iter != mPendingLOD.end())
		{
			for (U32 i = 0; i < iter->second.size(); ++i)
			{
				LODRequest req(mesh_params, iter->second[i]);
				mLODReqQ.push(req);
				LLMeshRepository::sLODProcessing++;
			}
			mPendingLOD.erase(iter);
		}
	}

	return true;
}

bool LLMeshRepoThread::lodReceived(const LLVolumeParams& mesh_params, S32 lod, U8* data, S32 data_size)
{
	if (data == NULL || data_size == 0)
	{
		return false;
	}

	LLPointer<LLVolume> volume = new LLVolume(mesh_params, LLVolumeLODGroup::getVolumeScaleFromDetail(lod));
	std::string mesh_string((char*) data, data_size);
	std::istringstream stream(mesh_string);

	if (volume->unpackVolumeFaces(stream, data_size))
	{
		if (volume->getNumFaces() > 0)
		{
			LoadedMesh mesh(volume, mesh_params, lod);
			{
				LLMutexLock lock(mMutex);
				mLoadedQ.push(mesh);
			}
			return true;
		}
	}

	return false;
}

bool LLMeshRepoThread::skinInfoReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD skin;

	if (data_size > 0)
	{
		std::string res_str((char*) data, data_size);

		std::istringstream stream(res_str);

		if (!unzip_llsd(skin, stream, data_size))
		{
			LL_WARNS(LOG_MESH) << "Mesh skin info parse error.  Not a valid mesh asset!  ID:  " << mesh_id
							   << LL_ENDL;
			return false;
		}
	}
	
	{
		LLMeshSkinInfo info(skin);
		info.mMeshID = mesh_id;

		// LL_DEBUGS(LOG_MESH) << "info pelvis offset" << info.mPelvisOffset << LL_ENDL;
		{
			LLMutexLock lock(mMutex);
			mSkinInfoQ.push_back(info);
		}
	}

	return true;
}

bool LLMeshRepoThread::decompositionReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD decomp;

	if (data_size > 0)
	{ 
		std::string res_str((char*) data, data_size);

		std::istringstream stream(res_str);

		if (!unzip_llsd(decomp, stream, data_size))
		{
			LL_WARNS(LOG_MESH) << "Mesh decomposition parse error.  Not a valid mesh asset!  ID:  " << mesh_id
							   << LL_ENDL;
			return false;
		}
	}
	
	{
		LLModel::Decomposition* d = new LLModel::Decomposition(decomp);
		d->mMeshID = mesh_id;
		{
			LLMutexLock lock(mMutex);
			mDecompositionQ.push_back(d);
		}
	}

	return true;
}

bool LLMeshRepoThread::physicsShapeReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD physics_shape;

	LLModel::Decomposition* d = new LLModel::Decomposition();
	d->mMeshID = mesh_id;

	if (data == NULL)
	{ //no data, no physics shape exists
		d->mPhysicsShapeMesh.clear();
	}
	else
	{
		LLVolumeParams volume_params;
		volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
		volume_params.setSculptID(mesh_id, LL_SCULPT_TYPE_MESH);
		LLPointer<LLVolume> volume = new LLVolume(volume_params,0);
		std::string mesh_string((char*) data, data_size);
		std::istringstream stream(mesh_string);

		if (volume->unpackVolumeFaces(stream, data_size))
		{
			//load volume faces into decomposition buffer
			S32 vertex_count = 0;
			S32 index_count = 0;

			for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
			{
				const LLVolumeFace& face = volume->getVolumeFace(i);
				vertex_count += face.mNumVertices;
				index_count += face.mNumIndices;
			}

			d->mPhysicsShapeMesh.clear();

			std::vector<LLVector3>& pos = d->mPhysicsShapeMesh.mPositions;
			std::vector<LLVector3>& norm = d->mPhysicsShapeMesh.mNormals;

			for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
			{
				const LLVolumeFace& face = volume->getVolumeFace(i);
			
				for (S32 i = 0; i < face.mNumIndices; ++i)
				{
					U16 idx = face.mIndices[i];

					pos.push_back(LLVector3(face.mPositions[idx].getF32ptr()));
					norm.push_back(LLVector3(face.mNormals[idx].getF32ptr()));				
				}			
			}
		}
	}

	{
		LLMutexLock lock(mMutex);
		mDecompositionQ.push_back(d);
	}
	return true;
}

LLMeshUploadThread::LLMeshUploadThread(LLMeshUploadThread::instance_list& data, LLVector3& scale, bool upload_textures,
									   bool upload_skin, bool upload_joints, const std::string & upload_url, bool do_upload,
									   LLHandle<LLWholeModelFeeObserver> fee_observer,
									   LLHandle<LLWholeModelUploadObserver> upload_observer)
  : LLThread("mesh upload"),
	LLCore::HttpHandler(),
	mDiscarded(false),
	mDoUpload(do_upload),
	mWholeModelUploadURL(upload_url),
	mFeeObserverHandle(fee_observer),
	mUploadObserverHandle(upload_observer)
{
	mInstanceList = data;
	mUploadTextures = upload_textures;
	mUploadSkin = upload_skin;
	mUploadJoints = upload_joints;
	mMutex = new LLMutex(NULL);
	mPendingUploads = 0;
	mFinished = false;
	mOrigin = gAgent.getPositionAgent();
	mHost = gAgent.getRegionHost();
	
	mWholeModelFeeCapability = gAgent.getRegion()->getCapability("NewFileAgentInventory");

	mOrigin += gAgent.getAtAxis() * scale.magVec();

	mMeshUploadTimeOut = gSavedSettings.getS32("MeshUploadTimeOut");

	mHttpRequest = new LLCore::HttpRequest;
	mHttpOptions = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);
	mHttpOptions->setTransferTimeout(mMeshUploadTimeOut);
	mHttpOptions->setUseRetryAfter(gSavedSettings.getBOOL("MeshUseHttpRetryAfter"));
	mHttpOptions->setRetries(UPLOAD_RETRY_LIMIT);
	mHttpHeaders = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders);
	mHttpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_LLSD_XML);
	mHttpPolicyClass = LLAppViewer::instance()->getAppCoreHttp().getPolicy(LLAppCoreHttp::AP_UPLOADS);
	mHttpPriority = 0;
}

LLMeshUploadThread::~LLMeshUploadThread()
{
	delete mHttpRequest;
	mHttpRequest = NULL;
}

LLMeshUploadThread::DecompRequest::DecompRequest(LLModel* mdl, LLModel* base_model, LLMeshUploadThread* thread)
{
	mStage = "single_hull";
	mModel = mdl;
	mDecompID = &mdl->mDecompID;
	mBaseModel = base_model;
	mThread = thread;
	
	//copy out positions and indices
	assignData(mdl) ;	

	mThread->mFinalDecomp = this;
	mThread->mPhysicsComplete = false;
}

void LLMeshUploadThread::DecompRequest::completed()
{
	if (mThread->mFinalDecomp == this)
	{
		mThread->mPhysicsComplete = true;
	}

	llassert(mHull.size() == 1);
	
	mThread->mHullMap[mBaseModel] = mHull[0];
}

//called in the main thread.
void LLMeshUploadThread::preStart()
{
	//build map of LLModel refs to instances for callbacks
	for (instance_list::iterator iter = mInstanceList.begin(); iter != mInstanceList.end(); ++iter)
	{
		mInstance[iter->mModel].push_back(*iter);
	}
}

void LLMeshUploadThread::discard()
{
	LLMutexLock lock(mMutex);
	mDiscarded = true;
}

bool LLMeshUploadThread::isDiscarded() const
{
	LLMutexLock lock(mMutex);
	return mDiscarded;
}

void LLMeshUploadThread::run()
{
	if (mDoUpload)
	{
		doWholeModelUpload();
	}
	else
	{
		requestWholeModelFee();
	}
}

void dump_llsd_to_file(const LLSD& content, std::string filename)
{
	if (gSavedSettings.getBOOL("MeshUploadLogXML"))
	{
		std::ofstream of(filename.c_str());
		LLSDSerialize::toPrettyXML(content,of);
	}
}

LLSD llsd_from_file(std::string filename)
{
	std::ifstream ifs(filename.c_str());
	LLSD result;
	LLSDSerialize::fromXML(result,ifs);
	return result;
}

void LLMeshUploadThread::wholeModelToLLSD(LLSD& dest, bool include_textures)
{
	LLSD result;

	LLSD res;
	result["folder_id"] = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);
	result["texture_folder_id"] = gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE);
	result["asset_type"] = "mesh";
	result["inventory_type"] = "object";
	result["description"] = "(No Description)";
	result["next_owner_mask"] = LLSD::Integer(LLFloaterPerms::getNextOwnerPerms("Uploads"));
	result["group_mask"] = LLSD::Integer(LLFloaterPerms::getGroupPerms("Uploads"));
	result["everyone_mask"] = LLSD::Integer(LLFloaterPerms::getEveryonePerms("Uploads"));

	res["mesh_list"] = LLSD::emptyArray();
	res["texture_list"] = LLSD::emptyArray();
	res["instance_list"] = LLSD::emptyArray();
	S32 mesh_num = 0;
	S32 texture_num = 0;
	
	std::set<LLViewerTexture* > textures;
	std::map<LLViewerTexture*,S32> texture_index;

	std::map<LLModel*,S32> mesh_index;
	std::string model_name;
	std::string model_metric;

	S32 instance_num = 0;
	
	for (instance_map::iterator iter = mInstance.begin(); iter != mInstance.end(); ++iter)
	{
		LLMeshUploadData data;
		data.mBaseModel = iter->first;

		if (data.mBaseModel->mSubmodelID)
		{
			// These are handled below to insure correct parenting order on creation
			// due to map walking being based on model address (aka random)
			continue;
		}

		LLModelInstance& first_instance = *(iter->second.begin());
		for (S32 i = 0; i < 5; i++)
		{
			data.mModel[i] = first_instance.mLOD[i];
		}

		if (mesh_index.find(data.mBaseModel) == mesh_index.end())
		{
			// Have not seen this model before - create a new mesh_list entry for it.
			if (model_name.empty())
			{
				model_name = data.mBaseModel->getName();
			}

			if (model_metric.empty())
			{
				model_metric = data.mBaseModel->getMetric();
			}

			std::stringstream ostr;
			
			LLModel::Decomposition& decomp =
				data.mModel[LLModel::LOD_PHYSICS].notNull() ? 
				data.mModel[LLModel::LOD_PHYSICS]->mPhysics : 
				data.mBaseModel->mPhysics;

			decomp.mBaseHull = mHullMap[data.mBaseModel];

			LLSD mesh_header = LLModel::writeModel(
				ostr,  
				data.mModel[LLModel::LOD_PHYSICS],
				data.mModel[LLModel::LOD_HIGH],
				data.mModel[LLModel::LOD_MEDIUM],
				data.mModel[LLModel::LOD_LOW],
				data.mModel[LLModel::LOD_IMPOSTOR], 
				decomp,
				mUploadSkin,
				mUploadJoints,
				FALSE,
				FALSE,
				data.mBaseModel->mSubmodelID);

			data.mAssetData = ostr.str();
			std::string str = ostr.str();

			res["mesh_list"][mesh_num] = LLSD::Binary(str.begin(),str.end()); 
			mesh_index[data.mBaseModel] = mesh_num;
			mesh_num++;
		}

		// For all instances that use this model
		for (instance_list::iterator instance_iter = iter->second.begin();
			 instance_iter != iter->second.end();
			 ++instance_iter)
		{

			LLModelInstance& instance = *instance_iter;
		
			LLSD instance_entry;
		
			for (S32 i = 0; i < 5; i++)
			{
				data.mModel[i] = instance.mLOD[i];
			}
		
			LLVector3 pos, scale;
			LLQuaternion rot;
			LLMatrix4 transformation = instance.mTransform;
			decomposeMeshMatrix(transformation,pos,rot,scale);
			instance_entry["position"] = ll_sd_from_vector3(pos);
			instance_entry["rotation"] = ll_sd_from_quaternion(rot);
			instance_entry["scale"] = ll_sd_from_vector3(scale);
		
			instance_entry["material"] = LL_MCODE_WOOD;
			instance_entry["physics_shape_type"] = data.mModel[LLModel::LOD_PHYSICS].notNull() ? (U8)(LLViewerObject::PHYSICS_SHAPE_PRIM) : (U8)(LLViewerObject::PHYSICS_SHAPE_CONVEX_HULL);
			instance_entry["mesh"] = mesh_index[data.mBaseModel];

			instance_entry["face_list"] = LLSD::emptyArray();

			// We want to be able to allow more than 8 materials...
			//
			S32 end = llmin((S32)instance.mMaterial.size(), instance.mModel->getNumVolumeFaces()) ;

			for (S32 face_num = 0; face_num < end; face_num++)
			{
				LLImportMaterial& material = instance.mMaterial[data.mBaseModel->mMaterialList[face_num]];
				LLSD face_entry = LLSD::emptyMap();

				LLViewerFetchedTexture *texture = NULL;

				if (material.mDiffuseMapFilename.size())
				{
					texture = FindViewerTexture(material);
				}
				
				if ((texture != NULL) &&
					(textures.find(texture) == textures.end()))
				{
					textures.insert(texture);
				}

				std::stringstream texture_str;
				if (texture != NULL && include_textures && mUploadTextures)
				{
					if(texture->hasSavedRawImage())
					{											
						LLPointer<LLImageJ2C> upload_file =
							LLViewerTextureList::convertToUploadFile(texture->getSavedRawImage());

						if (!upload_file.isNull() && upload_file->getDataSize())
						{
						texture_str.write((const char*) upload_file->getData(), upload_file->getDataSize());
					}
				}
				}

				if (texture != NULL &&
					mUploadTextures &&
					texture_index.find(texture) == texture_index.end())
				{
					texture_index[texture] = texture_num;
					std::string str = texture_str.str();
					res["texture_list"][texture_num] = LLSD::Binary(str.begin(),str.end());
					texture_num++;
				}

				// Subset of TextureEntry fields.
				if (texture != NULL && mUploadTextures)
				{
					face_entry["image"] = texture_index[texture];
					face_entry["scales"] = 1.0;
					face_entry["scalet"] = 1.0;
					face_entry["offsets"] = 0.0;
					face_entry["offsett"] = 0.0;
					face_entry["imagerot"] = 0.0;
				}
				face_entry["diffuse_color"] = ll_sd_from_color4(material.mDiffuseColor);
				face_entry["fullbright"] = material.mFullbright;
				instance_entry["face_list"][face_num] = face_entry;
		    }

			res["instance_list"][instance_num] = instance_entry;
			instance_num++;
		}
	}

	for (instance_map::iterator iter = mInstance.begin(); iter != mInstance.end(); ++iter)
	{
		LLMeshUploadData data;
		data.mBaseModel = iter->first;

		if (!data.mBaseModel->mSubmodelID)
		{
			// These were handled above already...
			//
			continue;
		}

		LLModelInstance& first_instance = *(iter->second.begin());
		for (S32 i = 0; i < 5; i++)
		{
			data.mModel[i] = first_instance.mLOD[i];
		}

		if (mesh_index.find(data.mBaseModel) == mesh_index.end())
		{
			// Have not seen this model before - create a new mesh_list entry for it.
			if (model_name.empty())
			{
				model_name = data.mBaseModel->getName();
			}

			if (model_metric.empty())
			{
				model_metric = data.mBaseModel->getMetric();
			}

			std::stringstream ostr;
			
			LLModel::Decomposition& decomp =
				data.mModel[LLModel::LOD_PHYSICS].notNull() ? 
				data.mModel[LLModel::LOD_PHYSICS]->mPhysics : 
				data.mBaseModel->mPhysics;

			decomp.mBaseHull = mHullMap[data.mBaseModel];

			LLSD mesh_header = LLModel::writeModel(
				ostr,  
				data.mModel[LLModel::LOD_PHYSICS],
				data.mModel[LLModel::LOD_HIGH],
				data.mModel[LLModel::LOD_MEDIUM],
				data.mModel[LLModel::LOD_LOW],
				data.mModel[LLModel::LOD_IMPOSTOR], 
				decomp,
				mUploadSkin,
				mUploadJoints,
				FALSE,
				FALSE,
				data.mBaseModel->mSubmodelID);

			data.mAssetData = ostr.str();
			std::string str = ostr.str();

			res["mesh_list"][mesh_num] = LLSD::Binary(str.begin(),str.end()); 
			mesh_index[data.mBaseModel] = mesh_num;
			mesh_num++;
		}

		// For all instances that use this model
		for (instance_list::iterator instance_iter = iter->second.begin();
			 instance_iter != iter->second.end();
			 ++instance_iter)
		{

			LLModelInstance& instance = *instance_iter;
		
			LLSD instance_entry;
		
			for (S32 i = 0; i < 5; i++)
			{
				data.mModel[i] = instance.mLOD[i];
			}
		
			LLVector3 pos, scale;
			LLQuaternion rot;
			LLMatrix4 transformation = instance.mTransform;
			decomposeMeshMatrix(transformation,pos,rot,scale);
			instance_entry["position"] = ll_sd_from_vector3(pos);
			instance_entry["rotation"] = ll_sd_from_quaternion(rot);
			instance_entry["scale"] = ll_sd_from_vector3(scale);
		
			instance_entry["material"] = LL_MCODE_WOOD;
			instance_entry["physics_shape_type"] = (U8)(LLViewerObject::PHYSICS_SHAPE_NONE);
			instance_entry["mesh"] = mesh_index[data.mBaseModel];

			instance_entry["face_list"] = LLSD::emptyArray();

			// We want to be able to allow more than 8 materials...
			//
			S32 end = llmin((S32)instance.mMaterial.size(), instance.mModel->getNumVolumeFaces()) ;

			for (S32 face_num = 0; face_num < end; face_num++)
			{
				LLImportMaterial& material = instance.mMaterial[data.mBaseModel->mMaterialList[face_num]];
				LLSD face_entry = LLSD::emptyMap();

				LLViewerFetchedTexture *texture = NULL;

				if (material.mDiffuseMapFilename.size())
				{
					texture = FindViewerTexture(material);
				}

				if ((texture != NULL) &&
					(textures.find(texture) == textures.end()))
				{
					textures.insert(texture);
				}

				std::stringstream texture_str;
				if (texture != NULL && include_textures && mUploadTextures)
				{
					if(texture->hasSavedRawImage())
					{											
						LLPointer<LLImageJ2C> upload_file =
							LLViewerTextureList::convertToUploadFile(texture->getSavedRawImage());

						if (!upload_file.isNull() && upload_file->getDataSize())
						{
						texture_str.write((const char*) upload_file->getData(), upload_file->getDataSize());
					}
				}
				}

				if (texture != NULL &&
					mUploadTextures &&
					texture_index.find(texture) == texture_index.end())
				{
					texture_index[texture] = texture_num;
					std::string str = texture_str.str();
					res["texture_list"][texture_num] = LLSD::Binary(str.begin(),str.end());
					texture_num++;
				}

				// Subset of TextureEntry fields.
				if (texture != NULL && mUploadTextures)
				{
					face_entry["image"] = texture_index[texture];
					face_entry["scales"] = 1.0;
					face_entry["scalet"] = 1.0;
					face_entry["offsets"] = 0.0;
					face_entry["offsett"] = 0.0;
					face_entry["imagerot"] = 0.0;
				}
				face_entry["diffuse_color"] = ll_sd_from_color4(material.mDiffuseColor);
				face_entry["fullbright"] = material.mFullbright;
				instance_entry["face_list"][face_num] = face_entry;
		    }

			res["instance_list"][instance_num] = instance_entry;
			instance_num++;
		}
	}

	if (model_name.empty()) model_name = "mesh model";
	result["name"] = model_name;
	if (model_metric.empty()) model_metric = "MUT_Unspecified";
	res["metric"] = model_metric;
	result["asset_resources"] = res;
	dump_llsd_to_file(result,make_dump_name("whole_model_",dump_num));

	dest = result;
}

void LLMeshUploadThread::generateHulls()
{
	bool has_valid_requests = false ;

	for (instance_map::iterator iter = mInstance.begin(); iter != mInstance.end(); ++iter)
	{
		LLMeshUploadData data;
		data.mBaseModel = iter->first;

		LLModelInstance& instance = *(iter->second.begin());

		for (S32 i = 0; i < 5; i++)
		{
			data.mModel[i] = instance.mLOD[i];
		}

		//queue up models for hull generation
		LLModel* physics = NULL;

		if (data.mModel[LLModel::LOD_PHYSICS].notNull())
		{
			physics = data.mModel[LLModel::LOD_PHYSICS];
		}
		else if (data.mModel[LLModel::LOD_LOW].notNull())
		{
			physics = data.mModel[LLModel::LOD_LOW];
		}
		else if (data.mModel[LLModel::LOD_MEDIUM].notNull())
		{
			physics = data.mModel[LLModel::LOD_MEDIUM];
		}
		else
		{
			physics = data.mModel[LLModel::LOD_HIGH];
		}

		llassert(physics != NULL);

		DecompRequest* request = new DecompRequest(physics, data.mBaseModel, this);
		if(request->isValid())
		{
			gMeshRepo.mDecompThread->submitRequest(request);
			has_valid_requests = true ;
		}
	}
		
	if (has_valid_requests)
	{
		// *NOTE:  Interesting livelock condition on shutdown.  If there
		// is an upload request in generateHulls() when shutdown starts,
		// the main thread isn't available to manage communication between
		// the decomposition thread and the upload thread and this loop
		// wouldn't complete in turn stalling the main thread.  The check
		// on isDiscarded() prevents that.
		while (! mPhysicsComplete && ! isDiscarded())
		{
			apr_sleep(100);
		}
	}	
}

void LLMeshUploadThread::doWholeModelUpload()
{
	LL_DEBUGS(LOG_MESH) << "Starting model upload.  Instances:  " << mInstance.size() << LL_ENDL;

	if (mWholeModelUploadURL.empty())
	{
		LL_WARNS(LOG_MESH) << "Missing mesh upload capability, unable to upload, fee request failed."
						   << LL_ENDL;
	}
	else
	{
		generateHulls();
		LL_DEBUGS(LOG_MESH) << "Hull generation completed." << LL_ENDL;

		mModelData = LLSD::emptyMap();
		wholeModelToLLSD(mModelData, true);
		LLSD body = mModelData["asset_resources"];

		dump_llsd_to_file(body, make_dump_name("whole_model_body_", dump_num));

		LLCore::HttpHandle handle = LLCoreHttpUtil::requestPostWithLLSD(mHttpRequest,
																		mHttpPolicyClass,
																		mHttpPriority,
																		mWholeModelUploadURL,
																		body,
																		mHttpOptions,
																		mHttpHeaders,
                                                                        LLCore::HttpHandler::ptr_t(this, &NoOpDeletor));
		if (LLCORE_HTTP_HANDLE_INVALID == handle)
		{
			mHttpStatus = mHttpRequest->getStatus();
		
			LL_WARNS(LOG_MESH) << "Couldn't issue request for full model upload.  Reason:  " << mHttpStatus.toString()
							   << " (" << mHttpStatus.toTerseString() << ")"
							   << LL_ENDL;
		}
		else
		{
			U32 sleep_time(10);
		
			LL_DEBUGS(LOG_MESH) << "POST request issued." << LL_ENDL;
			
			mHttpRequest->update(0);
			while (! LLApp::isQuitting() && ! finished() && ! isDiscarded())
			{
				ms_sleep(sleep_time);
				sleep_time = llmin(250U, sleep_time + sleep_time);
				mHttpRequest->update(0);
			}

			if (isDiscarded())
			{
				LL_DEBUGS(LOG_MESH) << "Mesh upload operation discarded." << LL_ENDL;
			}
			else
			{
				LL_DEBUGS(LOG_MESH) << "Mesh upload operation completed." << LL_ENDL;
			}
		}
	}
}

void LLMeshUploadThread::requestWholeModelFee()
{
	dump_num++;

	generateHulls();

	mModelData = LLSD::emptyMap();
	wholeModelToLLSD(mModelData, false);
	dump_llsd_to_file(mModelData, make_dump_name("whole_model_fee_request_", dump_num));
	LLCore::HttpHandle handle = LLCoreHttpUtil::requestPostWithLLSD(mHttpRequest,
																	mHttpPolicyClass,
																	mHttpPriority,
																	mWholeModelFeeCapability,
																	mModelData,
																	mHttpOptions,
																	mHttpHeaders,
                                                                    LLCore::HttpHandler::ptr_t(this, &NoOpDeletor));
	if (LLCORE_HTTP_HANDLE_INVALID == handle)
	{
		mHttpStatus = mHttpRequest->getStatus();
		
		LL_WARNS(LOG_MESH) << "Couldn't issue request for model fee.  Reason:  " << mHttpStatus.toString()
						   << " (" << mHttpStatus.toTerseString() << ")"
						   << LL_ENDL;
	}
	else
	{
		U32 sleep_time(10);
		
		mHttpRequest->update(0);
		while (! LLApp::isQuitting() && ! finished() && ! isDiscarded())
		{
			ms_sleep(sleep_time);
			sleep_time = llmin(250U, sleep_time + sleep_time);
			mHttpRequest->update(0);
		}
		if (isDiscarded())
		{
			LL_DEBUGS(LOG_MESH) << "Mesh fee query operation discarded." << LL_ENDL;
		}
	}
}


// Does completion duty for both fee queries and actual uploads.
void LLMeshUploadThread::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
{
	// QA/Devel:  0x2 to enable fake error import on upload, 0x1 on fee check
	const S32 fake_error(gSavedSettings.getS32("MeshUploadFakeErrors") & (mDoUpload ? 0xa : 0x5));
	LLCore::HttpStatus status(response->getStatus());
	if (fake_error)
	{
		status = (fake_error & 0x0c) ? LLCore::HttpStatus(500) : LLCore::HttpStatus(200);
	}
	std::string reason(status.toString());
	LLSD body;

	mFinished = true;

	if (mDoUpload)
	{
		// model upload case
		LLWholeModelUploadObserver * observer(mUploadObserverHandle.get());

		if (! status)
		{
			LL_WARNS(LOG_MESH) << "Upload failed.  Reason:  " << reason
							   << " (" << status.toTerseString() << ")"
							   << LL_ENDL;

			// Build a fake body for the alert generator
			body["error"] = LLSD::emptyMap();
			body["error"]["message"] = reason;
			body["error"]["identifier"] = "NetworkError";		// from asset-upload/upload_util.py
			log_upload_error(status, body, "upload", mModelData["name"].asString());

			if (observer)
			{
				doOnIdleOneTime(boost::bind(&LLWholeModelUploadObserver::onModelUploadFailure, observer));
			}
		}
		else
		{
			if (fake_error & 0x2)
			{
				body = llsd_from_file("fake_upload_error.xml");
			}
			else
			{
				// *TODO:  handle error in conversion process
				LLCoreHttpUtil::responseToLLSD(response, true, body);
			}
			dump_llsd_to_file(body, make_dump_name("whole_model_upload_response_", dump_num));

			if (body["state"].asString() == "complete")
			{
				// requested "mesh" asset type isn't actually the type
				// of the resultant object, fix it up here.
				mModelData["asset_type"] = "object";
				gMeshRepo.updateInventory(LLMeshRepository::inventory_data(mModelData, body));

				if (observer)
				{
					doOnIdleOneTime(boost::bind(&LLWholeModelUploadObserver::onModelUploadSuccess, observer));
				}
			}
			else
			{
				LL_WARNS(LOG_MESH) << "Upload failed.  Not in expected 'complete' state." << LL_ENDL;
				log_upload_error(status, body, "upload", mModelData["name"].asString());

				if (observer)
				{
					doOnIdleOneTime(boost::bind(&LLWholeModelUploadObserver::onModelUploadFailure, observer));
				}
			}
		}
	}
	else
	{
		// model fee case
		LLWholeModelFeeObserver* observer(mFeeObserverHandle.get());
		mWholeModelUploadURL.clear();
		
		if (! status)
		{
			LL_WARNS(LOG_MESH) << "Fee request failed.  Reason:  " << reason
							   << " (" << status.toTerseString() << ")"
							   << LL_ENDL;

			// Build a fake body for the alert generator
			body["error"] = LLSD::emptyMap();
			body["error"]["message"] = reason;
			body["error"]["identifier"] = "NetworkError";		// from asset-upload/upload_util.py
			log_upload_error(status, body, "fee", mModelData["name"].asString());

			if (observer)
			{
				observer->setModelPhysicsFeeErrorStatus(status.toULong(), reason);
			}
		}
		else
		{
			if (fake_error & 0x1)
			{
				body = llsd_from_file("fake_upload_error.xml");
			}
			else
			{
				// *TODO:  handle error in conversion process
				LLCoreHttpUtil::responseToLLSD(response, true, body);
			}
			dump_llsd_to_file(body, make_dump_name("whole_model_fee_response_", dump_num));
		
			if (body["state"].asString() == "upload")
			{
				mWholeModelUploadURL = body["uploader"].asString();

				if (observer)
				{
					body["data"]["upload_price"] = body["upload_price"];
					observer->onModelPhysicsFeeReceived(body["data"], mWholeModelUploadURL);
				}
			}
			else
			{
				LL_WARNS(LOG_MESH) << "Fee request failed.  Not in expected 'upload' state." << LL_ENDL;
				log_upload_error(status, body, "fee", mModelData["name"].asString());

				if (observer)
				{
					observer->setModelPhysicsFeeErrorStatus(status.toULong(), reason);
				}
			}
		}
	}
}


void LLMeshRepoThread::notifyLoadedMeshes()
{
	bool update_metrics(false);
	
	if (!mMutex)
	{
		return;
	}

	while (!mLoadedQ.empty())
	{
		mMutex->lock();
		if (mLoadedQ.empty())
		{
			mMutex->unlock();
			break;
		}
		LoadedMesh mesh = mLoadedQ.front();
		mLoadedQ.pop();
		mMutex->unlock();
		
		update_metrics = true;
		if (mesh.mVolume && mesh.mVolume->getNumVolumeFaces() > 0)
		{
			gMeshRepo.notifyMeshLoaded(mesh.mMeshParams, mesh.mVolume);
		}
		else
		{
			gMeshRepo.notifyMeshUnavailable(mesh.mMeshParams, 
				LLVolumeLODGroup::getVolumeDetailFromScale(mesh.mVolume->getDetail()));
		}
	}

	while (!mUnavailableQ.empty())
	{
		mMutex->lock();
		if (mUnavailableQ.empty())
		{
			mMutex->unlock();
			break;
		}
		
		LODRequest req = mUnavailableQ.front();
		mUnavailableQ.pop();
		mMutex->unlock();

		update_metrics = true;
		gMeshRepo.notifyMeshUnavailable(req.mMeshParams, req.mLOD);
	}

	if (! mSkinInfoQ.empty() || ! mDecompositionQ.empty())
	{
		if (mMutex->trylock())
		{
			std::list<LLMeshSkinInfo> skin_info_q;
			std::list<LLModel::Decomposition*> decomp_q;

			if (! mSkinInfoQ.empty())
			{
				skin_info_q.swap(mSkinInfoQ);
			}
			if (! mDecompositionQ.empty())
			{
				decomp_q.swap(mDecompositionQ);
			}

			mMutex->unlock();

			// Process the elements free of the lock
			while (! skin_info_q.empty())
			{
				gMeshRepo.notifySkinInfoReceived(skin_info_q.front());
				skin_info_q.pop_front();
			}

			while (! decomp_q.empty())
			{
				gMeshRepo.notifyDecompositionReceived(decomp_q.front());
				decomp_q.pop_front();
			}
		}
	}

	if (update_metrics)
	{
		// Ping time-to-load metrics for mesh download operations.
		LLMeshRepository::metricsProgress(0);
	}
	
}

S32 LLMeshRepoThread::getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod) 
{ //only ever called from main thread
	LLMutexLock lock(mHeaderMutex);
	mesh_header_map::iterator iter = mMeshHeader.find(mesh_params.getSculptID());

	if (iter != mMeshHeader.end())
	{
		LLSD& header = iter->second;

		return LLMeshRepository::getActualMeshLOD(header, lod);
	}

	return lod;
}

//static
S32 LLMeshRepository::getActualMeshLOD(LLSD& header, S32 lod)
{
	lod = llclamp(lod, 0, 3);

	S32 version = header["version"];

	if (header.has("404") || version > MAX_MESH_VERSION)
	{
		return -1;
	}

	if (header[header_lod[lod]]["size"].asInteger() > 0)
	{
		return lod;
	}

	//search down to find the next available lower lod
	for (S32 i = lod-1; i >= 0; --i)
	{
		if (header[header_lod[i]]["size"].asInteger() > 0)
		{
			return i;
		}
	}

	//search up to find then ext available higher lod
	for (S32 i = lod+1; i < 4; ++i)
	{
		if (header[header_lod[i]]["size"].asInteger() > 0)
		{
			return i;
		}
	}

	//header exists and no good lod found, treat as 404
	header["404"] = 1;
	return -1;
}

void LLMeshRepository::cacheOutgoingMesh(LLMeshUploadData& data, LLSD& header)
{
	mThread->mMeshHeader[data.mUUID] = header;

	// we cache the mesh for default parameters
	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
	volume_params.setSculptID(data.mUUID, LL_SCULPT_TYPE_MESH);

	for (U32 i = 0; i < 4; i++)
	{
		if (data.mModel[i].notNull())
		{
			LLPointer<LLVolume> volume = new LLVolume(volume_params, LLVolumeLODGroup::getVolumeScaleFromDetail(i));
			volume->copyVolumeFaces(data.mModel[i]);
			volume->setMeshAssetLoaded(TRUE);
		}
	}

}

// Handle failed or successful requests for mesh assets.
//
// Support for 200 responses was added for several reasons.  One,
// a service or cache can ignore range headers and give us a
// 200 with full asset should it elect to.  We also support
// a debug flag which disables range requests for those very
// few users that have some sort of problem with their networking
// services.  But the 200 response handling is suboptimal:  rather
// than cache the whole asset, we just extract the part that would
// have been sent in a 206 and process that.  Inefficient but these
// are cases far off the norm.
void LLMeshHandlerBase::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
{
	mProcessed = true;

	unsigned int retries(0U);
	response->getRetries(NULL, &retries);
	LLMeshRepository::sHTTPRetryCount += retries;

	LLCore::HttpStatus status(response->getStatus());
	if (! status || MESH_HTTP_RESPONSE_FAILED)
	{
		processFailure(status);
		++LLMeshRepository::sHTTPErrorCount;
	}
	else
	{
		// From texture fetch code and may apply here:
		//
		// A warning about partial (HTTP 206) data.  Some grid services
		// do *not* return a 'Content-Range' header in the response to
		// Range requests with a 206 status.  We're forced to assume
		// we get what we asked for in these cases until we can fix
		// the services.
		//
		// May also need to deal with 200 status (full asset returned
		// rather than partial) and 416 (request completely unsatisfyable).
		// Always been exposed to these but are less likely here where
		// speculative loads aren't done.
		LLCore::BufferArray * body(response->getBody());
		S32 body_offset(0);
		U8 * data(NULL);
		S32 data_size(body ? body->size() : 0);

		if (data_size > 0)
		{
			static const LLCore::HttpStatus par_status(HTTP_PARTIAL_CONTENT);
			
			unsigned int offset(0), length(0), full_length(0);
				
			if (par_status == status)
			{
				// 206 case
				response->getRange(&offset, &length, &full_length);
				if (! offset && ! length)
				{
					// This is the case where we receive a 206 status but
					// there wasn't a useful Content-Range header in the response.
					// This could be because it was badly formatted but is more
					// likely due to capabilities services which scrub headers
					// from responses.  Assume we got what we asked for...`
					// length = data_size;
					offset = mOffset;
				}
			}
			else
			{
				// 200 case, typically
				offset = 0;
			}
		
			// *DEBUG:  To test validation below
			// offset += 1;

			// Validate that what we think we received is consistent with
			// what we've asked for.  I.e. first byte we wanted lies somewhere
			// in the response.
			if (offset > mOffset
				|| (offset + data_size) <= mOffset
				|| (mOffset - offset) >= data_size)
			{
				// No overlap with requested range.  Fail request with
				// suitable error.  Shouldn't happen unless server/cache/ISP
				// is doing something awful.
				LL_WARNS(LOG_MESH) << "Mesh response (bytes ["
								   << offset << ".." << (offset + length - 1)
								   << "]) didn't overlap with request's origin (bytes ["
								   << mOffset << ".." << (mOffset + mRequestedBytes - 1)
								   << "])." << LL_ENDL;
				processFailure(LLCore::HttpStatus(LLCore::HttpStatus::LLCORE, LLCore::HE_INV_CONTENT_RANGE_HDR));
				++LLMeshRepository::sHTTPErrorCount;
				goto common_exit;
			}
			
			// *TODO: Try to get rid of data copying and add interfaces
			// that support BufferArray directly.  Introduce a two-phase
			// handler, optional first that takes a body, fallback second
			// that requires a temporary allocation and data copy.
			body_offset = mOffset - offset;
			data = new U8[data_size - body_offset];
			body->read(body_offset, (char *) data, data_size - body_offset);
			LLMeshRepository::sBytesReceived += data_size;
		}

		processData(body, body_offset, data, data_size - body_offset);

		delete [] data;
	}

	// Release handler
common_exit:
	gMeshRepo.mThread->mHttpRequestSet.erase(this->shared_from_this());
}


LLMeshHeaderHandler::~LLMeshHeaderHandler()
{
	if (!LLApp::isQuitting())
	{
		if (! mProcessed)
		{
			// something went wrong, retry
			LL_WARNS(LOG_MESH) << "Mesh header fetch canceled unexpectedly, retrying." << LL_ENDL;
			LLMeshRepoThread::HeaderRequest req(mMeshParams);
			LLMutexLock lock(gMeshRepo.mThread->mMutex);
			gMeshRepo.mThread->mHeaderReqQ.push(req);
		}
		LLMeshRepoThread::decActiveHeaderRequests();
	}
}

void LLMeshHeaderHandler::processFailure(LLCore::HttpStatus status)
{
	LL_WARNS(LOG_MESH) << "Error during mesh header handling.  ID:  " << mMeshParams.getSculptID()
					   << ", Reason:  " << status.toString()
					   << " (" << status.toTerseString() << ").  Not retrying."
					   << LL_ENDL;

	// Can't get the header so none of the LODs will be available
	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	for (int i(0); i < 4; ++i)
	{
		gMeshRepo.mThread->mUnavailableQ.push(LLMeshRepoThread::LODRequest(mMeshParams, i));
	}
}

void LLMeshHeaderHandler::processData(LLCore::BufferArray * /* body */, S32 /* body_offset */,
									  U8 * data, S32 data_size)
{
	LLUUID mesh_id = mMeshParams.getSculptID();
	bool success = (! MESH_HEADER_PROCESS_FAILED) && gMeshRepo.mThread->headerReceived(mMeshParams, data, data_size);
	llassert(success);
	if (! success)
	{
		// *TODO:  Get real reason for parse failure here.  Might we want to retry?
		LL_WARNS(LOG_MESH) << "Unable to parse mesh header.  ID:  " << mesh_id
						   << ", Unknown reason.  Not retrying."
						   << LL_ENDL;

		// Can't get the header so none of the LODs will be available
		LLMutexLock lock(gMeshRepo.mThread->mMutex);
		for (int i(0); i < 4; ++i)
		{
			gMeshRepo.mThread->mUnavailableQ.push(LLMeshRepoThread::LODRequest(mMeshParams, i));
		}
	}
	else if (data && data_size > 0)
	{
		// header was successfully retrieved from sim and parsed, cache in vfs
		S32 header_bytes = 0;
		LLSD header;

		gMeshRepo.mThread->mHeaderMutex->lock();
		LLMeshRepoThread::mesh_header_map::iterator iter = gMeshRepo.mThread->mMeshHeader.find(mesh_id);
		if (iter != gMeshRepo.mThread->mMeshHeader.end())
		{
			header_bytes = (S32)gMeshRepo.mThread->mMeshHeaderSize[mesh_id];
			header = iter->second;
		}
		gMeshRepo.mThread->mHeaderMutex->unlock();

		if (header_bytes > 0
			&& !header.has("404")
			&& header.has("version")
			&& header["version"].asInteger() <= MAX_MESH_VERSION)
		{
			std::stringstream str;

			S32 lod_bytes = 0;

			for (U32 i = 0; i < LLModel::LOD_PHYSICS; ++i)
			{
				// figure out how many bytes we'll need to reserve in the file
				const std::string & lod_name = header_lod[i];
				lod_bytes = llmax(lod_bytes, header[lod_name]["offset"].asInteger()+header[lod_name]["size"].asInteger());
			}
		
			// just in case skin info or decomposition is at the end of the file (which it shouldn't be)
			lod_bytes = llmax(lod_bytes, header["skin"]["offset"].asInteger() + header["skin"]["size"].asInteger());
			lod_bytes = llmax(lod_bytes, header["physics_convex"]["offset"].asInteger() + header["physics_convex"]["size"].asInteger());

			S32 header_bytes = (S32) gMeshRepo.mThread->mMeshHeaderSize[mesh_id];
			S32 bytes = lod_bytes + header_bytes; 

		
			// It's possible for the remote asset to have more data than is needed for the local cache
			// only allocate as much space in the VFS as is needed for the local cache
			data_size = llmin(data_size, bytes);

			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH, LLVFile::WRITE);
			if (file.getMaxSize() >= bytes || file.setMaxSize(bytes))
			{
				LLMeshRepository::sCacheBytesWritten += data_size;
				++LLMeshRepository::sCacheWrites;

				file.write(data, data_size);
			
				// zero out the rest of the file 
				U8 block[MESH_HEADER_SIZE];
				memset(block, 0, sizeof(block));

				while (bytes-file.tell() > sizeof(block))
				{
					file.write(block, sizeof(block));
				}

				S32 remaining = bytes-file.tell();
				if (remaining > 0)
				{
					file.write(block, remaining);
				}
			}
		}
		else
		{
			LL_WARNS(LOG_MESH) << "Trying to cache nonexistent mesh, mesh id: " << mesh_id << LL_ENDL;

			// headerReceived() parsed header, but header's data is invalid so none of the LODs will be available
			LLMutexLock lock(gMeshRepo.mThread->mMutex);
			for (int i(0); i < 4; ++i)
			{
				gMeshRepo.mThread->mUnavailableQ.push(LLMeshRepoThread::LODRequest(mMeshParams, i));
			}
		}
	}
}

LLMeshLODHandler::~LLMeshLODHandler()
{
	if (! LLApp::isQuitting())
	{
		if (! mProcessed)
		{
			LL_WARNS(LOG_MESH) << "Mesh LOD fetch canceled unexpectedly, retrying." << LL_ENDL;
			gMeshRepo.mThread->lockAndLoadMeshLOD(mMeshParams, mLOD);
		}
		LLMeshRepoThread::decActiveLODRequests();
	}
}

void LLMeshLODHandler::processFailure(LLCore::HttpStatus status)
{
	LL_WARNS(LOG_MESH) << "Error during mesh LOD handling.  ID:  " << mMeshParams.getSculptID()
					   << ", Reason:  " << status.toString()
					   << " (" << status.toTerseString() << ").  Not retrying."
					   << LL_ENDL;

	LLMutexLock lock(gMeshRepo.mThread->mMutex);
	gMeshRepo.mThread->mUnavailableQ.push(LLMeshRepoThread::LODRequest(mMeshParams, mLOD));
}

void LLMeshLODHandler::processData(LLCore::BufferArray * /* body */, S32 /* body_offset */,
								   U8 * data, S32 data_size)
{
	if ((! MESH_LOD_PROCESS_FAILED) && gMeshRepo.mThread->lodReceived(mMeshParams, mLOD, data, data_size))
	{
		// good fetch from sim, write to VFS for caching
		LLVFile file(gVFS, mMeshParams.getSculptID(), LLAssetType::AT_MESH, LLVFile::WRITE);

		S32 offset = mOffset;
		S32 size = mRequestedBytes;

		if (file.getSize() >= offset+size)
		{
			file.seek(offset);
			file.write(data, size);
			LLMeshRepository::sCacheBytesWritten += size;
			++LLMeshRepository::sCacheWrites;
		}
	}
	else
	{
		LL_WARNS(LOG_MESH) << "Error during mesh LOD processing.  ID:  " << mMeshParams.getSculptID()
						   << ", Unknown reason.  Not retrying."
						   << LL_ENDL;
		LLMutexLock lock(gMeshRepo.mThread->mMutex);
		gMeshRepo.mThread->mUnavailableQ.push(LLMeshRepoThread::LODRequest(mMeshParams, mLOD));
	}
}

LLMeshSkinInfoHandler::~LLMeshSkinInfoHandler()
{
	if (!mProcessed)
    {
        LL_WARNS(LOG_MESH) << "deleting unprocessed request handler (may be ok on exit)" << LL_ENDL;
    }
}

void LLMeshSkinInfoHandler::processFailure(LLCore::HttpStatus status)
{
	LL_WARNS(LOG_MESH) << "Error during mesh skin info handling.  ID:  " << mMeshID
					   << ", Reason:  " << status.toString()
					   << " (" << status.toTerseString() << ").  Not retrying."
					   << LL_ENDL;

	// *TODO:  Mark mesh unavailable on error.  For now, simply leave
	// request unfulfilled rather than retry forever.
}

void LLMeshSkinInfoHandler::processData(LLCore::BufferArray * /* body */, S32 /* body_offset */,
										U8 * data, S32 data_size)
{
	if ((! MESH_SKIN_INFO_PROCESS_FAILED) && gMeshRepo.mThread->skinInfoReceived(mMeshID, data, data_size))
	{
		// good fetch from sim, write to VFS for caching
		LLVFile file(gVFS, mMeshID, LLAssetType::AT_MESH, LLVFile::WRITE);

		S32 offset = mOffset;
		S32 size = mRequestedBytes;

		if (file.getSize() >= offset+size)
		{
			LLMeshRepository::sCacheBytesWritten += size;
			++LLMeshRepository::sCacheWrites;
			file.seek(offset);
			file.write(data, size);
		}
	}
	else
	{
		LL_WARNS(LOG_MESH) << "Error during mesh skin info processing.  ID:  " << mMeshID
						   << ", Unknown reason.  Not retrying."
						   << LL_ENDL;
		// *TODO:  Mark mesh unavailable on error
	}
}

LLMeshDecompositionHandler::~LLMeshDecompositionHandler()
{
	if (!mProcessed)
    {
        LL_WARNS(LOG_MESH) << "deleting unprocessed request handler (may be ok on exit)" << LL_ENDL;
    }
}

void LLMeshDecompositionHandler::processFailure(LLCore::HttpStatus status)
{
	LL_WARNS(LOG_MESH) << "Error during mesh decomposition handling.  ID:  " << mMeshID
					   << ", Reason:  " << status.toString()
					   << " (" << status.toTerseString() << ").  Not retrying."
					   << LL_ENDL;
	// *TODO:  Mark mesh unavailable on error.  For now, simply leave
	// request unfulfilled rather than retry forever.
}

void LLMeshDecompositionHandler::processData(LLCore::BufferArray * /* body */, S32 /* body_offset */,
											 U8 * data, S32 data_size)
{
	if ((! MESH_DECOMP_PROCESS_FAILED) && gMeshRepo.mThread->decompositionReceived(mMeshID, data, data_size))
	{
		// good fetch from sim, write to VFS for caching
		LLVFile file(gVFS, mMeshID, LLAssetType::AT_MESH, LLVFile::WRITE);

		S32 offset = mOffset;
		S32 size = mRequestedBytes;

		if (file.getSize() >= offset+size)
		{
			LLMeshRepository::sCacheBytesWritten += size;
			++LLMeshRepository::sCacheWrites;
			file.seek(offset);
			file.write(data, size);
		}
	}
	else
	{
		LL_WARNS(LOG_MESH) << "Error during mesh decomposition processing.  ID:  " << mMeshID
						   << ", Unknown reason.  Not retrying."
						   << LL_ENDL;
		// *TODO:  Mark mesh unavailable on error
	}
}

LLMeshPhysicsShapeHandler::~LLMeshPhysicsShapeHandler()
{
	if (!mProcessed)
    {
        LL_WARNS(LOG_MESH) << "deleting unprocessed request handler (may be ok on exit)" << LL_ENDL;
    }
}

void LLMeshPhysicsShapeHandler::processFailure(LLCore::HttpStatus status)
{
	LL_WARNS(LOG_MESH) << "Error during mesh physics shape handling.  ID:  " << mMeshID
					   << ", Reason:  " << status.toString()
					   << " (" << status.toTerseString() << ").  Not retrying."
					   << LL_ENDL;
	// *TODO:  Mark mesh unavailable on error
}

void LLMeshPhysicsShapeHandler::processData(LLCore::BufferArray * /* body */, S32 /* body_offset */,
											U8 * data, S32 data_size)
{
	if ((! MESH_PHYS_SHAPE_PROCESS_FAILED) && gMeshRepo.mThread->physicsShapeReceived(mMeshID, data, data_size))
	{
		// good fetch from sim, write to VFS for caching
		LLVFile file(gVFS, mMeshID, LLAssetType::AT_MESH, LLVFile::WRITE);

		S32 offset = mOffset;
		S32 size = mRequestedBytes;

		if (file.getSize() >= offset+size)
		{
			LLMeshRepository::sCacheBytesWritten += size;
			++LLMeshRepository::sCacheWrites;
			file.seek(offset);
			file.write(data, size);
		}
	}
	else
	{
		LL_WARNS(LOG_MESH) << "Error during mesh physics shape processing.  ID:  " << mMeshID
						   << ", Unknown reason.  Not retrying."
						   << LL_ENDL;
		// *TODO:  Mark mesh unavailable on error
	}
}

LLMeshRepository::LLMeshRepository()
: mMeshMutex(NULL),
  mMeshThreadCount(0),
  mThread(NULL),
  mGetMeshVersion(2)
{

}

void LLMeshRepository::init()
{
	mMeshMutex = new LLMutex(NULL);
	
	LLConvexDecomposition::getInstance()->initSystem();

	mDecompThread = new LLPhysicsDecomp();
	mDecompThread->start();

	while (!mDecompThread->mInited)
	{ //wait for physics decomp thread to init
		apr_sleep(100);
	}

	metrics_teleport_started_signal = LLViewerMessage::getInstance()->setTeleportStartedCallback(teleport_started);
	
	mThread = new LLMeshRepoThread();
	mThread->start();
}

void LLMeshRepository::shutdown()
{
	LL_INFOS(LOG_MESH) << "Shutting down mesh repository." << LL_ENDL;

	metrics_teleport_started_signal.disconnect();

	for (U32 i = 0; i < mUploads.size(); ++i)
	{
		LL_INFOS(LOG_MESH) << "Discard the pending mesh uploads." << LL_ENDL;
		mUploads[i]->discard() ; //discard the uploading requests.
	}

	mThread->mSignal->signal();
	
	while (!mThread->isStopped())
	{
		apr_sleep(10);
	}
	delete mThread;
	mThread = NULL;

	for (U32 i = 0; i < mUploads.size(); ++i)
	{
		LL_INFOS(LOG_MESH) << "Waiting for pending mesh upload " << (i + 1) << "/" << mUploads.size() << LL_ENDL;
		while (!mUploads[i]->isStopped())
		{
			apr_sleep(10);
		}
		delete mUploads[i];
	}

	mUploads.clear();

	delete mMeshMutex;
	mMeshMutex = NULL;

	LL_INFOS(LOG_MESH) << "Shutting down decomposition system." << LL_ENDL;

	if (mDecompThread)
	{
		mDecompThread->shutdown();		
		delete mDecompThread;
		mDecompThread = NULL;
	}

	LLConvexDecomposition::quitSystem();
}

//called in the main thread.
S32 LLMeshRepository::update()
{
	// Conditionally log a mesh metrics event
	metricsUpdate();
	
	if(mUploadWaitList.empty())
	{
		return 0 ;
	}

	S32 size = mUploadWaitList.size() ;
	for (S32 i = 0; i < size; ++i)
	{
		mUploads.push_back(mUploadWaitList[i]);
		mUploadWaitList[i]->preStart() ;
		mUploadWaitList[i]->start() ;
	}
	mUploadWaitList.clear() ;

	return size ;
}

S32 LLMeshRepository::loadMesh(LLVOVolume* vobj, const LLVolumeParams& mesh_params, S32 detail, S32 last_lod)
{
	LL_RECORD_BLOCK_TIME(FTM_MESH_FETCH);
	
	// Manage time-to-load metrics for mesh download operations.
	metricsProgress(1);

	if (detail < 0 || detail >= 4)
	{
		return detail;
	}

	{
		LLMutexLock lock(mMeshMutex);
		//add volume to list of loading meshes
		mesh_load_map::iterator iter = mLoadingMeshes[detail].find(mesh_params);
		if (iter != mLoadingMeshes[detail].end())
		{ //request pending for this mesh, append volume id to list
			iter->second.insert(vobj->getID());
		}
		else
		{
			//first request for this mesh
			mLoadingMeshes[detail][mesh_params].insert(vobj->getID());
			mPendingRequests.push_back(LLMeshRepoThread::LODRequest(mesh_params, detail));
			LLMeshRepository::sLODPending++;
		}
	}

	//do a quick search to see if we can't display something while we wait for this mesh to load
	LLVolume* volume = vobj->getVolume();

	if (volume)
	{
		LLVolumeParams params = volume->getParams();

		LLVolumeLODGroup* group = LLPrimitive::getVolumeManager()->getGroup(params);

		if (group)
		{
			//first, see if last_lod is available (don't transition down to avoid funny popping a la SH-641)
			if (last_lod >= 0)
			{
				LLVolume* lod = group->refLOD(last_lod);
				if (lod && lod->isMeshAssetLoaded() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return last_lod;
				}
				group->derefLOD(lod);
			}

			//next, see what the next lowest LOD available might be
			for (S32 i = detail-1; i >= 0; --i)
			{
				LLVolume* lod = group->refLOD(i);
				if (lod && lod->isMeshAssetLoaded() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return i;
				}

				group->derefLOD(lod);
			}

			//no lower LOD is a available, is a higher lod available?
			for (S32 i = detail+1; i < 4; ++i)
			{
				LLVolume* lod = group->refLOD(i);
				if (lod && lod->isMeshAssetLoaded() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return i;
				}

				group->derefLOD(lod);
			}
		}
	}

	return detail;
}

void LLMeshRepository::notifyLoadedMeshes()
{ //called from main thread
	LL_RECORD_BLOCK_TIME(FTM_MESH_FETCH);

	if (1 == mGetMeshVersion)
	{
		// Legacy GetMesh operation with high connection concurrency
		LLMeshRepoThread::sMaxConcurrentRequests = gSavedSettings.getU32("MeshMaxConcurrentRequests");
		LLMeshRepoThread::sRequestHighWater = llclamp(2 * S32(LLMeshRepoThread::sMaxConcurrentRequests),
													  REQUEST_HIGH_WATER_MIN,
													  REQUEST_HIGH_WATER_MAX);
		LLMeshRepoThread::sRequestLowWater = llclamp(LLMeshRepoThread::sRequestHighWater / 2,
													 REQUEST_LOW_WATER_MIN,
													 REQUEST_LOW_WATER_MAX);
	}
	else
	{
		// GetMesh2 operation with keepalives, etc.  With pipelining,
		// we'll increase this.  See llappcorehttp and llcorehttp for
		// discussion on connection strategies.
		LLAppCoreHttp & app_core_http(LLAppViewer::instance()->getAppCoreHttp());
		S32 scale(app_core_http.isPipelined(LLAppCoreHttp::AP_MESH2)
				  ? (2 * LLAppCoreHttp::PIPELINING_DEPTH)
				  : 5);

		LLMeshRepoThread::sMaxConcurrentRequests = gSavedSettings.getU32("Mesh2MaxConcurrentRequests");
		LLMeshRepoThread::sRequestHighWater = llclamp(scale * S32(LLMeshRepoThread::sMaxConcurrentRequests),
													  REQUEST2_HIGH_WATER_MIN,
													  REQUEST2_HIGH_WATER_MAX);
		LLMeshRepoThread::sRequestLowWater = llclamp(LLMeshRepoThread::sRequestHighWater / 2,
													 REQUEST2_LOW_WATER_MIN,
													 REQUEST2_LOW_WATER_MAX);
	}
	
	//clean up completed upload threads
	for (std::vector<LLMeshUploadThread*>::iterator iter = mUploads.begin(); iter != mUploads.end(); )
	{
		LLMeshUploadThread* thread = *iter;

		if (thread->isStopped() && thread->finished())
		{
			iter = mUploads.erase(iter);
			delete thread;
		}
		else
		{
			++iter;
		}
	}

	//update inventory
	if (!mInventoryQ.empty())
	{
		LLMutexLock lock(mMeshMutex);
		while (!mInventoryQ.empty())
		{
			inventory_data& data = mInventoryQ.front();

			LLAssetType::EType asset_type = LLAssetType::lookup(data.mPostData["asset_type"].asString());
			LLInventoryType::EType inventory_type = LLInventoryType::lookup(data.mPostData["inventory_type"].asString());

			// Handle addition of texture, if any.
			if ( data.mResponse.has("new_texture_folder_id") )
			{
				const LLUUID& folder_id = data.mResponse["new_texture_folder_id"].asUUID();

				if ( folder_id.notNull() )
				{
					LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE);

					std::string name;
					// Check if the server built a different name for the texture folder
					if ( data.mResponse.has("new_texture_folder_name") )
					{
						name = data.mResponse["new_texture_folder_name"].asString();
					}
					else
					{
						name = data.mPostData["name"].asString();
					}

					// Add the category to the internal representation
					LLPointer<LLViewerInventoryCategory> cat = 
						new LLViewerInventoryCategory(folder_id, parent_id, 
							LLFolderType::FT_NONE, name, gAgent.getID());
					cat->setVersion(LLViewerInventoryCategory::VERSION_UNKNOWN);

					LLInventoryModel::LLCategoryUpdate update(cat->getParentUUID(), 1);
					gInventory.accountForUpdate(update);
					gInventory.updateCategory(cat);
				}
			}

			on_new_single_inventory_upload_complete(
				asset_type,
				inventory_type,
				data.mPostData["asset_type"].asString(),
				data.mPostData["folder_id"].asUUID(),
				data.mPostData["name"],
				data.mPostData["description"],
				data.mResponse,
				data.mResponse["upload_price"]);
			//}
			
			mInventoryQ.pop();
		}
	}

	//call completed callbacks on finished decompositions
	mDecompThread->notifyCompleted();

	// For major operations, attempt to get the required locks
	// without blocking and punt if they're not available.  The
	// longest run of holdoffs is kept in sMaxLockHoldoffs just
	// to collect the data.  In testing, I've never seen a value
	// greater than 2 (written to log on exit).
	{
		LLMutexTrylock lock1(mMeshMutex);
		LLMutexTrylock lock2(mThread->mMutex);

		static U32 hold_offs(0);
		if (! lock1.isLocked() || ! lock2.isLocked())
		{
			// If we can't get the locks, skip and pick this up later.
			++hold_offs;
			sMaxLockHoldoffs = llmax(sMaxLockHoldoffs, hold_offs);
			return;
		}
		hold_offs = 0;
		
		if (gAgent.getRegion())
		{
			// Update capability urls
			static std::string region_name("never name a region this");
			
			if (gAgent.getRegion()->getName() != region_name && gAgent.getRegion()->capabilitiesReceived())
			{
				region_name = gAgent.getRegion()->getName();
				const bool use_v1(gSavedSettings.getBOOL("MeshUseGetMesh1"));
				const std::string mesh1(gAgent.getRegion()->getCapability("GetMesh"));
				const std::string mesh2(gAgent.getRegion()->getCapability("GetMesh2"));
				mGetMeshVersion = (mesh2.empty() || use_v1) ? 1 : 2;
				mThread->setGetMeshCaps(mesh1, mesh2, mGetMeshVersion);
				LL_DEBUGS(LOG_MESH) << "Retrieving caps for region '" << region_name
									<< "', GetMesh2:  " << mesh2
									<< ", GetMesh:  " << mesh1
									<< ", using version:  " << mGetMeshVersion
									<< LL_ENDL;
			}
		}
		
		//popup queued error messages from background threads
		while (!mUploadErrorQ.empty())
		{
			LLNotificationsUtil::add("MeshUploadError", mUploadErrorQ.front());
			mUploadErrorQ.pop();
		}

		S32 active_count = LLMeshRepoThread::sActiveHeaderRequests + LLMeshRepoThread::sActiveLODRequests;
		if (active_count < LLMeshRepoThread::sRequestLowWater)
		{
			S32 push_count = LLMeshRepoThread::sRequestHighWater - active_count;

			if (mPendingRequests.size() > push_count)
			{
				// More requests than the high-water limit allows so
				// sort and forward the most important.

				//calculate "score" for pending requests

				//create score map
				std::map<LLUUID, F32> score_map;

				for (U32 i = 0; i < 4; ++i)
				{
					for (mesh_load_map::iterator iter = mLoadingMeshes[i].begin();  iter != mLoadingMeshes[i].end(); ++iter)
					{
						F32 max_score = 0.f;
						for (std::set<LLUUID>::iterator obj_iter = iter->second.begin(); obj_iter != iter->second.end(); ++obj_iter)
						{
							LLViewerObject* object = gObjectList.findObject(*obj_iter);
							
							if (object)
							{
								LLDrawable* drawable = object->mDrawable;
								if (drawable)
								{
									F32 cur_score = drawable->getRadius()/llmax(drawable->mDistanceWRTCamera, 1.f);
									max_score = llmax(max_score, cur_score);
								}
							}
						}
				
						score_map[iter->first.getSculptID()] = max_score;
					}
				}

				//set "score" for pending requests
				for (std::vector<LLMeshRepoThread::LODRequest>::iterator iter = mPendingRequests.begin(); iter != mPendingRequests.end(); ++iter)
				{
					iter->mScore = score_map[iter->mMeshParams.getSculptID()];
				}

				//sort by "score"
				std::partial_sort(mPendingRequests.begin(), mPendingRequests.begin() + push_count,
								  mPendingRequests.end(), LLMeshRepoThread::CompareScoreGreater());
			}

			while (!mPendingRequests.empty() && push_count > 0)
			{
				LLMeshRepoThread::LODRequest& request = mPendingRequests.front();
				mThread->loadMeshLOD(request.mMeshParams, request.mLOD);
				mPendingRequests.erase(mPendingRequests.begin());
				LLMeshRepository::sLODPending--;
				push_count--;
			}
		}

		//send skin info requests
		while (!mPendingSkinRequests.empty())
		{
			mThread->loadMeshSkinInfo(mPendingSkinRequests.front());
			mPendingSkinRequests.pop();
		}
	
		//send decomposition requests
		while (!mPendingDecompositionRequests.empty())
		{
			mThread->loadMeshDecomposition(mPendingDecompositionRequests.front());
			mPendingDecompositionRequests.pop();
		}
	
		//send physics shapes decomposition requests
		while (!mPendingPhysicsShapeRequests.empty())
		{
			mThread->loadMeshPhysicsShape(mPendingPhysicsShapeRequests.front());
			mPendingPhysicsShapeRequests.pop();
		}
	
		mThread->notifyLoadedMeshes();
	}

	mThread->mSignal->signal();
}

void LLMeshRepository::notifySkinInfoReceived(LLMeshSkinInfo& info)
{
	mSkinMap[info.mMeshID] = info;

	skin_load_map::iterator iter = mLoadingSkins.find(info.mMeshID);
	if (iter != mLoadingSkins.end())
	{
		for (std::set<LLUUID>::iterator obj_id = iter->second.begin(); obj_id != iter->second.end(); ++obj_id)
		{
			LLVOVolume* vobj = (LLVOVolume*) gObjectList.findObject(*obj_id);
			if (vobj)
			{
				vobj->notifyMeshLoaded();
			}
		}
		mLoadingSkins.erase(info.mMeshID);
	}
}

void LLMeshRepository::notifyDecompositionReceived(LLModel::Decomposition* decomp)
{
	decomposition_map::iterator iter = mDecompositionMap.find(decomp->mMeshID);
	if (iter == mDecompositionMap.end())
	{ //just insert decomp into map
		mDecompositionMap[decomp->mMeshID] = decomp;
		mLoadingDecompositions.erase(decomp->mMeshID);
	}
	else
	{ //merge decomp with existing entry
		iter->second->merge(decomp);
		mLoadingDecompositions.erase(decomp->mMeshID);
		delete decomp;
	}
}

void LLMeshRepository::notifyMeshLoaded(const LLVolumeParams& mesh_params, LLVolume* volume)
{ //called from main thread
	S32 detail = LLVolumeLODGroup::getVolumeDetailFromScale(volume->getDetail());

	//get list of objects waiting to be notified this mesh is loaded
	mesh_load_map::iterator obj_iter = mLoadingMeshes[detail].find(mesh_params);

	if (volume && obj_iter != mLoadingMeshes[detail].end())
	{
		//make sure target volume is still valid
		if (volume->getNumVolumeFaces() <= 0)
		{
			LL_WARNS(LOG_MESH) << "Mesh loading returned empty volume.  ID:  " << mesh_params.getSculptID()
							   << LL_ENDL;
		}
		
		{ //update system volume
			LLVolume* sys_volume = LLPrimitive::getVolumeManager()->refVolume(mesh_params, detail);
			if (sys_volume)
			{
				sys_volume->copyVolumeFaces(volume);
				sys_volume->setMeshAssetLoaded(TRUE);
				LLPrimitive::getVolumeManager()->unrefVolume(sys_volume);
			}
			else
			{
				LL_WARNS(LOG_MESH) << "Couldn't find system volume for mesh " << mesh_params.getSculptID()
								   << LL_ENDL;
			}
		}

		//notify waiting LLVOVolume instances that their requested mesh is available
		for (std::set<LLUUID>::iterator vobj_iter = obj_iter->second.begin(); vobj_iter != obj_iter->second.end(); ++vobj_iter)
		{
			LLVOVolume* vobj = (LLVOVolume*) gObjectList.findObject(*vobj_iter);
			if (vobj)
			{
				vobj->notifyMeshLoaded();
			}
		}
		
		mLoadingMeshes[detail].erase(mesh_params);
	}
}

void LLMeshRepository::notifyMeshUnavailable(const LLVolumeParams& mesh_params, S32 lod)
{ //called from main thread
	//get list of objects waiting to be notified this mesh is loaded
	mesh_load_map::iterator obj_iter = mLoadingMeshes[lod].find(mesh_params);

	F32 detail = LLVolumeLODGroup::getVolumeScaleFromDetail(lod);

	if (obj_iter != mLoadingMeshes[lod].end())
	{
		for (std::set<LLUUID>::iterator vobj_iter = obj_iter->second.begin(); vobj_iter != obj_iter->second.end(); ++vobj_iter)
		{
			LLVOVolume* vobj = (LLVOVolume*) gObjectList.findObject(*vobj_iter);
			if (vobj)
			{
				LLVolume* obj_volume = vobj->getVolume();

				if (obj_volume && 
					obj_volume->getDetail() == detail &&
					obj_volume->getParams() == mesh_params)
				{ //should force volume to find most appropriate LOD
					vobj->setVolume(obj_volume->getParams(), lod);
				}
			}
		}
		
		mLoadingMeshes[lod].erase(mesh_params);
	}
}

S32 LLMeshRepository::getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{ 
	return mThread->getActualMeshLOD(mesh_params, lod);
}

const LLMeshSkinInfo* LLMeshRepository::getSkinInfo(const LLUUID& mesh_id, const LLVOVolume* requesting_obj)
{
	LL_RECORD_BLOCK_TIME(FTM_MESH_FETCH);

	if (mesh_id.notNull())
	{
		skin_map::iterator iter = mSkinMap.find(mesh_id);
		if (iter != mSkinMap.end())
		{
			return &(iter->second);
		}
		
		//no skin info known about given mesh, try to fetch it
		{
			LLMutexLock lock(mMeshMutex);
			//add volume to list of loading meshes
			skin_load_map::iterator iter = mLoadingSkins.find(mesh_id);
			if (iter == mLoadingSkins.end())
			{ //no request pending for this skin info
				mPendingSkinRequests.push(mesh_id);
			}
			mLoadingSkins[mesh_id].insert(requesting_obj->getID());
		}
	}

	return NULL;
}

void LLMeshRepository::fetchPhysicsShape(const LLUUID& mesh_id)
{
	LL_RECORD_BLOCK_TIME(FTM_MESH_FETCH);

	if (mesh_id.notNull())
	{
		LLModel::Decomposition* decomp = NULL;
		decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
		if (iter != mDecompositionMap.end())
		{
			decomp = iter->second;
		}
		
		//decomposition block hasn't been fetched yet
		if (!decomp || decomp->mPhysicsShapeMesh.empty())
		{
			LLMutexLock lock(mMeshMutex);
			//add volume to list of loading meshes
			std::set<LLUUID>::iterator iter = mLoadingPhysicsShapes.find(mesh_id);
			if (iter == mLoadingPhysicsShapes.end())
			{ //no request pending for this skin info
				// *FIXME:  Nothing ever deletes entries, can't be right
				mLoadingPhysicsShapes.insert(mesh_id);
				mPendingPhysicsShapeRequests.push(mesh_id);
			}
		}
	}
}

LLModel::Decomposition* LLMeshRepository::getDecomposition(const LLUUID& mesh_id)
{
	LL_RECORD_BLOCK_TIME(FTM_MESH_FETCH);

	LLModel::Decomposition* ret = NULL;

	if (mesh_id.notNull())
	{
		decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
		if (iter != mDecompositionMap.end())
		{
			ret = iter->second;
		}
		
		//decomposition block hasn't been fetched yet
		if (!ret || ret->mBaseHullMesh.empty())
		{
			LLMutexLock lock(mMeshMutex);
			//add volume to list of loading meshes
			std::set<LLUUID>::iterator iter = mLoadingDecompositions.find(mesh_id);
			if (iter == mLoadingDecompositions.end())
			{ //no request pending for this skin info
				mLoadingDecompositions.insert(mesh_id);
				mPendingDecompositionRequests.push(mesh_id);
			}
		}
	}

	return ret;
}

void LLMeshRepository::buildHull(const LLVolumeParams& params, S32 detail)
{
	LLVolume* volume = LLPrimitive::sVolumeManager->refVolume(params, detail);

	if (!volume->mHullPoints)
	{
		//all default params
		//execute first stage
		//set simplify mode to retain
		//set retain percentage to zero
		//run second stage
	}

	LLPrimitive::sVolumeManager->unrefVolume(volume);
}

bool LLMeshRepository::hasPhysicsShape(const LLUUID& mesh_id)
{
	LLSD mesh = mThread->getMeshHeader(mesh_id);
	if (mesh.has("physics_mesh") && mesh["physics_mesh"].has("size") && (mesh["physics_mesh"]["size"].asInteger() > 0))
	{
		return true;
	}

	LLModel::Decomposition* decomp = getDecomposition(mesh_id);
	if (decomp && !decomp->mHull.empty())
	{
		return true;
	}

	return false;
}

LLSD& LLMeshRepository::getMeshHeader(const LLUUID& mesh_id)
{
	LL_RECORD_BLOCK_TIME(FTM_MESH_FETCH);

	return mThread->getMeshHeader(mesh_id);
}

LLSD& LLMeshRepoThread::getMeshHeader(const LLUUID& mesh_id)
{
	static LLSD dummy_ret;
	if (mesh_id.notNull())
	{
		LLMutexLock lock(mHeaderMutex);
		mesh_header_map::iterator iter = mMeshHeader.find(mesh_id);
		if (iter != mMeshHeader.end() && mMeshHeaderSize[mesh_id] > 0)
		{
			return iter->second;
		}
	}

	return dummy_ret;
}


void LLMeshRepository::uploadModel(std::vector<LLModelInstance>& data, LLVector3& scale, bool upload_textures,
								   bool upload_skin, bool upload_joints, std::string upload_url, bool do_upload,
								   LLHandle<LLWholeModelFeeObserver> fee_observer, LLHandle<LLWholeModelUploadObserver> upload_observer)
{
	LLMeshUploadThread* thread = new LLMeshUploadThread(data, scale, upload_textures, upload_skin, upload_joints, upload_url, 
														do_upload, fee_observer, upload_observer);
	mUploadWaitList.push_back(thread);
}

S32 LLMeshRepository::getMeshSize(const LLUUID& mesh_id, S32 lod)
{
	if (mThread && mesh_id.notNull())
	{
		LLMutexLock lock(mThread->mHeaderMutex);
		LLMeshRepoThread::mesh_header_map::iterator iter = mThread->mMeshHeader.find(mesh_id);
		if (iter != mThread->mMeshHeader.end() && mThread->mMeshHeaderSize[mesh_id] > 0)
		{
			LLSD& header = iter->second;

			if (header.has("404"))
			{
				return -1;
			}

			S32 size = header[header_lod[lod]]["size"].asInteger();
			return size;
		}

	}

	return -1;
}

void LLMeshUploadThread::decomposeMeshMatrix(LLMatrix4& transformation,
											 LLVector3& result_pos,
											 LLQuaternion& result_rot,
											 LLVector3& result_scale)
{
	// check for reflection
	BOOL reflected = (transformation.determinant() < 0);

	// compute position
	LLVector3 position = LLVector3(0, 0, 0) * transformation;

	// compute scale
	LLVector3 x_transformed = LLVector3(1, 0, 0) * transformation - position;
	LLVector3 y_transformed = LLVector3(0, 1, 0) * transformation - position;
	LLVector3 z_transformed = LLVector3(0, 0, 1) * transformation - position;
	F32 x_length = x_transformed.normalize();
	F32 y_length = y_transformed.normalize();
	F32 z_length = z_transformed.normalize();
	LLVector3 scale = LLVector3(x_length, y_length, z_length);

    // adjust for "reflected" geometry
	LLVector3 x_transformed_reflected = x_transformed;
	if (reflected)
	{
		x_transformed_reflected *= -1.0;
	}
	
	// compute rotation
	LLMatrix3 rotation_matrix;
	rotation_matrix.setRows(x_transformed_reflected, y_transformed, z_transformed);
	LLQuaternion quat_rotation = rotation_matrix.quaternion();
	quat_rotation.normalize(); // the rotation_matrix might not have been orthoginal.  make it so here.
	LLVector3 euler_rotation;
	quat_rotation.getEulerAngles(&euler_rotation.mV[VX], &euler_rotation.mV[VY], &euler_rotation.mV[VZ]);

	result_pos = position + mOrigin;
	result_scale = scale;
	result_rot = quat_rotation; 
}

void LLMeshRepository::updateInventory(inventory_data data)
{
	LLMutexLock lock(mMeshMutex);
	dump_llsd_to_file(data.mPostData,make_dump_name("update_inventory_post_data_",dump_num));
	dump_llsd_to_file(data.mResponse,make_dump_name("update_inventory_response_",dump_num));
	mInventoryQ.push(data);
}

void LLMeshRepository::uploadError(LLSD& args)
{
	LLMutexLock lock(mMeshMutex);
	mUploadErrorQ.push(args);
}

F32 LLMeshRepository::getStreamingCost(LLUUID mesh_id, F32 radius, S32* bytes, S32* bytes_visible, S32 lod, F32 *unscaled_value)
{
    if (mThread && mesh_id.notNull())
    {
        LLMutexLock lock(mThread->mHeaderMutex);
        LLMeshRepoThread::mesh_header_map::iterator iter = mThread->mMeshHeader.find(mesh_id);
        if (iter != mThread->mMeshHeader.end() && mThread->mMeshHeaderSize[mesh_id] > 0)
        {
            return getStreamingCost(iter->second, radius, bytes, bytes_visible, lod, unscaled_value);
        }
    }
    return 0.f;
}

//static
F32 LLMeshRepository::getStreamingCost(LLSD& header, F32 radius, S32* bytes, S32* bytes_visible, S32 lod, F32 *unscaled_value)
{
	if (header.has("404")
		|| !header.has("lowest_lod")
		|| (header.has("version") && header["version"].asInteger() > MAX_MESH_VERSION))
	{
		return 0.f;
	}

	F32 max_distance = 512.f;

	F32 dlowest = llmin(radius/0.03f, max_distance);
	F32 dlow = llmin(radius/0.06f, max_distance);
	F32 dmid = llmin(radius/0.24f, max_distance);
	
	F32 METADATA_DISCOUNT = (F32) gSavedSettings.getU32("MeshMetaDataDiscount");  //discount 128 bytes to cover the cost of LLSD tags and compression domain overhead
	F32 MINIMUM_SIZE = (F32) gSavedSettings.getU32("MeshMinimumByteSize"); //make sure nothing is "free"

	F32 bytes_per_triangle = (F32) gSavedSettings.getU32("MeshBytesPerTriangle");

	S32 bytes_lowest = header["lowest_lod"]["size"].asInteger();
	S32 bytes_low = header["low_lod"]["size"].asInteger();
	S32 bytes_mid = header["medium_lod"]["size"].asInteger();
	S32 bytes_high = header["high_lod"]["size"].asInteger();

	if (bytes_high == 0)
	{
		return 0.f;
	}

	if (bytes_mid == 0)
	{
		bytes_mid = bytes_high;
	}

	if (bytes_low == 0)
	{
		bytes_low = bytes_mid;
	}

	if (bytes_lowest == 0)
	{
		bytes_lowest = bytes_low;
	}

	F32 triangles_lowest = llmax((F32) bytes_lowest-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	F32 triangles_low = llmax((F32) bytes_low-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	F32 triangles_mid = llmax((F32) bytes_mid-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	F32 triangles_high = llmax((F32) bytes_high-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;

	if (bytes)
	{
		*bytes = 0;
		*bytes += header["lowest_lod"]["size"].asInteger();
		*bytes += header["low_lod"]["size"].asInteger();
		*bytes += header["medium_lod"]["size"].asInteger();
		*bytes += header["high_lod"]["size"].asInteger();
	}

	if (bytes_visible)
	{
		lod = LLMeshRepository::getActualMeshLOD(header, lod);
		if (lod >= 0 && lod <= 3)
		{
			*bytes_visible = header[header_lod[lod]]["size"].asInteger();
		}
	}

	F32 max_area = 102944.f; //area of circle that encompasses region (see MAINT-6559)
	F32 min_area = 1.f;

	F32 high_area = llmin(F_PI*dmid*dmid, max_area);
	F32 mid_area = llmin(F_PI*dlow*dlow, max_area);
	F32 low_area = llmin(F_PI*dlowest*dlowest, max_area);
	F32 lowest_area = max_area;

	lowest_area -= low_area;
	low_area -= mid_area;
	mid_area -= high_area;

	high_area = llclamp(high_area, min_area, max_area);
	mid_area = llclamp(mid_area, min_area, max_area);
	low_area = llclamp(low_area, min_area, max_area);
	lowest_area = llclamp(lowest_area, min_area, max_area);

	F32 total_area = high_area + mid_area + low_area + lowest_area;
	high_area /= total_area;
	mid_area /= total_area;
	low_area /= total_area;
	lowest_area /= total_area;

	F32 weighted_avg = triangles_high*high_area +
					   triangles_mid*mid_area +
					   triangles_low*low_area +
					  triangles_lowest*lowest_area;

	if (unscaled_value)
	{
		*unscaled_value = weighted_avg;
	}

	return weighted_avg/gSavedSettings.getU32("MeshTriangleBudget")*15000.f;
}


LLPhysicsDecomp::LLPhysicsDecomp()
: LLThread("Physics Decomp")
{
	mInited = false;
	mQuitting = false;
	mDone = false;

	mSignal = new LLCondition(NULL);
	mMutex = new LLMutex(NULL);
}

LLPhysicsDecomp::~LLPhysicsDecomp()
{
	shutdown();

	delete mSignal;
	mSignal = NULL;
	delete mMutex;
	mMutex = NULL;
}

void LLPhysicsDecomp::shutdown()
{
	if (mSignal)
	{
		mQuitting = true;
		mSignal->signal();

		while (!isStopped())
		{
			apr_sleep(10);
		}
	}
}

void LLPhysicsDecomp::submitRequest(LLPhysicsDecomp::Request* request)
{
	LLMutexLock lock(mMutex);
	mRequestQ.push(request);
	mSignal->signal();
}

//static
S32 LLPhysicsDecomp::llcdCallback(const char* status, S32 p1, S32 p2)
{	
	if (gMeshRepo.mDecompThread && gMeshRepo.mDecompThread->mCurRequest.notNull())
	{
		return gMeshRepo.mDecompThread->mCurRequest->statusCallback(status, p1, p2);
	}

	return 1;
}

void LLPhysicsDecomp::setMeshData(LLCDMeshData& mesh, bool vertex_based)
{
	mesh.mVertexBase = mCurRequest->mPositions[0].mV;
	mesh.mVertexStrideBytes = 12;
	mesh.mNumVertices = mCurRequest->mPositions.size();

	if(!vertex_based)
	{
		mesh.mIndexType = LLCDMeshData::INT_16;
		mesh.mIndexBase = &(mCurRequest->mIndices[0]);
		mesh.mIndexStrideBytes = 6;
	
		mesh.mNumTriangles = mCurRequest->mIndices.size()/3;
	}

	if ((vertex_based || mesh.mNumTriangles > 0) && mesh.mNumVertices > 2)
	{
		LLCDResult ret = LLCD_OK;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			ret  = LLConvexDecomposition::getInstance()->setMeshData(&mesh, vertex_based);
		}

		if (ret)
		{
			LL_ERRS(LOG_MESH) << "Convex Decomposition thread valid but could not set mesh data." << LL_ENDL;
		}
	}
}

void LLPhysicsDecomp::doDecomposition()
{
	LLCDMeshData mesh;
	S32 stage = mStageID[mCurRequest->mStage];

	if (LLConvexDecomposition::getInstance() == NULL)
	{
		// stub library. do nothing.
		return;
	}

	//load data intoLLCD
	if (stage == 0)
	{
		setMeshData(mesh, false);
	}
		
	//build parameter map
	std::map<std::string, const LLCDParam*> param_map;

	static const LLCDParam* params = NULL;
	static S32 param_count = 0;
	if (!params)
	{
		param_count = LLConvexDecomposition::getInstance()->getParameters(&params);
	}
	
	for (S32 i = 0; i < param_count; ++i)
	{
		param_map[params[i].mName] = params+i;
	}

	U32 ret = LLCD_OK;
	//set parameter values
	for (decomp_params::iterator iter = mCurRequest->mParams.begin(); iter != mCurRequest->mParams.end(); ++iter)
	{
		const std::string& name = iter->first;
		const LLSD& value = iter->second;

		const LLCDParam* param = param_map[name];

		if (param == NULL)
		{ //couldn't find valid parameter
			continue;
		}


		if (param->mType == LLCDParam::LLCD_FLOAT)
		{
			ret = LLConvexDecomposition::getInstance()->setParam(param->mName, (F32) value.asReal());
		}
		else if (param->mType == LLCDParam::LLCD_INTEGER ||
			param->mType == LLCDParam::LLCD_ENUM)
		{
			ret = LLConvexDecomposition::getInstance()->setParam(param->mName, value.asInteger());
		}
		else if (param->mType == LLCDParam::LLCD_BOOLEAN)
		{
			ret = LLConvexDecomposition::getInstance()->setParam(param->mName, value.asBoolean());
		}
	}

	mCurRequest->setStatusMessage("Executing.");

	if (LLConvexDecomposition::getInstance() != NULL)
	{
		ret = LLConvexDecomposition::getInstance()->executeStage(stage);
	}

	if (ret)
	{
		LL_WARNS(LOG_MESH) << "Convex Decomposition thread valid but could not execute stage " << stage << "."
						   << LL_ENDL;
		LLMutexLock lock(mMutex);

		mCurRequest->mHull.clear();
		mCurRequest->mHullMesh.clear();

		mCurRequest->setStatusMessage("FAIL");
		
		completeCurrent();
	}
	else
	{
		mCurRequest->setStatusMessage("Reading results");

		S32 num_hulls =0;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			num_hulls = LLConvexDecomposition::getInstance()->getNumHullsFromStage(stage);
		}
		
		{
			LLMutexLock lock(mMutex);
			mCurRequest->mHull.clear();
			mCurRequest->mHull.resize(num_hulls);

			mCurRequest->mHullMesh.clear();
			mCurRequest->mHullMesh.resize(num_hulls);
		}

		for (S32 i = 0; i < num_hulls; ++i)
		{
			std::vector<LLVector3> p;
			LLCDHull hull;
			// if LLConvexDecomposition is a stub, num_hulls should have been set to 0 above, and we should not reach this code
			LLConvexDecomposition::getInstance()->getHullFromStage(stage, i, &hull);

			const F32* v = hull.mVertexBase;

			for (S32 j = 0; j < hull.mNumVertices; ++j)
			{
				LLVector3 vert(v[0], v[1], v[2]); 
				p.push_back(vert);
				v = (F32*) (((U8*) v) + hull.mVertexStrideBytes);
			}
			
			LLCDMeshData mesh;
			// if LLConvexDecomposition is a stub, num_hulls should have been set to 0 above, and we should not reach this code
			LLConvexDecomposition::getInstance()->getMeshFromStage(stage, i, &mesh);

			get_vertex_buffer_from_mesh(mesh, mCurRequest->mHullMesh[i]);
			
			{
				LLMutexLock lock(mMutex);
				mCurRequest->mHull[i] = p;
			}
		}
	
		{
			LLMutexLock lock(mMutex);
			mCurRequest->setStatusMessage("FAIL");
			completeCurrent();						
		}
	}
}

void LLPhysicsDecomp::completeCurrent()
{
	LLMutexLock lock(mMutex);
	mCompletedQ.push(mCurRequest);
	mCurRequest = NULL;
}

void LLPhysicsDecomp::notifyCompleted()
{
	if (!mCompletedQ.empty())
	{
		LLMutexLock lock(mMutex);
		while (!mCompletedQ.empty())
		{
			Request* req = mCompletedQ.front();
			req->completed();
			mCompletedQ.pop();
		}
	}
}


void make_box(LLPhysicsDecomp::Request * request)
{
	LLVector3 min,max;
	min = request->mPositions[0];
	max = min;

	for (U32 i = 0; i < request->mPositions.size(); ++i)
	{
		update_min_max(min, max, request->mPositions[i]);
	}

	request->mHull.clear();
	
	LLModel::hull box;
	box.push_back(LLVector3(min[0],min[1],min[2]));
	box.push_back(LLVector3(max[0],min[1],min[2]));
	box.push_back(LLVector3(min[0],max[1],min[2]));
	box.push_back(LLVector3(max[0],max[1],min[2]));
	box.push_back(LLVector3(min[0],min[1],max[2]));
	box.push_back(LLVector3(max[0],min[1],max[2]));
	box.push_back(LLVector3(min[0],max[1],max[2]));
	box.push_back(LLVector3(max[0],max[1],max[2]));

	request->mHull.push_back(box);
}


void LLPhysicsDecomp::doDecompositionSingleHull()
{
	LLConvexDecomposition* decomp = LLConvexDecomposition::getInstance();

	if (decomp == NULL)
	{
		//stub. do nothing.
		return;
	}
	
	LLCDMeshData mesh;	

	setMeshData(mesh, true);

	LLCDResult ret = decomp->buildSingleHull() ;
	if (ret)
	{
		LL_WARNS(LOG_MESH) << "Could not execute decomposition stage when attempting to create single hull." << LL_ENDL;
		make_box(mCurRequest);
	}
	else
	{
		{
			LLMutexLock lock(mMutex);
			mCurRequest->mHull.clear();
			mCurRequest->mHull.resize(1);
			mCurRequest->mHullMesh.clear();
		}

		std::vector<LLVector3> p;
		LLCDHull hull;
		
		// if LLConvexDecomposition is a stub, num_hulls should have been set to 0 above, and we should not reach this code
		decomp->getSingleHull(&hull);

		const F32* v = hull.mVertexBase;

		for (S32 j = 0; j < hull.mNumVertices; ++j)
		{
			LLVector3 vert(v[0], v[1], v[2]); 
			p.push_back(vert);
			v = (F32*) (((U8*) v) + hull.mVertexStrideBytes);
		}
					
		{
			LLMutexLock lock(mMutex);
			mCurRequest->mHull[0] = p;
		}
	}		

	{
		completeCurrent();
		
	}
}


void LLPhysicsDecomp::run()
{
	LLConvexDecomposition* decomp = LLConvexDecomposition::getInstance();
	if (decomp == NULL)
	{
		// stub library. Set init to true so the main thread
		// doesn't wait for this to finish.
		mInited = true;
		return;
	}

	decomp->initThread();
	mInited = true;

	static const LLCDStageData* stages = NULL;
	static S32 num_stages = 0;
	
	if (!stages)
	{
		num_stages = decomp->getStages(&stages);
	}

	for (S32 i = 0; i < num_stages; i++)
	{
		mStageID[stages[i].mName] = i;
	}

	while (!mQuitting)
	{
		mSignal->wait();
		while (!mQuitting && !mRequestQ.empty())
		{
			{
				LLMutexLock lock(mMutex);
				mCurRequest = mRequestQ.front();
				mRequestQ.pop();
			}

			S32& id = *(mCurRequest->mDecompID);
			if (id == -1)
			{
				decomp->genDecomposition(id);
			}
			decomp->bindDecomposition(id);

			if (mCurRequest->mStage == "single_hull")
			{
				doDecompositionSingleHull();
			}
			else
			{
				doDecomposition();
			}		
		}
	}

	decomp->quitThread();
	
	if (mSignal->isLocked())
	{ //let go of mSignal's associated mutex
		mSignal->unlock();
	}

	mDone = true;
}

void LLPhysicsDecomp::Request::assignData(LLModel* mdl) 
{
	if (!mdl)
	{
		return ;
	}

	U16 index_offset = 0;
	U16 tri[3] ;

	mPositions.clear();
	mIndices.clear();
	mBBox[1] = LLVector3(F32_MIN, F32_MIN, F32_MIN) ;
	mBBox[0] = LLVector3(F32_MAX, F32_MAX, F32_MAX) ;
		
	//queue up vertex positions and indices
	for (S32 i = 0; i < mdl->getNumVolumeFaces(); ++i)
	{
		const LLVolumeFace& face = mdl->getVolumeFace(i);
		if (mPositions.size() + face.mNumVertices > 65535)
		{
			continue;
		}

		for (U32 j = 0; j < face.mNumVertices; ++j)
		{
			mPositions.push_back(LLVector3(face.mPositions[j].getF32ptr()));
			for(U32 k = 0 ; k < 3 ; k++)
			{
				mBBox[0].mV[k] = llmin(mBBox[0].mV[k], mPositions[j].mV[k]) ;
				mBBox[1].mV[k] = llmax(mBBox[1].mV[k], mPositions[j].mV[k]) ;
			}
		}

		updateTriangleAreaThreshold() ;

		for (U32 j = 0; j+2 < face.mNumIndices; j += 3)
		{
			tri[0] = face.mIndices[j] + index_offset ;
			tri[1] = face.mIndices[j + 1] + index_offset ;
			tri[2] = face.mIndices[j + 2] + index_offset ;
				
			if(isValidTriangle(tri[0], tri[1], tri[2]))
			{
				mIndices.push_back(tri[0]);
				mIndices.push_back(tri[1]);
				mIndices.push_back(tri[2]);
			}
		}

		index_offset += face.mNumVertices;
	}

	return ;
}

void LLPhysicsDecomp::Request::updateTriangleAreaThreshold() 
{
	F32 range = mBBox[1].mV[0] - mBBox[0].mV[0] ;
	range = llmin(range, mBBox[1].mV[1] - mBBox[0].mV[1]) ;
	range = llmin(range, mBBox[1].mV[2] - mBBox[0].mV[2]) ;

	mTriangleAreaThreshold = llmin(0.0002f, range * 0.000002f) ;
}

//check if the triangle area is large enough to qualify for a valid triangle
bool LLPhysicsDecomp::Request::isValidTriangle(U16 idx1, U16 idx2, U16 idx3) 
{
	LLVector3 a = mPositions[idx2] - mPositions[idx1] ;
	LLVector3 b = mPositions[idx3] - mPositions[idx1] ;
	F32 c = a * b ;

	return ((a*a) * (b*b) - c * c) > mTriangleAreaThreshold ;
}

void LLPhysicsDecomp::Request::setStatusMessage(const std::string& msg)
{
	mStatusMessage = msg;
}

void LLMeshRepository::buildPhysicsMesh(LLModel::Decomposition& decomp)
{
	decomp.mMesh.resize(decomp.mHull.size());

	for (U32 i = 0; i < decomp.mHull.size(); ++i)
	{
		LLCDHull hull;
		hull.mNumVertices = decomp.mHull[i].size();
		hull.mVertexBase = decomp.mHull[i][0].mV;
		hull.mVertexStrideBytes = 12;

		LLCDMeshData mesh;
		LLCDResult res = LLCD_OK;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			res = LLConvexDecomposition::getInstance()->getMeshFromHull(&hull, &mesh);
		}
		if (res == LLCD_OK)
		{
			get_vertex_buffer_from_mesh(mesh, decomp.mMesh[i]);
		}
	}

	if (!decomp.mBaseHull.empty() && decomp.mBaseHullMesh.empty())
	{ //get mesh for base hull
		LLCDHull hull;
		hull.mNumVertices = decomp.mBaseHull.size();
		hull.mVertexBase = decomp.mBaseHull[0].mV;
		hull.mVertexStrideBytes = 12;

		LLCDMeshData mesh;
		LLCDResult res = LLCD_OK;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			res = LLConvexDecomposition::getInstance()->getMeshFromHull(&hull, &mesh);
		}
		if (res == LLCD_OK)
		{
			get_vertex_buffer_from_mesh(mesh, decomp.mBaseHullMesh);
		}
	}
}


bool LLMeshRepository::meshUploadEnabled()
{
	LLViewerRegion *region = gAgent.getRegion();
	if(gSavedSettings.getBOOL("MeshEnabled") &&
	   region)
	{
		return region->meshUploadEnabled();
	}
	return false;
}

bool LLMeshRepository::meshRezEnabled()
{
	LLViewerRegion *region = gAgent.getRegion();
	if(gSavedSettings.getBOOL("MeshEnabled") && 
	   region)
	{
		return region->meshRezEnabled();
	}
	return false;
}

// Threading:  main thread only
// static
void LLMeshRepository::metricsStart()
{
	++metrics_teleport_start_count;
	sQuiescentTimer.start(0);
}

// Threading:  main thread only
// static
void LLMeshRepository::metricsStop()
{
	sQuiescentTimer.stop(0);
}

// Threading:  main thread only
// static
void LLMeshRepository::metricsProgress(unsigned int this_count)
{
	static bool first_start(true);

	if (first_start)
	{
		metricsStart();
		first_start = false;
	}
	sQuiescentTimer.ringBell(0, this_count);
}

// Threading:  main thread only
// static
void LLMeshRepository::metricsUpdate()
{
	F64 started, stopped;
	U64 total_count(U64L(0)), user_cpu(U64L(0)), sys_cpu(U64L(0));
	
	if (sQuiescentTimer.isExpired(0, started, stopped, total_count, user_cpu, sys_cpu))
	{
		LLSD metrics;

		metrics["reason"] = "Mesh Download Quiescent";
		metrics["scope"] = metrics_teleport_start_count > 1 ? "Teleport" : "Login";
		metrics["start"] = started;
		metrics["stop"] = stopped;
		metrics["fetches"] = LLSD::Integer(total_count);
		metrics["teleports"] = LLSD::Integer(metrics_teleport_start_count);
		metrics["user_cpu"] = double(user_cpu) / 1.0e6;
		metrics["sys_cpu"] = double(sys_cpu) / 1.0e6;
		LL_INFOS(LOG_MESH) << "EventMarker " << metrics << LL_ENDL;
	}
}

// Threading:  main thread only
// static
void teleport_started()
{
	LLMeshRepository::metricsStart();
}


void on_new_single_inventory_upload_complete(
    LLAssetType::EType asset_type,
    LLInventoryType::EType inventory_type,
    const std::string inventory_type_string,
    const LLUUID& item_folder_id,
    const std::string& item_name,
    const std::string& item_description,
    const LLSD& server_response,
    S32 upload_price)
{
    bool success = false;

    if (upload_price > 0)
    {
        // this upload costed us L$, update our balance
        // and display something saying that it cost L$
        LLStatusBar::sendMoneyBalanceRequest();

        LLSD args;
        args["AMOUNT"] = llformat("%d", upload_price);
        LLNotificationsUtil::add("UploadPayment", args);
    }

    if (item_folder_id.notNull())
    {
        U32 everyone_perms = PERM_NONE;
        U32 group_perms = PERM_NONE;
        U32 next_owner_perms = PERM_ALL;
        if (server_response.has("new_next_owner_mask"))
        {
            // The server provided creation perms so use them.
            // Do not assume we got the perms we asked for in
            // since the server may not have granted them all.
            everyone_perms = server_response["new_everyone_mask"].asInteger();
            group_perms = server_response["new_group_mask"].asInteger();
            next_owner_perms = server_response["new_next_owner_mask"].asInteger();
        }
        else
        {
            // The server doesn't provide creation perms
            // so use old assumption-based perms.
            if (inventory_type_string != "snapshot")
            {
                next_owner_perms = PERM_MOVE | PERM_TRANSFER;
            }
        }

        LLPermissions new_perms;
        new_perms.init(
            gAgent.getID(),
            gAgent.getID(),
            LLUUID::null,
            LLUUID::null);

        new_perms.initMasks(
            PERM_ALL,
            PERM_ALL,
            everyone_perms,
            group_perms,
            next_owner_perms);

        U32 inventory_item_flags = 0;
        if (server_response.has("inventory_flags"))
        {
            inventory_item_flags = (U32)server_response["inventory_flags"].asInteger();
            if (inventory_item_flags != 0)
            {
                LL_INFOS() << "inventory_item_flags " << inventory_item_flags << LL_ENDL;
            }
        }
        S32 creation_date_now = time_corrected();
        LLPointer<LLViewerInventoryItem> item = new LLViewerInventoryItem(
            server_response["new_inventory_item"].asUUID(),
            item_folder_id,
            new_perms,
            server_response["new_asset"].asUUID(),
            asset_type,
            inventory_type,
            item_name,
            item_description,
            LLSaleInfo::DEFAULT,
            inventory_item_flags,
            creation_date_now);

        gInventory.updateItem(item);
        gInventory.notifyObservers();
        success = true;

        // Show the preview panel for textures and sounds to let
        // user know that the image (or snapshot) arrived intact.
        LLInventoryPanel* panel = LLInventoryPanel::getActiveInventoryPanel();
        if (panel)
        {
            LLFocusableElement* focus = gFocusMgr.getKeyboardFocus();

            panel->setSelection(
                server_response["new_inventory_item"].asUUID(),
                TAKE_FOCUS_NO);

            // restore keyboard focus
            gFocusMgr.setKeyboardFocus(focus);
        }
    }
    else
    {
        LL_WARNS() << "Can't find a folder to put it in" << LL_ENDL;
    }

    // remove the "Uploading..." message
    LLUploadDialog::modalUploadFinished();

    // Let the Snapshot floater know we have finished uploading a snapshot to inventory.
    LLFloater* floater_snapshot = LLFloaterReg::findInstance("snapshot");
    if (asset_type == LLAssetType::AT_TEXTURE && floater_snapshot)
    {
        floater_snapshot->notify(LLSD().with("set-finished", LLSD().with("ok", success).with("msg", "inventory")));
    }
}
