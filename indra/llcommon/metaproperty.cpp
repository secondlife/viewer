/** 
 * @file metaproperty.cpp
 * @author Babbage
 * @date 2006-05-15
 * @brief Implementation of LLMetaProperty.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "metaproperty.h"
#include "metaclass.h"

LLMetaProperty::LLMetaProperty(const std::string& name, const LLMetaClass& object_class) : 
	mName(name), mObjectClass(object_class) 
{
}

//virtual 
LLMetaProperty::~LLMetaProperty()
{
}

const LLMetaClass& LLMetaProperty::getObjectMetaClass() const
{
	return mObjectClass;
}

void LLMetaProperty::checkObjectClass(const LLReflective* object) const
{
	if(! mObjectClass.isInstance(object))
	{
		throw "class cast exception";
	}
}
