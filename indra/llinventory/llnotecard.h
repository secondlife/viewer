/** 
 * @file llnotecard.h
 * @brief LLNotecard class declaration
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_NOTECARD_H
#define LL_NOTECARD_H

#include "llmemory.h"
#include "llinventory.h"

class LLNotecard
{
public:
	/**
	 * @brief anonymous enumeration to set max size.
	 */
	enum
	{
		MAX_SIZE = 65536
	};
	
	LLNotecard(S32 max_text = LLNotecard::MAX_SIZE);
	virtual ~LLNotecard();

	bool importStream(std::istream& str);
	bool exportStream(std::ostream& str);

	const std::vector<LLPointer<LLInventoryItem> >& getItems() const;
	LLString& getText();

	void setItems(const std::vector<LLPointer<LLInventoryItem> >& items);
	void setText(const LLString& text);
	S32 getVersion() { return mVersion; }
	S32 getEmbeddedVersion() { return mEmbeddedVersion; }
	
private:
	bool importEmbeddedItemsStream(std::istream& str);
	bool exportEmbeddedItemsStream(std::ostream& str);
	std::vector<LLPointer<LLInventoryItem> > mItems;
	LLString mText;
	S32 mMaxText;
	S32 mVersion;
	S32 mEmbeddedVersion;
};

#endif /* LL_NOTECARD_H */
