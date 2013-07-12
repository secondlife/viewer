/** 
 * @file llvolumexml.cpp
 * @brief LLVolumeXml base class
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

#include "linden_common.h"

#include "llvolumexml.h"

//============================================================================

// LLVolumeXml is just a wrapper class; all members are static

//============================================================================

LLPointer<LLXMLNode> LLVolumeXml::exportProfileParams(const LLProfileParams* params)
{
	LLPointer<LLXMLNode> ret = new LLXMLNode("profile", FALSE);

	ret->createChild("curve_type", TRUE)->setByteValue(1, &params->getCurveType());
	ret->createChild("interval", FALSE)->setFloatValue(2, &params->getBegin());
	ret->createChild("hollow", FALSE)->setFloatValue(1, &params->getHollow());

	return ret;
}


LLPointer<LLXMLNode> LLVolumeXml::exportPathParams(const LLPathParams* params)
{
	LLPointer<LLXMLNode> ret = new LLXMLNode("path", FALSE); 
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


LLPointer<LLXMLNode> LLVolumeXml::exportVolumeParams(const LLVolumeParams* params)
{
	LLPointer<LLXMLNode> ret = new LLXMLNode("shape", FALSE);
	
	LLPointer<LLXMLNode> node ;
	node = exportPathParams(&params->getPathParams()) ;
	node->setParent(ret);
	node = exportProfileParams(&params->getProfileParams()) ;
	node->setParent(ret);

	return ret;
}

