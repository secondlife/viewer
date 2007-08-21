/** 
 * @file llviewermenufile.cpp
 * @brief "File" menu in the main menu bar.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewermenufile.h"

// project includes
#include "llagent.h"
#include "llfilepicker.h"
#include "llfloateranimpreview.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterimagepreview.h"
#include "llfloaterimport.h"
#include "llfloaternamedesc.h"
#include "llfloatersnapshot.h"
#include "llinventorymodel.h"	// gInventory
#include "llresourcedata.h"
#include "llstatusbar.h"
#include "llviewercontrol.h"	// gSavedSettings
#include "llviewerimagelist.h"
#include "llvieweruictrlfactory.h"
#include "llviewermenu.h"	// gMenuHolder
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "viewer.h"	// app_request_quit()

// linden libraries
#include "llassetuploadresponders.h"
#include "lleconomy.h"
#include "llhttpclient.h"
#include "llmemberlistener.h"
#include "llsdserialize.h"
#include "llstring.h"
#include "lltransactiontypes.h"
#include "lluuid.h"
#include "vorbisencode.h"

// system libraries
#include <boost/tokenizer.hpp>

typedef LLMemberListener<LLView> view_listener_t;


class LLFileEnableSaveAs : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gFloaterView->getFrontmost() && gFloaterView->getFrontmost()->canSaveAs();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLFileEnableUpload : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gStatusBar && gGlobalEconomy && (gStatusBar->getBalance() >= gGlobalEconomy->getPriceUpload());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

/**
   char* upload_pick(void* data)

   If applicable, brings up a file chooser in which the user selects a file
   to upload for a particular task.  If the file is valid for the given action,
   returns the string to the full path filename, else returns NULL.
   Data is the load filter for the type of file as defined in LLFilePicker.
**/
const char* upload_pick(void* data)
{
 	if( gAgent.cameraMouselook() )
	{
		gAgent.changeCameraToDefault();
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
		llinfos << "Couldn't import objects from file" << llendl;
		return NULL;
	}

 	const char* filename = picker.getFirstFile();
	const char* ext = strrchr(filename, '.');

	//strincmp doesn't like NULL pointers
	if (ext == NULL)
	{
		const char* short_name = strrchr(filename,
										 *gDirUtilp->getDirDelimiter().c_str());
		
		// No extension
		LLStringBase<char>::format_map_t args;
		args["[FILE]"] = LLString(short_name + 1);
		gViewerWindow->alertXml("NoFileExtension", args);
		return NULL;
	}
	else
	{
		//so there is an extension
		//loop over the valid extensions and compare to see
		//if the extension is valid

		//now grab the set of valid file extensions
		const char* valids = build_extensions_string(type);
		std::string valid_extensions = std::string(valids);

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
			const char* cur_token = token_iter->c_str();

			if (0 == strnicmp(cur_token, ext, strlen(cur_token)) ||		/* Flawfinder: ignore */
				0 == strnicmp(cur_token, "*.*", strlen(cur_token))) 		/* Flawfinder: ignore */
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
			LLStringBase<char>::format_map_t args;
			args["[EXTENSION]"] = ext;
			args["[VALIDS]"] = valids;
			gViewerWindow->alertXml("InvalidFileExtension", args);
			return NULL;
		}
	}//end else (non-null extension)

	//valid file extension
	
	//now we check to see
	//if the file is actually a valid image/sound/etc.
	if (type == LLFilePicker::FFLOAD_WAV)
	{
		// pre-qualify wavs to make sure the format is acceptable
		char error_msg[MAX_STRING];		/* Flawfinder: ignore */	
		if (check_for_invalid_wav_formats(filename,error_msg))
		{
			llinfos << error_msg << ": " << filename << llendl;
			LLStringBase<char>::format_map_t args;
			args["[FILE]"] = filename;
			gViewerWindow->alertXml( error_msg, args );
			return NULL;
		}
	}//end if a wave/sound file

	
	return filename;
}

/*
void handle_upload_object(void* data)
{
	const char* filename = upload_pick(data);
	if (filename)
	{
		// start the import
		LLFloaterImport* floaterp = new LLFloaterImport(filename);
		gUICtrlFactory->buildFloater(floaterp, "floater_import.xml");
	}
}*/

class LLFileUploadImage : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		const char* filename = upload_pick((void *)LLFilePicker::FFLOAD_IMAGE);
		if (filename)
		{
			LLFloaterImagePreview* floaterp = new LLFloaterImagePreview(filename);
			gUICtrlFactory->buildFloater(floaterp, "floater_image_preview.xml");
		}
		return TRUE;
	}
};

class LLFileUploadSound : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		const char* filename = upload_pick((void*)LLFilePicker::FFLOAD_WAV);
		if (filename)
		{
			LLFloaterNameDesc* floaterp = new LLFloaterNameDesc(filename);
			gUICtrlFactory->buildFloater(floaterp, "floater_sound_preview.xml");
		}
		return true;
	}
};

class LLFileUploadAnim : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		const char* filename = upload_pick((void*)LLFilePicker::FFLOAD_ANIM);
		if (filename)
		{
			LLFloaterAnimPreview* floaterp = new LLFloaterAnimPreview(filename);
			gUICtrlFactory->buildFloater(floaterp, "floater_animation_preview.xml");
		}
		return true;
	}
};

class LLFileUploadBulk : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if( gAgent.cameraMouselook() )
		{
			gAgent.changeCameraToDefault();
		}

		// TODO:
		// Iterate over all files
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
			const char* filename = picker.getFirstFile();
			const char* name = picker.getDirname();

			LLString asset_name = name;
			LLString::replaceNonstandardASCII( asset_name, '?' );
			LLString::replaceChar(asset_name, '|', '?');
			LLString::stripNonprintable(asset_name);
			LLString::trim(asset_name);

			char* asset_name_str = (char*)asset_name.c_str();
			char* end_p = strrchr(asset_name_str, '.');		 // strip extension if exists
			if( !end_p )
			{
				end_p = asset_name_str + strlen( asset_name_str );		/* Flawfinder: ignore */
			}
				
			S32 len = llmin( (S32) (DB_INV_ITEM_NAME_STR_LEN), (S32) (end_p - asset_name_str) );

			asset_name = asset_name.substr( 0, len );

			upload_new_resource(filename, asset_name, asset_name, 0, LLAssetType::AT_NONE, LLInventoryType::IT_NONE); // file
		}
		else
		{
			llinfos << "Couldn't import objects from file" << llendl;
		}
		return true;
	}
};

void upload_error(const char* error_message, const char* label, const std::string filename, const LLStringBase<char>::format_map_t args) 
{
	llwarns << error_message << llendl;
	gViewerWindow->alertXml(label, args);
	if(remove(filename.c_str()) == -1)
	{
		lldebugs << "unable to remove temp file" << llendl;
	}
	LLFilePicker::instance().reset();						
}

class LLFileEnableCloseWindow : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gFloaterView->getFocusedFloater() != NULL || gSnapshotFloaterView->getFocusedFloater() != NULL;
		// horrendously opaque, this code
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLFileCloseWindow : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloater::closeFocusedFloater();

		return true;
	}
};

class LLFileCloseAllWindows : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool app_quitting = false;
		gFloaterView->closeAllChildren(app_quitting);

		return true;
	}
};

class LLFileSaveTexture : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloater* top = gFloaterView->getFrontmost();
		if (top)
		{
			top->saveAs();
		}
		return true;
	}
};

class LLFileTakeSnapshot : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloaterSnapshot::show(NULL);
		return true;
	}
};

class LLFileTakeSnapshotToDisk : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLPointer<LLImageRaw> raw = new LLImageRaw;

		S32 width = gViewerWindow->getWindowDisplayWidth();
		S32 height = gViewerWindow->getWindowDisplayHeight();

		if (gSavedSettings.getBOOL("HighResSnapshot"))
		{
			width *= 2;
			height *= 2;
		}

		if (gViewerWindow->rawSnapshot(raw,
									   width,
									   height,
									   TRUE,
									   gSavedSettings.getBOOL("RenderUIInSnapshot"),
									   FALSE))
		{
			if (!gQuietSnapshot)
			{
				gViewerWindow->playSnapshotAnimAndSound();
			}
			LLImageBase::setSizeOverride(TRUE);
			gViewerWindow->saveImageNumbered(raw);
			LLImageBase::setSizeOverride(FALSE);
		}
		return true;
	}
};

class LLFileSaveMovie : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerWindow::saveMovieNumbered(NULL);
		return true;
	}
};

class LLFileSetWindowSize : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString size = userdata.asString();
		S32 width, height;
		sscanf(size.c_str(), "%d,%d", &width, &height);
		LLViewerWindow::movieSize(width, height);
		return true;
	}
};

class LLFileQuit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		app_user_quit();
		return true;
	}
};

void handle_upload(void* data)
{
	const char* filename = upload_pick(data);
	if (filename)
	{
		LLFloaterNameDesc* floaterp = new LLFloaterNameDesc(filename);
		gUICtrlFactory->buildFloater(floaterp, "floater_name_description.xml");
	}
}

void handle_compress_image(void*)
{
	LLFilePicker& picker = LLFilePicker::instance();
	if (picker.getOpenFile(LLFilePicker::FFLOAD_IMAGE))
	{
		std::string infile(picker.getFirstFile());
		std::string outfile = infile + ".j2c";

		llinfos << "Input:  " << infile << llendl;
		llinfos << "Output: " << outfile << llendl;

		BOOL success;

		success = LLViewerImageList::createUploadFile(infile, outfile, IMG_CODEC_TGA);

		if (success)
		{
			llinfos << "Compression complete" << llendl;
		}
		else
		{
			llinfos << "Compression failed: " << LLImageBase::getLastError() << llendl;
		}
	}
}

void upload_new_resource(const LLString& src_filename, std::string name,
						 std::string desc, S32 compression_info,
						 LLAssetType::EType destination_folder_type,
						 LLInventoryType::EType inv_type,
						 U32 next_owner_perm,
						 const LLString& display_name,
						 LLAssetStorage::LLStoreAssetCallback callback,
						 void *userdata)
{	
	// Generate the temporary UUID.
	LLString filename = gDirUtilp->getTempFilename();
	LLTransactionID tid;
	LLAssetID uuid;
	
	LLStringBase<char>::format_map_t args;

	LLString ext = src_filename.substr(src_filename.find_last_of('.'));
	LLAssetType::EType asset_type = LLAssetType::AT_NONE;
	char error_message[MAX_STRING];		/* Flawfinder: ignore */	
	error_message[0] = '\0';
	LLString temp_str;

	BOOL error = FALSE;
	
	if (ext.empty())
	{
		LLString::size_type offset = filename.find_last_of(gDirUtilp->getDirDelimiter());
		if (offset != LLString::npos)
			offset++;
		LLString short_name = filename.substr(offset);
		
		// No extension
		snprintf(error_message,		/* Flawfinder: ignore */
				MAX_STRING,
				"No file extension for the file: '%s'\nPlease make sure the file has a correct file extension",
				short_name.c_str());
		args["[FILE]"] = short_name;
 		upload_error(error_message, "NofileExtension", filename, args);
		return;
	}
	else if( LLString::compareInsensitive(ext.c_str(),".bmp") == 0 )
	{
		asset_type = LLAssetType::AT_TEXTURE;
		if (!LLViewerImageList::createUploadFile(src_filename,
												 filename,
												 IMG_CODEC_BMP ))
		{
			snprintf(error_message, MAX_STRING, "Problem with file %s:\n\n%s\n",		/* Flawfinder: ignore */
					src_filename.c_str(), LLImageBase::getLastError().c_str());
			args["[FILE]"] = src_filename;
			args["[ERROR]"] = LLImageBase::getLastError();
			upload_error(error_message, "ProblemWithFile", filename, args);
			return;
		}
	}
	else if( LLString::compareInsensitive(ext.c_str(),".tga") == 0 )
	{
		asset_type = LLAssetType::AT_TEXTURE;
		if (!LLViewerImageList::createUploadFile(src_filename,
												 filename,
												 IMG_CODEC_TGA ))
		{
			snprintf(error_message, MAX_STRING, "Problem with file %s:\n\n%s\n",		/* Flawfinder: ignore */
					src_filename.c_str(), LLImageBase::getLastError().c_str());
			args["[FILE]"] = src_filename;
			args["[ERROR]"] = LLImageBase::getLastError();
			upload_error(error_message, "ProblemWithFile", filename, args);
			return;
		}
	}
	else if( LLString::compareInsensitive(ext.c_str(),".jpg") == 0 || LLString::compareInsensitive(ext.c_str(),".jpeg") == 0)
	{
		asset_type = LLAssetType::AT_TEXTURE;
		if (!LLViewerImageList::createUploadFile(src_filename,
												 filename,
												 IMG_CODEC_JPEG ))
		{
			snprintf(error_message, MAX_STRING, "Problem with file %s:\n\n%s\n",		/* Flawfinder: ignore */
					src_filename.c_str(), LLImageBase::getLastError().c_str());
			args["[FILE]"] = src_filename;
			args["[ERROR]"] = LLImageBase::getLastError();
			upload_error(error_message, "ProblemWithFile", filename, args);
			return;
		}
	}
 	else if( LLString::compareInsensitive(ext.c_str(),".png") == 0 )
 	{
 		asset_type = LLAssetType::AT_TEXTURE;
 		if (!LLViewerImageList::createUploadFile(src_filename,
 												 filename,
 												 IMG_CODEC_PNG ))
 		{
 			sprintf(error_message, "Problem with file %s:\n\n%s\n",
 					src_filename.c_str(), LLImageBase::getLastError().c_str());
 			args["[FILE]"] = src_filename;
 			args["[ERROR]"] = LLImageBase::getLastError();
 			upload_error(error_message, "ProblemWithFile", filename, args);
 			return;
 		}
 	}
	else if(LLString::compareInsensitive(ext.c_str(),".wav") == 0)
	{
		asset_type = LLAssetType::AT_SOUND;  // tag it as audio
		S32 encode_result = 0;

		S32 bitrate = 128;

		if (compression_info)
		{
			bitrate = compression_info;
		}
		llinfos << "Attempting to encode wav as an ogg file at " << bitrate << "kbps" << llendl;

		encode_result = encode_vorbis_file_at(src_filename.c_str(), filename.c_str(), bitrate*1000);
		
		if (LLVORBISENC_NOERR != encode_result)
		{
			switch(encode_result)
			{
				case LLVORBISENC_DEST_OPEN_ERR:
                    snprintf(error_message, MAX_STRING, "Couldn't open temporary compressed sound file for writing: %s\n", filename.c_str());		/* Flawfinder: ignore */
					args["[FILE]"] = filename;
					upload_error(error_message, "CannotOpenTemporarySoundFile", filename, args);
					break;

				default:	
				  snprintf(error_message, MAX_STRING, "Unknown vorbis encode failure on: %s\n", src_filename.c_str());		/* Flawfinder: ignore */
					args["[FILE]"] = src_filename;
					upload_error(error_message, "UnknownVorbisEncodeFailure", filename, args);
					break;	
			}	
			return;
		}
	}
	else if(LLString::compareInsensitive(ext.c_str(),".tmp") == 0)	 	
	{	 	
		// This is a generic .lin resource file	 	
         asset_type = LLAssetType::AT_OBJECT;	 	
         FILE* in = LLFile::fopen(src_filename.c_str(), "rb");		/* Flawfinder: ignore */	 	
         if (in)	 	
         {	 	
                 // read in the file header	 	
                 char buf[16384];		/* Flawfinder: ignore */ 	
                 S32 read;		/* Flawfinder: ignore */	 	
                 S32  version;	 	
                 if (fscanf(in, "LindenResource\nversion %d\n", &version))	 	
                 {	 	
                         if (2 == version)	 	
                         {
								// *NOTE: This buffer size is hard coded into scanf() below.
                                 char label[MAX_STRING];		/* Flawfinder: ignore */	 	
                                 char value[MAX_STRING];		/* Flawfinder: ignore */	 	
                                 S32  tokens_read;	 	
                                 while (fgets(buf, 1024, in))	 	
                                 {	 	
                                         label[0] = '\0';	 	
                                         value[0] = '\0';	 	
                                         tokens_read = sscanf(	/* Flawfinder: ignore */
											 buf,
											 "%254s %254s\n",
											 label, value);	 	

                                         llinfos << "got: " << label << " = " << value	 	
                                                         << llendl;	 	

                                         if (EOF == tokens_read)	 	
                                         {	 	
                                                 fclose(in);	 	
                                                 snprintf(error_message, MAX_STRING, "corrupt resource file: %s", src_filename.c_str());		/* Flawfinder: ignore */
												 args["[FILE]"] = src_filename;
												 upload_error(error_message, "CorruptResourceFile", filename, args);
                                                 return;
                                         }	 	

                                         if (2 == tokens_read)	 	
                                         {	 	
                                                 if (! strcmp("type", label))	 	
                                                 {	 	
                                                         asset_type = (LLAssetType::EType)(atoi(value));	 	
                                                 }	 	
                                         }	 	
                                         else	 	
                                         {	 	
                                                 if (! strcmp("_DATA_", label))	 	
                                                 {	 	
                                                         // below is the data section	 	
                                                         break;	 	
                                                 }	 	
                                         }	 	
                                         // other values are currently discarded	 	
                                 }	 	

                         }	 	
                         else	 	
                         {	 	
                                 fclose(in);	 	
                                 snprintf(error_message, MAX_STRING, "unknown linden resource file version in file: %s", src_filename.c_str());		/* Flawfinder: ignore */
								 args["[FILE]"] = src_filename;
								 upload_error(error_message, "UnknownResourceFileVersion", filename, args);
                                 return;
                         }	 	
                 }	 	
                 else	 	
                 {	 	
                         // this is an original binary formatted .lin file	 	
                         // start over at the beginning of the file	 	
                         fseek(in, 0, SEEK_SET);	 	

                         const S32 MAX_ASSET_DESCRIPTION_LENGTH = 256;	 	
                         const S32 MAX_ASSET_NAME_LENGTH = 64;	 	
                         S32 header_size = 34 + MAX_ASSET_DESCRIPTION_LENGTH + MAX_ASSET_NAME_LENGTH;	 	
                         S16     type_num;	 	

                         // read in and throw out most of the header except for the type	 	
                         if (fread(buf, header_size, 1, in) != 1)
						 {
							 llwarns << "Short read" << llendl;
						 }
                         memcpy(&type_num, buf + 16, sizeof(S16));		/* Flawfinder: ignore */	 	
                         asset_type = (LLAssetType::EType)type_num;	 	
                 }	 	

                 // copy the file's data segment into another file for uploading	 	
                 FILE* out = LLFile::fopen(filename.c_str(), "wb");		/* Flawfinder: ignore */	
                 if (out)	 	
                 {	 	
                         while((read = fread(buf, 1, 16384, in)))		/* Flawfinder: ignore */	 	
                         {	 	
							 if (fwrite(buf, 1, read, out) != read)
							 {
								 llwarns << "Short write" << llendl;
							 }
                         }	 	
                         fclose(out);	 	
                 }	 	
                 else	 	
                 {	 	
                         fclose(in);	 	
                         snprintf(error_message, MAX_STRING, "Unable to create output file: %s", filename.c_str());		/* Flawfinder: ignore */
						 args["[FILE]"] = filename;
						 upload_error(error_message, "UnableToCreateOutputFile", filename, args);
                         return;
                 }	 	

                 fclose(in);	 	
         }	 	
         else	 	
         {	 	
                 llinfos << "Couldn't open .lin file " << src_filename << llendl;	 	
         }	 	
	}
	else if (LLString::compareInsensitive(ext.c_str(),".bvh") == 0)
	{
		snprintf(error_message, MAX_STRING, "We do not currently support bulk upload of animation files\n");		/* Flawfinder: ignore */
		upload_error(error_message, "DoNotSupportBulkAnimationUpload", filename, args);
		return;
	}
	else
	{
		// Unknown extension
		snprintf(error_message, MAX_STRING, "Unknown file extension %s\nExpected .wav, .tga, .bmp, .jpg, .jpeg, or .bvh", ext.c_str());		/* Flawfinder: ignore */
		error = TRUE;;
	}

	// gen a new transaction ID for this asset
	tid.generate();

	if (!error)
	{
		uuid = tid.makeAssetID(gAgent.getSecureSessionID());
		// copy this file into the vfs for upload
		S32 file_size;
		apr_file_t* fp = ll_apr_file_open(filename, LL_APR_RB, &file_size);
		if (fp)
		{
			LLVFile file(gVFS, uuid, asset_type, LLVFile::WRITE);

			file.setMaxSize(file_size);

			const S32 buf_size = 65536;
			U8 copy_buf[buf_size];
			while ((file_size = ll_apr_file_read(fp, copy_buf, buf_size)))
			{
				file.write(copy_buf, file_size);
			}
			apr_file_close(fp);
		}
		else
		{
			snprintf(error_message, MAX_STRING, "Unable to access output file: %s", filename.c_str());		/* Flawfinder: ignore */
			error = TRUE;
		}
	}

	if (!error)
	{
		LLString t_disp_name = display_name;
		if (t_disp_name.empty())
		{
			t_disp_name = src_filename;
		}
		upload_new_resource(tid, asset_type, name, desc, compression_info, // tid
							destination_folder_type, inv_type, next_owner_perm,
							display_name, callback, userdata);
	}
	else
	{
		llwarns << error_message << llendl;
		LLStringBase<char>::format_map_t args;
		args["[ERROR_MESSAGE]"] = error_message;
		gViewerWindow->alertXml("ErrorMessage", args);
		if(LLFile::remove(filename.c_str()) == -1)
		{
			lldebugs << "unable to remove temp file" << llendl;
		}
		LLFilePicker::instance().reset();
	}
}

void upload_done_callback(const LLUUID& uuid, void* user_data, S32 result, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLResourceData* data = (LLResourceData*)user_data;
	//LLAssetType::EType pref_loc = data->mPreferredLocation;
	BOOL is_balance_sufficient = TRUE;
	if(result >= 0)
	{
		LLAssetType::EType dest_loc = (data->mPreferredLocation == LLAssetType::AT_NONE) ? data->mAssetInfo.mType : data->mPreferredLocation;

		if (LLAssetType::AT_SOUND == data->mAssetInfo.mType ||
			LLAssetType::AT_TEXTURE == data->mAssetInfo.mType ||
			LLAssetType::AT_ANIMATION == data->mAssetInfo.mType)
		{
			// Charge the user for the upload.
			LLViewerRegion* region = gAgent.getRegion();
			S32 upload_cost = gGlobalEconomy->getPriceUpload();

			if(!(can_afford_transaction(upload_cost)))
			{
				LLFloaterBuyCurrency::buyCurrency(
					llformat("Uploading %s costs",
						data->mAssetInfo.getName().c_str()),
					upload_cost);
				is_balance_sufficient = FALSE;
			}
			else if(region)
			{
				// Charge user for upload
				gStatusBar->debitBalance(upload_cost);
				
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_MoneyTransferRequest);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_MoneyData);
				msg->addUUIDFast(_PREHASH_SourceID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_DestID, LLUUID::null);
				msg->addU8("Flags", 0);
				msg->addS32Fast(_PREHASH_Amount, upload_cost);
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
			llinfos << "Adding " << uuid << " to inventory." << llendl;
			LLUUID folder_id(gInventory.findCategoryUUIDForType(dest_loc));
			if(folder_id.notNull())
			{
				U32 next_owner_perm = data->mNextOwnerPerm;
				if(PERM_NONE == next_owner_perm)
				{
					next_owner_perm = PERM_MOVE | PERM_TRANSFER;
				}
				create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
					folder_id, data->mAssetInfo.mTransactionID, data->mAssetInfo.getName(),
					data->mAssetInfo.getDescription(), data->mAssetInfo.mType,
					data->mInventoryType, NOT_WEARABLE, next_owner_perm,
					LLPointer<LLInventoryCallback>(NULL));
			}
			else
			{
				llwarns << "Can't find a folder to put it in" << llendl;
			}
		}
	}
	else // 	if(result >= 0)
	{
		LLStringBase<char>::format_map_t args;
		args["[FILE]"] = LLInventoryType::lookupHumanReadable(data->mInventoryType);
		args["[REASON]"] = LLString(LLAssetStorage::getErrorString(result));
		gViewerWindow->alertXml("CannotUploadReason", args);
	}

	LLUploadDialog::modalUploadFinished();
	delete data;

	// *NOTE: This is a pretty big hack. What this does is check the
	// file picker if there are any more pending uploads. If so,
	// upload that file.
	const char* next_file = LLFilePicker::instance().getNextFile();
	if(is_balance_sufficient && next_file)
	{
		const char* name = LLFilePicker::instance().getDirname();

		LLString asset_name = name;
		LLString::replaceNonstandardASCII( asset_name, '?' );
		LLString::replaceChar(asset_name, '|', '?');
		LLString::stripNonprintable(asset_name);
		LLString::trim(asset_name);

		char* asset_name_str = (char*)asset_name.c_str();
		char* end_p = strrchr(asset_name_str, '.');		 // strip extension if exists
		if( !end_p )
		{
			end_p = asset_name_str + strlen( asset_name_str );		/* Flawfinder: ignore */
		}
			
		S32 len = llmin( (S32) (DB_INV_ITEM_NAME_STR_LEN), (S32) (end_p - asset_name_str) );

		asset_name = asset_name.substr( 0, len );

		upload_new_resource(next_file, asset_name, asset_name,	// file
							0, LLAssetType::AT_NONE, LLInventoryType::IT_NONE);
	}
}

void upload_new_resource(const LLTransactionID &tid, LLAssetType::EType asset_type,
						 std::string name,
						 std::string desc, S32 compression_info,
						 LLAssetType::EType destination_folder_type,
						 LLInventoryType::EType inv_type,
						 U32 next_owner_perm,
						 const LLString& display_name,
						 LLAssetStorage::LLStoreAssetCallback callback,
						 void *userdata)
{
	LLAssetID uuid = tid.makeAssetID(gAgent.getSecureSessionID());
	
	if( LLAssetType::AT_SOUND == asset_type )
	{
		gViewerStats->incStat(LLViewerStats::ST_UPLOAD_SOUND_COUNT );
	}
	else
	if( LLAssetType::AT_TEXTURE == asset_type )
	{
		gViewerStats->incStat(LLViewerStats::ST_UPLOAD_TEXTURE_COUNT );
	}
	else
	if( LLAssetType::AT_ANIMATION == asset_type)
	{
		gViewerStats->incStat(LLViewerStats::ST_UPLOAD_ANIM_COUNT );
	}

	if(LLInventoryType::IT_NONE == inv_type)
	{
		inv_type = LLInventoryType::defaultForAssetType(asset_type);
	}
	LLString::stripNonprintable(name);
	LLString::stripNonprintable(desc);
	if(name.empty())
	{
		name = "(No Name)";
	}
	if(desc.empty())
	{
		desc = "(No Description)";
	}
	
	// At this point, we're ready for the upload.
	LLString upload_message = "Uploading...\n\n";
	upload_message.append(display_name);
	LLUploadDialog::modalUploadDialog(upload_message);

	llinfos << "*** Uploading: " << llendl;
	llinfos << "Type: " << LLAssetType::lookup(asset_type) << llendl;
	llinfos << "UUID: " << uuid << llendl;
	llinfos << "Name: " << name << llendl;
	llinfos << "Desc: " << desc << llendl;
	lldebugs << "Folder: " << gInventory.findCategoryUUIDForType((destination_folder_type == LLAssetType::AT_NONE) ? asset_type : destination_folder_type) << llendl;
	lldebugs << "Asset Type: " << LLAssetType::lookup(asset_type) << llendl;
	std::string url = gAgent.getRegion()->getCapability("NewFileAgentInventory");
	if (!url.empty())
	{
		llinfos << "New Agent Inventory via capability" << llendl;
		LLSD body;
		body["folder_id"] = gInventory.findCategoryUUIDForType((destination_folder_type == LLAssetType::AT_NONE) ? asset_type : destination_folder_type);
		body["asset_type"] = LLAssetType::lookup(asset_type);
		body["inventory_type"] = LLInventoryType::lookup(inv_type);
		body["name"] = name;
		body["description"] = desc;
		
		std::ostringstream llsdxml;
		LLSDSerialize::toXML(body, llsdxml);
		lldebugs << "posting body to capability: " << llsdxml.str() << llendl;
		LLHTTPClient::post(url, body, new LLNewAgentInventoryResponder(body, uuid, asset_type));
	}
	else
	{
		llinfos << "NewAgentInventory capability not found, new agent inventory via asset system." << llendl;
		// check for adequate funds
		// TODO: do this check on the sim
		if (LLAssetType::AT_SOUND == asset_type ||
			LLAssetType::AT_TEXTURE == asset_type ||
			LLAssetType::AT_ANIMATION == asset_type)
		{
			S32 upload_cost = gGlobalEconomy->getPriceUpload();
			S32 balance = gStatusBar->getBalance();
			if (balance < upload_cost)
			{
				// insufficient funds, bail on this upload
				LLFloaterBuyCurrency::buyCurrency("Uploading costs", upload_cost);
				return;
			}
		}

		LLResourceData* data = new LLResourceData;
		data->mAssetInfo.mTransactionID = tid;
		data->mAssetInfo.mUuid = uuid;
		data->mAssetInfo.mType = asset_type;
		data->mAssetInfo.mCreatorID = gAgentID;
		data->mInventoryType = inv_type;
		data->mNextOwnerPerm = next_owner_perm;
		data->mUserData = userdata;
		data->mAssetInfo.setName(name);
		data->mAssetInfo.setDescription(desc);
		data->mPreferredLocation = destination_folder_type;

		LLAssetStorage::LLStoreAssetCallback asset_callback = &upload_done_callback;
		if (callback)
		{
			asset_callback = callback;
		}
		gAssetStorage->storeAssetData(data->mAssetInfo.mTransactionID, data->mAssetInfo.mType,
										asset_callback,
										(void*)data,
										FALSE);
	}
}


void init_menu_file()
{
	(new LLFileUploadImage())->registerListener(gMenuHolder, "File.UploadImage");
	(new LLFileUploadSound())->registerListener(gMenuHolder, "File.UploadSound");
	(new LLFileUploadAnim())->registerListener(gMenuHolder, "File.UploadAnim");
	(new LLFileUploadBulk())->registerListener(gMenuHolder, "File.UploadBulk");
	(new LLFileCloseWindow())->registerListener(gMenuHolder, "File.CloseWindow");
	(new LLFileCloseAllWindows())->registerListener(gMenuHolder, "File.CloseAllWindows");
	(new LLFileEnableCloseWindow())->registerListener(gMenuHolder, "File.EnableCloseWindow");
	(new LLFileSaveTexture())->registerListener(gMenuHolder, "File.SaveTexture");
	(new LLFileTakeSnapshot())->registerListener(gMenuHolder, "File.TakeSnapshot");
	(new LLFileTakeSnapshotToDisk())->registerListener(gMenuHolder, "File.TakeSnapshotToDisk");
	(new LLFileSaveMovie())->registerListener(gMenuHolder, "File.SaveMovie");
	(new LLFileSetWindowSize())->registerListener(gMenuHolder, "File.SetWindowSize");
	(new LLFileQuit())->registerListener(gMenuHolder, "File.Quit");

	(new LLFileEnableUpload())->registerListener(gMenuHolder, "File.EnableUpload");
	(new LLFileEnableSaveAs())->registerListener(gMenuHolder, "File.EnableSaveAs");
}
