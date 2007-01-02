/** 
 * @file llfloaterbuyland.h
 * @brief LLFloaterBuyLand class definition
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERBUYLAND_H
#define LL_LLFLOATERBUYLAND_H

class LLParcel;
class LLViewerRegion;
class LLViewerTextEditor;

class LLFloaterBuyLand
{
public:
	static void buyLand(LLViewerRegion* region,
						LLParcel* parcel,
						bool is_for_group);
	static void updateCovenantText(const std::string& string, const LLUUID& asset_id);
	static void updateEstateName(const std::string& name);
	static void updateLastModified(const std::string& text);
	static void updateEstateOwnerName(const std::string& name);
};

#endif
