/** 
 * @file metaclass.cpp
 * @author Babbage
 * @date 2006-05-15
 * @brief Implementation of LLMetaClass
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

