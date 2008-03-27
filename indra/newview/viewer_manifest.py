#!/usr/bin/python
# @file viewer_manifest.py
# @author Ryan Williams
# @brief Description of all installer viewer files, and methods for packaging
#        them into installers for all supported platforms.
#
# $LicenseInfo:firstyear=2006&license=viewergpl$
# 
# Copyright (c) 2006-2007, Linden Research, Inc.
# 
# Second Life Viewer Source Code
# The source code in this file ("Source Code") is provided by Linden Lab
# to you under the terms of the GNU General Public License, version 2.0
# ("GPL"), unless you have obtained a separate licensing agreement
# ("Other License"), formally executed by you and Linden Lab.  Terms of
# the GPL can be found in doc/GPL-license.txt in this distribution, or
# online at http://secondlife.com/developers/opensource/gplv2
# 
# There are special exceptions to the terms and conditions of the GPL as
# it is applied to this Source Code. View the full text of the exception
# in the file doc/FLOSS-exception.txt in this software distribution, or
# online at http://secondlife.com/developers/opensource/flossexception
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
                        self.path("*.vp")
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

                # XUI
                if self.prefix(src="skins"):
                        # include the entire textures directory recursively
                        self.path("textures")
                        self.path("paths.xml")
                        self.path("xui/*/*.xml")
                        self.path('words.*.txt')

                        # Local HTML files (e.g. loading screen)
                        if self.prefix(src="html"):
                                self.path("*.png")
                                self.path("*/*/*.html")
                                self.path("*/*/*.gif")
                                self.end_prefix("html")
                        self.end_prefix("skins")

                self.path("releasenotes.txt")
                self.path("lsl_guide.html")
                self.path("gpu_table.txt")

        def login_channel(self):
                """Channel reported for login and upgrade purposes ONLY; used for A/B testing"""
                # NOTE: Do not return the normal channel if login_channel is not specified, as
                # some code may branch depending on whether or not this is present
                return self.args.get('login_channel')

        def channel(self):
                return self.args['channel']
        def channel_unique(self):
                return self.channel().replace("Second Life", "").strip()
        def channel_oneword(self):
                return "".join(self.channel_unique().split())
        def channel_lowerword(self):
                return self.channel_oneword().lower()

        def flags_list(self):
                """ Convenience function that returns the command-line flags for the grid"""
                channel_flags = ''
                grid_flags = ''
                if not self.default_grid():
                        if self.default_channel():
                                # beta grid viewer
                                channel_flags = '--settings settings_beta.xml'
                        grid_flags = "--helperuri http://preview-%(grid)s.secondlife.com/helpers/ --loginuri https://login.%(grid)s.lindenlab.com/cgi-bin/login.cgi" % {'grid':self.args['grid']}
                        
                if not self.default_channel():
                        # some channel on some grid
                        channel_flags = '--settings settings_%s.xml --channel "%s"' % (self.channel_lowerword(), self.channel())
                elif self.login_channel():
                        # Report a special channel during login, but use default channel elsewhere
                        channel_flags = '--channel "%s"' % (self.login_channel())
                        
                return " ".join((channel_flags, grid_flags)).strip()

        def login_url(self):
                """ Convenience function that returns the appropriate login url for the grid"""
                if(self.args.get('login_url')):
                        return self.args['login_url']
                else:
                        if(self.default_grid()):
                                if(self.default_channel()):
                                        # agni release
                                        return 'http://secondlife.com/app/login/'
                                else:
                                        # first look (or other) on agni
                                        return 'http://secondlife.com/app/login/%s/' % self.channel_lowerword()
                        else:
                                # beta grid
                                return 'http://secondlife.com/app/login/beta/'

        def replace_login_url(self):
                # set the login page to point to a url appropriate for the type of client
                self.replace_in("skins/xui/en-us/panel_login.xml", searchdict={'http://secondlife.com/app/login/':self.login_url()})


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
                # the final exe is complicated because we're not sure where it's coming from,
                # nor do we have a fixed name for the executable
                self.path(self.find_existing_file('ReleaseForDownload/Secondlife.exe', 'Secondlife.exe', 'ReleaseNoOpt/newview_noopt.exe'), dst=self.final_exe())
                # need to get the kdu dll from any of the build directories as well
                self.path(self.find_existing_file('ReleaseForDownload/llkdu.dll', 'llkdu.dll', '../../libraries/i686-win32/lib_release/llkdu.dll'), dst='llkdu.dll')
                self.path(src="licenses-win32.txt", dst="licenses.txt")

                self.path("featuretable.txt")

                # For use in crash reporting (generates minidumps)
                self.path("dbghelp.dll")

                # For using FMOD for sound... DJS
                self.path("fmod.dll")

                # For textures
                if self.prefix(src="../../libraries/i686-win32/lib_release", dst=""):
                        self.path("openjpeg.dll")
                        self.end_prefix()

                # Mozilla appears to force a dependency on these files so we need to ship it (CP)
                self.path("msvcr71.dll")
                self.path("msvcp71.dll")

                # Mozilla runtime DLLs (CP)
                if self.prefix(src="../../libraries/i686-win32/lib_release", dst=""):
                        self.path("freebl3.dll")
                        self.path("gksvggdiplus.dll")
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

                # Vivox runtimes
                if self.prefix(src="vivox-runtime/i686-win32", dst=""):
                        self.path("SLVoice.exe")
                        self.path("SLVoiceAgent.exe")
                        self.path("libeay32.dll")
                        self.path("srtp.dll")
                        self.path("ssleay32.dll")
                        self.path("tntk.dll")
                        self.path("alut.dll")
                        self.path("vivoxsdk.dll")
                        self.path("ortp.dll")
                        self.path("wrap_oal.dll")
                        self.end_prefix()

                # pull in the crash logger and updater from other projects
                self.path(src="../win_crash_logger/win_crash_logger.exe", dst="win_crash_logger.exe")
                self.path(src="../win_updater/updater.exe", dst="updater.exe")
                self.replace_login_url()

        def nsi_file_commands(self, install=True):
                def wpath(path):
                        if(path.endswith('/') or path.endswith(os.path.sep)):
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
                                if(install):
                                        out_path = installed_dir
                                        result += 'SetOutPath ' + out_path + '\n'
                        if(install):
                                result += 'File ' + pkg_file + '\n'
                        else:
                                result += 'Delete ' + wpath(os.path.join('$INSTDIR', rel_file)) + '\n'
                # at the end of a delete, just rmdir all the directories
                if(not install):
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
                if(self.args.has_key('installer_name')):
                        installer_file = self.args['installer_name']
                else:
                        installer_file = installer_file % substitution_strings
                substitution_strings['installer_file'] = installer_file

                tempfile = "../secondlife_setup_tmp.nsi"
                # the following replaces strings in the nsi template
                # it also does python-style % substitution
                self.replace_in("installers/windows/installer_template.nsi", tempfile, {
                        "%%VERSION%%":version_vars,
                        "%%GRID_VARS%%":grid_vars_template % substitution_strings,
                        "%%INSTALL_FILES%%":self.nsi_file_commands(True),
                        "%%DELETE_FILES%%":self.nsi_file_commands(False)})

                NSIS_path = 'C:\\Program Files\\NSIS\\makensis.exe'
                self.run_command('"' + proper_windows_path(NSIS_path) + '" ' + self.dst_path_of(tempfile))
                # self.remove(self.dst_path_of(tempfile))
                self.created_path(installer_file)


class DarwinManifest(ViewerManifest):
        def construct(self):
                # copy over the build result (this is a no-op if run within the xcode script)
                self.path("build/" + self.args['configuration'] + "/Second Life.app", dst="")

                if self.prefix(src="", dst="Contents"):  # everything goes in Contents
                        # Expand the tar file containing the assorted mozilla bits into
                        #  <bundle>/Contents/MacOS/
                        self.contents_of_tar('mozilla-universal-darwin.tgz', 'MacOS')

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
                                self.path("secondlife.icns")

                                # llkdu dynamic library
                                self.path("../../libraries/universal-darwin/lib_release/libllkdu.dylib", "libllkdu.dylib")

                                # command line arguments for connecting to the proper grid
                                self.put_in_file(self.flags_list(), 'arguments.txt')

                                # set the proper login url
                                self.replace_login_url()

                                self.end_prefix("Resources")

                        self.end_prefix("Contents")
                        
                # NOTE: the -S argument to strip causes it to keep enough info for
                # annotated backtraces (i.e. function names in the crash log).  'strip' with no
                # arguments yields a slightly smaller binary but makes crash logs mostly useless.
                # This may be desirable for the final release.  Or not.
                if("package" in self.args['actions'] or
                   "unpacked" in self.args['actions']):
                    self.run_command('strip -S "%(viewer_binary)s"' %
                                 { 'viewer_binary' : self.dst_path_of('Contents/MacOS/Second Life')})


        def package_finish(self):
                channel_standin = 'Second Life'  # hah, our default channel is not usable on its own
                if not self.default_channel():
                        channel_standin = self.channel()
                        
                imagename="SecondLife_" + '_'.join(self.args['version'])
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

                self.run_command('hdiutil create "%(sparse)s" -volname "%(channel)s" -fs HFS+ -type SPARSE -megabytes 300 -layout SPUD' % {
                        'sparse':sparsename,
                        'channel':channel_standin})

                # mount the image and get the name of the mount point and device node
                hdi_output = self.run_command('hdiutil attach -private "' + sparsename + '"')
                devfile = re.search("/dev/disk([0-9]+)[^s]", hdi_output).group(0).strip()
                volpath = re.search('HFS\s+(.+)', hdi_output).group(1).strip()

                # Copy everything in to the mounted .dmg
                if self.default_channel() and not self.default_grid():
                        app_name = "Second Life " + self.args['grid']
                else:
                        app_name = channel_standin.strip()
                        
                for s,d in {self.get_dst_prefix():app_name + ".app",
                                        "lsl_guide.html":"Linden Scripting Language Guide.html",
                                        "releasenotes.txt":"Release Notes.txt",
                                        "installers/darwin/mac_image_hidden":".hidden",
                                        "installers/darwin/mac_image_background.tga":"background.tga",
                                        "installers/darwin/mac_image_DS_Store":".DS_Store"}.items():
                        print "Copying to dmg", s, d
                        self.copy_action(self.src_path_of(s), os.path.join(volpath, d))

                # Unmount the image
                self.run_command('hdiutil detach -force "' + devfile + '"')

                print "Converting temp disk image to final disk image"
                self.run_command('hdiutil convert "%(sparse)s" -format UDZO -imagekey zlib-level=9 -o "%(final)s"' % {'sparse':sparsename, 'final':finalname})
                # get rid of the temp file
                self.remove(sparsename)

class LinuxManifest(ViewerManifest):
        def construct(self):
                super(LinuxManifest, self).construct()
                self.path("licenses-linux.txt","licenses.txt")
                #self.path("res/ll_icon.ico","secondlife.ico")
                if self.prefix("linux_tools", ""):
                        self.path("client-readme.txt","README-linux.txt")
                        self.path("client-readme-voice.txt","README-linux-voice.txt")
                        self.path("wrapper.sh","secondlife")
                        self.path("handle_secondlifeprotocol.sh")
                        self.path("register_secondlifeprotocol.sh")
                        self.path("unicode.ttf","unicode.ttf")
                        self.end_prefix("linux_tools")

                # Create an appropriate gridargs.dat for this package, denoting required grid.
                self.put_in_file(self.flags_list(), 'gridargs.dat')
                # set proper login url
                self.replace_login_url()

                # stripping all the libs removes a few megabytes from the end-user package
                for s,d in self.file_list:
                        if re.search("lib/lib.+\.so.*", d):
                                self.run_command('strip -S %s' % d)

                # Fixing access permissions
                for s,d in self.dir_list:
                    self.run_command("chmod 755 '%s'" % d)
                for s,d in self.file_list:
                    if os.access(d, os.X_OK):
                        self.run_command("chmod 755 '%s'" % d)
                    else:
                        self.run_command("chmod 644 '%s'" % d) 


        def package_finish(self):
                if(self.args.has_key('installer_name')):
                        installer_name = self.args['installer_name']
                else:
                        installer_name = '_'.join('SecondLife_', self.args.get('arch'), *self.args['version'])
                        if self.default_channel():
                                if not self.default_grid():
                                        installer_name += '_' + self.args['grid'].upper()
                        else:
                                installer_name += '_' + self.channel_oneword().upper()

                # temporarily move directory tree so that it has the right name in the tarfile
                self.run_command("mv %(dst)s %(inst)s" % {'dst':self.get_dst_prefix(),'inst':self.src_path_of(installer_name)})
                # --numeric-owner hides the username of the builder for security etc.
                self.run_command('tar -C %(dir)s --numeric-owner -cjf %(inst_path)s.tar.bz2 %(inst_name)s' % {'dir':self.get_src_prefix(), 'inst_name': installer_name, 'inst_path':self.src_path_of(installer_name)})
                self.run_command("mv %(inst)s %(dst)s" % {'dst':self.get_dst_prefix(),'inst':self.src_path_of(installer_name)})

class Linux_i686Manifest(LinuxManifest):
        def construct(self):
                super(Linux_i686Manifest, self).construct()
                self.path("secondlife-i686-bin-stripped","bin/do-not-directly-run-secondlife-bin")
                self.path("../linux_crash_logger/linux-crash-logger-i686-bin-stripped","linux-crash-logger.bin")
                self.path("linux_tools/launch_url.sh","launch_url.sh")
                if self.prefix("res-sdl"):
                        self.path("*")
                        # recurse
                        self.end_prefix("res-sdl")

                self.path("featuretable_linux.txt")
                #self.path("secondlife-i686.supp")

                self.path("app_settings/mozilla-runtime-linux-i686")

                if self.prefix("../../libraries/i686-linux/lib_release_client", "lib"):
                        self.path("libkdu_v42R.so")
                        self.path("libfmod-3.75.so")
                        self.path("libapr-1.so.0")
                        self.path("libaprutil-1.so.0")
                        self.path("libdb-4.2.so")
                        self.path("libcrypto.so.0.9.7")
                        self.path("libssl.so.0.9.7")
                        self.path("libexpat.so.1")
                        self.path("libstdc++.so.6")
                        self.path("libuuid.so", "libuuid.so.1")
                        self.path("libSDL-1.2.so.0")
                        self.path("libELFIO.so")
                        self.path("libopenjpeg.so.2")
                        #self.path("libtcmalloc.so.0") - bugged
                        #self.path("libstacktrace.so.0") - probably bugged
                        self.path("libllkdu.so", "../bin/libllkdu.so") # llkdu goes in bin for some reason
                        self.end_prefix("lib")

                # Vivox runtimes
                if self.prefix(src="vivox-runtime/i686-linux", dst=""):
                        self.path("SLVoice")
                        self.end_prefix()
                if self.prefix(src="vivox-runtime/i686-linux", dst="lib"):
                        self.path("libopenal.so.1")
                        self.path("libortp.so")
                        self.path("libvivoxsdk.so")
                        self.path("libalut.so")
                        self.end_prefix("lib")

class Linux_x86_64Manifest(LinuxManifest):
        def construct(self):
                super(Linux_x86_64Manifest, self).construct()
                self.path("secondlife-x86_64-bin-stripped","bin/do-not-directly-run-secondlife-bin")
                self.path("../linux_crash_logger/linux-crash-logger-x86_64-bin-stripped","linux-crash-logger.bin")
                self.path("linux_tools/launch_url.sh","launch_url.sh")
                if self.prefix("res-sdl"):
                        self.path("*")
                        # recurse
                        self.end_prefix("res-sdl")

                self.path("featuretable_linux.txt")
                self.path("secondlife-i686.supp")

if __name__ == "__main__":
        main(srctree=viewer_dir, dsttree=os.path.join(viewer_dir, "packaged"))
