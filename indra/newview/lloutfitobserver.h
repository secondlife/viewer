/**
 * @file lloutfitobserver.h
 * @brief Outfit observer facade.
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

#ifndef LL_OUTFITOBSERVER_H
#define LL_OUTFITOBSERVER_H

#include "llsingleton.h"
#include "llmd5.h"

/**
 * Outfit observer facade that provides simple possibility to subscribe on
 * BOF(base outfit) replaced, BOF changed, COF(current outfit) changed events.
 */
class LLOutfitObserver: public LLInventoryObserver, public LLSingleton<LLOutfitObserver>
{
public:
	virtual ~LLOutfitObserver();

	friend class LLSingleton<LLOutfitObserver>;

	virtual void changed(U32 mask);

	void notifyOutfitLockChanged() { mOutfitLockChanged();  }

	typedef boost::signals2::signal<void (void)> signal_t;

	void addBOFReplacedCallback(const signal_t::slot_type& cb) { mBOFReplaced.connect(cb); }

	void addBOFChangedCallback(const signal_t::slot_type& cb) { mBOFChanged.connect(cb); }

	void addCOFChangedCallback(const signal_t::slot_type& cb) { mCOFChanged.connect(cb); }

	void addCOFSavedCallback(const signal_t::slot_type& cb) { mCOFSaved.connect(cb); }

	void addOutfitLockChangedCallback(const signal_t::slot_type& cb) { mOutfitLockChanged.connect(cb); }

protected:
	LLOutfitObserver();

	/** Get a version of an inventory category specified by its UUID */
	static S32 getCategoryVersion(const LLUUID& cat_id);

	static const std::string& getCategoryName(const LLUUID& cat_id);

	bool checkCOF();

	void hashItemNames(LLMD5& itemNameHash);

	void checkBaseOutfit();

	//last version number of a COF category
	S32 mCOFLastVersion;

	LLUUID mBaseOutfitId;

	S32 mBaseOutfitLastVersion;
	std::string mLastBaseOutfitName;

	bool mLastOutfitDirtiness;

	LLMD5 mItemNameHash;

private:
	signal_t mBOFReplaced;
	signal_t mBOFChanged;
	signal_t mCOFChanged;
	signal_t mCOFSaved;

	/**
	 * Signal for changing state of outfit lock.
	 */
	signal_t mOutfitLockChanged;
};

#endif /* LL_OUTFITOBSERVER_H */
