/** 
 * @file llpanelmarketplaceoutboxinventory.h
 * @brief LLOutboxInventoryPanel class declaration
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_OUTBOXINVENTORYPANEL_H
#define LL_OUTBOXINVENTORYPANEL_H


#include "llinventorypanel.h"
#include "llfolderviewitem.h"


class LLOutboxInventoryPanel : public LLInventoryPanel
{
public:
	struct Params : public LLInitParam::Block<Params, LLInventoryPanel::Params>
	{
		Params() {}
	};
	
	LLOutboxInventoryPanel(const Params& p);
	~LLOutboxInventoryPanel();

	// virtual
	void buildFolderView(const LLInventoryPanel::Params& params);

	// virtual
	LLFolderViewFolder *	createFolderViewFolder(LLInvFVBridge * bridge);
	LLFolderViewItem *		createFolderViewItem(LLInvFVBridge * bridge);
};


class LLOutboxFolderViewFolder : public LLFolderViewFolder
{
public:
	struct Params : public LLInitParam::Block<Params, LLFolderViewFolder::Params>
	{
		Params() {}
	};
	
	LLOutboxFolderViewFolder(const Params& p);
};


class LLOutboxFolderViewItem : public LLFolderViewItem
{
public:
	LLOutboxFolderViewItem(const Params& p);

	// virtual
	BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
	void openItem();
};


#endif //LL_OUTBOXINVENTORYPANEL_H
