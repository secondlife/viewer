/** 
 * @file llfloatermap.h
 * @brief The "mini-map" or radar in the upper right part of the screen.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERMAP_H
#define LL_LLFLOATERMAP_H

#include "llfloater.h"

class LLNetMap;
class LLSliderCtrl;
class LLStatGraph;
class LLTextBox;
class LLUICtrl;
class LLViewerImage;

//
// Classes
//
class LLFloaterMap
:	public LLFloater
{
public:
	LLFloaterMap(const std::string& name);
	virtual ~LLFloaterMap();

	static void		toggle(void*);

	/*virtual*/ void	setVisible(BOOL visible);
	/*virtual*/ void	draw();
	/*virtual*/ void	onClose(bool app_quitting);
	/*virtual*/ BOOL	canClose();

protected:
	LLNetMap*		mMap;
};


//
// Globals
//

extern LLFloaterMap *gFloaterMap;

#endif  // LL_LLFLOATERMAP_H
