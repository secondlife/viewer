/** 
 * @file llfloaterbuy.h
 * @author James Cook
 * @brief LLFloaterBuy class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

/**
 * Floater that appears when buying an object, giving a preview
 * of its contents and their permissions.
 */ 

#ifndef LL_LLFLOATERBUY_H
#define LL_LLFLOATERBUY_H

#include "llfloater.h"
#include "llvoinventorylistener.h"
#include "llsaleinfo.h"
#include "llinventory.h"

class LLViewerObject;
class LLSaleInfo;
class LLObjectSelection;

class LLFloaterBuy
: public LLFloater, public LLVOInventoryListener
{
public:
	static void show(const LLSaleInfo& sale_info);

protected:
	LLFloaterBuy();
	~LLFloaterBuy();

	/*virtual*/ void onClose(bool app_quitting);
	void reset();

	void requestObjectInventories();
	/*virtual*/ void inventoryChanged(LLViewerObject* obj,
								 InventoryObjectList* inv,
								 S32 serial_num,
								 void* data);

	static void onClickBuy(void*);
	static void onClickCancel(void*);

private:
	static LLFloaterBuy* sInstance;

	LLSafeHandle<LLObjectSelection>	mObjectSelection;
	LLSaleInfo mSaleInfo;
};

#endif
