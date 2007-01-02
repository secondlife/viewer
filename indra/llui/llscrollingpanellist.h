/** 
 * @file llscrollingpanellist.h
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include <vector>

#include "llui.h"
#include "lluictrlfactory.h"
#include "llview.h"
#include "llpanel.h"

// virtual class for scrolling panels
class LLScrollingPanel : public LLPanel
{
public:
	LLScrollingPanel(const LLString& name, const LLRect& rect)
		: LLPanel(name, rect)
	{
	}
	virtual void updatePanel(BOOL allow_modify) = 0;
	
};
	
// A set of panels that are displayed in a vertical sequence inside a scroll container.
class LLScrollingPanelList : public LLUICtrl
{
public:
	LLScrollingPanelList(const LLString& name, const LLRect& rect)
		:	LLUICtrl(name, rect, TRUE, NULL, NULL, FOLLOWS_LEFT | FOLLOWS_BOTTOM ) {}

	virtual void setValue(const LLSD& value);
	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual LLXMLNodePtr getXML(bool save_children) const;
	
	virtual void		draw();

	void				clearPanels();
	void				addPanel( LLScrollingPanel* panel );
	void				updatePanels(BOOL allow_modify);

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	
protected:
	void				updatePanelVisiblilty();

protected:
	std::deque<LLScrollingPanel*> mPanelList;
};
