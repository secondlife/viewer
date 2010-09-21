/** 
 * @file llvolumexml.cpp
 * @brief LLVolumeXml base class
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

#include "linden_common.h"

#include "llvolumexml.h"

//============================================================================

// LLVolumeXml is just a wrapper class; all members are static

//============================================================================

LLXMLNode *LLVolumeXml::exportProfileParams(const LLProfileParams* params)
{
	LLXMLNode *ret = new LLXMLNode("profile", FALSE);

	ret->createChild("curve_type", TRUE)->setByteValue(1, &params->getCurveType());
	ret->createChild("interval", FALSE)->setFloatValue(2, &params->getBegin());
	ret->createChild("hollow", FALSE)->setFloatValue(1, &params->getHollow());

	return ret;
}


LLXMLNode *LLVolumeXml::exportPathParams(const LLPathParams* params)
{
	LLXMLNode *ret = new LLXMLNode("path", FALSE); 
	ret->createChild("curve_type", TRUE)->setByteValue(1, &params->getCurveType());
	ret->createChild("interval", FALSE)->setFloatValue(2, &params->getBegin());
	ret->createChild("scale", FALSE)->setFloatValue(2, params->getScale().mV);
	ret->createChild("shear", FALSE)->setFloatValue(2, params->getShear().mV);
	ret->createChild("twist_interval", FALSE)->setFloatValue(2, &params->getTwistBegin());
	ret->createChild("radius_offset", FALSE)->setFloatValue(1, &params->getRadiusOffset());
	ret->createChild("taper", FALSE)->setFloatValue(2, params->getTaper().mV);
	ret->createChild("revolutions", FALSE)->setFloatValue(1, &params->getRevolutions());
	ret->createChild("skew", FALSE)->setFloatValue(1, &params->getSkew());

	return ret;
}


LLXMLNode *LLVolumeXml::exportVolumeParams(const LLVolumeParams* params)
{
	LLXMLNode *ret = new LLXMLNode("shape", FALSE);
	
	exportPathParams(&params->getPathParams())->setParent(ret);
	exportProfileParams(&params->getProfileParams())->setParent(ret);

	return ret;
}

