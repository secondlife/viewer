/** 
 * @file llcontrolgroupreader.h
 * @brief Interface providing readonly access to LLControlGroup (intended for unit testing)
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLCONTROLGROUPREADER_H
#define LL_LLCONTROLGROUPREADER_H

#include "stdtypes.h"
#include <string>

#include "v3math.h"
#include "v3dmath.h"
#include "v3color.h"
#include "v4color.h"
#include "llrect.h"

class LLControlGroupReader
{
public:
	LLControlGroupReader() {}
	virtual ~LLControlGroupReader() {}

	virtual std::string getString(const std::string& name) { return ""; }
	virtual LLWString	getWString(const std::string& name) { return LLWString(); }
	virtual std::string	getText(const std::string& name) { return ""; }
	virtual LLVector3	getVector3(const std::string& name) { return LLVector3(); }
	virtual LLVector3d	getVector3d(const std::string& name) { return LLVector3d(); }
	virtual LLRect		getRect(const std::string& name) { return LLRect(); }
	virtual BOOL		getBOOL(const std::string& name) { return FALSE; }
	virtual S32			getS32(const std::string& name) { return 0; }
	virtual F32			getF32(const std::string& name) {return 0.0f; }
	virtual U32			getU32(const std::string& name) {return 0; }
	virtual LLSD        getLLSD(const std::string& name) { return LLSD(); }

	virtual LLColor4	getColor(const std::string& name) { return LLColor4(); }
	virtual LLColor4	getColor4(const std::string& name) { return LLColor4(); }
	virtual LLColor3	getColor3(const std::string& name) { return LLColor3(); }
	
	virtual void		setBOOL(const std::string& name, BOOL val) {}
	virtual void		setS32(const std::string& name, S32 val) {}
	virtual void		setF32(const std::string& name, F32 val) {}
	virtual void		setU32(const std::string& name, U32 val) {}
	virtual void		setString(const std::string&  name, const std::string& val) {}
	virtual void		setVector3(const std::string& name, const LLVector3 &val) {}
	virtual void		setVector3d(const std::string& name, const LLVector3d &val) {}
	virtual void		setRect(const std::string& name, const LLRect &val) {}
	virtual void		setColor4(const std::string& name, const LLColor4 &val) {}
	virtual void    	setLLSD(const std::string& name, const LLSD& val) {}
};

#endif /* LL_LLCONTROLGROUPREADER_H */







