#include "llviewerprecompiledheaders.h"


#include "llpanelprofile.h"
#include "lluictrlfactory.h"

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


BOOL LLPanelExperiences::postBuild( void )
{
	mExperiencesList = getChild<LLFlatListView>("experiences_list");
	if(hasString("no_experiences"))
	{
		mExperiencesList->setNoItemsCommentText(getString("no_experiences"));
	}

	LLExperienceItem* item = new LLExperienceItem();
	item->setExperienceName("experience 1");
	item->setExperienceDescription("hey, I\'m an experience!");
	mExperiencesList->addItem(item);
	
	item = new LLExperienceItem();
	item->setExperienceName("experience 2");
	item->setExperienceDescription("hey, I\'m another experience!");
	mExperiencesList->addItem(item);

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

		/*
		mNoItemsLabel->setValue(LLTrans::getString("PicksClassifiedsLoadingText"));
		mNoItemsLabel->setVisible(TRUE);

		mPicksList->clear();
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarPicksRequest(getAvatarId());

		mClassifiedsList->clear();
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarClassifiedsRequest(getAvatarId());
		*/
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

void LLExperienceItem::init( LLExperienceData* experience_data )
{

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
