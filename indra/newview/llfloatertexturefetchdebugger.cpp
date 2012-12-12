/** 
 * @file llfloatertexturefetchdebugger.cpp
 * @brief LLFloaterTextureFetchDebugger class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llfloatertexturefetchdebugger.h"

#include "lluictrlfactory.h"
#include "llbutton.h"
#include "llspinctrl.h"
#include "llresmgr.h"

#include "llmath.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "lltexturefetch.h"
#include "llviewercontrol.h"

LLFloaterTextureFetchDebugger::LLFloaterTextureFetchDebugger(const LLSD& key)
	: LLFloater(key),
	mDebugger(NULL)
{
	setTitle("Texture Fetching Debugger Floater");
	
	mCommitCallbackRegistrar.add("TexFetchDebugger.ChangeTexelPixelRatio",	boost::bind(&LLFloaterTextureFetchDebugger::onChangeTexelPixelRatio, this));

	mCommitCallbackRegistrar.add("TexFetchDebugger.Start",	boost::bind(&LLFloaterTextureFetchDebugger::onClickStart, this));
	mCommitCallbackRegistrar.add("TexFetchDebugger.Clear",	boost::bind(&LLFloaterTextureFetchDebugger::onClickClear, this));
	mCommitCallbackRegistrar.add("TexFetchDebugger.Close",	boost::bind(&LLFloaterTextureFetchDebugger::onClickClose, this));

	mCommitCallbackRegistrar.add("TexFetchDebugger.CacheRead",	boost::bind(&LLFloaterTextureFetchDebugger::onClickCacheRead, this));
	mCommitCallbackRegistrar.add("TexFetchDebugger.CacheWrite",	boost::bind(&LLFloaterTextureFetchDebugger::onClickCacheWrite, this));
	mCommitCallbackRegistrar.add("TexFetchDebugger.HTTPLoad",	boost::bind(&LLFloaterTextureFetchDebugger::onClickHTTPLoad, this));
	mCommitCallbackRegistrar.add("TexFetchDebugger.Decode",	boost::bind(&LLFloaterTextureFetchDebugger::onClickDecode, this));
	mCommitCallbackRegistrar.add("TexFetchDebugger.GLTexture",	boost::bind(&LLFloaterTextureFetchDebugger::onClickGLTexture, this));

	mCommitCallbackRegistrar.add("TexFetchDebugger.RefetchVisCache",	boost::bind(&LLFloaterTextureFetchDebugger::onClickRefetchVisCache, this));
	mCommitCallbackRegistrar.add("TexFetchDebugger.RefetchVisHTTP",	boost::bind(&LLFloaterTextureFetchDebugger::onClickRefetchVisHTTP, this));
	mCommitCallbackRegistrar.add("TexFetchDebugger.RefetchAllCache",	boost::bind(&LLFloaterTextureFetchDebugger::onClickRefetchAllCache, this));
	mCommitCallbackRegistrar.add("TexFetchDebugger.RefetchAllHTTP",	boost::bind(&LLFloaterTextureFetchDebugger::onClickRefetchAllHTTP, this));
}
//----------------------------------------------

BOOL LLFloaterTextureFetchDebugger::postBuild(void) 
{	
	mDebugger = LLAppViewer::getTextureFetch()->getFetchDebugger();
	mStartStatus = (S32)LLTextureFetchDebugger::IDLE;

	//set states for buttons
	mButtonStateMap["start_btn"] = true;
	mButtonStateMap["close_btn"] = true;
	mButtonStateMap["clear_btn"] = true;
	mButtonStateMap["cacheread_btn"] = false;
	mButtonStateMap["cachewrite_btn"] = false;
	mButtonStateMap["http_btn"] = false;
	mButtonStateMap["decode_btn"] = false;
	mButtonStateMap["gl_btn"] = false;

	mButtonStateMap["refetchviscache_btn"] = false;
	mButtonStateMap["refetchvishttp_btn"] = false;
	mButtonStateMap["refetchallcache_btn"] = false;
	mButtonStateMap["refetchallhttp_btn"] = false;

	updateButtons();

	getChild<LLUICtrl>("texel_pixel_ratio")->setValue(gSavedSettings.getF32("TexelPixelRatio"));

	return TRUE ;
}

LLFloaterTextureFetchDebugger::~LLFloaterTextureFetchDebugger()
{
	//stop everything
	mDebugger->setStopDebug();
}

void LLFloaterTextureFetchDebugger::updateButtons()
{
	for(std::map<std::string, bool>::iterator iter = mButtonStateMap.begin(); iter != mButtonStateMap.end(); ++iter)
	{
		if(iter->second)
		{
			childEnable(iter->first.c_str());
		}
		else
		{
			childDisable(iter->first.c_str());
		}
	}
}

void LLFloaterTextureFetchDebugger::disableButtons()
{
	childDisable("start_btn");
	childDisable("clear_btn");
	childDisable("cacheread_btn");
	childDisable("cachewrite_btn");
	childDisable("http_btn");
	childDisable("decode_btn");
	childDisable("gl_btn");
	childDisable("refetchviscache_btn");
	childDisable("refetchvishttp_btn");
	childDisable("refetchallcache_btn");
	childDisable("refetchallhttp_btn");
}
void LLFloaterTextureFetchDebugger::setStartStatus(S32 status)
{
	llassert_always(LLTextureFetchDebugger::IDLE == (LLTextureFetchDebugger::e_debug_state)mStartStatus) ;
	mStartStatus = status;
}
	
bool LLFloaterTextureFetchDebugger::idleStart()
{
	if(mStartStatus != (S32)LLTextureFetchDebugger::IDLE)
	{
		mDebugger->startWork((LLTextureFetchDebugger::e_debug_state)mStartStatus);
		mStartStatus = (S32)LLTextureFetchDebugger::IDLE;
		return true;
	}

	return false;
}

void LLFloaterTextureFetchDebugger::idle()
{	
	if(idleStart())
	{
		return;
	}

	const F32 max_time = 0.005f; //5ms
	LLTextureFetchDebugger::e_debug_state state = mDebugger->getState();	
	if(mDebugger->update(max_time))
	{
		switch(state)
		{
		case LLTextureFetchDebugger::IDLE:
			break;
		case LLTextureFetchDebugger::START_DEBUG:
			mButtonStateMap["cacheread_btn"] = true;
			mButtonStateMap["http_btn"] = true;
			mButtonStateMap["refetchviscache_btn"] = true;
			mButtonStateMap["refetchvishttp_btn"] = true;
			mButtonStateMap["refetchallcache_btn"] = true;
			mButtonStateMap["refetchallhttp_btn"] = true;
			break;
		case LLTextureFetchDebugger::READ_CACHE:			
			mButtonStateMap["decode_btn"] = true;			
			break;
		case LLTextureFetchDebugger::WRITE_CACHE:			
			break;
		case LLTextureFetchDebugger::DECODING:
			mButtonStateMap["gl_btn"] = true;			
			break;
		case LLTextureFetchDebugger::HTTP_FETCHING:
			mButtonStateMap["cacheread_btn"] = true;
			mButtonStateMap["cachewrite_btn"] = true;
			mButtonStateMap["decode_btn"] = true;			
			break;
		case LLTextureFetchDebugger::GL_TEX:			
			break;
		case LLTextureFetchDebugger::REFETCH_VIS_CACHE:			
			break;
		case LLTextureFetchDebugger::REFETCH_VIS_HTTP:			
			break;
		case LLTextureFetchDebugger::REFETCH_ALL_CACHE:			
			break;
		case LLTextureFetchDebugger::REFETCH_ALL_HTTP:
			break;
		default:
			break;
		}

		if(state != LLTextureFetchDebugger::IDLE)
		{
			updateButtons();
		}
	}
}

//----------------------
void LLFloaterTextureFetchDebugger::onChangeTexelPixelRatio()
{
	gSavedSettings.setF32("TexelPixelRatio", getChild<LLUICtrl>("texel_pixel_ratio")->getValue().asReal());
}

void LLFloaterTextureFetchDebugger::onClickStart()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::START_DEBUG);	

	mButtonStateMap["start_btn"] = false;

	updateButtons();
}

void LLFloaterTextureFetchDebugger::onClickClose()
{
	setVisible(FALSE);
	
	//stop everything
	mDebugger->setStopDebug();

	delete this;
}

void LLFloaterTextureFetchDebugger::onClickClear()
{
	mButtonStateMap["start_btn"] = true;
	mButtonStateMap["close_btn"] = true;
	mButtonStateMap["clear_btn"] = true;
	mButtonStateMap["cacheread_btn"] = false;
	mButtonStateMap["cachewrite_btn"] = false;
	mButtonStateMap["http_btn"] = false;
	mButtonStateMap["decode_btn"] = false;
	mButtonStateMap["gl_btn"] = false;
	mButtonStateMap["refetchviscache_btn"] = true;
	mButtonStateMap["refetchvishttp_btn"] = true;
	updateButtons();

	//stop everything
	mDebugger->setStopDebug();
	mDebugger->clearHistory();
}

void LLFloaterTextureFetchDebugger::onClickCacheRead()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::READ_CACHE);
}

void LLFloaterTextureFetchDebugger::onClickCacheWrite()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::WRITE_CACHE);
}

void LLFloaterTextureFetchDebugger::onClickHTTPLoad()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::HTTP_FETCHING);
}

void LLFloaterTextureFetchDebugger::onClickDecode()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::DECODING);
}

void LLFloaterTextureFetchDebugger::onClickGLTexture()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::GL_TEX);	
}

void LLFloaterTextureFetchDebugger::onClickRefetchVisCache()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::REFETCH_VIS_CACHE);
}

void LLFloaterTextureFetchDebugger::onClickRefetchVisHTTP()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::REFETCH_VIS_HTTP);	
}

void LLFloaterTextureFetchDebugger::onClickRefetchAllCache()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::REFETCH_ALL_CACHE);
}

void LLFloaterTextureFetchDebugger::onClickRefetchAllHTTP()
{
	disableButtons();

	setStartStatus((S32)LLTextureFetchDebugger::REFETCH_ALL_HTTP);	
}

void LLFloaterTextureFetchDebugger::draw()
{
	//total number of fetched textures
	{
		getChild<LLUICtrl>("total_num_fetched_label")->setTextArg("[NUM]", llformat("%d", mDebugger->getNumFetchedTextures()));
	}

	//total number of fetching requests
	{
		getChild<LLUICtrl>("total_num_fetching_requests_label")->setTextArg("[NUM]", llformat("%d", mDebugger->getNumFetchingRequests()));
	}

	//total number of cache hits
	{
		getChild<LLUICtrl>("total_num_cache_hits_label")->setTextArg("[NUM]", llformat("%d", mDebugger->getNumCacheHits()));
	}

	//total number of visible textures
	{
		getChild<LLUICtrl>("total_num_visible_tex_label")->setTextArg("[NUM]", llformat("%d", mDebugger->getNumVisibleFetchedTextures()));
	}

	//total number of visible texture fetching requests
	{
		getChild<LLUICtrl>("total_num_visible_tex_fetch_req_label")->setTextArg("[NUM]", llformat("%d", mDebugger->getNumVisibleFetchingRequests()));
	}

	//total number of fetched data
	{
		getChild<LLUICtrl>("total_fetched_data_label")->setTextArg("[SIZE1]", llformat("%d", mDebugger->getFetchedData() >> 10));		
		getChild<LLUICtrl>("total_fetched_data_label")->setTextArg("[SIZE2]", llformat("%d", mDebugger->getDecodedData() >> 10));
		getChild<LLUICtrl>("total_fetched_data_label")->setTextArg("[PIXEL]", llformat("%.3f", mDebugger->getFetchedPixels() / 1000000.f));
	}

	//total number of visible fetched data
	{		
		getChild<LLUICtrl>("total_fetched_vis_data_label")->setTextArg("[SIZE1]", llformat("%d", mDebugger->getVisibleFetchedData() >> 10));		
		getChild<LLUICtrl>("total_fetched_vis_data_label")->setTextArg("[SIZE2]", llformat("%d", mDebugger->getVisibleDecodedData() >> 10));
	}

	//total number of rendered fetched data
	{
		getChild<LLUICtrl>("total_fetched_rendered_data_label")->setTextArg("[SIZE1]", llformat("%d", mDebugger->getRenderedData() >> 10));		
		getChild<LLUICtrl>("total_fetched_rendered_data_label")->setTextArg("[SIZE2]", llformat("%d", mDebugger->getRenderedDecodedData() >> 10));
		getChild<LLUICtrl>("total_fetched_rendered_data_label")->setTextArg("[PIXEL]", llformat("%.3f", mDebugger->getRenderedPixels() / 1000000.f));
	}

	//total time on cache readings
	if(mDebugger->getCacheReadTime() < 0.f)
	{
		getChild<LLUICtrl>("total_time_cache_read_label")->setTextArg("[TIME]", std::string("----"));
	}
	else
	{
		getChild<LLUICtrl>("total_time_cache_read_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getCacheReadTime()));
	}

	//total time on cache writings
	if(mDebugger->getCacheWriteTime() < 0.f)
	{
		getChild<LLUICtrl>("total_time_cache_write_label")->setTextArg("[TIME]", std::string("----"));
	}
	else
	{
		getChild<LLUICtrl>("total_time_cache_write_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getCacheWriteTime()));
	}

	//total time on decoding
	if(mDebugger->getDecodeTime() < 0.f)
	{
		getChild<LLUICtrl>("total_time_decode_label")->setTextArg("[TIME]", std::string("----"));
	}
	else
	{
		getChild<LLUICtrl>("total_time_decode_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getDecodeTime()));
	}

	//total time on gl texture creation
	if(mDebugger->getGLCreationTime() < 0.f)
	{
		getChild<LLUICtrl>("total_time_gl_label")->setTextArg("[TIME]", std::string("----"));
	}
	else
	{
		getChild<LLUICtrl>("total_time_gl_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getGLCreationTime()));
	}

	//total time on HTTP fetching
	if(mDebugger->getHTTPTime() < 0.f)
	{
		getChild<LLUICtrl>("total_time_http_label")->setTextArg("[TIME]", std::string("----"));
	}
	else
	{
		getChild<LLUICtrl>("total_time_http_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getHTTPTime()));
	}

	//total time on entire fetching
	{
		getChild<LLUICtrl>("total_time_fetch_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getTotalFetchingTime()));
	}

	//total time on refetching visible textures from cache
	if(mDebugger->getRefetchVisCacheTime() < 0.f)
	{
		getChild<LLUICtrl>("total_time_refetch_vis_cache_label")->setTextArg("[TIME]", std::string("----"));
		getChild<LLUICtrl>("total_time_refetch_vis_cache_label")->setTextArg("[SIZE]", std::string("----"));
		getChild<LLUICtrl>("total_time_refetch_vis_cache_label")->setTextArg("[PIXEL]", std::string("----"));
	}
	else
	{
		getChild<LLUICtrl>("total_time_refetch_vis_cache_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getRefetchVisCacheTime()));
		getChild<LLUICtrl>("total_time_refetch_vis_cache_label")->setTextArg("[SIZE]", llformat("%d", mDebugger->getRefetchedVisData() >> 10));
		getChild<LLUICtrl>("total_time_refetch_vis_cache_label")->setTextArg("[PIXEL]", llformat("%.3f", mDebugger->getRefetchedVisPixels() / 1000000.f));
	}

	//total time on refetching all textures from cache
	if(mDebugger->getRefetchAllCacheTime() < 0.f)
	{
		getChild<LLUICtrl>("total_time_refetch_all_cache_label")->setTextArg("[TIME]", std::string("----"));
		getChild<LLUICtrl>("total_time_refetch_all_cache_label")->setTextArg("[SIZE]", std::string("----"));
		getChild<LLUICtrl>("total_time_refetch_all_cache_label")->setTextArg("[PIXEL]", std::string("----"));
	}
	else
	{
		getChild<LLUICtrl>("total_time_refetch_all_cache_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getRefetchAllCacheTime()));
		getChild<LLUICtrl>("total_time_refetch_all_cache_label")->setTextArg("[SIZE]", llformat("%d", mDebugger->getRefetchedAllData() >> 10));
		getChild<LLUICtrl>("total_time_refetch_all_cache_label")->setTextArg("[PIXEL]", llformat("%.3f", mDebugger->getRefetchedAllPixels() / 1000000.f));
	}

	//total time on refetching visible textures from http
	if(mDebugger->getRefetchVisHTTPTime() < 0.f)
	{
		getChild<LLUICtrl>("total_time_refetch_vis_http_label")->setTextArg("[TIME]", std::string("----"));
		getChild<LLUICtrl>("total_time_refetch_vis_http_label")->setTextArg("[SIZE]", std::string("----"));
		getChild<LLUICtrl>("total_time_refetch_vis_http_label")->setTextArg("[PIXEL]", std::string("----"));
	}
	else
	{
		getChild<LLUICtrl>("total_time_refetch_vis_http_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getRefetchVisHTTPTime()));
		getChild<LLUICtrl>("total_time_refetch_vis_http_label")->setTextArg("[SIZE]", llformat("%d", mDebugger->getRefetchedVisData() >> 10));
		getChild<LLUICtrl>("total_time_refetch_vis_http_label")->setTextArg("[PIXEL]", llformat("%.3f", mDebugger->getRefetchedVisPixels() / 1000000.f));
	}

	//total time on refetching all textures from http
	if(mDebugger->getRefetchAllHTTPTime() < 0.f)
	{
		getChild<LLUICtrl>("total_time_refetch_all_http_label")->setTextArg("[TIME]", std::string("----"));
		getChild<LLUICtrl>("total_time_refetch_all_http_label")->setTextArg("[SIZE]", std::string("----"));
		getChild<LLUICtrl>("total_time_refetch_all_http_label")->setTextArg("[PIXEL]", std::string("----"));
	}
	else
	{
		getChild<LLUICtrl>("total_time_refetch_all_http_label")->setTextArg("[TIME]", llformat("%.3f", mDebugger->getRefetchAllHTTPTime()));
		getChild<LLUICtrl>("total_time_refetch_all_http_label")->setTextArg("[SIZE]", llformat("%d", mDebugger->getRefetchedAllData() >> 10));
		getChild<LLUICtrl>("total_time_refetch_all_http_label")->setTextArg("[PIXEL]", llformat("%.3f", mDebugger->getRefetchedAllPixels() / 1000000.f));
	}

	LLFloater::draw();
}
