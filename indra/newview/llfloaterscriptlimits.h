/** 
 * @file llfloaterscriptlimits.h
 * @author Gabriel Lee
 * @brief Declaration of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLFLOATERSCRIPTLIMITS_H
#define LL_LLFLOATERSCRIPTLIMITS_H

#include <vector>
#include "llfloater.h"
#include "llhost.h"
#include "llpanel.h"
#include "llremoteparcelrequest.h"

class LLPanelScriptLimitsInfo;
class LLTabContainer;

class LLPanelScriptLimitsRegionMemory;

class LLFloaterScriptLimits : public LLFloater
{
	friend class LLFloaterReg;
public:

	/*virtual*/ BOOL postBuild();

	// from LLPanel
	virtual void refresh();

private:

	LLFloaterScriptLimits(const LLSD& seed);
	~LLFloaterScriptLimits();

protected:

	LLTabContainer* mTab;
	typedef std::vector<LLPanelScriptLimitsInfo*> info_panels_t;
	info_panels_t mInfoPanels;
};


// Base class for all script limits information panels.
class LLPanelScriptLimitsInfo : public LLPanel
{
public:
	LLPanelScriptLimitsInfo();
	
	virtual BOOL postBuild();
	virtual void updateChild(LLUICtrl* child_ctrl);
	
protected:
	void initCtrl(const std::string& name);
	
	typedef std::vector<std::string> strings_t;
	
	LLHost mHost;
};

/////////////////////////////////////////////////////////////////////////////
// Responders
/////////////////////////////////////////////////////////////////////////////

class fetchScriptLimitsRegionInfoResponder: public LLHTTPClient::Responder
{
	public:
		fetchScriptLimitsRegionInfoResponder(const LLSD& info) : mInfo(info) {};

		void result(const LLSD& content);
		void errorWithContent(U32 status, const std::string& reason, const LLSD& content);
	public:
	protected:
		LLSD mInfo;
};

class fetchScriptLimitsRegionSummaryResponder: public LLHTTPClient::Responder
{
	public:
		fetchScriptLimitsRegionSummaryResponder(const LLSD& info) : mInfo(info) {};

		void result(const LLSD& content);
		void errorWithContent(U32 status, const std::string& reason, const LLSD& content);
	public:
	protected:
		LLSD mInfo;
};

class fetchScriptLimitsRegionDetailsResponder: public LLHTTPClient::Responder
{
	public:
		fetchScriptLimitsRegionDetailsResponder(const LLSD& info) : mInfo(info) {};

		void result(const LLSD& content);
		void errorWithContent(U32 status, const std::string& reason, const LLSD& content);
	public:
	protected:
		LLSD mInfo;
};

class fetchScriptLimitsAttachmentInfoResponder: public LLHTTPClient::Responder
{
	public:
		fetchScriptLimitsAttachmentInfoResponder() {};

		void result(const LLSD& content);
		void errorWithContent(U32 status, const std::string& reason, const LLSD& content);
	public:
	protected:
};

/////////////////////////////////////////////////////////////////////////////
// Memory panel
/////////////////////////////////////////////////////////////////////////////

class LLPanelScriptLimitsRegionMemory : public LLPanelScriptLimitsInfo, LLRemoteParcelInfoObserver
{
	
public:
	LLPanelScriptLimitsRegionMemory()
		: LLPanelScriptLimitsInfo(), LLRemoteParcelInfoObserver(),

		mParcelId(LLUUID()),
		mGotParcelMemoryUsed(false),
		mGotParcelMemoryMax(false),
		mParcelMemoryMax(0),
		mParcelMemoryUsed(0) {};

	~LLPanelScriptLimitsRegionMemory();
	
	// LLPanel
	virtual BOOL postBuild();

	void setRegionDetails(LLSD content);
	void setRegionSummary(LLSD content);

	BOOL StartRequestChain();

	BOOL getLandScriptResources();
	void clearList();
	void showBeacon();
	void returnObjectsFromParcel(S32 local_id);
	void returnObjects();

private:
	void onNameCache(const LLUUID& id,
						 const std::string& name);

	LLSD mContent;
	LLUUID mParcelId;
	bool mGotParcelMemoryUsed;
	bool mGotParcelMemoryUsedDetails;
	bool mGotParcelMemoryMax;
	S32 mParcelMemoryMax;
	S32 mParcelMemoryUsed;
	S32 mParcelMemoryUsedDetails;
	
	bool mGotParcelURLsUsed;
	bool mGotParcelURLsUsedDetails;
	bool mGotParcelURLsMax;
	S32 mParcelURLsMax;
	S32 mParcelURLsUsed;
	S32 mParcelURLsUsedDetails;
	
	std::vector<LLSD> mObjectListItems;
		
protected:

// LLRemoteParcelInfoObserver interface:
/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);
/*virtual*/ void setParcelID(const LLUUID& parcel_id);
/*virtual*/ void setErrorStatus(U32 status, const std::string& reason);
	
	static void onClickRefresh(void* userdata);
	static void onClickHighlight(void* userdata);
	static void onClickReturn(void* userdata);
};

/////////////////////////////////////////////////////////////////////////////
// Attachment panel
/////////////////////////////////////////////////////////////////////////////

class LLPanelScriptLimitsAttachment : public LLPanelScriptLimitsInfo
{
	
public:
	LLPanelScriptLimitsAttachment()
		:	LLPanelScriptLimitsInfo(),
		mGotAttachmentMemoryUsed(false),
		mGotAttachmentMemoryUsedDetails(false),
		mGotAttachmentMemoryMax(false),
		mAttachmentMemoryMax(0),
		mAttachmentMemoryUsed(0),
		mAttachmentMemoryUsedDetails(0),
		mGotAttachmentURLsUsed(false),
		mGotAttachmentURLsUsedDetails(false),
		mGotAttachmentURLsMax(false),
		mAttachmentURLsMax(0),
		mAttachmentURLsUsed(0),
		mAttachmentURLsUsedDetails(0)
		{};

	~LLPanelScriptLimitsAttachment()
	{
	};
	
	// LLPanel
	virtual BOOL postBuild();

	void setAttachmentDetails(LLSD content);

	void setAttachmentSummary(LLSD content);
	BOOL requestAttachmentDetails();
	void clearList();

private:

	bool mGotAttachmentMemoryUsed;
	bool mGotAttachmentMemoryUsedDetails;
	bool mGotAttachmentMemoryMax;
	S32 mAttachmentMemoryMax;
	S32 mAttachmentMemoryUsed;
	S32 mAttachmentMemoryUsedDetails;
	
	bool mGotAttachmentURLsUsed;
	bool mGotAttachmentURLsUsedDetails;
	bool mGotAttachmentURLsMax;
	S32 mAttachmentURLsMax;
	S32 mAttachmentURLsUsed;
	S32 mAttachmentURLsUsedDetails;

protected:
	
	static void onClickRefresh(void* userdata);
};

#endif
