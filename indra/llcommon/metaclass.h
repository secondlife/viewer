/** 
 * @file metaclass.h
 * @author Babbage
 * @date 2006-05-15
 * @brief Reflective meta information describing a class.
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

#ifndef LL_METACLASS_H
#define LL_METACLASS_H

#include <string>
#include <map>

#include "stdtypes.h"

class LLReflective;
class LLMetaProperty;
class LLMetaMethod;
class LL_COMMON_API LLMetaClass
{
public:

	LLMetaClass();
	virtual ~LLMetaClass();
  
	// Create instance of this MetaClass. NULL if class is abstract. 
	// Gives ownership of returned object.
	// virtual LLReflective* create() const = 0;
  
	// Returns named property or NULL.
	const LLMetaProperty* findProperty(const std::string& name) const;
  
	// Add property to metaclass. Takes ownership of given property.
	void addProperty(const LLMetaProperty* property);
	
	typedef std::map<std::string, const LLMetaProperty*>::const_iterator PropertyIterator;
	
	U32 getPropertyCount() const;
	
	PropertyIterator beginProperties() const;
	PropertyIterator endProperties() const;
  
	// Returns named property or NULL.
	// const LLMetaMethod* findMethod(const std::string& name) const;
  
	// Add method to metaclass. Takes ownership of given method.
	// void addMethod(const LLMetaMethod* method);
  
	// Find MetaClass by name. NULL if name is unknown.
	// static LLMetaClass* findClass(const std::string& name);
	
	// True if object is instance of this meta class.
	bool isInstance(const LLReflective* object) const;
  
private:

	typedef std::map<std::string, const LLMetaProperty*> PropertyMap;
	PropertyMap mProperties;
};

#endif // LL_METACLASS_H
