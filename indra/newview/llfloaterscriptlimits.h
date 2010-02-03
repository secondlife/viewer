/** 
 * @file llfloaterscriptlimits.h
 * @author Gabriel Lee
 * @brief Declaration of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
		void error(U32 status, const std::string& reason);
	public:
	protected:
		LLSD mInfo;
};

class fetchScriptLimitsRegionSummaryResponder: public LLHTTPClient::Responder
{
	public:
		fetchScriptLimitsRegionSummaryResponder(const LLSD& info) : mInfo(info) {};

		void result(const LLSD& content);
		void error(U32 status, const std::string& reason);
	public:
	protected:
		LLSD mInfo;
};

class fetchScriptLimitsRegionDetailsResponder: public LLHTTPClient::Responder
{
	public:
		fetchScriptLimitsRegionDetailsResponder(const LLSD& info) : mInfo(info) {};

		void result(const LLSD& content);
		void error(U32 status, const std::string& reason);
	public:
	protected:
		LLSD mInfo;
};

class fetchScriptLimitsAttachmentInfoResponder: public LLHTTPClient::Responder
{
	public:
		fetchScriptLimitsAttachmentInfoResponder() {};

		void result(const LLSD& content);
		void error(U32 status, const std::string& reason);
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
		mGotParcelMemoryUsed(FALSE),
		mGotParcelMemoryMax(FALSE),
		mParcelMemoryMax(0),
		mParcelMemoryUsed(0) {};

	~LLPanelScriptLimitsRegionMemory()
	{
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelId, this);
	};
	
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
			 const std::string& first_name,
			 const std::string& last_name);

	LLSD mContent;
	LLUUID mParcelId;
	BOOL mGotParcelMemoryUsed;
	BOOL mGotParcelMemoryUsedDetails;
	BOOL mGotParcelMemoryMax;
	S32 mParcelMemoryMax;
	S32 mParcelMemoryUsed;
	S32 mParcelMemoryUsedDetails;
	
	BOOL mGotParcelURLsUsed;
	BOOL mGotParcelURLsUsedDetails;
	BOOL mGotParcelURLsMax;
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
		:	LLPanelScriptLimitsInfo() {};
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

	BOOL mGotAttachmentMemoryUsed;
	BOOL mGotAttachmentMemoryUsedDetails;
	BOOL mGotAttachmentMemoryMax;
	S32 mAttachmentMemoryMax;
	S32 mAttachmentMemoryUsed;
	S32 mAttachmentMemoryUsedDetails;
	
	BOOL mGotAttachmentURLsUsed;
	BOOL mGotAttachmentURLsUsedDetails;
	BOOL mGotAttachmentURLsMax;
	S32 mAttachmentURLsMax;
	S32 mAttachmentURLsUsed;
	S32 mAttachmentURLsUsedDetails;

protected:
	
	static void onClickRefresh(void* userdata);
};

#endif
