/** 
 * @file metaclass.cpp
 * @author Babbage
 * @date 2006-05-15
 * @brief Implementation of LLMetaClass
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "metaclass.h"

#include "metaproperty.h"
#include "reflective.h"

LLMetaClass::LLMetaClass()
{
}

//virtual 
LLMetaClass::~LLMetaClass()
{
}

const LLMetaProperty* LLMetaClass::findProperty(const std::string& name) const
{
	PropertyIterator iter = mProperties.find(name);
	if(iter == mProperties.end())
	{
		return NULL;
	}
	return (*iter).second;
}

void LLMetaClass::addProperty(const LLMetaProperty* property)
{
	mProperties.insert(std::make_pair(property->getName(), property));
}

U32 LLMetaClass::getPropertyCount() const
{
	return mProperties.size();
}

LLMetaClass::PropertyIterator LLMetaClass::beginProperties() const
{
	return mProperties.begin();
}

LLMetaClass::PropertyIterator LLMetaClass::endProperties() const
{
	return mProperties.end();
}

bool LLMetaClass::isInstance(const LLReflective* object) const
{
	// TODO: Babbage: Search through super classes of objects MetaClass.
	const LLMetaClass* object_meta_class = &(object->getMetaClass());
	return (object_meta_class == this);
}

