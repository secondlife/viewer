/** 
 * @file llviewermenufile.cpp
 * @brief "File" menu in the main menu bar.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llviewermenufile.h"

// project includes
#include "llagent.h"
#include "llagentbenefits.h"
#include "llagentcamera.h"
#include "llfilepicker.h"
#include "llfloaterreg.h"
#include "llbuycurrencyhtml.h"
#include "llfloatermap.h"
#include "llfloatermodelpreview.h"
#include "llmaterialeditor.h"
#include "llfloaterperms.h"
#include "llfloatersnapshot.h"
#include "llfloatersimplesnapshot.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagepng.h"
#include "llimagej2c.h"
#include "llimagejpeg.h"
#include "llimagetga.h"
#include "llinventorymodel.h"	// gInventory
#include "llpluginclassmedia.h"
#include "llresourcedata.h"
#include "llstatusbar.h"
#include "lltinygltfhelper.h"
#include "lltoast.h"
#include "llviewercontrol.h"	// gSavedSettings
#include "llviewertexturelist.h"
#include "lluictrlfactory.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"	// gMenuHolder
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "lluploaddialog.h"
#include "lltrans.h"
#include "llfloaterbuycurrency.h"
#include "llviewerassetupload.h"

// linden libraries
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llstring.h"
#include "lltransactiontypes.h"
#include "lluuid.h"
#include "llvorbisencode.h"
#include "message.h"

// system libraries
#include <boost/tokenizer.hpp>

class LLFileEnableUpload : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
        return true;
	}
};

class LLFileEnableUploadModel : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLFloaterModelPreview* fmp = (LLFloaterModelPreview*) LLFloaterReg::findInstance("upload_model");
		if (fmp && fmp->isModelLoading())
		{
			return false;
		}

		return true;
	}
};

class LLFileEnableUploadMaterial : public view_listener_t
{
    bool handleEvent(const LLSD& userdata)
    {
        if (gAgent.getRegionCapability("UpdateMaterialAgentInventory").empty())
        {
            return false;
        }

        return true;
    }
};

class LLMeshEnabled : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return gSavedSettings.getBOOL("MeshEnabled");
	}
};

class LLMeshUploadVisible : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		return gMeshRepo.meshUploadEnabled();
	}
};

LLMutex* LLFilePickerThread::sMutex = NULL;
std::queue<LLFilePickerThread*> LLFilePickerThread::sDeadQ;

void LLFilePickerThread::getFile()
{
#if LL_WINDOWS
    // Todo: get rid of LLFilePickerThread and make this modeless
	start();
#elif LL_DARWIN
    runModeless();
#else
	run();
#endif
}

//virtual 
void LLFilePickerThread::run()
{
#if LL_WINDOWS
	bool blocking = false;
#else
	bool blocking = true; // modal
#endif

	LLFilePicker picker;

	if (mIsSaveDialog)
	{
		if (picker.getSaveFile(mSaveFilter, mProposedName, blocking))
		{
			mResponses.push_back(picker.getFirstFile());
		}
	}
	else
	{
		bool result = mIsGetMultiple ? picker.getMultipleOpenFiles(mLoadFilter, blocking) : picker.getOpenFile(mLoadFilter, blocking);
		if (result)
		{
			std::string filename = picker.getFirstFile(); // consider copying mFiles directly
			do
			{
				mResponses.push_back(filename);
				filename = picker.getNextFile();
			}
			while (mIsGetMultiple && !filename.empty());
		}
	}

	{
		LLMutexLock lock(sMutex);
		sDeadQ.push(this);
	}
}

void LLFilePickerThread::runModeless()
{
    bool result = false;
    LLFilePicker picker;

    if (mIsSaveDialog)
    {
        result = picker.getSaveFileModeless(mSaveFilter,
                                            mProposedName,
                                            modelessStringCallback,
                                            this);
    }
    else if (mIsGetMultiple)
    {
        result = picker.getMultipleOpenFilesModeless(mLoadFilter, modelessVectorCallback, this);
    }
    else
    {
        result = picker.getOpenFileModeless(mLoadFilter, modelessVectorCallback, this);
    }
    
    if (!result)
    {
        LLMutexLock lock(sMutex);
        sDeadQ.push(this);
    }
}

void LLFilePickerThread::modelessStringCallback(bool success,
                                          std::string &response,
                                          void *user_data)
{
    LLFilePickerThread *picker = (LLFilePickerThread*)user_data;
    if (success)
    {
        picker->mResponses.push_back(response);
    }
    
    {
        LLMutexLock lock(sMutex);
        sDeadQ.push(picker);
    }
}

void LLFilePickerThread::modelessVectorCallback(bool success,
                                          std::vector<std::string> &responses,
                                          void *user_data)
{
    LLFilePickerThread *picker = (LLFilePickerThread*)user_data;
    if (success)
    {
        if (picker->mIsGetMultiple)
        {
            picker->mResponses = responses;
        }
        else
        {
            std::vector<std::string>::iterator iter = responses.begin();
            while (iter != responses.end())
            {
                if (!iter->empty())
                {
                    picker->mResponses.push_back(*iter);
                    break;
                }
                iter++;
            }
        }
    }
    
    {
        LLMutexLock lock(sMutex);
        sDeadQ.push(picker);
    }
}

//static
void LLFilePickerThread::initClass()
{
	sMutex = new LLMutex();
}

//static
void LLFilePickerThread::cleanupClass()
{
	clearDead();
	
	delete sMutex;
	sMutex = NULL;
}

//static
void LLFilePickerThread::clearDead()
{
	if (!sDeadQ.empty())
	{
		LLMutexLock lock(sMutex);
		while (!sDeadQ.empty())
		{
			LLFilePickerThread* thread = sDeadQ.front();
			thread->notify(thread->mResponses);
			delete thread;
			sDeadQ.pop();
		}
	}
}

LLFilePickerReplyThread::LLFilePickerReplyThread(const file_picked_signal_t::slot_type& cb, LLFilePicker::ELoadFilter filter, bool get_multiple, const file_picked_signal_t::slot_type& failure_cb)
	: LLFilePickerThread(filter, get_multiple),
	mLoadFilter(filter),
	mSaveFilter(LLFilePicker::FFSAVE_ALL),
	mFilePickedSignal(NULL),
	mFailureSignal(NULL)
{
	mFilePickedSignal = new file_picked_signal_t();
	mFilePickedSignal->connect(cb);

	mFailureSignal = new file_picked_signal_t();
	mFailureSignal->connect(failure_cb);
}

LLFilePickerReplyThread::LLFilePickerReplyThread(const file_picked_signal_t::slot_type& cb, LLFilePicker::ESaveFilter filter, const std::string &proposed_name, const file_picked_signal_t::slot_type& failure_cb)
	: LLFilePickerThread(filter, proposed_name),
	mLoadFilter(LLFilePicker::FFLOAD_ALL),
	mSaveFilter(filter),
	mFilePickedSignal(NULL),
	mFailureSignal(NULL)
{
	mFilePickedSignal = new file_picked_signal_t();
	mFilePickedSignal->connect(cb);

	mFailureSignal = new file_picked_signal_t();
	mFailureSignal->connect(failure_cb);
}

LLFilePickerReplyThread::~LLFilePickerReplyThread()
{
	delete mFilePickedSignal;
	delete mFailureSignal;
}

void LLFilePickerReplyThread::startPicker(const file_picked_signal_t::slot_type & cb, LLFilePicker::ELoadFilter filter, bool get_multiple, const file_picked_signal_t::slot_type & failure_cb)
{
    (new LLFilePickerReplyThread(cb, filter, get_multiple, failure_cb))->getFile();
}

void LLFilePickerReplyThread::startPicker(const file_picked_signal_t::slot_type & cb, LLFilePicker::ESaveFilter filter, const std::string & proposed_name, const file_picked_signal_t::slot_type & failure_cb)
{
    (new LLFilePickerReplyThread(cb, filter, proposed_name, failure_cb))->getFile();
}

void LLFilePickerReplyThread::notify(const std::vector<std::string>& filenames)
{
	if (filenames.empty())
	{
		if (mFailureSignal)
		{
			(*mFailureSignal)(filenames, mLoadFilter, mSaveFilter);
		}
	}
	else
	{
		if (mFilePickedSignal)
		{
			(*mFilePickedSignal)(filenames, mLoadFilter, mSaveFilter);
		}
	}
}


LLMediaFilePicker::LLMediaFilePicker(LLPluginClassMedia* plugin, LLFilePicker::ELoadFilter filter, bool get_multiple)
    : LLFilePickerThread(filter, get_multiple),
    mPlugin(plugin->getSharedPtr())
{
}

LLMediaFilePicker::LLMediaFilePicker(LLPluginClassMedia* plugin, LLFilePicker::ESaveFilter filter, const std::string &proposed_name)
    : LLFilePickerThread(filter, proposed_name),
    mPlugin(plugin->getSharedPtr())
{
}

void LLMediaFilePicker::notify(const std::vector<std::string>& filenames)
{
    mPlugin->sendPickFileResponse(mResponses);
    mPlugin = NULL;
}

//============================================================================

#if LL_WINDOWS
static std::string SOUND_EXTENSIONS = "wav";
static std::string IMAGE_EXTENSIONS = "tga bmp jpg jpeg png";
static std::string ANIM_EXTENSIONS =  "bvh anim";
static std::string XML_EXTENSIONS = "xml";
static std::string SLOBJECT_EXTENSIONS = "slobject";
#endif
static std::string ALL_FILE_EXTENSIONS = "*.*";
static std::string MODEL_EXTENSIONS = "dae";
static std::string MATERIAL_EXTENSIONS = "gltf glb";

std::string build_extensions_string(LLFilePicker::ELoadFilter filter)
{
	switch(filter)
	{
#if LL_WINDOWS
	case LLFilePicker::FFLOAD_IMAGE:
		return IMAGE_EXTENSIONS;
	case LLFilePicker::FFLOAD_WAV:
		return SOUND_EXTENSIONS;
	case LLFilePicker::FFLOAD_ANIM:
		return ANIM_EXTENSIONS;
	case LLFilePicker::FFLOAD_SLOBJECT:
		return SLOBJECT_EXTENSIONS;
	case LLFilePicker::FFLOAD_MODEL:
		return MODEL_EXTENSIONS;
    case LLFilePicker::FFLOAD_MATERIAL:
        return MATERIAL_EXTENSIONS;
	case LLFilePicker::FFLOAD_XML:
	    return XML_EXTENSIONS;
    case LLFilePicker::FFLOAD_ALL:
    case LLFilePicker::FFLOAD_EXE:
		return ALL_FILE_EXTENSIONS;
#endif
    default:
	return ALL_FILE_EXTENSIONS;
	}
}


const bool check_file_extension(const std::string& filename, LLFilePicker::ELoadFilter type)
{
	std::string ext = gDirUtilp->getExtension(filename);

	//strincmp doesn't like NULL pointers
	if (ext.empty())
	{
		std::string short_name = gDirUtilp->getBaseFileName(filename);

		// No extension
		LLSD args;
		args["FILE"] = short_name;
		LLNotificationsUtil::add("NoFileExtension", args);
		return false;
	}
	else
	{
		//so there is an extension
		//loop over the valid extensions and compare to see
		//if the extension is valid

		//now grab the set of valid file extensions
		std::string valid_extensions = build_extensions_string(type);

		bool ext_valid = false;

		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep(" ");
		tokenizer tokens(valid_extensions, sep);
		tokenizer::iterator token_iter;

		//now loop over all valid file extensions
		//and compare them to the extension of the file
		//to be uploaded
		for (token_iter = tokens.begin();
			token_iter != tokens.end() && ext_valid != true;
			++token_iter)
		{
			const std::string& cur_token = *token_iter;

			if (cur_token == ext || cur_token == "*.*")
			{
				//valid extension
				//or the acceptable extension is any
				ext_valid = true;
			}
		}//end for (loop over all tokens)

		if (ext_valid == false)
		{
			//should only get here if the extension exists
			//but is invalid
			LLSD args;
			args["EXTENSION"] = ext;
			args["VALIDS"] = valid_extensions;
			LLNotificationsUtil::add("InvalidFileExtension", args);
			return false;
		}
	}//end else (non-null extension)
	return true;
}

const void upload_single_file(const std::vector<std::string>& filenames, LLFilePicker::ELoadFilter type)
{
	std::string filename = filenames[0];
	if (!check_file_extension(filename, type)) return;
	
	if (!filename.empty())
	{
		if (type == LLFilePicker::FFLOAD_WAV)
		{
			// pre-qualify wavs to make sure the format is acceptable
			std::string error_msg;
			if (check_for_invalid_wav_formats(filename, error_msg))
			{
				LL_INFOS() << error_msg << ": " << filename << LL_ENDL;
				LLSD args;
				args["FILE"] = filename;
				LLNotificationsUtil::add(error_msg, args);
				return;
			}
			else
			{
				LLFloaterReg::showInstance("upload_sound", LLSD(filename));
			}
		}
		if (type == LLFilePicker::FFLOAD_IMAGE)
		{
			LLFloaterReg::showInstance("upload_image", LLSD(filename));
		}
		if (type == LLFilePicker::FFLOAD_ANIM)
		{
			std::string filename_lc(filename);
			LLStringUtil::toLower(filename_lc);
			if (filename_lc.rfind(".anim") != std::string::npos)
			{
				LLFloaterReg::showInstance("upload_anim_anim", LLSD(filename));
			}
			else
			{
				LLFloaterReg::showInstance("upload_anim_bvh", LLSD(filename));
			}
		}		
	}
	return;
}

void do_bulk_upload(std::vector<std::string> filenames, const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0)
	{
		// Cancel upload
		return;
	}

	for (std::vector<std::string>::const_iterator in_iter = filenames.begin(); in_iter != filenames.end(); ++in_iter)
	{
		std::string filename = (*in_iter);
		
		std::string name = gDirUtilp->getBaseFileName(filename, true);
		std::string asset_name = name;
		LLStringUtil::replaceNonstandardASCII(asset_name, '?');
		LLStringUtil::replaceChar(asset_name, '|', '?');
		LLStringUtil::stripNonprintable(asset_name);
		LLStringUtil::trim(asset_name);

		std::string ext = gDirUtilp->getExtension(filename);
		LLAssetType::EType asset_type;
		U32 codec;
		S32 expected_upload_cost;
		if (LLResourceUploadInfo::findAssetTypeAndCodecOfExtension(ext, asset_type, codec) &&
			LLAgentBenefitsMgr::current().findUploadCost(asset_type, expected_upload_cost))
		{
            LLResourceUploadInfo::ptr_t uploadInfo(new LLNewFileResourceUploadInfo(
                filename,
                asset_name,
                asset_name, 0,
                LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
                LLFloaterPerms::getNextOwnerPerms("Uploads"),
                LLFloaterPerms::getGroupPerms("Uploads"),
                LLFloaterPerms::getEveryonePerms("Uploads"),
                expected_upload_cost));

            upload_new_resource(uploadInfo);
        }

        // gltf does not use normal upload procedure
        if (ext == "gltf" || ext == "glb")
        {
            tinygltf::Model model;
            if (LLTinyGLTFHelper::loadModel(filename, model))
            {
                S32 materials_in_file = model.materials.size();

                for (S32 i = 0; i < materials_in_file; i++)
                {
                    // Todo:
                    // 1. Decouple bulk upload from material editor
                    // 2. Take into account possiblity of identical textures
                    LLMaterialEditor::uploadMaterialFromModel(filename, model, i);
                }
            }
        }
    }
}

bool get_bulk_upload_expected_cost(const std::vector<std::string>& filenames, S32& total_cost, S32& file_count, S32& bvh_count)
{
	total_cost = 0;
	file_count = 0;
	bvh_count = 0;
	for (std::vector<std::string>::const_iterator in_iter = filenames.begin(); in_iter != filenames.end(); ++in_iter)
	{
		std::string filename = (*in_iter);
		std::string ext = gDirUtilp->getExtension(filename);

		if (ext == "bvh")
		{
			bvh_count++;
		}

		LLAssetType::EType asset_type;
		U32 codec;
		S32 cost;

		if (LLResourceUploadInfo::findAssetTypeAndCodecOfExtension(ext, asset_type, codec) &&
			LLAgentBenefitsMgr::current().findUploadCost(asset_type, cost))
		{
			total_cost += cost;
			file_count++;
		}

        if (ext == "gltf" || ext == "glb")
        {
            S32 texture_upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();
            
            tinygltf::Model model;

            if (LLTinyGLTFHelper::loadModel(filename, model))
            {
                S32 materials_in_file = model.materials.size();

                for (S32 i = 0; i < materials_in_file; i++)
                {
                    LLPointer<LLFetchedGLTFMaterial> material = new LLFetchedGLTFMaterial();
                    std::string material_name;
                    bool decode_successful = LLTinyGLTFHelper::getMaterialFromModel(filename, model, i, material.get(), material_name);

                    if (decode_successful)
                    {
                        // Todo: make it account for possibility of same texture in different
                        // materials and even in scope of same material
                        S32 texture_count = 0;
                        if (material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR].notNull())
                        {
                            texture_count++;
                        }
                        if (material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS].notNull())
                        {
                            texture_count++;
                        }
                        if (material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL].notNull())
                        {
                            texture_count++;
                        }
                        if (material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE].notNull())
                        {
                            texture_count++;
                        }
                        total_cost += texture_count * texture_upload_cost;
                        file_count++;
                    }
                }
            }
        }
	}
	
    return file_count > 0;
}

const void upload_bulk(const std::vector<std::string>& filenames, LLFilePicker::ELoadFilter type)
{
	// TODO:
	// Check user balance for entire cost
	// Charge user entire cost
	// Loop, uploading
	// If an upload fails, refund the user for that one
	//
	// Also fix single upload to charge first, then refund

	// FIXME PREMIUM what about known types that can't be bulk uploaded
	// (bvh)? These will fail in the item by item upload but won't be
	// mentioned in the notification.
	std::vector<std::string> filtered_filenames;
	for (std::vector<std::string>::const_iterator in_iter = filenames.begin(); in_iter != filenames.end(); ++in_iter)
	{
		const std::string& filename = *in_iter;
		if (check_file_extension(filename, type))
		{
			filtered_filenames.push_back(filename);
		}
	}

	S32 expected_upload_cost;
	S32 expected_upload_count;
	S32 bvh_count;
	if (get_bulk_upload_expected_cost(filtered_filenames, expected_upload_cost, expected_upload_count, bvh_count))
	{
		LLSD args;
		args["COST"] = expected_upload_cost;
		args["COUNT"] = expected_upload_count;
		LLNotificationsUtil::add("BulkUploadCostConfirmation",  args, LLSD(), boost::bind(do_bulk_upload, filtered_filenames, _1, _2));

		if (filtered_filenames.size() > expected_upload_count)
		{
			if (bvh_count == filtered_filenames.size() - expected_upload_count)
			{
				LLNotificationsUtil::add("DoNotSupportBulkAnimationUpload");
			}
			else
			{
				LLNotificationsUtil::add("BulkUploadIncompatibleFiles");
			}
		}
	}
	else if (bvh_count == filtered_filenames.size())
	{
		LLNotificationsUtil::add("DoNotSupportBulkAnimationUpload");
	}
	else
	{
		LLNotificationsUtil::add("BulkUploadNoCompatibleFiles");
	}

}

class LLFileUploadImage : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (gAgentCamera.cameraMouselook())
		{
			gAgentCamera.changeCameraToDefault();
		}
		LLFilePickerReplyThread::startPicker(boost::bind(&upload_single_file, _1, _2), LLFilePicker::FFLOAD_IMAGE, false);
		return true;
	}
};

class LLFileUploadModel : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
        LLFloaterModelPreview::showModelPreview();
        return true;
	}
};

class LLFileUploadMaterial : public view_listener_t
{
    bool handleEvent(const LLSD& userdata)
    {
        LLMaterialEditor::importMaterial();
        return true;
    }
};

class LLFileUploadSound : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (gAgentCamera.cameraMouselook())
		{
			gAgentCamera.changeCameraToDefault();
		}
		LLFilePickerReplyThread::startPicker(boost::bind(&upload_single_file, _1, _2), LLFilePicker::FFLOAD_WAV, false);
		return true;
	}
};

class LLFileUploadAnim : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (gAgentCamera.cameraMouselook())
		{
			gAgentCamera.changeCameraToDefault();
		}
		LLFilePickerReplyThread::startPicker(boost::bind(&upload_single_file, _1, _2), LLFilePicker::FFLOAD_ANIM, false);
		return true;
	}
};

class LLFileUploadBulk : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if (gAgentCamera.cameraMouselook())
		{
			gAgentCamera.changeCameraToDefault();
		}
		LLFilePickerReplyThread::startPicker(boost::bind(&upload_bulk, _1, _2), LLFilePicker::FFLOAD_ALL, true);
		return true;
	}
};

void upload_error(const std::string& error_message, const std::string& label, const std::string& filename, const LLSD& args) 
{
	LL_WARNS() << error_message << LL_ENDL;
	LLNotificationsUtil::add(label, args);
	if(LLFile::remove(filename) == -1)
	{
		LL_DEBUGS() << "unable to remove temp file" << LL_ENDL;
	}
	LLFilePicker::instance().reset();						
}

class LLFileEnableCloseWindow : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool frontmost_fl_exists = (NULL != gFloaterView->getFrontmostClosableFloater());
		bool frontmost_snapshot_fl_exists = (NULL != gSnapshotFloaterView->getFrontmostClosableFloater());

		return !LLNotificationsUI::LLToast::isAlertToastShown() && (frontmost_fl_exists || frontmost_snapshot_fl_exists);
	}
};

class LLFileCloseWindow : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool frontmost_fl_exists = (NULL != gFloaterView->getFrontmostClosableFloater());
		LLFloater* snapshot_floater = gSnapshotFloaterView->getFrontmostClosableFloater();

		if(snapshot_floater && (!frontmost_fl_exists || snapshot_floater->hasFocus()))
		{
			snapshot_floater->closeFloater();
			if (gFocusMgr.getKeyboardFocus() == NULL)
			{
				gFloaterView->focusFrontFloater();
			}
		}
		else
		{
			LLFloater::closeFrontmostFloater();
		}
		if (gMenuHolder) gMenuHolder->hideMenus();
		return true;
	}
};

class LLFileEnableCloseAllWindows : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLFloaterSnapshot* floater_snapshot = LLFloaterSnapshot::findInstance();
		bool is_floaters_snapshot_opened = (floater_snapshot && floater_snapshot->isInVisibleChain());
		bool open_children = gFloaterView->allChildrenClosed() && !is_floaters_snapshot_opened;
		return !open_children && !LLNotificationsUI::LLToast::isAlertToastShown();
	}
};

class LLFileCloseAllWindows : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool app_quitting = false;
		gFloaterView->closeAllChildren(app_quitting);
		LLFloaterSnapshot* floater_snapshot = LLFloaterSnapshot::findInstance();
		if (floater_snapshot)
			floater_snapshot->closeFloater(app_quitting);
		if (gMenuHolder) gMenuHolder->hideMenus();
		return true;
	}
};

class LLFileTakeSnapshotToDisk : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLPointer<LLImageRaw> raw = new LLImageRaw;

		S32 width = gViewerWindow->getWindowWidthRaw();
		S32 height = gViewerWindow->getWindowHeightRaw();

		bool render_ui = gSavedSettings.getBOOL("RenderUIInSnapshot");
		bool render_hud = gSavedSettings.getBOOL("RenderHUDInSnapshot");
		bool render_no_post = gSavedSettings.getBOOL("RenderSnapshotNoPost");

		bool high_res = gSavedSettings.getBOOL("HighResSnapshot");
		if (high_res)
		{
			width *= 2;
			height *= 2;
			// not compatible with UI/HUD
			render_ui = false;
			render_hud = false;
		}

		if (gViewerWindow->rawSnapshot(raw,
									   width,
									   height,
									   true,
									   false,
									   render_ui,
									   render_hud,
									   false,
									   render_no_post,
									   LLSnapshotModel::SNAPSHOT_TYPE_COLOR,
									   high_res ? S32_MAX : MAX_SNAPSHOT_IMAGE_SIZE)) //per side
		{
			LLPointer<LLImageFormatted> formatted;
            LLSnapshotModel::ESnapshotFormat fmt = (LLSnapshotModel::ESnapshotFormat) gSavedSettings.getS32("SnapshotFormat");
			switch (fmt)
			{
			case LLSnapshotModel::SNAPSHOT_FORMAT_JPEG:
				formatted = new LLImageJPEG(gSavedSettings.getS32("SnapshotQuality"));
				break;
			default:
				LL_WARNS() << "Unknown local snapshot format: " << fmt << LL_ENDL;
			case LLSnapshotModel::SNAPSHOT_FORMAT_PNG:
				formatted = new LLImagePNG;
				break;
			case LLSnapshotModel::SNAPSHOT_FORMAT_BMP:
				formatted = new LLImageBMP;
				break;
			}
			formatted->enableOverSize() ;
			formatted->encode(raw, 0);
			formatted->disableOverSize() ;
			LLSnapshotLivePreview::saveLocal(formatted);
		}
		return true;
	}
};

class LLFileQuit : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLAppViewer::instance()->userQuit();
		return true;
	}
};


void handle_compress_image(void*)
{
	LLFilePicker& picker = LLFilePicker::instance();
	if (picker.getMultipleOpenFiles(LLFilePicker::FFLOAD_IMAGE))
	{
		std::string infile = picker.getFirstFile();
		while (!infile.empty())
		{
			std::string outfile = infile + ".j2c";

			LL_INFOS() << "Input:  " << infile << LL_ENDL;
			LL_INFOS() << "Output: " << outfile << LL_ENDL;

			bool success;

			success = LLViewerTextureList::createUploadFile(infile, outfile, IMG_CODEC_TGA);

			if (success)
			{
				LL_INFOS() << "Compression complete" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "Compression failed: " << LLImage::getLastError() << LL_ENDL;
			}

			infile = picker.getNextFile();
		}
	}
}

// No convinient check in LLFile, and correct way would be something
// like GetFileSizeEx, which is too OS specific for current purpose
// so doing dirty, but OS independent fopen and fseek
size_t get_file_size(std::string &filename)
{
    LLFILE* file = LLFile::fopen(filename, "rb");		/*Flawfinder: ignore*/
    if (!file)
    {
        LL_WARNS() << "Error opening " << filename << LL_ENDL;
        return 0;
    }

    // read in the whole file
    fseek(file, 0L, SEEK_END);
    size_t file_length = (size_t)ftell(file);
    fclose(file);
    return file_length;
}

void handle_compress_file_test(void*)
{
    LLFilePicker& picker = LLFilePicker::instance();
    if (picker.getOpenFile())
    {
        std::string infile = picker.getFirstFile();
        if (!infile.empty())
        {
            std::string packfile = infile + ".pack_test";
            std::string unpackfile = infile + ".unpack_test";

            S64Bytes initial_size = S64Bytes(get_file_size(infile));

            bool success;

            F64 total_seconds = LLTimer::getTotalSeconds();
            success = gzip_file(infile, packfile);
            F64 result_pack_seconds = LLTimer::getTotalSeconds() - total_seconds;

            if (success)
            {
                S64Bytes packed_size = S64Bytes(get_file_size(packfile));

                LL_INFOS() << "Packing complete, time: " << result_pack_seconds << " size: " << packed_size << LL_ENDL;
                total_seconds = LLTimer::getTotalSeconds();
                success = gunzip_file(packfile, unpackfile);
                F64 result_unpack_seconds = LLTimer::getTotalSeconds() - total_seconds;

                if (success)
                {
                    S64Bytes unpacked_size = S64Bytes(get_file_size(unpackfile));

                    LL_INFOS() << "Unpacking complete, time: " << result_unpack_seconds << " size: " << unpacked_size << LL_ENDL;

                    LLSD args;
                    args["FILE"] = infile;
                    args["PACK_TIME"] = result_pack_seconds;
                    args["UNPACK_TIME"] = result_unpack_seconds;
                    args["SIZE"] = LLSD::Integer(initial_size.valueInUnits<LLUnits::Kilobytes>());
                    args["PSIZE"] = LLSD::Integer(packed_size.valueInUnits<LLUnits::Kilobytes>());
                    args["USIZE"] = LLSD::Integer(unpacked_size.valueInUnits<LLUnits::Kilobytes>());
                    LLNotificationsUtil::add("CompressionTestResults", args);

                    LLFile::remove(packfile);
                    LLFile::remove(unpackfile);
                }
                else
                {
                    LL_INFOS() << "Failed to uncompress file: " << packfile << LL_ENDL;
                    LLFile::remove(packfile);
                }

            }
            else
            {
                LL_INFOS() << "Failed to compres file: " << infile << LL_ENDL;
            }
        }
        else
        {
            LL_INFOS() << "Failed to open file" << LL_ENDL;
        }
    }
    else
    {
        LL_INFOS() << "Failed to open file" << LL_ENDL;
    }
}


LLUUID upload_new_resource(
	const std::string& src_filename,
	std::string name,
	std::string desc,
	S32 compression_info,
	LLFolderType::EType destination_folder_type,
	LLInventoryType::EType inv_type,
	U32 next_owner_perms,
	U32 group_perms,
	U32 everyone_perms,
	const std::string& display_name,
	LLAssetStorage::LLStoreAssetCallback callback,
	S32 expected_upload_cost,
	void *userdata,
	bool show_inventory)
{	

    LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLNewFileResourceUploadInfo>(
        src_filename,
        name, desc, compression_info,
        destination_folder_type, inv_type,
        next_owner_perms, group_perms, everyone_perms,
        expected_upload_cost, show_inventory));
    upload_new_resource(uploadInfo, callback, userdata);

    return LLUUID::null;
}

void upload_done_callback(
	const LLUUID& uuid,
	void* user_data,
	S32 result,
	LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLResourceData* data = (LLResourceData*)user_data;
	S32 expected_upload_cost = data ? data->mExpectedUploadCost : 0;
	//LLAssetType::EType pref_loc = data->mPreferredLocation;
	bool is_balance_sufficient = true;

	if(data)
	{
		if (result >= 0)
		{
			LLFolderType::EType dest_loc = (data->mPreferredLocation == LLFolderType::FT_NONE) ? LLFolderType::assetTypeToFolderType(data->mAssetInfo.mType) : data->mPreferredLocation;
			
			if (LLAssetType::AT_SOUND == data->mAssetInfo.mType ||
			    LLAssetType::AT_TEXTURE == data->mAssetInfo.mType ||
			    LLAssetType::AT_ANIMATION == data->mAssetInfo.mType)
			{
				// Charge the user for the upload.
				LLViewerRegion* region = gAgent.getRegion();
				
				if(!(can_afford_transaction(expected_upload_cost)))
				{
					LLBuyCurrencyHTML::openCurrencyFloater( "", expected_upload_cost );
					is_balance_sufficient = false;
				}
				else if(region)
				{
					// Charge user for upload
					gStatusBar->debitBalance(expected_upload_cost);
					
					LLMessageSystem* msg = gMessageSystem;
					msg->newMessageFast(_PREHASH_MoneyTransferRequest);
					msg->nextBlockFast(_PREHASH_AgentData);
					msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
					msg->nextBlockFast(_PREHASH_MoneyData);
					msg->addUUIDFast(_PREHASH_SourceID, gAgent.getID());
					msg->addUUIDFast(_PREHASH_DestID, LLUUID::null);
					msg->addU8("Flags", 0);
					// we tell the sim how much we were expecting to pay so it
					// can respond to any discrepancy
					msg->addS32Fast(_PREHASH_Amount, expected_upload_cost);
					msg->addU8Fast(_PREHASH_AggregatePermNextOwner, (U8)LLAggregatePermissions::AP_EMPTY);
					msg->addU8Fast(_PREHASH_AggregatePermInventory, (U8)LLAggregatePermissions::AP_EMPTY);
					msg->addS32Fast(_PREHASH_TransactionType, TRANS_UPLOAD_CHARGE);
					msg->addStringFast(_PREHASH_Description, NULL);
					msg->sendReliable(region->getHost());
				}
			}

			if(is_balance_sufficient)
			{
				// Actually add the upload to inventory
				LL_INFOS() << "Adding " << uuid << " to inventory." << LL_ENDL;
				const LLUUID folder_id = gInventory.findCategoryUUIDForType(dest_loc);
				if(folder_id.notNull())
				{
					U32 next_owner_perms = data->mNextOwnerPerm;
					if(PERM_NONE == next_owner_perms)
					{
						next_owner_perms = PERM_MOVE | PERM_TRANSFER;
					}
					create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
							      folder_id, data->mAssetInfo.mTransactionID, data->mAssetInfo.getName(),
							      data->mAssetInfo.getDescription(), data->mAssetInfo.mType,
                                  data->mInventoryType, NO_INV_SUBTYPE, next_owner_perms,
							      LLPointer<LLInventoryCallback>(NULL));
				}
				else
				{
					LL_WARNS() << "Can't find a folder to put it in" << LL_ENDL;
				}
			}
		}
		else // 	if(result >= 0)
		{
			LLSD args;
			args["FILE"] = LLInventoryType::lookupHumanReadable(data->mInventoryType);
			args["REASON"] = std::string(LLAssetStorage::getErrorString(result));
			LLNotificationsUtil::add("CannotUploadReason", args);
		}

		delete data;
		data = NULL;
	}

	LLUploadDialog::modalUploadFinished();

	// *NOTE: This is a pretty big hack. What this does is check the
	// file picker if there are any more pending uploads. If so,
	// upload that file.
	const std::string& next_file = LLFilePicker::instance().getNextFile();
	if(is_balance_sufficient && !next_file.empty())
	{
		std::string asset_name = gDirUtilp->getBaseFileName(next_file, true);
		LLStringUtil::replaceNonstandardASCII( asset_name, '?' );
		LLStringUtil::replaceChar(asset_name, '|', '?');
		LLStringUtil::stripNonprintable(asset_name);
		LLStringUtil::trim(asset_name);

		std::string display_name = LLStringUtil::null;
		LLAssetStorage::LLStoreAssetCallback callback;
		void *userdata = NULL;
		upload_new_resource(
			next_file,
			asset_name,
			asset_name,	// file
			0,
			LLFolderType::FT_NONE,
			LLInventoryType::IT_NONE,
			LLFloaterPerms::getNextOwnerPerms("Uploads"),
			LLFloaterPerms::getGroupPerms("Uploads"),
			LLFloaterPerms::getEveryonePerms("Uploads"),
			display_name,
			callback,
			expected_upload_cost, // assuming next in a group of uploads is of roughly the same type, i.e. same upload cost
			userdata);
	}
}

void upload_new_resource(
    LLResourceUploadInfo::ptr_t &uploadInfo,
    LLAssetStorage::LLStoreAssetCallback callback,
    void *userdata)
{
	if(gDisconnected)
	{
		return ;
	}

//     uploadInfo->setAssetType(assetType);
//     uploadInfo->setTransactionId(tid);


	std::string url = gAgent.getRegionCapability("NewFileAgentInventory");

	if ( !url.empty() )
	{
        LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
	}
	else
	{
        uploadInfo->prepareUpload();
        uploadInfo->logPreparedUpload();

		LL_INFOS() << "NewAgentInventory capability not found, new agent inventory via asset system." << LL_ENDL;
		// check for adequate funds
		// TODO: do this check on the sim
		if (LLAssetType::AT_SOUND == uploadInfo->getAssetType() ||
            LLAssetType::AT_TEXTURE == uploadInfo->getAssetType() ||
            LLAssetType::AT_ANIMATION == uploadInfo->getAssetType())
		{
			S32 balance = gStatusBar->getBalance();
			if (balance < uploadInfo->getExpectedUploadCost())
			{
				// insufficient funds, bail on this upload
                LLBuyCurrencyHTML::openCurrencyFloater("", uploadInfo->getExpectedUploadCost());
				return;
			}
		}

		LLResourceData* data = new LLResourceData;
		data->mAssetInfo.mTransactionID = uploadInfo->getTransactionId();
		data->mAssetInfo.mUuid = uploadInfo->getAssetId();
        data->mAssetInfo.mType = uploadInfo->getAssetType();
		data->mAssetInfo.mCreatorID = gAgentID;
		data->mInventoryType = uploadInfo->getInventoryType();
		data->mNextOwnerPerm = uploadInfo->getNextOwnerPerms();
		data->mExpectedUploadCost = uploadInfo->getExpectedUploadCost();
		data->mUserData = userdata;
		data->mAssetInfo.setName(uploadInfo->getName());
		data->mAssetInfo.setDescription(uploadInfo->getDescription());
		data->mPreferredLocation = uploadInfo->getDestinationFolderType();

		LLAssetStorage::LLStoreAssetCallback asset_callback = &upload_done_callback;
		if (callback)
		{
			asset_callback = callback;
		}
		gAssetStorage->storeAssetData(
			data->mAssetInfo.mTransactionID,
			data->mAssetInfo.mType,
			asset_callback,
			(void*)data,
			false);
	}
}


void init_menu_file()
{
	view_listener_t::addCommit(new LLFileUploadImage(), "File.UploadImage");
	view_listener_t::addCommit(new LLFileUploadSound(), "File.UploadSound");
	view_listener_t::addCommit(new LLFileUploadAnim(), "File.UploadAnim");
	view_listener_t::addCommit(new LLFileUploadModel(), "File.UploadModel");
    view_listener_t::addCommit(new LLFileUploadMaterial(), "File.UploadMaterial");
	view_listener_t::addCommit(new LLFileUploadBulk(), "File.UploadBulk");
	view_listener_t::addCommit(new LLFileCloseWindow(), "File.CloseWindow");
	view_listener_t::addCommit(new LLFileCloseAllWindows(), "File.CloseAllWindows");
	view_listener_t::addEnable(new LLFileEnableCloseWindow(), "File.EnableCloseWindow");
	view_listener_t::addEnable(new LLFileEnableCloseAllWindows(), "File.EnableCloseAllWindows");
	view_listener_t::addCommit(new LLFileTakeSnapshotToDisk(), "File.TakeSnapshotToDisk");
	view_listener_t::addCommit(new LLFileQuit(), "File.Quit");

	view_listener_t::addEnable(new LLFileEnableUpload(), "File.EnableUpload");
	view_listener_t::addEnable(new LLFileEnableUploadModel(), "File.EnableUploadModel");
    view_listener_t::addEnable(new LLFileEnableUploadMaterial(), "File.EnableUploadMaterial");
	view_listener_t::addMenu(new LLMeshEnabled(), "File.MeshEnabled");
	view_listener_t::addMenu(new LLMeshUploadVisible(), "File.VisibleUploadModel");

	// "File.SaveTexture" moved to llpanelmaininventory so that it can be properly handled.
}
