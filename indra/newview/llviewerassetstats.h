/** 
 * @file llviewerassetstats.h
 * @brief Client-side collection of asset request statistics
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLVIEWERASSETSTATUS_H
#define	LL_LLVIEWERASSETSTATUS_H


#include "linden_common.h"

#include "llviewerassettype.h"
#include "llviewerassetstorage.h"
#include "llsimplestat.h"
#include "llsd.h"

/**
 * @class LLViewerAssetStats
 * @brief Records events and performance of asset put/get operations.
 *
 * The asset system is a combination of common code and server-
 * and viewer-overridden derivations.  The common code is presented
 * in here as the 'front-end' and deriviations (really the server)
 * are presented as 'back-end'.  The distinction isn't perfect as
 * there are legacy asset transfer systems which mostly appear
 * as front-end stats.
 *
 * Statistics collected are fairly basic:
 *  - Counts of enqueue and dequeue operations
 *  - Counts of duplicated request fetches
 *  - Min/Max/Mean of asset transfer operations
 *
 * While the stats collection interfaces appear to be fairly
 * orthogonal across methods (GET, PUT) and asset types (texture,
 * bodypart, etc.), the actual internal collection granularity
 * varies greatly.  GET's operations found in the cache are
 * treated as a single group as are duplicate requests.  Non-
 * cached items are broken down into three groups:  textures,
 * wearables (bodyparts, clothing) and the rest.  PUT operations
 * are broken down into two categories:  temporary assets and
 * non-temp.  Back-end operations do not distinguish asset types,
 * only GET, PUT (temp) and PUT (non-temp).
 * 
 * No coverage for Estate Assets or Inventory Item Assets which use
 * some different interface conventions.  It could be expanded to cover
 * them.
 *
 * Access to results is by conversion to an LLSD with some standardized
 * key names.  The intent of this structure is to be emitted as
 * standard syslog-based metrics formatting where it can be picked
 * up by interested parties.
 *
 * For convenience, a set of free functions in namespace LLAssetStatsFF
 * are provided which operate on various counters in a way that
 * is highly-compatible with the simulator code.
 */
class LLViewerAssetStats
{
public:
	LLViewerAssetStats();
	// Default destructor and assignment operator are correct.
	
	enum EViewerAssetCategories
	{
		EVACTextureTempHTTPGet,			//< Texture GETs
		EVACTextureTempUDPGet,			//< Texture GETs
		EVACTextureNonTempHTTPGet,		//< Texture GETs
		EVACTextureNonTempUDPGet,		//< Texture GETs
		EVACWearableUDPGet,				//< Wearable GETs
		EVACSoundUDPGet,				//< Sound GETs
		EVACGestureUDPGet,				//< Gesture GETs
		EVACOtherGet,					//< Other GETs
		
		EVACCount						// Must be last
	};
	
	void reset();

	// Non-Cached GET Requests
	void recordGetEnqueued(LLViewerAssetType::EType at, bool with_http, bool is_temp);
	void recordGetDequeued(LLViewerAssetType::EType at, bool with_http, bool is_temp);
	void recordGetServiced(LLViewerAssetType::EType at, bool with_http, bool is_temp, F64 duration);

	// Report Generation
	const LLSD asLLSD() const;
	
protected:

	struct 
	{
		LLSimpleStatCounter		mEnqueued;
		LLSimpleStatCounter		mDequeued;
		LLSimpleStatMMM<>		mResponse;
	} mRequests [EVACCount];
};


/**
 * Expectation is that the simulator and other asset-handling
 * code will create a single instance of the stats class and
 * make it available here.  The free functions examine this
 * for non-zero and perform their functions conditionally.  The
 * instance methods themselves make no assumption about this.
 */
extern LLViewerAssetStats * gViewerAssetStats;

namespace LLViewerAssetStatsFF
{

void record_enqueue(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_dequeue(LLViewerAssetType::EType at, bool with_http, bool is_temp);

void record_response(LLViewerAssetType::EType at, bool with_http, bool is_temp, F64 duration);

} // namespace LLViewerAssetStatsFF


#endif	// LL_LLVIEWERASSETSTATUS_H
