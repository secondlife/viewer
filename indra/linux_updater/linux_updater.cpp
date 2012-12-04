/**
 * @file linux_updater.cpp
 * @author Kyle Ambroff <ambroff@lindenlab.com>, Tofu Linden
 * @brief Viewer update program for unix platforms that support GTK+
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "linden_common.h"
#include "llerrorcontrol.h"
#include "llfile.h"
#include "lldir.h"
#include "lldiriterator.h"

/*==========================================================================*|
// IQA-490: Use of LLTrans -- by this program at least -- appears to be buggy.
// With it, the 3.3.2 beta 1 linux-updater.bin crashes; without it seems stable.
#include "llxmlnode.h"
#include "lltrans.h"
|*==========================================================================*/

static class LLTrans
{
public:
	LLTrans();
	static std::string getString(const std::string& key);

private:
	std::string _getString(const std::string& key) const;

	typedef std::map<std::string, std::string> MessageMap;
	MessageMap mMessages;
} sLLTransInstance;

#include <curl/curl.h>
#include <map>
#include <boost/foreach.hpp>

extern "C" {
#include <gtk/gtk.h>
}

const guint UPDATE_PROGRESS_TIMEOUT = 100;
const guint UPDATE_PROGRESS_TEXT_TIMEOUT = 1000;
const guint ROTATE_IMAGE_TIMEOUT = 8000;

typedef struct _updater_app_state {
	std::string app_name;
	std::string url;
	std::string file;
	std::string image_dir;
	std::string dest_dir;
	std::string strings_dirs;
	std::string strings_file;

	LLDirIterator *image_dir_iter;

	GtkWidget *window;
	GtkWidget *progress_bar;
	GtkWidget *image;

	double progress_value;
	bool activity_mode;

	guint image_rotation_timeout_id;
	guint progress_update_timeout_id;
	guint update_progress_text_timeout_id;

	bool failure;
} UpdaterAppState;

// List of entries from strings.xml to always replace
static std::set<std::string> default_trans_args;
void init_default_trans_args()
{
        default_trans_args.insert("SECOND_LIFE"); // World
        default_trans_args.insert("APP_NAME");
        default_trans_args.insert("SECOND_LIFE_GRID");
        default_trans_args.insert("SUPPORT_SITE");
}

bool translate_init(std::string comma_delim_path_list,
		    std::string base_xml_name)
{
	return true;
/*==========================================================================*|
	init_default_trans_args();

	// extract paths string vector from comma-delimited flat string
	std::vector<std::string> paths;
	LLStringUtil::getTokens(comma_delim_path_list, paths, ","); // split over ','

	for(std::vector<std::string>::iterator it = paths.begin(), end_it = paths.end();
		it != end_it;
		++it)
	{
		(*it) = gDirUtilp->findSkinnedFilename(*it, base_xml_name);
	}

	// suck the translation xml files into memory
	LLXMLNodePtr root;
	bool success = LLXMLNode::getLayeredXMLNode(root, paths);
	if (!success)
	{
		// couldn't load string table XML
		return false;
	}
	else
	{
		// get those strings out of the XML
		LLTrans::parseStrings(root, default_trans_args);
		return true;
	}
|*==========================================================================*/
}


void updater_app_ui_init(void);
void updater_app_quit(UpdaterAppState *app_state);
void parse_args_and_init(int argc, char **argv, UpdaterAppState *app_state);
std::string next_image_filename(std::string& image_path, LLDirIterator& iter);
void display_error(GtkWidget *parent, std::string title, std::string message);
BOOL install_package(std::string package_file, std::string destination);
BOOL spawn_viewer(UpdaterAppState *app_state);

extern "C" {
	void on_window_closed(GtkWidget *sender, GdkEvent *event, gpointer state);
	gpointer worker_thread_cb(gpointer *data);
	int download_progress_cb(gpointer data, double t, double d, double utotal, double ulnow);
	gboolean rotate_image_cb(gpointer data);
	gboolean progress_update_timeout(gpointer data);
	gboolean update_progress_text_timeout(gpointer data);
}

void updater_app_ui_init(UpdaterAppState *app_state)
{
	GtkWidget *vbox;
	GtkWidget *summary_label;
	GtkWidget *description_label;
	GtkWidget *frame;

	llassert(app_state != NULL);

	// set up window and main container
	std::string window_title = LLTrans::getString("UpdaterWindowTitle");
	app_state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(app_state->window),
			     window_title.c_str());
	gtk_window_set_resizable(GTK_WINDOW(app_state->window), FALSE);
	gtk_window_set_position(GTK_WINDOW(app_state->window),
				GTK_WIN_POS_CENTER_ALWAYS);

	gtk_container_set_border_width(GTK_CONTAINER(app_state->window), 12);
	g_signal_connect(G_OBJECT(app_state->window), "delete-event",
			 G_CALLBACK(on_window_closed), app_state);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(app_state->window), vbox);

	// set top label
	std::ostringstream label_ostr;
	label_ostr << "<big><b>"
		   << LLTrans::getString("UpdaterNowUpdating")
		   << "</b></big>";

	summary_label = gtk_label_new(NULL);
	gtk_label_set_use_markup(GTK_LABEL(summary_label), TRUE);
	gtk_label_set_markup(GTK_LABEL(summary_label),
			     label_ostr.str().c_str());
	gtk_misc_set_alignment(GTK_MISC(summary_label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), summary_label, FALSE, FALSE, 0);

	// create the description label
	description_label = gtk_label_new(LLTrans::getString("UpdaterUpdatingDescriptive").c_str());
	gtk_label_set_line_wrap(GTK_LABEL(description_label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(description_label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), description_label, FALSE, FALSE, 0);

	// If an image path has been set, load the background images
	if (!app_state->image_dir.empty()) {
		frame = gtk_frame_new(NULL);
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
		gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

		// load the first image
		app_state->image = gtk_image_new_from_file
			(next_image_filename(app_state->image_dir, *app_state->image_dir_iter).c_str());
		gtk_widget_set_size_request(app_state->image, 340, 310);
		gtk_container_add(GTK_CONTAINER(frame), app_state->image);

		// rotate the images every 5 seconds
		app_state->image_rotation_timeout_id = g_timeout_add
			(ROTATE_IMAGE_TIMEOUT, rotate_image_cb, app_state);
	}

	// set up progress bar, and update it roughly every 1/10 of a second
	app_state->progress_bar = gtk_progress_bar_new();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app_state->progress_bar),
				  LLTrans::getString("UpdaterProgressBarTextWithEllipses").c_str());
	gtk_box_pack_start(GTK_BOX(vbox),
			   app_state->progress_bar, FALSE, TRUE, 0);
	app_state->progress_update_timeout_id = g_timeout_add
		(UPDATE_PROGRESS_TIMEOUT, progress_update_timeout, app_state);
	app_state->update_progress_text_timeout_id = g_timeout_add
		(UPDATE_PROGRESS_TEXT_TIMEOUT, update_progress_text_timeout, app_state);

	gtk_widget_show_all(app_state->window);
}

gboolean rotate_image_cb(gpointer data)
{
	UpdaterAppState *app_state;
	std::string filename;

	llassert(data != NULL);
	app_state = (UpdaterAppState *) data;

	filename = next_image_filename(app_state->image_dir, *app_state->image_dir_iter);

	gdk_threads_enter();
	gtk_image_set_from_file(GTK_IMAGE(app_state->image), filename.c_str());
	gdk_threads_leave();

	return TRUE;
}

std::string next_image_filename(std::string& image_path, LLDirIterator& iter)
{
	std::string image_filename;
	iter.next(image_filename);
	return gDirUtilp->add(image_path, image_filename);
}

void on_window_closed(GtkWidget *sender, GdkEvent* event, gpointer data)
{
	UpdaterAppState *app_state;

	llassert(data != NULL);
	app_state = (UpdaterAppState *) data;

	updater_app_quit(app_state);
}

void updater_app_quit(UpdaterAppState *app_state)
{
	if (app_state != NULL)
	{
		g_source_remove(app_state->progress_update_timeout_id);

		if (!app_state->image_dir.empty())
		{
			g_source_remove(app_state->image_rotation_timeout_id);
		}
	}

	gtk_main_quit();
}

void display_error(GtkWidget *parent, std::string title, std::string message)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"%s", message.c_str());
	gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

gpointer worker_thread_cb(gpointer data)
{
	UpdaterAppState *app_state;
	CURL *curl;
	CURLcode result;
	FILE *package_file;
	GError *error = NULL;
	int fd;

	//g_return_val_if_fail (data != NULL, NULL);
	app_state = (UpdaterAppState *) data;

	try {

		if(!app_state->url.empty())
		{
			char* tmp_local_filename = NULL;
			// create temporary file to store the package.
			fd = g_file_open_tmp
				("secondlife-update-XXXXXX", &tmp_local_filename, &error);
			if (error != NULL)
			{
				llerrs << "Unable to create temporary file: "
					   << error->message
					   << llendl;

				g_error_free(error);
				throw 0;
			}

			if(tmp_local_filename != NULL)
			{
				app_state->file = tmp_local_filename;
				g_free(tmp_local_filename);
			}

			package_file = fdopen(fd, "wb");
			if (package_file == NULL)
			{
				llerrs << "Failed to create temporary file: "
					   << app_state->file.c_str()
					   << llendl;

				gdk_threads_enter();
				display_error(app_state->window,
							  LLTrans::getString("UpdaterFailDownloadTitle"),
							  LLTrans::getString("UpdaterFailUpdateDescriptive"));
				gdk_threads_leave();
				throw 0;
			}

			// initialize curl and start downloading the package
			llinfos << "Downloading package: " << app_state->url << llendl;

			curl = curl_easy_init();
			if (curl == NULL)
			{
				llerrs << "Failed to initialize libcurl" << llendl;

				gdk_threads_enter();
				display_error(app_state->window,
							  LLTrans::getString("UpdaterFailDownloadTitle"),
							  LLTrans::getString("UpdaterFailUpdateDescriptive"));
				gdk_threads_leave();
				throw 0;
			}

			curl_easy_setopt(curl, CURLOPT_URL, app_state->url.c_str());
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, TRUE);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, TRUE);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, package_file);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,
							 &download_progress_cb);
			curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, app_state);

			result = curl_easy_perform(curl);
			fclose(package_file);
			curl_easy_cleanup(curl);

			if (result)
			{
				llerrs << "Failed to download update: "
					   << app_state->url
					   << llendl;

				gdk_threads_enter();
				display_error(app_state->window,
							  LLTrans::getString("UpdaterFailDownloadTitle"),
							  LLTrans::getString("UpdaterFailUpdateDescriptive"));
				gdk_threads_leave();

				throw 0;
			}
		}

		// now pulse the progres bar back and forth while the package is
		// being unpacked
		gdk_threads_enter();
		std::string installing_msg = LLTrans::getString("UpdaterNowInstalling");
		gtk_progress_bar_set_text(
			GTK_PROGRESS_BAR(app_state->progress_bar),
			installing_msg.c_str());
		app_state->activity_mode = TRUE;
		gdk_threads_leave();

		// *TODO: if the destination is not writable, terminate this
		// thread and show file chooser?
		if (!install_package(app_state->file.c_str(), app_state->dest_dir))
		{
			llwarns << "Failed to install package to destination: "
				<< app_state->dest_dir
				<< llendl;

			gdk_threads_enter();
			display_error(app_state->window,
						  LLTrans::getString("UpdaterFailInstallTitle"),
						  LLTrans::getString("UpdaterFailUpdateDescriptive"));
			//"Failed to update " + app_state->app_name,
			gdk_threads_leave();
			throw 0;
		}

		// try to spawn the new viewer
		if (!spawn_viewer(app_state))
		{
			llwarns << "Viewer was not installed properly in : "
				<< app_state->dest_dir
				<< llendl;

			gdk_threads_enter();
			display_error(app_state->window,
						  LLTrans::getString("UpdaterFailStartTitle"),
						  LLTrans::getString("UpdaterFailUpdateDescriptive"));
			gdk_threads_leave();
			throw 0;
		}
	}
	catch (...)
	{
		app_state->failure = TRUE;
	}

	gdk_threads_enter();
	updater_app_quit(app_state);
	gdk_threads_leave();

	return NULL;
}


gboolean less_anal_gspawnsync(gchar **argv,
			      gchar **stderr_output,
			      gint *child_exit_status,
			      GError **spawn_error)
{
	// store current SIGCHLD handler if there is one, replace with default
	// handler to make glib happy
	struct sigaction sigchld_backup;
	struct sigaction sigchld_appease_glib;
	sigchld_appease_glib.sa_handler = SIG_DFL;
	sigemptyset(&sigchld_appease_glib.sa_mask);
	sigchld_appease_glib.sa_flags = 0;
	sigaction(SIGCHLD, &sigchld_appease_glib, &sigchld_backup);

	gboolean rtn = g_spawn_sync(NULL,
				    argv,
				    NULL,
				    (GSpawnFlags) (G_SPAWN_STDOUT_TO_DEV_NULL),
				    NULL,
				    NULL,
				    NULL,
				    stderr_output,
				    child_exit_status,
				    spawn_error);

	// restore SIGCHLD handler
	sigaction(SIGCHLD, &sigchld_backup, NULL);

	return rtn;
}


// perform a rename, or perform a (prompted) root rename if that fails
int
rename_with_sudo_fallback(const std::string& filename, const std::string& newname)
{
	int rtncode = ::rename(filename.c_str(), newname.c_str());
	lldebugs << "rename result is: " << rtncode << " / " << errno << llendl;
	if (rtncode && (EACCES == errno || EPERM == errno || EXDEV == errno))
	{
		llinfos << "Permission problem in rename, or moving between different mount points.  Retrying as a mv under a sudo." << llendl;
		// failed due to permissions, try again as a gksudo or kdesu mv wrapper hack
		char *sudo_cmd = NULL;
		sudo_cmd = g_find_program_in_path("gksudo");
		if (!sudo_cmd)
		{
			sudo_cmd = g_find_program_in_path("kdesu");
		}
		if (sudo_cmd)
		{
			char *mv_cmd = NULL;
			mv_cmd = g_find_program_in_path("mv");
			if (mv_cmd)
			{
				char *src_string_copy = g_strdup(filename.c_str());
				char *dst_string_copy = g_strdup(newname.c_str());
				char* argv[] =
					{
						sudo_cmd,
						mv_cmd,
						src_string_copy,
						dst_string_copy,
						NULL
					};

				gchar *stderr_output = NULL;
				gint child_exit_status = 0;
				GError *spawn_error = NULL;
				if (!less_anal_gspawnsync(argv, &stderr_output,
							  &child_exit_status, &spawn_error))
				{
					llwarns << "Failed to spawn child process: "
						<< spawn_error->message
						<< llendl;
				}
				else if (child_exit_status)
				{
					llwarns << "mv command failed: "
						<< (stderr_output ? stderr_output : "(no reason given)")
						<< llendl;
				}
				else
				{
					// everything looks good, clear the error code
					rtncode = 0;
				}

				g_free(src_string_copy);
				g_free(dst_string_copy);
				if (spawn_error) g_error_free(spawn_error);
			}
		}
	}
	return rtncode;
}

gboolean install_package(std::string package_file, std::string destination)
{
	char *tar_cmd = NULL;
	std::ostringstream command;

	// Find the absolute path to the 'tar' command.
	tar_cmd = g_find_program_in_path("tar");
	if (!tar_cmd)
	{
		llerrs << "`tar' was not found in $PATH" << llendl;
		return FALSE;
	}
	llinfos << "Found tar command: " << tar_cmd << llendl;

	// Unpack the tarball in a temporary place first, then move it to
	// its final destination
	std::string tmp_dest_dir = gDirUtilp->getTempFilename();
	if (LLFile::mkdir(tmp_dest_dir, 0744))
	{
		llerrs << "Failed to create directory: "
		       << destination
		       << llendl;

		return FALSE;
	}

	char *package_file_string_copy = g_strdup(package_file.c_str());
	char *tmp_dest_dir_string_copy = g_strdup(tmp_dest_dir.c_str());
	gchar *argv[8] = {
		tar_cmd,
		const_cast<gchar*>("--strip"), const_cast<gchar*>("1"),
		const_cast<gchar*>("-xjf"),
		package_file_string_copy,
		const_cast<gchar*>("-C"), tmp_dest_dir_string_copy,
		NULL,
	};

	llinfos << "Untarring package: " << package_file << llendl;

	// store current SIGCHLD handler if there is one, replace with default
	// handler to make glib happy
	struct sigaction sigchld_backup;
	struct sigaction sigchld_appease_glib;
	sigchld_appease_glib.sa_handler = SIG_DFL;
	sigemptyset(&sigchld_appease_glib.sa_mask);
	sigchld_appease_glib.sa_flags = 0;
	sigaction(SIGCHLD, &sigchld_appease_glib, &sigchld_backup);

	gchar *stderr_output = NULL;
	gint child_exit_status = 0;
	GError *untar_error = NULL;
	if (!less_anal_gspawnsync(argv, &stderr_output,
				  &child_exit_status, &untar_error))
	{
		llwarns << "Failed to spawn child process: "
			<< untar_error->message
			<< llendl;
		return FALSE;
	}

	if (child_exit_status)
	{
	 	llwarns << "Untar command failed: "
			<< (stderr_output ? stderr_output : "(no reason given)")
			<< llendl;
		return FALSE;
	}

	g_free(tar_cmd);
	g_free(package_file_string_copy);
	g_free(tmp_dest_dir_string_copy);
	g_free(stderr_output);
	if (untar_error) g_error_free(untar_error);

	// move the existing package out of the way if it exists
	if (gDirUtilp->fileExists(destination))
	{
		std::string backup_dir = destination + ".backup";
		int oldcounter = 1;
		while (gDirUtilp->fileExists(backup_dir))
		{
			// find a foo.backup.N folder name that isn't taken yet
			backup_dir = destination + ".backup." + llformat("%d", oldcounter);
			++oldcounter;
		}

		if (rename_with_sudo_fallback(destination, backup_dir))
		{
			llwarns << "Failed to move directory: '"
				<< destination << "' -> '" << backup_dir
				<< llendl;
			return FALSE;
		}
	}

	// The package has been unpacked in a staging directory, now we just
	// need to move it to its destination.
	if (rename_with_sudo_fallback(tmp_dest_dir, destination))
	{
		llwarns << "Failed to move installation to the destination: "
			<< destination
			<< llendl;
		return FALSE;
	}

	// \0/ Success!
	return TRUE;
}

gboolean progress_update_timeout(gpointer data)
{
	UpdaterAppState *app_state;

	llassert(data != NULL);

	app_state = (UpdaterAppState *) data;

	gdk_threads_enter();
	if (app_state->activity_mode)
	{
		gtk_progress_bar_pulse
			(GTK_PROGRESS_BAR(app_state->progress_bar));
	}
	else
	{
		gtk_progress_set_value(GTK_PROGRESS(app_state->progress_bar),
				       app_state->progress_value);
	}
	gdk_threads_leave();

	return TRUE;
}

gboolean update_progress_text_timeout(gpointer data)
{
	UpdaterAppState *app_state;

	llassert(data != NULL);
	app_state = (UpdaterAppState *) data;

	if (app_state->activity_mode == TRUE)
	{
		// We no longer need this timeout, it will be removed.
		return FALSE;
	}

	if (!app_state->progress_value)
	{
		return TRUE;
	}

	std::string progress_text = llformat((LLTrans::getString("UpdaterProgressBarText")+" (%.0f%%)").c_str(), app_state->progress_value);

	gdk_threads_enter();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app_state->progress_bar),
				  progress_text.c_str());
	gdk_threads_leave();

	return TRUE;
}

int download_progress_cb(gpointer data,
			 double t,
			 double d,
			 double utotal,
			 double ulnow)
{
	UpdaterAppState *app_state;

	llassert(data != NULL);
	app_state = (UpdaterAppState *) data;

	if (t <= 0.0)
	{
		app_state->progress_value = 0;
	}
	else
	{
		app_state->progress_value = d * 100.0 / t;
	}
	return 0;
}

BOOL spawn_viewer(UpdaterAppState *app_state)
{
	llassert(app_state != NULL);

	std::string cmd = app_state->dest_dir + "/secondlife";
	GError *error = NULL;

	// We want to spawn the Viewer on the same display as the updater app
	gboolean success = gdk_spawn_command_line_on_screen
		(gtk_widget_get_screen(app_state->window), cmd.c_str(), &error);

	if (!success)
	{
		llwarns << "Failed to launch viewer: " << error->message
			<< llendl;
	}

	if (error) g_error_free(error);

	return success;
}

void show_usage_and_exit()
{
	std::cout << "Usage: linux-updater <--url URL | --file FILE> --name NAME --dest PATH --stringsdir PATH1,PATH2 --stringsfile FILE"
		  << "[--image-dir PATH]"
		  << std::endl;
	exit(1);
}

void parse_args_and_init(int argc, char **argv, UpdaterAppState *app_state)
{
	int i;

	for (i = 1; i < argc; i++)
	{
		if ((!strcmp(argv[i], "--url")) && (++i < argc))
		{
			app_state->url = argv[i];
		}
		else if ((!strcmp(argv[i], "--file")) && (++i < argc))
		{
			app_state->file = argv[i];
		}
		else if ((!strcmp(argv[i], "--name")) && (++i < argc))
		{
			app_state->app_name = argv[i];
		}
		else if ((!strcmp(argv[i], "--image-dir")) && (++i < argc))
		{
			app_state->image_dir = argv[i];
			app_state->image_dir_iter = new LLDirIterator(argv[i], "*.jpg");
		}
		else if ((!strcmp(argv[i], "--dest")) && (++i < argc))
		{
			app_state->dest_dir = argv[i];
		}
		else if ((!strcmp(argv[i], "--stringsdir")) && (++i < argc))
		{
			app_state->strings_dirs = argv[i];
		}
		else if ((!strcmp(argv[i], "--stringsfile")) && (++i < argc))
		{
			app_state->strings_file = argv[i];
		}
		else
		{
			// show usage, an invalid option was given.
			show_usage_and_exit();
		}
	}

	if (app_state->app_name.empty()
	    || (app_state->url.empty() && app_state->file.empty())
	    || app_state->dest_dir.empty())
	{
		show_usage_and_exit();
	}

	app_state->progress_value = 0.0;
	app_state->activity_mode = FALSE;
	app_state->failure = FALSE;

	translate_init(app_state->strings_dirs, app_state->strings_file);
}

int main(int argc, char **argv)
{
	UpdaterAppState* app_state = new UpdaterAppState;
	GThread *worker_thread;

	parse_args_and_init(argc, argv, app_state);

	// Initialize logger, and rename old log file
	gDirUtilp->initAppDirs("SecondLife");
	LLError::initForApplication
		(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));
	std::string old_log_file = gDirUtilp->getExpandedFilename
		(LL_PATH_LOGS, "updater.log.old");
	std::string log_file =
		gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "updater.log");
	LLFile::rename(log_file, old_log_file);
	LLError::logToFile(log_file);

	// initialize gthreads and gtk+
	if (!g_thread_supported())
	{
		g_thread_init(NULL);
		gdk_threads_init();
	}

	gtk_init(&argc, &argv);

	// create UI
	updater_app_ui_init(app_state);

	//llinfos << "SAMPLE TRANSLATION IS: " << LLTrans::getString("LoginInProgress") << llendl;

	// create download thread
	worker_thread = g_thread_create
		(GThreadFunc(worker_thread_cb), app_state, FALSE, NULL);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	// Delete the file only if created from url download.
	if(!app_state->url.empty() && !app_state->file.empty())
	{
		if (gDirUtilp->fileExists(app_state->file))
		{
			LLFile::remove(app_state->file);
		}
	}

	bool success = !app_state->failure;
	delete app_state->image_dir_iter;
	delete app_state;
	return success ? 0 : 1;
}

/*****************************************************************************
*   Dummy LLTrans implementation (IQA-490)
*****************************************************************************/
static LLTrans sStaticStrings;

// lookup
std::string LLTrans::_getString(const std::string& key) const
{
	MessageMap::const_iterator found = mMessages.find(key);
	if (found != mMessages.end())
	{
		return found->second;
	}
	LL_WARNS("linux_updater") << "No message for key '" << key
							  << "' -- add to LLTrans::LLTrans() in linux_updater.cpp"
							  << LL_ENDL;
	return key;
}

// static lookup
std::string LLTrans::getString(const std::string& key)
{
    return sLLTransInstance._getString(key);
}

// initialization
LLTrans::LLTrans()
{
	typedef std::pair<const char*, const char*> Pair;
	static const Pair data[] =
	{
		Pair("UpdaterFailDownloadTitle",
			 "Failed to download update"),
		Pair("UpdaterFailInstallTitle",
			 "Failed to install update"),
		Pair("UpdaterFailStartTitle",
			 "Failed to start viewer"),
		Pair("UpdaterFailUpdateDescriptive",
			 "An error occurred while updating Second Life. "
			 "Please download the latest version from www.secondlife.com."),
		Pair("UpdaterNowInstalling",
			 "Installing Second Life..."),
		Pair("UpdaterNowUpdating",
			 "Now updating Second Life..."),
		Pair("UpdaterProgressBarText",
			 "Downloading update"),
		Pair("UpdaterProgressBarTextWithEllipses",
			 "Downloading update..."),
		Pair("UpdaterUpdatingDescriptive",
			 "Your Second Life Viewer is being updated to the latest release. "
			 "This may take some time, so please be patient."),
		Pair("UpdaterWindowTitle",
			 "Second Life Update")
	};

	BOOST_FOREACH(Pair pair, data)
	{
		mMessages[pair.first] = pair.second;
	}
}
