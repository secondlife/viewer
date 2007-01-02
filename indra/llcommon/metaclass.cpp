/** 
 * @file metaclass.cpp
 * @author Babbage
 * @date 2006-05-15
 * @brief Implementation of LLMetaClass
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

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

