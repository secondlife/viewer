/** 
 * @file lllAccountingQuotaManager.h
 * @
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_ACCOUNTINGQUOTAMANAGER_H
#define LL_ACCOUNTINGQUOTAMANAGER_H
//===============================================================================
#include "llaccountingquota.h"
//===============================================================================
class LLAccountingQuotaManager : public LLSingleton<LLAccountingQuotaManager>
{
public:
	//Ctor
	LLAccountingQuotaManager();
	//Store an object that will be eventually fetched
	void updateObjectCost( const LLUUID& objectID );
	//Request quotas for object list
	void fetchQuotas( const std::string& url );
	//Delete a specific object from the pending list
	void removePendingObjectQuota( const LLUUID& objectID );
	
private:
	//Set of objects that need to update their cost
	std::set<LLUUID> mUpdateObjectQuota;
	//During fetchQuota we move object into a the pending set to signify that 
	//a fetch has been instigated.
	std::set<LLUUID> mPendingObjectQuota;
	typedef std::set<LLUUID>::iterator IDIt;
};
//===============================================================================

#endif
