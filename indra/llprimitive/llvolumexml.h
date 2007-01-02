/** 
 * @file llvolumexml.h
 * @brief LLVolumeXml base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOLUMEXML_H
#define LL_LLVOLUMEXML_H

#include "llvolume.h"
#include "llxmlnode.h"

// wrapper class for some volume/message functions
class LLVolumeXml
{
public:
	static LLXMLNode* exportProfileParams(const LLProfileParams* params);

	static LLXMLNode* exportPathParams(const LLPathParams* params);

	static LLXMLNode* exportVolumeParams(const LLVolumeParams* params);
};

#endif // LL_LLVOLUMEXML_H

