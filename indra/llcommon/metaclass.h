/** 
 * @file metaclass.h
 * @author Babbage
 * @date 2006-05-15
 * @brief Reflective meta information describing a class.
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

#ifndef LL_METACLASS_H
#define LL_METACLASS_H

#include <string>
#include <map>

#include "stdtypes.h"

class LLReflective;
class LLMetaProperty;
class LLMetaMethod;
class LLMetaClass
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
