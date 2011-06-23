/** 
 * @file lllAccountingQuotaManager.h
 * @
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#endif // LLACCOUNTINGQUOTAMANAGER

