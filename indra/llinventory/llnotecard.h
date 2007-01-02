/** 
 * @file llnotecard.h
 * @brief LLNotecard class declaration
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_NOTECARD_H
#define LL_NOTECARD_H

const S32 MAX_NOTECARD_SIZE = 65536;

class LLNotecard
{
public:
	LLNotecard(U32 max_text);
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
	U32 mMaxText;
	S32 mVersion;
	S32 mEmbeddedVersion;
};

#endif /* LL_NOTECARD_H */
