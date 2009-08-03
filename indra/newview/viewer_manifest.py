#!/usr/bin/python
# @file viewer_manifest.py
# @author Ryan Williams
# @brief Description of all installer viewer files, and methods for packaging
#        them into installers for all supported platforms.
#
# $LicenseInfo:firstyear=2006&license=viewergpl$
# 
# Copyright (c) 2006-2009, Linden Research, Inc.
# 
# Second Life Viewer Source Code
# The source code in this file ("Source Code") is provided by Linden Lab
# to you under the terms of the GNU General Public License, version 2.0
# ("GPL"), unless you have obtained a separate licensing agreement
# ("Other License"), formally executed by you and Linden Lab.  Terms of
# the GPL can be found in doc/GPL-license.txt in this distribution, or
# online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
# 
# There are special exceptions to the terms and conditions of the GPL as
# it is applied to this Source Code. View the full text of the exception
# in the file doc/FLOSS-exception.txt in this software distribution, or
# online at
# http://secondlifegrid.net/programs/open_source/licensing/flossexception
# 
# By copying, modifying or distributing this software, you acknowledge
# that you have read and understood your obligations described above,
# and agree to abide by those obligations.
# 
# ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
# WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
# COMPLETENESS OR PERFORMANCE.
# $/LicenseInfo$
import sys
import os.path
import re
import tarfile
viewer_dir = os.path.dirname(__file__)
# add llmanifest library to our path so we don't have to muck with PYTHONPATH
sys.path.append(os.path.join(viewer_dir, '../lib/python/indra/util'))
from llmanifest import LLManifest, main, proper_windows_path, path_ancestors

class ViewerManifest(LLManifest):
    def construct(self):
        super(ViewerManifest, self).construct()
        self.exclude("*.svn*")
        self.path(src="../../scripts/messages/message_template.msg", dst="app_settings/message_template.msg")
        self.path(src="../../etc/message.xml", dst="app_settings/message.xml")

        if self.prefix(src="app_settings"):
            self.exclude("logcontrol.xml")
            self.exclude("logcontrol-dev.xml")
            self.path("*.pem")
            self.path("*.ini")
            self.path("*.xml")
            self.path("*.db2")

            # include the entire shaders directory recursively
            self.path("shaders")
            # ... and the entire windlight directory
            self.path("windlight")
            self.end_prefix("app_settings")

        if self.prefix(src="character"):
            self.path("*.llm")
            self.path("*.xml")
            self.path("*.tga")
            self.end_prefix("character")

        # Include our fonts
        if self.prefix(src="fonts"):
            self.path("*.ttf")
            self.path("*.txt")
            self.end_prefix("fonts")

        # skins
        if self.prefix(src="skins"):
                self.path("paths.xml")
                # include the entire textures directory recursively
                if self.prefix(src="*/textures"):
                        self.path("*/*.tga")
                        self.path("*/*.j2c")
                        self.path("*/*.jpg")
                        self.path("*/*.png")
                        self.path("*.tga")
                        self.path("*.j2c")
                        self.path("*.jpg")
                        self.path("*.png")
                        self.path("textures.xml")
                        self.end_prefix("*/textures")
                self.path("*/xui/*/*.xml")
                self.path("*/xui/*/widgets/*.xml")
                self.path("*/*.xml")
                
                # Local HTML files (e.g. loading screen)
                if self.prefix(src="*/html"):
                        self.path("*.png")
                        self.path("*/*/*.html")
                        self.path("*/*/*.gif")
                        self.end_prefix("*/html")
                self.end_prefix("skins")
        
        # Files in the newview/ directory
        self.path("gpu_table.txt")

    def login_channel(self):
        """Channel reported for login and upgrade purposes ONLY;
        used for A/B testing"""
        # NOTE: Do not return the normal channel if login_channel
        # is not specified, as some code may branch depending on
        # whether or not this is present
        return self.args.get('login_channel')

    def grid(self):
        return self.args['grid']
    def channel(self):
        return self.args['channel']
    def channel_unique(self):
        return self.channel().replace("Second Life", "").strip()
    def channel_oneword(self):
        return "".join(self.channel_unique().split())
    def channel_lowerword(self):
        return self.channel_oneword().lower()

    def flags_list(self):
        """ Convenience function that returns the command-line flags
        for the grid"""

        # Set command line flags relating to the target grid
        grid_flags = ''
        if not self.default_grid():
            grid_flags = "--grid %(grid)s "\
                         "--helperuri http://preview-%(grid)s.secondlife.com/helpers/" %\
                           {'grid':self.grid()}

        # set command line flags for channel
        channel_flags = ''
        if self.login_channel() and self.login_channel() != self.channel():
            # Report a special channel during login, but use default
            channel_flags = '--channel "%s"' % (self.login_channel())
        elif not self.default_channel():
            channel_flags = '--channel "%s"' % self.channel()

        # Deal with settings 
        setting_flags = ''
        if not self.default_channel() or not self.default_grid():
            if self.default_grid():
                setting_flags = '--settings settings_%s.xml'\
                                % self.channel_lowerword()
            else:
                setting_flags = '--settings settings_%s_%s.xml'\
                                % (self.grid(), self.channel_lowerword())
                                                
        return " ".join((channel_flags, grid_flags, setting_flags)).strip()


class WindowsManifest(ViewerManifest):
    def final_exe(self):
        if self.default_channel():
            if self.default_grid():
                return "SecondLife.exe"
            else:
                return "SecondLifePreview.exe"
        else:
            return ''.join(self.channel().split()) + '.exe'


    def construct(self):
        super(WindowsManifest, self).construct()
        # Find secondlife-bin.exe in the 'configuration' dir, then rename it to the result of final_exe.
        self.path(src='%s/secondlife-bin.exe' % self.args['configuration'], dst=self.final_exe())

        # need to get the llcommon.dll from the build directory as well
        if self.prefix(src=self.args['configuration'], dst=""):
            try:
                self.path('llcommon.dll')
                self.path('libapr-1.dll')
                self.path('libaprutil-1.dll')
                self.path('libapriconv-1.dll')
            except:
                print "Skipping llcommon.dll (assuming llcommon was linked statically)"
                pass
        self.end_prefix()

        # need to get the kdu dll from the build directory as well
        try:
            self.path('%s/llkdu.dll' % self.args['configuration'], dst='llkdu.dll')
            pass
        except:
            print "Skipping llkdu.dll"
            pass
        self.path(src="licenses-win32.txt", dst="licenses.txt")

        self.path("featuretable.txt")

        # For use in crash reporting (generates minidumps)
        self.path("dbghelp.dll")

        # For using FMOD for sound... DJS
        self.path("fmod.dll")

        # For textures
        if self.prefix(src=self.args['configuration'], dst=""):
            if(self.args['configuration'].lower() == 'debug'):
                self.path("openjpegd.dll")
            else:
                self.path("openjpeg.dll")
            self.end_prefix()

        # Mozilla appears to force a dependency on these files so we need to ship it (CP) - updated to vc8 versions (nyx)
        # These need to be installed as a SxS assembly, currently a 'private' assembly.
        # See http://msdn.microsoft.com/en-us/library/ms235291(VS.80).aspx
        if self.prefix(src=self.args['configuration'], dst=""):
            if self.args['configuration'] == 'Debug':
                self.path("msvcr80d.dll")
                self.path("msvcp80d.dll")
                self.path("Microsoft.VC80.DebugCRT.manifest")
            else:
                self.path("msvcr80.dll")
                self.path("msvcp80.dll")
                self.path("Microsoft.VC80.CRT.manifest")
            self.end_prefix()

        # Mozilla runtime DLLs (CP)
        if self.prefix(src=self.args['configuration'], dst=""):
            self.path("freebl3.dll")
            self.path("js3250.dll")
            self.path("nspr4.dll")
            self.path("nss3.dll")
            self.path("nssckbi.dll")
            self.path("plc4.dll")
            self.path("plds4.dll")
            self.path("smime3.dll")
            self.path("softokn3.dll")
            self.path("ssl3.dll")
            self.path("xpcom.dll")
            self.path("xul.dll")
            self.end_prefix()

        # Mozilla runtime misc files (CP)
        if self.prefix(src="app_settings/mozilla"):
            self.path("chrome/*.*")
            self.path("components/*.*")
            self.path("greprefs/*.*")
            self.path("plugins/*.*")
            self.path("res/*.*")
            self.path("res/*/*")
            self.end_prefix()

        # Mozilla hack to get it to accept newer versions of msvc*80.dll than are listed in manifest
        # necessary as llmozlib2-vc80.lib refers to an old version of msvc*80.dll - can be removed when new version of llmozlib is built - Nyx
        # The config file name needs to match the exe's name.
        self.path("SecondLife.exe.config", dst=self.final_exe() + ".config")

        # Vivox runtimes
        if self.prefix(src=self.args['configuration'], dst=""):
            self.path("SLVoice.exe")
            self.path("alut.dll")
            self.path("vivoxsdk.dll")
            self.path("ortp.dll")
            self.path("wrap_oal.dll")
            self.end_prefix()

        # pull in the crash logger and updater from other projects
        # tag:"crash-logger" here as a cue to the exporter
        self.path(src='../win_crash_logger/%s/windows-crash-logger.exe' % self.args['configuration'],
                  dst="win_crash_logger.exe")
        self.path(src='../win_updater/%s/windows-updater.exe' % self.args['configuration'],
                  dst="updater.exe")

        # For google-perftools tcmalloc allocator.
        try:
            self.path('%s/libtcmalloc_minimal.dll' % self.args['configuration'])
        except:
            print "Skipping libtcmalloc_minimal.dll"
            pass           

    def nsi_file_commands(self, install=True):
        def wpath(path):
            if path.endswith('/') or path.endswith(os.path.sep):
                path = path[:-1]
            path = path.replace('/', '\\')
            return path

        result = ""
        dest_files = [pair[1] for pair in self.file_list if pair[0] and os.path.isfile(pair[1])]
        # sort deepest hierarchy first
        dest_files.sort(lambda a,b: cmp(a.count(os.path.sep),b.count(os.path.sep)) or cmp(a,b))
        dest_files.reverse()
        out_path = None
        for pkg_file in dest_files:
            rel_file = os.path.normpath(pkg_file.replace(self.get_dst_prefix()+os.path.sep,''))
            installed_dir = wpath(os.path.join('$INSTDIR', os.path.dirname(rel_file)))
            pkg_file = wpath(os.path.normpath(pkg_file))
            if installed_dir != out_path:
                if install:
                    out_path = installed_dir
                    result += 'SetOutPath ' + out_path + '\n'
            if install:
                result += 'File ' + pkg_file + '\n'
            else:
                result += 'Delete ' + wpath(os.path.join('$INSTDIR', rel_file)) + '\n'
        # at the end of a delete, just rmdir all the directories
        if not install:
            deleted_file_dirs = [os.path.dirname(pair[1].replace(self.get_dst_prefix()+os.path.sep,'')) for pair in self.file_list]
            # find all ancestors so that we don't skip any dirs that happened to have no non-dir children
            deleted_dirs = []
            for d in deleted_file_dirs:
                deleted_dirs.extend(path_ancestors(d))
            # sort deepest hierarchy first
            deleted_dirs.sort(lambda a,b: cmp(a.count(os.path.sep),b.count(os.path.sep)) or cmp(a,b))
            deleted_dirs.reverse()
            prev = None
            for d in deleted_dirs:
                if d != prev:   # skip duplicates
                    result += 'RMDir ' + wpath(os.path.join('$INSTDIR', os.path.normpath(d))) + '\n'
                prev = d

        return result

    def package_finish(self):
        # a standard map of strings for replacing in the templates
        substitution_strings = {
            'version' : '.'.join(self.args['version']),
            'version_short' : '.'.join(self.args['version'][:-1]),
            'version_dashes' : '-'.join(self.args['version']),
            'final_exe' : self.final_exe(),
            'grid':self.args['grid'],
            'grid_caps':self.args['grid'].upper(),
            # escape quotes becase NSIS doesn't handle them well
            'flags':self.flags_list().replace('"', '$\\"'),
            'channel':self.channel(),
            'channel_oneword':self.channel_oneword(),
            'channel_unique':self.channel_unique(),
            }

        version_vars = """
        !define INSTEXE  "%(final_exe)s"
        !define VERSION "%(version_short)s"
        !define VERSION_LONG "%(version)s"
        !define VERSION_DASHES "%(version_dashes)s"
        """ % substitution_strings
        if self.default_channel():
            if self.default_grid():
                # release viewer
                installer_file = "Second_Life_%(version_dashes)s_Setup.exe"
                grid_vars_template = """
                OutFile "%(installer_file)s"
                !define INSTFLAGS "%(flags)s"
                !define INSTNAME   "SecondLife"
                !define SHORTCUT   "Second Life"
                !define URLNAME   "secondlife"
                Caption "Second Life ${VERSION}"
                """
            else:
                # beta grid viewer
                installer_file = "Second_Life_%(version_dashes)s_(%(grid_caps)s)_Setup.exe"
                grid_vars_template = """
                OutFile "%(installer_file)s"
                !define INSTFLAGS "%(flags)s"
                !define INSTNAME   "SecondLife%(grid_caps)s"
                !define SHORTCUT   "Second Life (%(grid_caps)s)"
                !define URLNAME   "secondlife%(grid)s"
                !define UNINSTALL_SETTINGS 1
                Caption "Second Life %(grid)s ${VERSION}"
                """
        else:
            # some other channel on some grid
            installer_file = "Second_Life_%(version_dashes)s_%(channel_oneword)s_Setup.exe"
            grid_vars_template = """
            OutFile "%(installer_file)s"
            !define INSTFLAGS "%(flags)s"
            !define INSTNAME   "SecondLife%(channel_oneword)s"
            !define SHORTCUT   "%(channel)s"
            !define URLNAME   "secondlife"
            !define UNINSTALL_SETTINGS 1
            Caption "%(channel)s ${VERSION}"
            """
        if 'installer_name' in self.args:
            installer_file = self.args['installer_name']
        else:
            installer_file = installer_file % substitution_strings
        substitution_strings['installer_file'] = installer_file

        tempfile = "secondlife_setup_tmp.nsi"
        # the following replaces strings in the nsi template
        # it also does python-style % substitution
        self.replace_in("installers/windows/installer_template.nsi", tempfile, {
                "%%VERSION%%":version_vars,
                "%%SOURCE%%":self.get_src_prefix(),
                "%%GRID_VARS%%":grid_vars_template % substitution_strings,
                "%%INSTALL_FILES%%":self.nsi_file_commands(True),
                "%%DELETE_FILES%%":self.nsi_file_commands(False)})

        # We use the Unicode version of NSIS, available from
        # http://www.scratchpaper.com/
        # Check two paths, one for Program Files, and one for Program Files (x86).
        # Yay 64bit windows.
        NSIS_path = os.path.expandvars('${ProgramFiles}\\NSIS\\Unicode\\makensis.exe')
        if not os.path.exists(NSIS_path):
            NSIS_path = os.path.expandvars('${ProgramFiles(x86)}\\NSIS\\Unicode\\makensis.exe')
        self.run_command('"' + proper_windows_path(NSIS_path) + '" ' + self.dst_path_of(tempfile))
        # self.remove(self.dst_path_of(tempfile))
        # If we're on a build machine, sign the code using our Authenticode certificate. JC
        sign_py = 'C:\\buildscripts\\code-signing\\sign.py'
        if os.path.exists(sign_py):
            self.run_command(sign_py + ' ' + self.dst_path_of(installer_file))
        else:
            print "Skipping code signing,", sign_py, "does not exist"
        self.created_path(self.dst_path_of(installer_file))
        self.package_file = installer_file


class DarwinManifest(ViewerManifest):
    def construct(self):
        # copy over the build result (this is a no-op if run within the xcode script)
        self.path(self.args['configuration'] + "/Second Life.app", dst="")

        if self.prefix(src="", dst="Contents"):  # everything goes in Contents
            # Expand the tar file containing the assorted mozilla bits into
            #  <bundle>/Contents/MacOS/
            self.contents_of_tar(self.args['source']+'/mozilla-universal-darwin.tgz', 'MacOS')

            self.path("Info-SecondLife.plist", dst="Info.plist")

            # copy additional libs in <bundle>/Contents/MacOS/
            self.path("../../libraries/universal-darwin/lib_release/libndofdev.dylib", dst="MacOS/libndofdev.dylib")

            # replace the default theme with our custom theme (so scrollbars work).
            if self.prefix(src="mozilla-theme", dst="MacOS/chrome"):
                self.path("classic.jar")
                self.path("classic.manifest")
                self.end_prefix("MacOS/chrome")

            # most everything goes in the Resources directory
            if self.prefix(src="", dst="Resources"):
                super(DarwinManifest, self).construct()

                if self.prefix("cursors_mac"):
                    self.path("*.tif")
                    self.end_prefix("cursors_mac")

                self.path("licenses-mac.txt", dst="licenses.txt")
                self.path("featuretable_mac.txt")
                self.path("SecondLife.nib")

                # If we are not using the default channel, use the 'Firstlook
                # icon' to show that it isn't a stable release.
                if self.default_channel() and self.default_grid():
                    self.path("secondlife.icns")
                else:
                    self.path("secondlife_firstlook.icns", "secondlife.icns")
                self.path("SecondLife.nib")
                
                # Translations
                self.path("English.lproj")
                self.path("German.lproj")
                self.path("Japanese.lproj")
                self.path("Korean.lproj")
                self.path("da.lproj")
                self.path("es.lproj")
                self.path("fr.lproj")
                self.path("hu.lproj")
                self.path("it.lproj")
                self.path("nl.lproj")
                self.path("pl.lproj")
                self.path("pt.lproj")
                self.path("ru.lproj")
                self.path("tr.lproj")
                self.path("uk.lproj")
                self.path("zh-Hans.lproj")

                # SLVoice and vivox lols
                self.path("vivox-runtime/universal-darwin/libalut.dylib", "libalut.dylib")
                self.path("vivox-runtime/universal-darwin/libopenal.dylib", "libopenal.dylib")
                self.path("vivox-runtime/universal-darwin/libortp.dylib", "libortp.dylib")
                self.path("vivox-runtime/universal-darwin/libvivoxsdk.dylib", "libvivoxsdk.dylib")
                self.path("vivox-runtime/universal-darwin/SLVoice", "SLVoice")

                libdir = "../../libraries/universal-darwin/lib_release"
                dylibs = {}

                # need to get the kdu dll from any of the build directories as well
                for lib in "llkdu", "llcommon":
                    libfile = "lib%s.dylib" % lib
                    try:
                        self.path(self.find_existing_file('../%s/%s/%s' %
                                                          (lib, self.args['configuration'], libfile),
                                                          os.path.join(libdir, libfile)),
                                  dst=libfile)
                    except RuntimeError:
                        print "Skipping %s" % libfile
                        dylibs[lib] = False
                    else:
                        dylibs[lib] = True

                if dylibs["llcommon"]:
                    for libfile in ("libapr-1.0.3.7.dylib", "libaprutil-1.0.3.8.dylib"):
                        self.path(os.path.join(libdir, libfile), libfile)

                #libfmodwrapper.dylib
                self.path(self.args['configuration'] + "/libfmodwrapper.dylib", "libfmodwrapper.dylib")
                
                # our apps
                self.path("../mac_crash_logger/" + self.args['configuration'] + "/mac-crash-logger.app", "mac-crash-logger.app")
                self.path("../mac_updater/" + self.args['configuration'] + "/mac-updater.app", "mac-updater.app")

                # command line arguments for connecting to the proper grid
                self.put_in_file(self.flags_list(), 'arguments.txt')

                self.end_prefix("Resources")

            self.end_prefix("Contents")

        # NOTE: the -S argument to strip causes it to keep enough info for
        # annotated backtraces (i.e. function names in the crash log).  'strip' with no
        # arguments yields a slightly smaller binary but makes crash logs mostly useless.
        # This may be desirable for the final release.  Or not.
        if ("package" in self.args['actions'] or 
            "unpacked" in self.args['actions']):
            self.run_command('strip -S "%(viewer_binary)s"' %
                             { 'viewer_binary' : self.dst_path_of('Contents/MacOS/Second Life')})


    def package_finish(self):
        channel_standin = 'Second Life'  # hah, our default channel is not usable on its own
        if not self.default_channel():
            channel_standin = self.channel()

        imagename="SecondLife_" + '_'.join(self.args['version'])

        # MBW -- If the mounted volume name changes, it breaks the .DS_Store's background image and icon positioning.
        #  If we really need differently named volumes, we'll need to create multiple DS_Store file images, or use some other trick.

        volname="Second Life Installer"  # DO NOT CHANGE without understanding comment above

        if self.default_channel():
            if not self.default_grid():
                # beta case
                imagename = imagename + '_' + self.args['grid'].upper()
        else:
            # first look, etc
            imagename = imagename + '_' + self.channel_oneword().upper()

        sparsename = imagename + ".sparseimage"
        finalname = imagename + ".dmg"
        # make sure we don't have stale files laying about
        self.remove(sparsename, finalname)

        self.run_command('hdiutil create "%(sparse)s" -volname "%(vol)s" -fs HFS+ -type SPARSE -megabytes 300 -layout SPUD' % {
                'sparse':sparsename,
                'vol':volname})

        # mount the image and get the name of the mount point and device node
        hdi_output = self.run_command('hdiutil attach -private "' + sparsename + '"')
        devfile = re.search("/dev/disk([0-9]+)[^s]", hdi_output).group(0).strip()
        volpath = re.search('HFS\s+(.+)', hdi_output).group(1).strip()

        # Copy everything in to the mounted .dmg

        if self.default_channel() and not self.default_grid():
            app_name = "Second Life " + self.args['grid']
        else:
            app_name = channel_standin.strip()

        # Hack:
        # Because there is no easy way to coerce the Finder into positioning
        # the app bundle in the same place with different app names, we are
        # adding multiple .DS_Store files to svn. There is one for release,
        # one for release candidate and one for first look. Any other channels
        # will use the release .DS_Store, and will look broken.
        # - Ambroff 2008-08-20
        dmg_template = os.path.join(
            'installers', 
            'darwin',
            '%s-dmg' % "".join(self.channel_unique().split()).lower())

        if not os.path.exists (self.src_path_of(dmg_template)):
            dmg_template = os.path.join ('installers', 'darwin', 'release-dmg')

        for s,d in {self.get_dst_prefix():app_name + ".app",
                    os.path.join(dmg_template, "_VolumeIcon.icns"): ".VolumeIcon.icns",
                    os.path.join(dmg_template, "background.jpg"): "background.jpg",
                    os.path.join(dmg_template, "_DS_Store"): ".DS_Store"}.items():
            print "Copying to dmg", s, d
            self.copy_action(self.src_path_of(s), os.path.join(volpath, d))

        # Hide the background image, DS_Store file, and volume icon file (set their "visible" bit)
        self.run_command('SetFile -a V "' + os.path.join(volpath, ".VolumeIcon.icns") + '"')
        self.run_command('SetFile -a V "' + os.path.join(volpath, "background.jpg") + '"')
        self.run_command('SetFile -a V "' + os.path.join(volpath, ".DS_Store") + '"')

        # Create the alias file (which is a resource file) from the .r
        self.run_command('rez "' + self.src_path_of("installers/darwin/release-dmg/Applications-alias.r") + '" -o "' + os.path.join(volpath, "Applications") + '"')

        # Set the alias file's alias and custom icon bits
        self.run_command('SetFile -a AC "' + os.path.join(volpath, "Applications") + '"')

        # Set the disk image root's custom icon bit
        self.run_command('SetFile -a C "' + volpath + '"')

        # Unmount the image
        self.run_command('hdiutil detach -force "' + devfile + '"')

        print "Converting temp disk image to final disk image"
        self.run_command('hdiutil convert "%(sparse)s" -format UDZO -imagekey zlib-level=9 -o "%(final)s"' % {'sparse':sparsename, 'final':finalname})
        # get rid of the temp file
        self.package_file = finalname
        self.remove(sparsename)

class LinuxManifest(ViewerManifest):
    def construct(self):
        super(LinuxManifest, self).construct()
        self.path("licenses-linux.txt","licenses.txt")
        self.path("res/ll_icon.png","secondlife_icon.png")
        if self.prefix("linux_tools", dst=""):
            self.path("client-readme.txt","README-linux.txt")
            self.path("client-readme-voice.txt","README-linux-voice.txt")
            self.path("client-readme-joystick.txt","README-linux-joystick.txt")
            self.path("wrapper.sh","secondlife")
            self.path("handle_secondlifeprotocol.sh")
            self.path("register_secondlifeprotocol.sh")
            self.end_prefix("linux_tools")

        # Create an appropriate gridargs.dat for this package, denoting required grid.
        self.put_in_file(self.flags_list(), 'gridargs.dat')


    def package_finish(self):
        if 'installer_name' in self.args:
            installer_name = self.args['installer_name']
        else:
            installer_name_components = ['SecondLife_', self.args.get('arch')]
            installer_name_components.extend(self.args['version'])
            installer_name = "_".join(installer_name_components)
            if self.default_channel():
                if not self.default_grid():
                    installer_name += '_' + self.args['grid'].upper()
            else:
                installer_name += '_' + self.channel_oneword().upper()

        # Fix access permissions
        self.run_command("""
                find %(dst)s -type d | xargs --no-run-if-empty chmod 755;
                find %(dst)s -type f -perm 0700 | xargs --no-run-if-empty chmod 0755;
                find %(dst)s -type f -perm 0500 | xargs --no-run-if-empty chmod 0555;
                find %(dst)s -type f -perm 0600 | xargs --no-run-if-empty chmod 0644;
                find %(dst)s -type f -perm 0400 | xargs --no-run-if-empty chmod 0444;
                true""" %  {'dst':self.get_dst_prefix() })
        self.package_file = installer_name + '.tar.bz2'

        # temporarily move directory tree so that it has the right
        # name in the tarfile
        self.run_command("mv %(dst)s %(inst)s" % {
            'dst': self.get_dst_prefix(),
            'inst': self.build_path_of(installer_name)})
        try:
            # --numeric-owner hides the username of the builder for
            # security etc.
            self.run_command('tar -C %(dir)s --numeric-owner -cjf '
                             '%(inst_path)s.tar.bz2 %(inst_name)s' % {
                'dir': self.get_build_prefix(),
                'inst_name': installer_name,
                'inst_path':self.build_path_of(installer_name)})
        finally:
            self.run_command("mv %(inst)s %(dst)s" % {
                'dst': self.get_dst_prefix(),
                'inst': self.build_path_of(installer_name)})

class Linux_i686Manifest(LinuxManifest):
    def construct(self):
        super(Linux_i686Manifest, self).construct()

        # install either the libllkdu we just built, or a prebuilt one, in
        # decreasing order of preference.  for linux package, this goes to bin/
        try:
            self.path(self.find_existing_file('../llkdu/libllkdu.so',
                '../../libraries/i686-linux/lib_release_client/libllkdu.so'), 
                  dst='bin/libllkdu.so')
            # keep this one to preserve syntax, open source mangling removes previous lines
            pass
        except:
            print "Skipping libllkdu.so - not found"
            pass

        self.path("secondlife-stripped","bin/do-not-directly-run-secondlife-bin")
        self.path("../linux_crash_logger/linux-crash-logger-stripped","linux-crash-logger.bin")
        self.path("linux_tools/launch_url.sh","launch_url.sh")
        if self.prefix("res-sdl"):
            self.path("*")
            # recurse
            self.end_prefix("res-sdl")

        self.path("featuretable_linux.txt")
        #self.path("secondlife-i686.supp")

        self.path("app_settings/mozilla-runtime-linux-i686")

        if self.prefix("../../libraries/i686-linux/lib_release_client", dst="lib"):
            #self.path("libkdu_v42R.so", "libkdu.so")
            self.path("libfmod-3.75.so")
            self.path("libapr-1.so.0")
            self.path("libaprutil-1.so.0")
            self.path("libdb-4.2.so")
            self.path("libcrypto.so.0.9.7")
            self.path("libexpat.so.1")
            self.path("libssl.so.0.9.7")
            self.path("libuuid.so", "libuuid.so.1")
            self.path("libSDL-1.2.so.0")
            self.path("libELFIO.so")
            self.path("libopenjpeg.so.1.3.0", "libopenjpeg.so.1.3")
            self.path("libalut.so")
            self.path("libopenal.so", "libopenal.so.1")
            self.end_prefix("lib")

            # Vivox runtimes
            if self.prefix(src="vivox-runtime/i686-linux", dst="bin"):
                    self.path("SLVoice")
                    self.end_prefix()
            if self.prefix(src="vivox-runtime/i686-linux", dst="lib"):
                    self.path("libortp.so")
                    self.path("libvivoxsdk.so")
                    self.end_prefix("lib")

class Linux_x86_64Manifest(LinuxManifest):
    def construct(self):
        super(Linux_x86_64Manifest, self).construct()
        self.path("secondlife-stripped","bin/do-not-directly-run-secondlife-bin")
        self.path("../linux_crash_logger/linux-crash-logger-stripped","linux-crash-logger.bin")
        self.path("linux_tools/launch_url.sh","launch_url.sh")
        if self.prefix("res-sdl"):
            self.path("*")
            # recurse
            self.end_prefix("res-sdl")

        self.path("featuretable_linux.txt")
        self.path("secondlife-i686.supp")

if __name__ == "__main__":
    main()
