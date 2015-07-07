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
#include "llagentcamera.h"
#include "llfilepicker.h"
#include "llfloaterreg.h"
#include "llbuycurrencyhtml.h"
#include "llfloatermap.h"
#include "llfloatermodelpreview.h"
#include "llfloatersnapshot.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagepng.h"
#include "llimagej2c.h"
#include "llimagejpeg.h"
#include "llimagetga.h"
#include "llinventorymodel.h"	// gInventory
#include "llresourcedata.h"
#include "llfloaterperms.h"
#include "llstatusbar.h"
#include "llviewercontrol.h"	// gSavedSettings
#include "llviewertexturelist.h"
#include "lluictrlfactory.h"
#include "llvfile.h"
#include "llvfs.h"
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
#include "llcoproceduremanager.h"
#include "llviewerassetupload.h"

// linden libraries
#include "llassetuploadresponders.h"
#include "lleconomy.h"
#include "llhttpclient.h"
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
// 		bool new_value = gStatusBar && LLGlobalEconomy::Singleton::getInstance() && (gStatusBar->getBalance() >= LLGlobalEconomy::Singleton::getInstance()->getPriceUpload());
// 		return new_value;
	}
};

class LLFileEnableUploadModel : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
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
	start();
#else
	run();
#endif
}

//virtual 
void LLFilePickerThread::run()
{
	LLFilePicker picker;
#if LL_WINDOWS
	if (picker.getOpenFile(mFilter, false))
	{
		mFile = picker.getFirstFile();
	}
#else
	if (picker.getOpenFile(mFilter, true))
	{
		mFile = picker.getFirstFile();
	}
#endif

	{
		LLMutexLock lock(sMutex);
		sDeadQ.push(this);
	}

}

//static
void LLFilePickerThread::initClass()
{
	sMutex = new LLMutex(NULL);
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
			thread->notify(thread->mFile);
			delete thread;
			sDeadQ.pop();
		}
	}
}


//============================================================================

#if LL_WINDOWS
static std::string SOUND_EXTENSIONS = "wav";
static std::string IMAGE_EXTENSIONS = "tga bmp jpg jpeg png";
static std::string ANIM_EXTENSIONS =  "bvh anim";
#ifdef _CORY_TESTING
static std::string GEOMETRY_EXTENSIONS = "slg";
#endif
static std::string XML_EXTENSIONS = "xml";
static std::string SLOBJECT_EXTENSIONS = "slobject";
#endif
static std::string ALL_FILE_EXTENSIONS = "*.*";
static std::string MODEL_EXTENSIONS = "dae";

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
#ifdef _CORY_TESTING
	case LLFilePicker::FFLOAD_GEOMETRY:
		return GEOMETRY_EXTENSIONS;
#endif
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

/**
   char* upload_pick(void* data)

   If applicable, brings up a file chooser in which the user selects a file
   to upload for a particular task.  If the file is valid for the given action,
   returns the string to the full path filename, else returns NULL.
   Data is the load filter for the type of file as defined in LLFilePicker.
**/
const std::string upload_pick(void* data)
{
 	if( gAgentCamera.cameraMouselook() )
	{
		gAgentCamera.changeCameraToDefault();
		// This doesn't seem necessary. JC
		// display();
	}

	LLFilePicker::ELoadFilter type;
	if(data)
	{
		type = (LLFilePicker::ELoadFilter)((intptr_t)data);
	}
	else
	{
		type = LLFilePicker::FFLOAD_ALL;
	}

	LLFilePicker& picker = LLFilePicker::instance();
	if (!picker.getOpenFile(type))
	{
		LL_INFOS() << "Couldn't import objects from file" << LL_ENDL;
		return std::string();
	}

	
	const std::string& filename = picker.getFirstFile();
	std::string ext = gDirUtilp->getExtension(filename);

	//strincmp doesn't like NULL pointers
	if (ext.empty())
	{
		std::string short_name = gDirUtilp->getBaseFileName(filename);
		
		// No extension
		LLSD args;
		args["FILE"] = short_name;
		LLNotificationsUtil::add("NoFileExtension", args);
		return std::string();
	}
	else
	{
		//so there is an extension
		//loop over the valid extensions and compare to see
		//if the extension is valid

		//now grab the set of valid file extensions
		std::string valid_extensions = build_extensions_string(type);

		BOOL ext_valid = FALSE;
		
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep(" ");
		tokenizer tokens(valid_extensions, sep);
		tokenizer::iterator token_iter;

		//now loop over all valid file extensions
		//and compare them to the extension of the file
		//to be uploaded
		for( token_iter = tokens.begin();
			 token_iter != tokens.end() && ext_valid != TRUE;
			 ++token_iter)
		{
			const std::string& cur_token = *token_iter;

			if (cur_token == ext || cur_token == "*.*")
			{
				//valid extension
				//or the acceptable extension is any
				ext_valid = TRUE;
			}
		}//end for (loop over all tokens)

		if (ext_valid == FALSE)
		{
			//should only get here if the extension exists
			//but is invalid
			LLSD args;
			args["EXTENSION"] = ext;
			args["VALIDS"] = valid_extensions;
			LLNotificationsUtil::add("InvalidFileExtension", args);
			return std::string();
		}
	}//end else (non-null extension)

	//valid file extension
	
	//now we check to see
	//if the file is actually a valid image/sound/etc.
	if (type == LLFilePicker::FFLOAD_WAV)
	{
		// pre-qualify wavs to make sure the format is acceptable
		std::string error_msg;
		if (check_for_invalid_wav_formats(filename,error_msg))
		{
			LL_INFOS() << error_msg << ": " << filename << LL_ENDL;
			LLSD args;
			args["FILE"] = filename;
			LLNotificationsUtil::add( error_msg, args );
			return std::string();
		}
	}//end if a wave/sound file

	
	return filename;
}

class LLFileUploadImage : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string filename = upload_pick((void *)LLFilePicker::FFLOAD_IMAGE);
		if (!filename.empty())
		{
			LLFloaterReg::showInstance("upload_image", LLSD(filename));
		}
		return TRUE;
	}
};

class LLFileUploadModel : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		LLFloaterModelPreview* fmp = (LLFloaterModelPreview*) LLFloaterReg::getInstance("upload_model");
		if (fmp)
		{
			fmp->loadModel(3);
		}
		
		return TRUE;
	}
};
	
class LLFileUploadSound : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		std::string filename = upload_pick((void*)LLFilePicker::FFLOAD_WAV);
		if (!filename.empty())
		{
			LLFloaterReg::showInstance("upload_sound", LLSD(filename));
		}
		return true;
	}
};

class LLFileUploadAnim : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		const std::string filename = upload_pick((void*)LLFilePicker::FFLOAD_ANIM);
		if (!filename.empty())
		{
			if (filename.rfind(".anim") != std::string::npos)
			{
				LLFloaterReg::showInstance("upload_anim_anim", LLSD(filename));
			}
			else
			{
				LLFloaterReg::showInstance("upload_anim_bvh", LLSD(filename));
			}
		}
		return true;
	}
};

class LLFileUploadBulk : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		if( gAgentCamera.cameraMouselook() )
		{
			gAgentCamera.changeCameraToDefault();
		}

		// TODO:
		// Check extensions for uploadability, cost
		// Check user balance for entire cost
		// Charge user entire cost
		// Loop, uploading
		// If an upload fails, refund the user for that one
		//
		// Also fix single upload to charge first, then refund

		LLFilePicker& picker = LLFilePicker::instance();
		if (picker.getMultipleOpenFiles())
		{
            std::string filename = picker.getFirstFile();
            S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();

            while (!filename.empty())
            {
                std::string name = gDirUtilp->getBaseFileName(filename, true);

                std::string asset_name = name;
                LLStringUtil::replaceNonstandardASCII( asset_name, '?' );
                LLStringUtil::replaceChar(asset_name, '|', '?');
                LLStringUtil::stripNonprintable(asset_name);
                LLStringUtil::trim(asset_name);

                NewResourceUploadInfo::ptr_t uploadInfo(new NewFileResourceUploadInfo(
                    filename,
                    asset_name,
                    asset_name, 0,
                    LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
                    LLFloaterPerms::getNextOwnerPerms("Uploads"),
                    LLFloaterPerms::getGroupPerms("Uploads"),
                    LLFloaterPerms::getEveryonePerms("Uploads"),
                    expected_upload_cost));

                upload_new_resource(uploadInfo, NULL, NULL);

                filename = picker.getNextFile();
            }
#if 0
			const std::string& filename = picker.getFirstFile();
			std::string name = gDirUtilp->getBaseFileName(filename, true);
			
			std::string asset_name = name;
			LLStringUtil::replaceNonstandardASCII( asset_name, '?' );
			LLStringUtil::replaceChar(asset_name, '|', '?');
			LLStringUtil::stripNonprintable(asset_name);
			LLStringUtil::trim(asset_name);
			
			std::string display_name = LLStringUtil::null;
			LLAssetStorage::LLStoreAssetCallback callback = NULL;
			S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
			void *userdata = NULL;

			upload_new_resource(
				filename,
				asset_name,
				asset_name,
				0,
				LLFolderType::FT_NONE,
				LLInventoryType::IT_NONE,
				LLFloaterPerms::getNextOwnerPerms("Uploads"),
				LLFloaterPerms::getGroupPerms("Uploads"),
				LLFloaterPerms::getEveryonePerms("Uploads"),
				display_name,
				callback,
				expected_upload_cost,
				userdata);

			// *NOTE: Ew, we don't iterate over the file list here,
			// we handle the next files in upload_done_callback()
#endif
		}
		else
		{
			LL_INFOS() << "Couldn't import objects from file" << LL_ENDL;
		}
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

		return frontmost_fl_exists || frontmost_snapshot_fl_exists;
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
		bool is_floater_snapshot_opened = floater_snapshot && floater_snapshot->isInVisibleChain();
		bool open_children = gFloaterView->allChildrenClosed() && !is_floater_snapshot_opened;
		return !open_children;
	}
};

class LLFileCloseAllWindows : public view_listener_t
{
	bool handleEvent(const LLSD& userdata)
	{
		bool app_quitting = false;
		gFloaterView->closeAllChildren(app_quitting);
		LLFloaterSnapshot::getInstance()->closeFloater(app_quitting);
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

		if (gSavedSettings.getBOOL("HighResSnapshot"))
		{
			width *= 2;
			height *= 2;
		}

		if (gViewerWindow->rawSnapshot(raw,
									   width,
									   height,
									   TRUE,
									   FALSE,
									   gSavedSettings.getBOOL("RenderUIInSnapshot"),
									   FALSE))
		{
			gViewerWindow->playSnapshotAnimAndSound();
			LLPointer<LLImageFormatted> formatted;
			LLFloaterSnapshot::ESnapshotFormat fmt = (LLFloaterSnapshot::ESnapshotFormat) gSavedSettings.getS32("SnapshotFormat");
			switch (fmt)
			{
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG:
				formatted = new LLImageJPEG(gSavedSettings.getS32("SnapshotQuality"));
				break;
			default:
				LL_WARNS() << "Unknown local snapshot format: " << fmt << LL_ENDL;
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG:
				formatted = new LLImagePNG;
				break;
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP:
				formatted = new LLImageBMP;
				break;
			}
			formatted->enableOverSize() ;
			formatted->encode(raw, 0);
			formatted->disableOverSize() ;
			gViewerWindow->saveImageNumbered(formatted);
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

			BOOL success;

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
	void *userdata)
{	

    NewResourceUploadInfo::ptr_t uploadInfo(new NewFileResourceUploadInfo(
        src_filename,
        name, desc, compression_info,
        destination_folder_type, inv_type,
        next_owner_perms, group_perms, everyone_perms,
        expected_upload_cost));
    upload_new_resource(uploadInfo, callback, userdata);

    return LLUUID::null;

#if 0
	// Generate the temporary UUID.
	std::string filename = gDirUtilp->getTempFilename();
	LLTransactionID tid;
	LLAssetID uuid;
	
	LLSD args;

	std::string exten = gDirUtilp->getExtension(src_filename);
	U32 codec = LLImageBase::getCodecFromExtension(exten);
	LLAssetType::EType asset_type = LLAssetType::AT_NONE;
	std::string error_message;

	BOOL error = FALSE;
	
	if (exten.empty())
	{
		std::string short_name = gDirUtilp->getBaseFileName(filename);
		
		// No extension
		error_message = llformat(
				"No file extension for the file: '%s'\nPlease make sure the file has a correct file extension",
				short_name.c_str());
		args["FILE"] = short_name;
 		upload_error(error_message, "NoFileExtension", filename, args);
		return LLUUID();
	}
	else if (codec != IMG_CODEC_INVALID)
	{
		// It's an image file, the upload procedure is the same for all
		asset_type = LLAssetType::AT_TEXTURE;
		if (!LLViewerTextureList::createUploadFile(src_filename, filename, codec ))
		{
			error_message = llformat( "Problem with file %s:\n\n%s\n",
									 src_filename.c_str(), LLImage::getLastError().c_str());
			args["FILE"] = src_filename;
			args["ERROR"] = LLImage::getLastError();
			upload_error(error_message, "ProblemWithFile", filename, args);
			return LLUUID();
		}
	}
	else if(exten == "wav")
	{
		asset_type = LLAssetType::AT_SOUND;  // tag it as audio
		S32 encode_result = 0;

		LL_INFOS() << "Attempting to encode wav as an ogg file" << LL_ENDL;

		encode_result = encode_vorbis_file(src_filename, filename);
		
		if (LLVORBISENC_NOERR != encode_result)
		{
			switch(encode_result)
			{
				case LLVORBISENC_DEST_OPEN_ERR:
				    error_message = llformat( "Couldn't open temporary compressed sound file for writing: %s\n", filename.c_str());
					args["FILE"] = filename;
					upload_error(error_message, "CannotOpenTemporarySoundFile", filename, args);
					break;

				default:	
				  error_message = llformat("Unknown vorbis encode failure on: %s\n", src_filename.c_str());
					args["FILE"] = src_filename;
					upload_error(error_message, "UnknownVorbisEncodeFailure", filename, args);
					break;	
			}	
			return LLUUID();
		}
	}
	else if (exten == "bvh")
	{
		error_message = llformat("We do not currently support bulk upload of animation files\n");
		upload_error(error_message, "DoNotSupportBulkAnimationUpload", filename, args);
		return LLUUID();
	}
	else if (exten == "anim")
	{
		asset_type = LLAssetType::AT_ANIMATION;
		filename = src_filename;
	}
	else
	{
		// Unknown extension
		error_message = llformat(LLTrans::getString("UnknownFileExtension").c_str(), exten.c_str());
		error = TRUE;;
	}

	// gen a new transaction ID for this asset
	tid.generate();

	if (!error)
	{
		uuid = tid.makeAssetID(gAgent.getSecureSessionID());
		// copy this file into the vfs for upload
		S32 file_size;
		LLAPRFile infile ;
		infile.open(filename, LL_APR_RB, NULL, &file_size);
		if (infile.getFileHandle())
		{
			LLVFile file(gVFS, uuid, asset_type, LLVFile::WRITE);

			file.setMaxSize(file_size);

			const S32 buf_size = 65536;
			U8 copy_buf[buf_size];
			while ((file_size = infile.read(copy_buf, buf_size)))
			{
				file.write(copy_buf, file_size);
			}
		}
		else
		{
			error_message = llformat( "Unable to access output file: %s", filename.c_str());
			error = TRUE;
		}
	}

	if (!error)
	{
        NewResourceUploadInfo::ptr_t uploadInfo(new NewResourceUploadInfo(
            tid, asset_type,
            name, desc, compression_info,
            destination_folder_type, inv_type,
            next_owner_perms, group_perms, everyone_perms,
            expected_upload_cost));

        upload_new_resource(uploadInfo, 
                callback, userdata);
	}
	else
	{
		LL_WARNS() << error_message << LL_ENDL;
		LLSD args;
		args["ERROR_MESSAGE"] = error_message;
		LLNotificationsUtil::add("ErrorMessage", args);
		if(LLFile::remove(filename) == -1)
		{
			LL_DEBUGS() << "unable to remove temp file" << LL_ENDL;
		}
		LLFilePicker::instance().reset();
	}

	return uuid;
#endif
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
	BOOL is_balance_sufficient = TRUE;

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
					LLStringUtil::format_map_t args;
					args["NAME"] = data->mAssetInfo.getName();
					args["AMOUNT"] = llformat("%d", expected_upload_cost);
					LLBuyCurrencyHTML::openCurrencyFloater( LLTrans::getString("UploadingCosts", args), expected_upload_cost );
					is_balance_sufficient = FALSE;
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
							      data->mInventoryType, NOT_WEARABLE, next_owner_perms,
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
		LLAssetStorage::LLStoreAssetCallback callback = NULL;
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
    NewResourceUploadInfo::ptr_t &uploadInfo,
    LLAssetStorage::LLStoreAssetCallback callback,
    void *userdata)
{
	if(gDisconnected)
	{
		return ;
	}

//     uploadInfo->setAssetType(assetType);
//     uploadInfo->setTransactionId(tid);


	std::string url = gAgent.getRegion()->getCapability("NewFileAgentInventory");

	if ( !url.empty() )
	{
        LLCoprocedureManager::CoProcedure_t proc = boost::bind(&LLViewerAssetUpload::AssetInventoryUploadCoproc, _1, _2, _3, url, uploadInfo);

        LLCoprocedureManager::getInstance()->enqueueCoprocedure("LLViewerAssetUpload::AssetInventoryUploadCoproc", proc);
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
				LLStringUtil::format_map_t args;
				args["NAME"] = uploadInfo->getName();
                args["AMOUNT"] = llformat("%d", uploadInfo->getExpectedUploadCost());
                LLBuyCurrencyHTML::openCurrencyFloater(LLTrans::getString("UploadingCosts", args), uploadInfo->getExpectedUploadCost());
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
			FALSE);
	}
}


void init_menu_file()
{
	view_listener_t::addCommit(new LLFileUploadImage(), "File.UploadImage");
	view_listener_t::addCommit(new LLFileUploadSound(), "File.UploadSound");
	view_listener_t::addCommit(new LLFileUploadAnim(), "File.UploadAnim");
	view_listener_t::addCommit(new LLFileUploadModel(), "File.UploadModel");
	view_listener_t::addCommit(new LLFileUploadBulk(), "File.UploadBulk");
	view_listener_t::addCommit(new LLFileCloseWindow(), "File.CloseWindow");
	view_listener_t::addCommit(new LLFileCloseAllWindows(), "File.CloseAllWindows");
	view_listener_t::addEnable(new LLFileEnableCloseWindow(), "File.EnableCloseWindow");
	view_listener_t::addEnable(new LLFileEnableCloseAllWindows(), "File.EnableCloseAllWindows");
	view_listener_t::addCommit(new LLFileTakeSnapshotToDisk(), "File.TakeSnapshotToDisk");
	view_listener_t::addCommit(new LLFileQuit(), "File.Quit");

	view_listener_t::addEnable(new LLFileEnableUpload(), "File.EnableUpload");
	view_listener_t::addEnable(new LLFileEnableUploadModel(), "File.EnableUploadModel");
	view_listener_t::addMenu(new LLMeshEnabled(), "File.MeshEnabled");
	view_listener_t::addMenu(new LLMeshUploadVisible(), "File.VisibleUploadModel");

	// "File.SaveTexture" moved to llpanelmaininventory so that it can be properly handled.
}

LLSD NewResourceUploadInfo::prepareUpload()
{
    if (mAssetId.isNull())
        generateNewAssetId();

    incrementUploadStats();
    assignDefaults();

    return LLSD().with("success", LLSD::Boolean(true));
}

std::string NewResourceUploadInfo::getAssetTypeString() const
{
    return LLAssetType::lookup(mAssetType);
}

std::string NewResourceUploadInfo::getInventoryTypeString() const
{
    return LLInventoryType::lookup(mInventoryType);
}

LLSD NewResourceUploadInfo::generatePostBody()
{
    LLSD body;

    body["folder_id"] = mFolderId;
    body["asset_type"] = getAssetTypeString();
    body["inventory_type"] = getInventoryTypeString();
    body["name"] = mName;
    body["description"] = mDescription;
    body["next_owner_mask"] = LLSD::Integer(mNextOwnerPerms);
    body["group_mask"] = LLSD::Integer(mGroupPerms);
    body["everyone_mask"] = LLSD::Integer(mEveryonePerms);

    return body;

}

void NewResourceUploadInfo::logPreparedUpload()
{
    LL_INFOS() << "*** Uploading: " << std::endl << 
        "Type: " << LLAssetType::lookup(mAssetType) << std::endl <<
        "UUID: " << mAssetId.asString() << std::endl << 
        "Name: " << mName << std::endl << 
        "Desc: " << mDescription << std::endl <<
        "Expected Upload Cost: " << mExpectedUploadCost << std::endl <<
        "Folder: " << mFolderId << std::endl <<
        "Asset Type: " << LLAssetType::lookup(mAssetType) << LL_ENDL;
}

LLUUID NewResourceUploadInfo::finishUpload(LLSD &result)
{
    if (getFolderId().isNull())
    {
        return LLUUID::null;
    }

    U32 permsEveryone = PERM_NONE;
    U32 permsGroup = PERM_NONE;
    U32 permsNextOwner = PERM_ALL;

    if (result.has("new_next_owner_mask"))
    {
        // The server provided creation perms so use them.
        // Do not assume we got the perms we asked for in
        // since the server may not have granted them all.
        permsEveryone = result["new_everyone_mask"].asInteger();
        permsGroup = result["new_group_mask"].asInteger();
        permsNextOwner = result["new_next_owner_mask"].asInteger();
    }
    else
    {
        // The server doesn't provide creation perms
        // so use old assumption-based perms.
        if (getAssetTypeString() != "snapshot")
        {
            permsNextOwner = PERM_MOVE | PERM_TRANSFER;
        }
    }

    LLPermissions new_perms;
    new_perms.init(
        gAgent.getID(),
        gAgent.getID(),
        LLUUID::null,
        LLUUID::null);

    new_perms.initMasks(
        PERM_ALL,
        PERM_ALL,
        permsEveryone,
        permsGroup,
        permsNextOwner);

    U32 flagsInventoryItem = 0;
    if (result.has("inventory_flags"))
    {
        flagsInventoryItem = static_cast<U32>(result["inventory_flags"].asInteger());
        if (flagsInventoryItem != 0)
        {
            LL_INFOS() << "inventory_item_flags " << flagsInventoryItem << LL_ENDL;
        }
    }
    S32 creationDate = time_corrected();

    LLUUID serverInventoryItem = result["new_inventory_item"].asUUID();
    LLUUID serverAssetId = result["new_asset"].asUUID();

    LLPointer<LLViewerInventoryItem> item = new LLViewerInventoryItem(
        serverInventoryItem,
        getFolderId(),
        new_perms,
        serverAssetId,
        getAssetType(),
        getInventoryType(),
        getName(),
        getDescription(),
        LLSaleInfo::DEFAULT,
        flagsInventoryItem,
        creationDate);

    gInventory.updateItem(item);
    gInventory.notifyObservers();

    return serverInventoryItem;
}


LLAssetID NewResourceUploadInfo::generateNewAssetId()
{
    if (gDisconnected)
    {
        LLAssetID rv;

        rv.setNull();
        return rv;
    }
    mAssetId = mTransactionId.makeAssetID(gAgent.getSecureSessionID());

    return mAssetId;
}

void NewResourceUploadInfo::incrementUploadStats() const
{
    if (LLAssetType::AT_SOUND == mAssetType)
    {
        add(LLStatViewer::UPLOAD_SOUND, 1);
    }
    else if (LLAssetType::AT_TEXTURE == mAssetType)
    {
        add(LLStatViewer::UPLOAD_TEXTURE, 1);
    }
    else if (LLAssetType::AT_ANIMATION == mAssetType)
    {
        add(LLStatViewer::ANIMATION_UPLOADS, 1);
    }
}

void NewResourceUploadInfo::assignDefaults()
{
    if (LLInventoryType::IT_NONE == mInventoryType)
    {
        mInventoryType = LLInventoryType::defaultForAssetType(mAssetType);
    }
    LLStringUtil::stripNonprintable(mName);
    LLStringUtil::stripNonprintable(mDescription);

    if (mName.empty())
    {
        mName = "(No Name)";
    }
    if (mDescription.empty())
    {
        mDescription = "(No Description)";
    }

    mFolderId = gInventory.findCategoryUUIDForType(
        (mDestinationFolderType == LLFolderType::FT_NONE) ?
        (LLFolderType::EType)mAssetType : mDestinationFolderType);

}

std::string NewResourceUploadInfo::getDisplayName() const
{ 
    return (mName.empty()) ? mAssetId.asString() : mName; 
};


NewFileResourceUploadInfo::NewFileResourceUploadInfo(
        std::string fileName,
        std::string name,
        std::string description,
        S32 compressionInfo,
        LLFolderType::EType destinationType,
        LLInventoryType::EType inventoryType,
        U32 nextOWnerPerms,
        U32 groupPerms,
        U32 everyonePerms,
        S32 expectedCost):
    NewResourceUploadInfo(name, description, compressionInfo,
        destinationType, inventoryType,
        nextOWnerPerms, groupPerms, everyonePerms, expectedCost),
    mFileName(fileName)
{
    LLTransactionID tid;
    tid.generate();
    setTransactionId(tid);
}



LLSD NewFileResourceUploadInfo::prepareUpload()
{
    generateNewAssetId();

    LLSD result = exportTempFile();
    if (result.has("error"))
        return result;

    return NewResourceUploadInfo::prepareUpload();
}

LLSD NewFileResourceUploadInfo::exportTempFile()
{
    std::string filename = gDirUtilp->getTempFilename();

    std::string exten = gDirUtilp->getExtension(getFileName());
    U32 codec = LLImageBase::getCodecFromExtension(exten);

    LLAssetType::EType assetType = LLAssetType::AT_NONE;
    std::string errorMessage;
    std::string errorLabel;

    bool error = false;

    if (exten.empty())
    {
        std::string shortName = gDirUtilp->getBaseFileName(filename);

        // No extension
        errorMessage = llformat(
            "No file extension for the file: '%s'\nPlease make sure the file has a correct file extension",
            shortName.c_str());
        errorLabel = "NoFileExtension";
        error = true;
    }
    else if (codec != IMG_CODEC_INVALID)
    {
        // It's an image file, the upload procedure is the same for all
        assetType = LLAssetType::AT_TEXTURE;
        if (!LLViewerTextureList::createUploadFile(getFileName(), filename, codec))
        {
            errorMessage = llformat("Problem with file %s:\n\n%s\n",
                getFileName().c_str(), LLImage::getLastError().c_str());
            errorLabel = "ProblemWithFile";
            error = true;
        }
    }
    else if (exten == "wav")
    {
        assetType = LLAssetType::AT_SOUND;  // tag it as audio
        S32 encodeResult = 0;

        LL_INFOS() << "Attempting to encode wav as an ogg file" << LL_ENDL;

        encodeResult = encode_vorbis_file(getFileName(), filename);

        if (LLVORBISENC_NOERR != encodeResult)
        {
            switch (encodeResult)
            {
            case LLVORBISENC_DEST_OPEN_ERR:
                errorMessage = llformat("Couldn't open temporary compressed sound file for writing: %s\n", filename.c_str());
                errorLabel = "CannotOpenTemporarySoundFile";
                break;

            default:
                errorMessage = llformat("Unknown vorbis encode failure on: %s\n", getFileName().c_str());
                errorLabel = "UnknownVorbisEncodeFailure";
                break;
            }
            error = true;
        }
    }
    else if (exten == "bvh")
    {
        errorMessage = llformat("We do not currently support bulk upload of animation files\n");
        errorLabel = "DoNotSupportBulkAnimationUpload";
        error = true;
    }
    else if (exten == "anim")
    {
        assetType = LLAssetType::AT_ANIMATION;
        filename = getFileName();
    }
    else
    {
        // Unknown extension
        errorMessage = llformat(LLTrans::getString("UnknownFileExtension").c_str(), exten.c_str());
        errorLabel = "ErrorMessage";
        error = TRUE;;
    }

    if (error)
    {
        LLSD errorResult(LLSD::emptyMap());

        errorResult["error"] = LLSD::Binary(true);
        errorResult["message"] = errorMessage;
        errorResult["label"] = errorLabel;
        return errorResult;
    }

    setAssetType(assetType);

    // copy this file into the vfs for upload
    S32 file_size;
    LLAPRFile infile;
    infile.open(filename, LL_APR_RB, NULL, &file_size);
    if (infile.getFileHandle())
    {
        LLVFile file(gVFS, getAssetId(), assetType, LLVFile::WRITE);

        file.setMaxSize(file_size);

        const S32 buf_size = 65536;
        U8 copy_buf[buf_size];
        while ((file_size = infile.read(copy_buf, buf_size)))
        {
            file.write(copy_buf, file_size);
        }
    }
    else
    {
        errorMessage = llformat("Unable to access output file: %s", filename.c_str());
        LLSD errorResult(LLSD::emptyMap());

        errorResult["error"] = LLSD::Binary(true);
        errorResult["message"] = errorMessage;
        return errorResult;
    }

    return LLSD();

}
