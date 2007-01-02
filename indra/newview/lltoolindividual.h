/** 
 * @file lltoolindividual.h
 * @brief LLToolIndividual class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTOOLINDIVIDUAL_H
#define LL_LLTOOLINDIVIDUAL_H

#include "lltool.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class lltoolindividual
//
// A tool to select individual objects rather than linked sets.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLToolIndividual : public LLTool
{
public:
	LLToolIndividual();
	virtual ~LLToolIndividual();

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual void handleSelect();
	//virtual void handleDeselect();
	//virtual void render();

	static void pickCallback(S32 x, S32 y, MASK mask);

protected:

};

extern LLToolIndividual* gToolIndividual;


#endif // LL_LLTOOLINDIVIDUAL_H
