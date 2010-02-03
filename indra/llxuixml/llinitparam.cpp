/** 
 * @file llinitparam.cpp
 * @brief parameter block abstraction for creating complex objects and 
 * parsing construction parameters from xml and LLSD
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llinitparam.h"


namespace LLInitParam
{
	//
	// Param
	//
	Param::Param(BaseBlock* enclosing_block)
	:	mIsProvided(false)
	{
		const U8* my_addr = reinterpret_cast<const U8*>(this);
		const U8* block_addr = reinterpret_cast<const U8*>(enclosing_block);
		mEnclosingBlockOffset = (U16)(my_addr - block_addr);
	}

	//
	// Parser
	//
	Parser::~Parser()
	{}

	void Parser::parserWarning(const std::string& message)
	{
		if (mParseSilently) return;
		llwarns << message << llendl;
	}
	
	void Parser::parserError(const std::string& message)
	{
		if (mParseSilently) return;
		llerrs << message << llendl;
	}


	//
	// BlockDescriptor
	//
	void BlockDescriptor::aggregateBlockData(BlockDescriptor& src_block_data) 
	{
		mNamedParams.insert(src_block_data.mNamedParams.begin(), src_block_data.mNamedParams.end());
		mSynonyms.insert(src_block_data.mSynonyms.begin(), src_block_data.mSynonyms.end());
		std::copy(src_block_data.mUnnamedParams.begin(), src_block_data.mUnnamedParams.end(), std::back_inserter(mUnnamedParams));
		std::copy(src_block_data.mValidationList.begin(), src_block_data.mValidationList.end(), std::back_inserter(mValidationList));
		std::copy(src_block_data.mAllParams.begin(), src_block_data.mAllParams.end(), std::back_inserter(mAllParams));
	}

	//
	// BaseBlock
	//
	BaseBlock::BaseBlock()
	:	mChangeVersion(0),
		mBlockDescriptor(NULL)
	{}

	BaseBlock::~BaseBlock()
	{}

	// called by each derived class in least to most derived order
	void BaseBlock::init(BlockDescriptor& descriptor, BlockDescriptor& base_descriptor, size_t block_size)
	{
		mBlockDescriptor = &descriptor;

		descriptor.mCurrentBlockPtr = this;
		descriptor.mMaxParamOffset = block_size;

		switch(descriptor.mInitializationState)
		{
		case BlockDescriptor::UNINITIALIZED:
			// copy params from base class here
			descriptor.aggregateBlockData(base_descriptor);

			descriptor.mInitializationState = BlockDescriptor::INITIALIZING;
			break;
		case BlockDescriptor::INITIALIZING:
			descriptor.mInitializationState = BlockDescriptor::INITIALIZED;
			break;
		case BlockDescriptor::INITIALIZED:
			// nothing to do
			break;
		}
	}

	param_handle_t BaseBlock::getHandleFromParam(const Param* param) const
	{
		const U8* param_address = reinterpret_cast<const U8*>(param);
		const U8* baseblock_address = reinterpret_cast<const U8*>(this);
		return (param_address - baseblock_address);
	}

	bool BaseBlock::submitValue(const Parser::name_stack_t& name_stack, Parser& p, bool silent)
	{
		if (!deserializeBlock(p, std::make_pair(name_stack.begin(), name_stack.end())))
		{
			if (!silent)
			{
				p.parserWarning(llformat("Failed to parse parameter \"%s\"", p.getCurrentElementName().c_str()));
			}
			return false;
		}
		return true;
	}


	bool BaseBlock::validateBlock(bool silent) const
	{
		const BlockDescriptor& block_data = getBlockDescriptor();
		for (BlockDescriptor::param_validation_list_t::const_iterator it = block_data.mValidationList.begin(); it != block_data.mValidationList.end(); ++it)
		{
			const Param* param = getParamFromHandle(it->first);
			if (!it->second(param))
			{
				if (!silent)
				{
					llwarns << "Invalid param \"" << getParamName(block_data, param) << "\"" << llendl;
				}
				return false;
			}
		}
		return true;
	}

	bool BaseBlock::serializeBlock(Parser& parser, Parser::name_stack_t name_stack, const LLInitParam::BaseBlock* diff_block) const
	{
		// named param is one like LLView::Params::follows
		// unnamed param is like LLView::Params::rect - implicit
		const BlockDescriptor& block_data = getBlockDescriptor();

		for (BlockDescriptor::param_list_t::const_iterator it = block_data.mUnnamedParams.begin(); 
			it != block_data.mUnnamedParams.end(); 
			++it)
		{
			param_handle_t param_handle = (*it)->mParamHandle;
			const Param* param = getParamFromHandle(param_handle);
			ParamDescriptor::serialize_func_t serialize_func = (*it)->mSerializeFunc;
			if (serialize_func)
			{
				const Param* diff_param = diff_block ? diff_block->getParamFromHandle(param_handle) : NULL;
				// each param descriptor remembers its serial number
				// so we can inspect the same param under different names
				// and see that it has the same number
				(*it)->mGeneration = parser.newParseGeneration();
				name_stack.push_back(std::make_pair("", (*it)->mGeneration));
				serialize_func(*param, parser, name_stack, diff_param);
				name_stack.pop_back();
			}
		}

		for(BlockDescriptor::param_map_t::const_iterator it = block_data.mNamedParams.begin();
			it != block_data.mNamedParams.end();
			++it)
		{
			param_handle_t param_handle = it->second->mParamHandle;
			const Param* param = getParamFromHandle(param_handle);
			ParamDescriptor::serialize_func_t serialize_func = it->second->mSerializeFunc;
			if (serialize_func)
			{
				// Ensure this param has not already been serialized
				// Prevents <rect> from being serialized as its own tag.
				bool duplicate = false;
				for (BlockDescriptor::param_list_t::const_iterator it2 = block_data.mUnnamedParams.begin(); 
					it2 != block_data.mUnnamedParams.end(); 
					++it2)
				{
					if (param_handle == (*it2)->mParamHandle)
					{
						duplicate = true;
						break;
					}
				}

				//FIXME: for now, don't attempt to serialize values under synonyms, as current parsers
				// don't know how to detect them
				if (duplicate) 
				{
					continue;
				}

				if (!duplicate)
				{
					it->second->mGeneration = parser.newParseGeneration();
				}

				name_stack.push_back(std::make_pair(it->first, it->second->mGeneration));
				const Param* diff_param = diff_block ? diff_block->getParamFromHandle(param_handle) : NULL;
				serialize_func(*param, parser, name_stack, diff_param);
				name_stack.pop_back();
			}
		}

		return true;
	}

	bool BaseBlock::inspectBlock(Parser& parser, Parser::name_stack_t name_stack) const
	{
		// named param is one like LLView::Params::follows
		// unnamed param is like LLView::Params::rect - implicit
		const BlockDescriptor& block_data = getBlockDescriptor();

		for (BlockDescriptor::param_list_t::const_iterator it = block_data.mUnnamedParams.begin(); 
			it != block_data.mUnnamedParams.end(); 
			++it)
		{
			param_handle_t param_handle = (*it)->mParamHandle;
			const Param* param = getParamFromHandle(param_handle);
			ParamDescriptor::inspect_func_t inspect_func = (*it)->mInspectFunc;
			if (inspect_func)
			{
				(*it)->mGeneration = parser.newParseGeneration();
				name_stack.push_back(std::make_pair("", (*it)->mGeneration));
				inspect_func(*param, parser, name_stack, (*it)->mMinCount, (*it)->mMaxCount);
				name_stack.pop_back();
			}
		}

		for(BlockDescriptor::param_map_t::const_iterator it = block_data.mNamedParams.begin();
			it != block_data.mNamedParams.end();
			++it)
		{
			param_handle_t param_handle = it->second->mParamHandle;
			const Param* param = getParamFromHandle(param_handle);
			ParamDescriptor::inspect_func_t inspect_func = it->second->mInspectFunc;
			if (inspect_func)
			{
				// Ensure this param has not already been inspected
				bool duplicate = false;
				for (BlockDescriptor::param_list_t::const_iterator it2 = block_data.mUnnamedParams.begin(); 
					it2 != block_data.mUnnamedParams.end(); 
					++it2)
				{
					if (param_handle == (*it2)->mParamHandle)
					{
						duplicate = true;
						break;
					}
				}

				if (!duplicate)
				{
					it->second->mGeneration = parser.newParseGeneration();
				}
				name_stack.push_back(std::make_pair(it->first, it->second->mGeneration));
				inspect_func(*param, parser, name_stack, it->second->mMinCount, it->second->mMaxCount);
				name_stack.pop_back();
			}
		}

		for(BlockDescriptor::param_map_t::const_iterator it = block_data.mSynonyms.begin();
			it != block_data.mSynonyms.end();
			++it)
		{
			param_handle_t param_handle = it->second->mParamHandle;
			const Param* param = getParamFromHandle(param_handle);
			ParamDescriptor::inspect_func_t inspect_func = it->second->mInspectFunc;
			if (inspect_func)
			{
				// use existing serial number for param
				name_stack.push_back(std::make_pair(it->first, it->second->mGeneration));
				inspect_func(*param, parser, name_stack, it->second->mMinCount, it->second->mMaxCount);
				name_stack.pop_back();
			}
		}

		return true;
	}

	bool BaseBlock::deserializeBlock(Parser& p, Parser::name_stack_range_t name_stack)
	{
		BlockDescriptor& block_data = getBlockDescriptor();
		bool names_left = name_stack.first != name_stack.second;

		if (names_left)
		{
			const std::string& top_name = name_stack.first->first;

			ParamDescriptor::deserialize_func_t deserialize_func = NULL;
			Param* paramp = NULL;

			BlockDescriptor::param_map_t::iterator found_it = block_data.mNamedParams.find(top_name);
			if (found_it != block_data.mNamedParams.end())
			{
				// find pointer to member parameter from offset table
				paramp = getParamFromHandle(found_it->second->mParamHandle);
				deserialize_func = found_it->second->mDeserializeFunc;
			}
			else
			{
				BlockDescriptor::param_map_t::iterator found_it = block_data.mSynonyms.find(top_name);
				if (found_it != block_data.mSynonyms.end())
				{
					// find pointer to member parameter from offset table
					paramp = getParamFromHandle(found_it->second->mParamHandle);
					deserialize_func = found_it->second->mDeserializeFunc;
				}
			}
					
			Parser::name_stack_range_t new_name_stack(name_stack.first, name_stack.second);
			++new_name_stack.first;
			if (deserialize_func)
			{
				return deserialize_func(*paramp, p, new_name_stack, name_stack.first == name_stack.second ? -1 : name_stack.first->second);
			}
		}

		// try to parse unnamed parameters, in declaration order
		for ( BlockDescriptor::param_list_t::iterator it = block_data.mUnnamedParams.begin(); 
			it != block_data.mUnnamedParams.end(); 
			++it)
		{
			Param* paramp = getParamFromHandle((*it)->mParamHandle);
			ParamDescriptor::deserialize_func_t deserialize_func = (*it)->mDeserializeFunc;

			if (deserialize_func && deserialize_func(*paramp, p, name_stack, name_stack.first == name_stack.second ? -1 : name_stack.first->second))
			{
				return true;
			}
		}

		return false;
	}

	//static 
	void BaseBlock::addParam(BlockDescriptor& block_data, const ParamDescriptor& in_param, const char* char_name)
	{
		// create a copy of the paramdescriptor in allparams
		// so other data structures can store a pointer to it
		block_data.mAllParams.push_back(in_param);
		ParamDescriptor& param(block_data.mAllParams.back());

		std::string name(char_name);
		if ((size_t)param.mParamHandle > block_data.mMaxParamOffset)
		{
			llerrs << "Attempted to register param with block defined for parent class, make sure to derive from LLInitParam::Block<YOUR_CLASS, PARAM_BLOCK_BASE_CLASS>" << llendl;
		}

		if (name.empty())
		{
			block_data.mUnnamedParams.push_back(&param);
		}
		else
		{
			// don't use insert, since we want to overwrite existing entries
			block_data.mNamedParams[name] = &param;
		}

		if (param.mValidationFunc)
		{
			block_data.mValidationList.push_back(std::make_pair(param.mParamHandle, param.mValidationFunc));
		}
	}

	void BaseBlock::addSynonym(Param& param, const std::string& synonym)
	{
		BlockDescriptor& block_data = getBlockDescriptor();
		if (block_data.mInitializationState == BlockDescriptor::INITIALIZING)
		{
			param_handle_t handle = getHandleFromParam(&param);
			
			// check for invalid derivation from a paramblock (i.e. without using
			// Block<T, Base_Class>
			if ((size_t)handle > block_data.mMaxParamOffset)
			{
				llerrs << "Attempted to register param with block defined for parent class, make sure to derive from LLInitParam::Block<YOUR_CLASS, PARAM_BLOCK_BASE_CLASS>" << llendl;
			}

			ParamDescriptor* param_descriptor = findParamDescriptor(handle);
			if (param_descriptor)
			{
				if (synonym.empty())
				{
					block_data.mUnnamedParams.push_back(param_descriptor);
				}
				else
				{
					block_data.mSynonyms[synonym] = param_descriptor;
				}
			}
		}
	}

	void BaseBlock::setLastChangedParam(const Param& last_param, bool user_provided)
	{ 
		if (user_provided)
		{
		mChangeVersion++;
	}
	}

	const std::string& BaseBlock::getParamName(const BlockDescriptor& block_data, const Param* paramp) const
	{
		param_handle_t handle = getHandleFromParam(paramp);
		for (BlockDescriptor::param_map_t::const_iterator it = block_data.mNamedParams.begin(); it != block_data.mNamedParams.end(); ++it)
		{
			if (it->second->mParamHandle == handle)
			{
				return it->first;
			}
		}

		for (BlockDescriptor::param_map_t::const_iterator it = block_data.mSynonyms.begin(); it != block_data.mSynonyms.end(); ++it)
		{
			if (it->second->mParamHandle == handle)
			{
				return it->first;
			}
		}

		return LLStringUtil::null;
	}

	ParamDescriptor* BaseBlock::findParamDescriptor(param_handle_t handle)
	{
		BlockDescriptor& descriptor = getBlockDescriptor();
		BlockDescriptor::all_params_list_t::iterator end_it = descriptor.mAllParams.end();
		for (BlockDescriptor::all_params_list_t::iterator it = descriptor.mAllParams.begin();
			it != end_it;
			++it)
		{
			if (it->mParamHandle == handle) return &(*it);
		}
		return NULL;
	}

	// take all provided params from other and apply to self
	// NOTE: this requires that "other" is of the same derived type as this
	bool BaseBlock::overwriteFromImpl(BlockDescriptor& block_data, const BaseBlock& other)
	{
		bool param_changed = false;
		BlockDescriptor::all_params_list_t::const_iterator end_it = block_data.mAllParams.end();
		for (BlockDescriptor::all_params_list_t::const_iterator it = block_data.mAllParams.begin();
			it != end_it;
			++it)
		{
			const Param* other_paramp = other.getParamFromHandle(it->mParamHandle);
			ParamDescriptor::merge_func_t merge_func = it->mMergeFunc;
			if (merge_func)
			{
				Param* paramp = getParamFromHandle(it->mParamHandle);
				param_changed |= merge_func(*paramp, *other_paramp, true);
			}
		}
		return param_changed;
	}

	// take all provided params that are not already provided, and apply to self
	bool BaseBlock::fillFromImpl(BlockDescriptor& block_data, const BaseBlock& other)
	{
		bool param_changed = false;
		BlockDescriptor::all_params_list_t::const_iterator end_it = block_data.mAllParams.end();
		for (BlockDescriptor::all_params_list_t::const_iterator it = block_data.mAllParams.begin();
			it != end_it;
			++it)
		{
			const Param* other_paramp = other.getParamFromHandle(it->mParamHandle);
			ParamDescriptor::merge_func_t merge_func = it->mMergeFunc;
			if (merge_func)
			{
				Param* paramp = getParamFromHandle(it->mParamHandle);
				param_changed |= merge_func(*paramp, *other_paramp, false);
			}
		}
		return param_changed;
	}

	bool ParamCompare<LLSD, false>::equals(const LLSD &a, const LLSD &b)
	{
		return false;
	}
}
