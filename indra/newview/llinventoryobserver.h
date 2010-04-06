/** 
 * @file llinventoryobserver.h
 * @brief LLInventoryObserver class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLINVENTORYOBSERVERS_H
#define LL_LLINVENTORYOBSERVERS_H

#include "lluuid.h"
#include <string>
#include <vector>

class LLViewerInventoryCategory;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryObserver
//
// This class is designed to be a simple abstract base class which can
// relay messages when the inventory changes.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryObserver
{
public:
	// This enumeration is a way to refer to what changed in a more
	// human readable format. You can mask the value provided by
	// chaged() to see if the observer is interested in the change.
	enum 
	{
		NONE = 0,
		LABEL = 1,			// name changed
		INTERNAL = 2,		// internal change (e.g. asset uuid different)
		ADD = 4,			// something added
		REMOVE = 8,			// something deleted
		STRUCTURE = 16,		// structural change (eg item or folder moved)
		CALLING_CARD = 32,	// (eg online, grant status, cancel)
		GESTURE = 64,
		REBUILD = 128, 		// item UI changed (eg item type different)
		SORT = 256, 		// folder needs to be resorted.
		ALL = 0xffffffff
	};
	LLInventoryObserver();
	virtual ~LLInventoryObserver();
	virtual void changed(U32 mask) = 0;
	std::string mMessageName; // used by Agent Inventory Service only. [DEV-20328]
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryCompletionObserver
//
// Class which can be used as a base class for doing something when
// when all observed items are locally complete. This class implements
// the changed() method of LLInventoryObserver and declares a new
// method named done() which is called when all watched items have
// complete information in the inventory model.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryCompletionObserver : public LLInventoryObserver
{
public:
	LLInventoryCompletionObserver() {}
	virtual void changed(U32 mask);

	void watchItem(const LLUUID& id);

protected:
	virtual void done() = 0;

	uuid_vec_t mComplete;
	uuid_vec_t mIncomplete;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchObserver
//
// This class is much like the LLInventoryCompletionObserver, except
// that it handles all the the fetching necessary. Override the done()
// method to do the thing you want.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryFetchObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchObserver(bool retry_if_missing = false);
	virtual void changed(U32 mask);

	bool isEverythingComplete() const;
	void fetchItems(const uuid_vec_t& ids);
	virtual void done() {};

protected:
	bool mRetryIfMissing;
	uuid_vec_t mComplete;
	uuid_vec_t mIncomplete;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchDescendentsObserver
//
// This class is much like the LLInventoryCompletionObserver, except
// that it handles fetching based on category. Override the done()
// method to do the thing you want.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchDescendentsObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchDescendentsObserver() {}
	virtual void changed(U32 mask);

	void fetchDescendents(const uuid_vec_t& ids);
	bool isEverythingComplete() const;
	virtual void done() = 0;

protected:
	bool isComplete(LLViewerInventoryCategory* cat);
	uuid_vec_t mIncompleteFolders;
	uuid_vec_t mCompleteFolders;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchComboObserver
//
// This class does an appropriate combination of fetch descendents and
// item fetches based on completion of categories and items. Much like
// the fetch and fetch descendents, this will call done() when everything
// has arrived.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchComboObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchComboObserver() : mDone(false) {}
	virtual void changed(U32 mask);

	void fetch(const uuid_vec_t& folder_ids, const uuid_vec_t& item_ids);

	virtual void done() = 0;

protected:
	bool mDone;
	uuid_vec_t mCompleteFolders;
	uuid_vec_t mIncompleteFolders;
	uuid_vec_t mCompleteItems;
	uuid_vec_t mIncompleteItems;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryExistenceObserver
//
// This class is used as a base class for doing somethign when all the
// observed item ids exist in the inventory somewhere. You can derive
// a class from this class and implement the done() method to do
// something useful.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryExistenceObserver : public LLInventoryObserver
{
public:
	LLInventoryExistenceObserver() {}
	virtual void changed(U32 mask);

	void watchItem(const LLUUID& id);

protected:
	virtual void done() = 0;
	uuid_vec_t mExist;
	uuid_vec_t mMIA;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryAddedObserver
//
// This class is used as a base class for doing something when 
// a new item arrives in inventory.
// It does not watch for a certain UUID, rather it acts when anything is added
// Derive a class from this class and implement the done() method to do
// something useful.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryAddedObserver : public LLInventoryObserver
{
public:
	LLInventoryAddedObserver() : mAdded() {}
	virtual void changed(U32 mask);

protected:
	virtual void done() = 0;

	uuid_vec_t mAdded;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryTransactionObserver
//
// Class which can be used as a base class for doing something when an
// inventory transaction completes.
//
// *NOTE: This class is not quite complete. Avoid using unless you fix up it's
// functionality gaps.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryTransactionObserver : public LLInventoryObserver
{
public:
	LLInventoryTransactionObserver(const LLTransactionID& transaction_id);
	virtual void changed(U32 mask);

protected:
	virtual void done(const uuid_vec_t& folders, const uuid_vec_t& items) = 0;

	LLTransactionID mTransactionID;
};


#endif // LL_LLINVENTORYOBSERVERS_H

