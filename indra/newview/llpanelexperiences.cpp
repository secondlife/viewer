#include "llviewerprecompiledheaders.h"


#include "llpanelprofile.h"
#include "lluictrlfactory.h"
#include "llexperiencecache.h"
#include "llagent.h"

#include "llpanelexperiences.h"


static LLRegisterPanelClassWrapper<LLPanelExperiences> register_experiences_panel("experiences_panel");


LLPanelExperiences::LLPanelExperiences(  )
	:	mExperiencesList(NULL),
	mExperiencesAccTab(NULL),
	mProfilePanel(NULL),
	mPanelExperienceInfo(NULL),
	mNoExperiences(false)
{

}

void* LLPanelExperiences::create( void* data )
{
	return new LLPanelExperiences();
}

void ExperienceResult(LLHandle<LLPanelExperiences> panel, const LLSD& experience)
{
	LLPanelExperiences* experiencePanel = panel.get();
	if(experiencePanel)
	{
		experiencePanel->addExperienceInfo(experience);
	}
}

class LLExperienceListResponder : public LLHTTPClient::Responder
{
public:
	LLExperienceListResponder(const LLHandle<LLPanelExperiences>& parent):mParent(parent)
	{
	}

	LLHandle<LLPanelExperiences> mParent;

	virtual void result(const LLSD& content)
	{
		if(mParent.isDead())
			return;

		LLSD experiences = content["experiences"];
		LLSD::array_const_iterator it = experiences.beginArray();
		for( /**/ ; it != experiences.endArray(); ++it)
		{
			LLUUID public_key = it->asUUID();

			LLExperienceCache::get(public_key, LLExperienceCache::PUBLIC_KEY, boost::bind(ExperienceResult, mParent, _1));
		}
	}
};

void LLPanelExperiences::addExperienceInfo(const LLSD& experience)
{
	LLExperienceItem* item = new LLExperienceItem();
	if(experience.has(LLExperienceCache::NAME))
	{
		item->setExperienceName(experience[LLExperienceCache::NAME].asString());
	}
	else if(experience.has("error"))
	{
		item->setExperienceName(experience["error"].asString());
	}

	if(experience.has(LLExperienceCache::PUBLIC_KEY))
	{
		item->setExperienceDescription(experience[LLExperienceCache::PUBLIC_KEY].asString());
	}

	mExperiencesList->addItem(item);

}


BOOL LLPanelExperiences::postBuild( void )
{
	mExperiencesList = getChild<LLFlatListView>("experiences_list");
	if(hasString("no_experiences"))
	{
		mExperiencesList->setNoItemsCommentText(getString("no_experiences"));
	}


	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		std::string lookup_url=region->getCapability("GetExperiences"); 
		if(!lookup_url.empty())
		{
			LLHTTPClient::get(lookup_url, new LLExperienceListResponder(getDerivedHandle<LLPanelExperiences>()));
		}
	}

	mExperiencesAccTab = getChild<LLAccordionCtrlTab>("tab_experiences");
	mExperiencesAccTab->setDropDownStateChangedCallback(boost::bind(&LLPanelExperiences::onAccordionStateChanged, this, mExperiencesAccTab));
	mExperiencesAccTab->setDisplayChildren(true);

	return TRUE;
}

void LLPanelExperiences::onOpen( const LLSD& key )
{
	LLPanel::onOpen(key);
}

void LLPanelExperiences::onClosePanel()
{
	if (mPanelExperienceInfo)
	{
		onPanelExperienceClose(mPanelExperienceInfo);
	}
}

void LLPanelExperiences::updateData()
{
	if(isDirty())
	{
		mNoExperiences = false;
	}
}

LLExperienceItem* LLPanelExperiences::getSelectedExperienceItem()
{
	LLPanel* selected_item = mExperiencesList->getSelectedItem();
	if (!selected_item) return NULL;

	return dynamic_cast<LLExperienceItem*>(selected_item);
}

void LLPanelExperiences::setProfilePanel( LLPanelProfile* profile_panel )
{
	mProfilePanel = profile_panel;
}

void LLPanelExperiences::onListCommit( const LLFlatListView* f_list )
{
	if(f_list == mExperiencesList)
	{
		mExperiencesList->resetSelection(true);
	}
	else
	{
		llwarns << "Unknown list" << llendl;
	}
	
	//updateButtons();
}

void LLPanelExperiences::onAccordionStateChanged( const LLAccordionCtrlTab* acc_tab )
{
	if(!mExperiencesAccTab->getDisplayChildren())
	{
		mExperiencesList->resetSelection(true);
	}

}

void LLPanelExperiences::openExperienceInfo()
{
	LLSD selected_value = mExperiencesList->getSelectedValue();
	if(selected_value.isUndefined())
	{
		return;
	}

	LLExperienceItem* experience = (LLExperienceItem*)mExperiencesList->getSelectedItem();

	createExperienceInfoPanel();

	LLSD params;
	params["experience_name"] = experience->getExperienceName();
	params["experience_desc"] = experience->getExperienceDescription();

	getProfilePanel()->openPanel(mPanelExperienceInfo, params);

}


void LLPanelExperiences::createExperienceInfoPanel()
{
	if(!mPanelExperienceInfo)
	{
		mPanelExperienceInfo = LLPanelExperienceInfo::create();
		mPanelExperienceInfo->setExitCallback(boost::bind(&LLPanelExperiences::onPanelExperienceClose, this, mPanelExperienceInfo));
		mPanelExperienceInfo->setVisible(FALSE);
	}
}

void LLPanelExperiences::onPanelExperienceClose( LLPanel* panel )
{
	getProfilePanel()->closePanel(panel);
}

LLPanelProfile* LLPanelExperiences::getProfilePanel()
{
	llassert_always(NULL != mProfilePanel);
	
	return mProfilePanel;
}











LLExperienceItem::LLExperienceItem()
{
	buildFromFile("panel_experience_info.xml");
}

void LLExperienceItem::init( LLSD* experience_data )
{
	if(experience_data)
	{
		setExperienceDescription(experience_data->has(LLExperienceCache::PUBLIC_KEY)?(*experience_data)[LLExperienceCache::PUBLIC_KEY].asString() : std::string());
		setExperienceName(experience_data->has(LLExperienceCache::NAME)?(*experience_data)[LLExperienceCache::NAME].asString() : std::string());
	}
}

void LLExperienceItem::setExperienceDescription( const std::string& val )
{
	mExperienceDescription = val;
	getChild<LLUICtrl>("experience_desc")->setValue(val);
}

void LLExperienceItem::setExperienceName( const std::string& val )
{
	mExperienceName = val;
	getChild<LLUICtrl>("experience_name")->setValue(val);
}

BOOL LLExperienceItem::postBuild()
{
	return TRUE;
}

void LLExperienceItem::update()
{

}

void LLExperienceItem::processProperties( void* data, EAvatarProcessorType type )
{

}

LLExperienceItem::~LLExperienceItem()
{

}


void LLPanelExperienceInfo::setExperienceName( const std::string& name )
{
	getChild<LLUICtrl>("experience_name")->setValue(name);
}

void LLPanelExperienceInfo::setExperienceDesc( const std::string& desc )
{
	getChild<LLUICtrl>("experience_desc")->setValue(desc);
}

void LLPanelExperienceInfo::onOpen( const LLSD& key )
{
	setExperienceName(key["experience_name"]);
	setExperienceDesc(key["experience_desc"]);

	/*
	LLAvatarPropertiesProcessor::getInstance()->addObserver(
	getAvatarId(), this);
	LLAvatarPropertiesProcessor::getInstance()->sendPickInfoRequest(
	getAvatarId(), getPickId());
	*/
}

LLPanelExperienceInfo* LLPanelExperienceInfo::create()
{
	LLPanelExperienceInfo* panel = new LLPanelExperienceInfo();
	panel->buildFromFile("panel_experience_info.xml");
	return panel;
}

void LLPanelExperienceInfo::setExitCallback( const commit_callback_t& cb )
{
	getChild<LLButton>("back_btn")->setClickedCallback(cb);
}
