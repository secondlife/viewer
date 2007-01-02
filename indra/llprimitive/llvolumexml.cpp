/** 
 * @file llvolumexml.cpp
 * @brief LLVolumeXml base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

