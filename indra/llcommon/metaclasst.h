/** 
 * @file metaclasst.h
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_METACLASST_H
#define LL_METACLASST_H

#include "metaclass.h"

template<class TObject>
class LLMetaClassT : public LLMetaClass
{
	public:
		
		virtual ~LLMetaClassT() {;}
		
		static const LLMetaClassT& instance()
		{
			static const LLMetaClassT& instance = buildMetaClass();
			return instance;
		}
	
	private:
	
		static const LLMetaClassT& buildMetaClass()
		{
			LLMetaClassT& meta_class = *(new LLMetaClassT());
			reflectProperties(meta_class);
			return meta_class;
		}
	
		LLMetaClassT() {;}
	
		static void reflectProperties(LLMetaClass&)
		{
		}
};

#endif // LL_METACLASST_H
