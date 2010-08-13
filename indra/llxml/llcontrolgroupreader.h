/** 
 * @file llcontrolgroupreader.h
 * @brief Interface providing readonly access to LLControlGroup (intended for unit testing)
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
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







