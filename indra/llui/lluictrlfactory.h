/** 
 * @file lluictrlfactory.h
 * @brief Factory class for creating UI controls
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LLUICTRLFACTORY_H
#define LLUICTRLFACTORY_H

#include "llcallbackmap.h"
#include "llinitparam.h"
#include "llxmlnode.h"

#include <boost/function.hpp>
#include <iosfwd>
#include <stack>

class LLPanel;
class LLFloater;
class LLView;

class LLXUIParser : public LLInitParam::Parser, public LLSingleton<LLXUIParser>
{
LOG_CLASS(LLXUIParser);

protected:
	LLXUIParser();
	friend class LLSingleton<LLXUIParser>;
public:
	typedef LLInitParam::Parser::name_stack_t name_stack_t;

	/*virtual*/ std::string getCurrentElementName();
	/*virtual*/ void parserWarning(const std::string& message);
	/*virtual*/ void parserError(const std::string& message);

	void readXUI(LLXMLNodePtr node, LLInitParam::BaseBlock& block, bool silent=false);
	void writeXUI(LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const LLInitParam::BaseBlock* diff_block = NULL);

private:
	typedef std::list<std::pair<std::string, bool> >	token_list_t;

	bool readXUIImpl(LLXMLNodePtr node, const std::string& scope, LLInitParam::BaseBlock& block);
	bool readAttributes(LLXMLNodePtr nodep, LLInitParam::BaseBlock& block);

	//reader helper functions
	bool readBoolValue(void* val_ptr);
	bool readStringValue(void* val_ptr);
	bool readU8Value(void* val_ptr);
	bool readS8Value(void* val_ptr);
	bool readU16Value(void* val_ptr);
	bool readS16Value(void* val_ptr);
	bool readU32Value(void* val_ptr);
	bool readS32Value(void* val_ptr);
	bool readF32Value(void* val_ptr);
	bool readF64Value(void* val_ptr);
	bool readColor4Value(void* val_ptr);
	bool readUIColorValue(void* val_ptr);
	bool readUUIDValue(void* val_ptr);
	bool readSDValue(void* val_ptr);

	//writer helper functions
	bool writeBoolValue(const void* val_ptr, const name_stack_t&);
	bool writeStringValue(const void* val_ptr, const name_stack_t&);
	bool writeU8Value(const void* val_ptr, const name_stack_t&);
	bool writeS8Value(const void* val_ptr, const name_stack_t&);
	bool writeU16Value(const void* val_ptr, const name_stack_t&);
	bool writeS16Value(const void* val_ptr, const name_stack_t&);
	bool writeU32Value(const void* val_ptr, const name_stack_t&);
	bool writeS32Value(const void* val_ptr, const name_stack_t&);
	bool writeF32Value(const void* val_ptr, const name_stack_t&);
	bool writeF64Value(const void* val_ptr, const name_stack_t&);
	bool writeColor4Value(const void* val_ptr, const name_stack_t&);
	bool writeUIColorValue(const void* val_ptr, const name_stack_t&);
	bool writeUUIDValue(const void* val_ptr, const name_stack_t&);
	bool writeSDValue(const void* val_ptr, const name_stack_t&);

	LLXMLNodePtr getNode(const name_stack_t& stack);

private:
	Parser::name_stack_t			mNameStack;
	LLXMLNodePtr					mCurReadNode;
	// Root of the widget XML sub-tree, for example, "line_editor"
	LLXMLNodePtr					mWriteRootNode;
	S32								mLastWriteGeneration;
	LLXMLNodePtr					mLastWrittenChild;
};

// global static instance for registering all widget types
typedef boost::function<LLView* (LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node)> LLWidgetCreatorFunc;

class LLWidgetCreatorRegistry : public LLRegistrySingleton<std::string, LLWidgetCreatorFunc, LLWidgetCreatorRegistry>
{
protected:
	LLWidgetCreatorRegistry() {}
	
private:
	friend class LLSingleton<LLWidgetCreatorRegistry>;
};

struct LLCompareTypeID
{
	bool operator()(const std::type_info* lhs, const std::type_info* rhs) const
	{
		return lhs->before(*rhs);
	}
};


class LLWidgetTemplateRegistry 
:	public LLRegistrySingleton<const std::type_info*, std::string, LLWidgetTemplateRegistry, LLCompareTypeID>
{

};

// local static instance for registering a particular widget
template<typename T, typename PARAM_BLOCK = typename T::Params>
class LLRegisterWidget
:	public LLWidgetCreatorRegistry::StaticRegistrar
{
public:
	// register with either the provided builder, or the generic templated builder
	LLRegisterWidget(const char* tag, LLWidgetCreatorFunc func = NULL);
};

class LLUICtrlFactory : public LLSingleton<LLUICtrlFactory>
{
private:
	friend class LLSingleton<LLUICtrlFactory>;
	LLUICtrlFactory();
	~LLUICtrlFactory();

	// only partial specialization allowed in inner classes, so use extra dummy parameter
	template <typename T, int DUMMY>
	class ParamDefaults : public LLSingleton<ParamDefaults<T, DUMMY> > 
	{
	public:
		ParamDefaults()
		{
			// recursively initialize from base class param block
			((typename T::base_block_t&)mPrototype).fillFrom(ParamDefaults<typename T::base_block_t, DUMMY>::instance().get());
			// after initializing base classes, look up template file for this param block
			std::string* param_block_tag = LLWidgetTemplateRegistry::instance().getValue(&typeid(T));
			if (param_block_tag)
			{
				LLUICtrlFactory::loadWidgetTemplate(*param_block_tag, mPrototype);
			}
		}

		const T& get() { return mPrototype; }

	private:
		T mPrototype;
	};

	// base case for recursion, there are NO base classes of LLInitParam::BaseBlock
	template<int DUMMY>
	class ParamDefaults<LLInitParam::BaseBlock, DUMMY> : public LLSingleton<ParamDefaults<LLInitParam::BaseBlock, DUMMY> >
	{
	public:
		const LLInitParam::BaseBlock& get() { return mBaseBlock; }
	private:
		LLInitParam::BaseBlock mBaseBlock;
	};

public:

	template<typename T>
	static const T& getDefaultParams()
	{
		//#pragma message("Generating ParamDefaults")
		return ParamDefaults<T, 0>::instance().get();
	}

	void buildFloater(LLFloater* floaterp, const std::string &filename, BOOL open_floater = TRUE, LLXMLNodePtr output_node = NULL);
	LLFloater* buildFloaterFromXML(const std::string& filename, BOOL open_floater = TRUE);
	BOOL buildPanel(LLPanel* panelp, const std::string &filename, LLXMLNodePtr output_node = NULL);

	// Does what you want for LLFloaters and LLPanels
	// Returns 0 on success
	S32 saveToXML(LLView* viewp, const std::string& filename);

	std::string getCurFileName() { return mFileNames.empty() ? "" : mFileNames.back(); }

	static BOOL getAttributeColor(LLXMLNodePtr node, const std::string& name, LLColor4& color);

	LLPanel* createFactoryPanel(const std::string& name);

	void pushFactoryFunctions(const LLCallbackMap::map_t* map);
	void popFactoryFunctions();

	template<typename T>
	static T* create(typename T::Params& params, LLView* parent = NULL)
	{
		//#pragma message("Generating LLUICtrlFactory::create")
		params.fillFrom(ParamDefaults<typename T::Params, 0>::instance().get());
		//S32 foo = "test";

		if (!params.validateBlock())
		{
			llwarns << getInstance()->getCurFileName() << ": Invalid parameter block for " << typeid(T).name() << llendl;
		}
		T* widget = new T(params);
		widget->initFromParams(params);
		if (parent)
			widget->setParent(parent);
		return widget;
	}

	LLView* createFromXML(LLXMLNodePtr node, LLView* parent, const std::string& filename = LLStringUtil::null,  LLXMLNodePtr output_node = NULL);

	template<typename T>
	static T* createFromFile(const std::string &filename, LLView *parent)
	{
		//#pragma message("Generating LLUICtrlFactory::createFromFile")
		T* widget = NULL;

		std::string skinned_filename = findSkinnedFilename(filename);
		getInstance()->mFileNames.push_back(skinned_filename);
		{
			LLXMLNodePtr root_node;

			if (LLUICtrlFactory::getLayeredXMLNode(filename, root_node))
			{
				LLView* view = getInstance()->createFromXML(root_node, parent, filename);
				if (view)
				{
					widget = dynamic_cast<T*>(view);
					// not of right type, so delete it
					if (!widget) 
					{
						delete view;
						view = NULL;
					}
				
				}
			}
			else
			{
				llwarns << "Couldn't parse XUI file: " << skinned_filename << llendl;
			}
		}
		getInstance()->mFileNames.pop_back();

		return widget;
	}

	template <class T> 
	static T* createDummyWidget(const std::string& name) 
	{
		//#pragma message("Generating LLUICtrlFactory::createDummyWidget")
		typename T::Params params;
		params.name(name);
		
		return create<T>(params);
	}

	template<typename T, typename PARAM_BLOCK>
	static T* defaultBuilder(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node)
	{
		//#pragma message("Generating LLUICtrlFactory::defaultBuilder")
		PARAM_BLOCK params(getDefaultParams<PARAM_BLOCK>());

		LLXUIParser::instance().readXUI(node, params);

		if (output_node)
		{
			// We always want to output top-left coordinates
			PARAM_BLOCK output_params(params);
			T::setupParamsForExport(output_params, parent);
			// Export only the differences between this any default params
			PARAM_BLOCK default_params(getDefaultParams<PARAM_BLOCK>());
			output_node->setName(node->getName()->mString);
			LLXUIParser::instance().writeXUI(
				output_node, output_params, &default_params);
		}

		// Apply layout transformations, usually munging rect
		T::setupParams(params, parent);

		if (!params.validateBlock())
		{
			llwarns << getInstance()->getCurFileName() << ": Invalid parameter block for " << typeid(T).name() << llendl;
		}
		T* widget = new T(params);
		widget->initFromParams(params);

		if (parent)
		{
			S32 tab_group = params.tab_group.isProvided() ? params.tab_group() : -1;
			setCtrlParent(widget, parent, tab_group);
		}

		widget->addChildren(node, output_node);

		if (!widget->postBuild())
		{
			delete widget;
			return NULL;
		}

		return widget;
	}

	static bool getLayeredXMLNode(const std::string &filename, LLXMLNodePtr& root);

	static void loadWidgetTemplate(const std::string& widget_tag, LLInitParam::BaseBlock& block);

private:
	//static void setCtrlValue(LLView* view, LLXMLNodePtr node);
	static void setCtrlParent(LLView* view, LLView* parent, S32 tab_group);

	// Avoid directly using LLUI and LLDir in the template code
	static std::string findSkinnedFilename(const std::string& filename);

	typedef std::deque<const LLCallbackMap::map_t*> factory_stack_t;
	factory_stack_t					mFactoryStack;

	LLPanel*		mDummyPanel;
	std::vector<std::string>	mFileNames;
};

// this is here to make gcc happy with reference to LLUICtrlFactory
template<typename T, typename PARAM_BLOCK>
LLRegisterWidget<T, PARAM_BLOCK>::LLRegisterWidget(const char* tag, LLWidgetCreatorFunc func)
:	LLWidgetCreatorRegistry::StaticRegistrar(tag, func.empty() ? (LLWidgetCreatorFunc)&LLUICtrlFactory::defaultBuilder<T, PARAM_BLOCK> : func)
{
	//FIXME: inventory_panel will register itself with LLPanel::Params but it does have its own params...:(
	LLWidgetTemplateRegistry::instance().defaultRegistrar().add(&typeid(PARAM_BLOCK), tag);
}


#endif //LLUICTRLFACTORY_H
