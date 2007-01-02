/** 
 * @file llstyle.h
 * @brief Text style class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSTYLE_H
#define LL_LLSTYLE_H

#include "v4color.h"
#include "llresmgr.h"
#include "llfont.h"
#include "llimagegl.h"

class LLStyle
{
public:
	LLStyle();
	LLStyle(const LLStyle &style);
	LLStyle(BOOL is_visible, const LLColor4 &color, const LLString& font_name);

	LLStyle &operator=(const LLStyle &rhs);

	virtual ~LLStyle();

	virtual void init (BOOL is_visible, const LLColor4 &color, const LLString& font_name);
	virtual void free ();

	bool operator==(const LLStyle &rhs) const;
	bool operator!=(const LLStyle &rhs) const;

	virtual const LLColor4& getColor() const;
	virtual void setColor(const LLColor4 &color);

	virtual BOOL isVisible() const;
	virtual void setVisible(BOOL is_visible);

	virtual const LLString& getFontString() const;
	virtual void setFontName(const LLString& fontname);
	virtual LLFONT_ID getFontID() const;

	virtual const LLString& getLinkHREF() const;
	virtual void setLinkHREF(const LLString& fontname);
	virtual BOOL isLink() const;

	virtual LLImageGL *getImage() const;
	virtual void setImage(const LLString& src);
	virtual BOOL isImage() const;
	virtual void setImageSize(S32 width, S32 height);

	BOOL	getIsEmbeddedItem() const	{ return mIsEmbeddedItem; }
	void	setIsEmbeddedItem( BOOL b ) { mIsEmbeddedItem = b; }

public:	
	BOOL        mItalic;
	BOOL        mBold;
	BOOL        mUnderline;
	BOOL		mDropShadow;
	S32         mImageWidth;
	S32         mImageHeight;

protected:
	BOOL		mVisible;
	LLColor4	mColor;
	LLString	mFontName;
	LLFONT_ID   mFontID;
	LLString	mLink;
	LLPointer<LLImageGL> mImagep;

	BOOL		mIsEmbeddedItem;
};

#endif  // LL_LLSTYLE_H
