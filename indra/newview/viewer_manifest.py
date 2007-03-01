#!/usr/bin/python
# @file viewer_manifest.py
# @author Ryan Williams
# @brief Description of all installer viewer files, and methods for packaging
#        them into installers for all supported platforms.
#
# Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
# $License$
import sys
import os.path
import re
import tarfile
viewer_dir = os.path.dirname(__file__)
# add llmanifest library to our path so we don't have to muck with PYTHONPATH
sys.path.append(os.path.join(viewer_dir, '../lib/python/indra'))
from llmanifest import LLManifest, main, proper_windows_path, path_ancestors

class ViewerManifest(LLManifest):
        def construct(self):
                super(ViewerManifest, self).construct()
                self.exclude("*.svn*")
                self.path(src="../../scripts/messages/message_template.msg", dst="app_settings/message_template.msg")

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
                        self.path("paths.xml")
                        self.path("xui/*/*.xml")
                        self.path('words.*.txt')

                        # Local HTML files (e.g. loading screen)
                        if self.prefix("html/*"):
                                self.path("*.html")
                                self.path("*.gif")
                                self.path("*.jpg")
                                self.path("*.css")
                                self.end_prefix("html/*")
                        self.end_prefix("skins")

                self.path("featuretable.txt")
                self.path("releasenotes.txt")
                self.path("lsl_guide.html")
                self.path("gpu_table.txt")

        def flags_list(self):
                """ Convenience function that returns the command-line flags for the grid"""
                if(self.args['grid'] == ''):
                        return ""
                elif(self.args['grid'] == 'firstlook'):
                        return '-settings settings_firstlook.xml'
                else:
                        return ("-settings settings_beta.xml --%(grid)s -helperuri http://preview-%(grid)s.secondlife.com/helpers/" % {'grid':self.args['grid']})

        def login_url(self):
                """ Convenience function that returns the appropriate login url for the grid"""
                if(self.args.get('login_url')):
                        return self.args['login_url']
                else:
                        if(self.args['grid'] == ''):
                                return 'http://secondlife.com/app/login/'
                        elif(self.args['grid'] == 'firstlook'):
                                return 'http://secondlife.com/app/login/firstlook/'
                        else:
                                return 'http://secondlife.com/app/login/beta/'

        def replace_login_url(self):
                # set the login page to point to a url appropriate for the type of client
                self.replace_in("skins/xui/en-us/panel_login.xml", searchdict={'http://secondlife.com/app/login/':self.login_url()})


class WindowsManifest(ViewerManifest):
        def final_exe(self):
                # *NOTE: these are the only two executable names that the crash reporter recognizes
                if self.args['grid'] == '':
                        return "SecondLife.exe"
                else:
                        return "SecondLifePreview.exe"
                        # return "SecondLifePreview%s.exe" % (self.args['grid'], )

        def construct(self):
                super(WindowsManifest, self).construct()
                # the final exe is complicated because we're not sure where it's coming from,
                # nor do we have a fixed name for the executable
                self.path(self.find_existing_file('ReleaseForDownload/Secondlife.exe', 'Secondlife.exe', 'ReleaseNoOpt/newview_noopt.exe'), dst=self.final_exe())
                # need to get the kdu dll from any of the build directories as well
                self.path(self.find_existing_file('ReleaseForDownload/llkdu.dll', 'llkdu.dll', '../../libraries/i686-win32/lib_release/llkdu.dll'), dst='llkdu.dll')
                self.path(src="licenses-win32.txt", dst="licenses.txt")

                # For use in crash reporting (generates minidumps)
                self.path("dbghelp.dll")

                # For using FMOD for sound... DJS
                self.path("fmod.dll")

                # Mozilla appears to force a dependency on these files so we need to ship it (CP)
                self.path("msvcr71.dll")
                self.path("msvcp71.dll")

                # Mozilla runtime DLLs (CP)
                if self.prefix(src="../../libraries/i686-win32/lib_release", dst=""):
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
                version_vars_template = """
                !define INSTEXE  "%(final_exe)s"
                !define VERSION "%(version_short)s"
                !define VERSION_LONG "%(version)s"
                !define VERSION_DASHES "%(version_dashes)s"
                """
                if(self.args['grid'] == ''):
                        installer_file = "Second Life %(version_dashes)s Setup.exe"
                        grid_vars_template = """
                        OutFile "%(outfile)s"
                        !define INSTFLAGS "%(flags)s"
                        !define INSTNAME   "SecondLife"
                        !define SHORTCUT   "Second Life"
                        !define URLNAME   "secondlife"
                        Caption "Second Life ${VERSION}"
                        """
                else:
                        installer_file = "Second Life %(version_dashes)s (%(grid_caps)s) Setup.exe"
                        grid_vars_template = """
                        OutFile "%(outfile)s"
                        !define INSTFLAGS "%(flags)s"
                        !define INSTNAME   "SecondLife%(grid_caps)s"
                        !define SHORTCUT   "Second Life (%(grid_caps)s)"
                        !define URLNAME   "secondlife%(grid)s"
                        !define UNINSTALL_SETTINGS 1
                        Caption "Second Life %(grid)s ${VERSION}"
                        """
                if(self.args.has_key('installer_name')):
                        installer_file = self.args['installer_name']
                else:
                        installer_file = installer_file % {'version_dashes' : '-'.join(self.args['version']),
                                                                                           'grid_caps' : self.args['grid'].upper()}
                tempfile = "../secondlife_setup.nsi"
                # the following is an odd sort of double-string replacement
                self.replace_in("installers/windows/installer_template.nsi", tempfile, {
                        "%%VERSION%%":version_vars_template%{'version_short' : '.'.join(self.args['version'][:-1]),
                                                                                                 'version' : '.'.join(self.args['version']),
                                                                                                 'version_dashes' : '-'.join(self.args['version']),
                                                                                                 'final_exe' : self.final_exe()},
                        "%%GRID_VARS%%":grid_vars_template%{'grid':self.args['grid'],
                                                                                                'grid_caps':self.args['grid'].upper(),
                                                                                                'outfile':installer_file,
                                                                                                'flags':self.flags_list()},
                        "%%INSTALL_FILES%%":self.nsi_file_commands(True),
                        "%%DELETE_FILES%%":self.nsi_file_commands(False)})

                NSIS_path = 'C:\\Program Files\\NSIS\\makensis.exe'
                self.run_command('"' + proper_windows_path(NSIS_path) + '" ' + self.dst_path_of(tempfile))
                self.remove(self.dst_path_of(tempfile))
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

                                # the trial directory seems to be not used [rdw]
                                self.path('trial')

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
                imagename="SecondLife_" + '_'.join(self.args['version'])
                if(self.args['grid'] != ''):
                        imagename = imagename + '_' + self.args['grid'].upper()

                sparsename = imagename + ".sparseimage"
                finalname = imagename + ".dmg"
                # make sure we don't have stale files laying about
                self.remove(sparsename, finalname)

                self.run_command('hdiutil create "%(sparse)s" -volname "Second Life" -fs HFS+ -type SPARSE -megabytes 300' % {'sparse':sparsename})

                # mount the image and get the name of the mount point and device node
                hdi_output = self.run_command('hdiutil attach -private "' + sparsename + '"')
                devfile = re.search("/dev/disk([0-9]+)[^s]", hdi_output).group(0).strip()
                volpath = re.search('HFS\s+(.+)', hdi_output).group(1).strip()

                # Copy everything in to the mounted .dmg
                for s,d in {self.get_dst_prefix():("Second Life " + self.args['grid']).strip()+ ".app",
                                        "lsl_guide.html":"Linden Scripting Language Guide.html",
                                        "releasenotes.txt":"Release Notes.txt",
                                        "installers/darwin/mac_image_hidden":".hidden",
                                        "installers/darwin/mac_image_background.tga":"background.tga",
                                        "installers/darwin/mac_image_DS_Store":".DS_Store"}.items():
                        print "Copying to dmg", s, d
                        self.copy_action(self.src_path_of(s), os.path.join(volpath, d))

                # Unmount the image
                self.run_command('hdiutil detach "' + devfile + '"')

                print "Converting temp disk image to final disk image"
                self.run_command('hdiutil convert "%(sparse)s" -format UDZO -imagekey zlib-level=9 -o "%(final)s"' % {'sparse':sparsename, 'final':finalname})
                # get rid of the temp file
                self.remove(sparsename)

class LinuxManifest(ViewerManifest):
        def construct(self):
                super(LinuxManifest, self).construct()
                self.path("licenses-linux.txt","licenses.txt")
                self.path("res/ll_icon.ico","secondlife.ico")
                if self.prefix("linux_tools", ""):
                        self.path("client-readme.txt","README-linux.txt")
                        self.path("wrapper.sh","secondlife")
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


        def package_finish(self):
                if(self.args.has_key('installer_name')):
                        installer_name = self.args['installer_name']
                else:
                        installer_name = '_'.join('SecondLife_', self.args.get('arch'), *self.args['version'])
                        if grid != '':
                                installer_name += "_" + grid.upper()

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

                self.path("app_settings/mozilla-runtime-linux-i686", "app_settings/mozilla")

                if self.prefix("../../libraries/i686-linux/lib_release_client", "lib"):
                        self.path("libkdu_v42R.so")
                        self.path("libfmod-3.75.so")
                        self.path("libapr-1.so.0")
                        self.path("libaprutil-1.so.0")
                        self.path("libdb-4.2.so")
                        self.path("libogg.so.0")
                        self.path("libvorbis.so.0")
                        self.path("libvorbisfile.so.0")
                        self.path("libvorbisenc.so.0")
                        self.path("libcurl.so.3")
                        self.path("libcrypto.so.0.9.7")
                        self.path("libssl.so.0.9.7")
                        self.path("libexpat.so.1")
                        self.path("libstdc++.so.6")
                        self.path("libelfio.so")
                        self.path("libuuid.so", "libuuid.so.1")
                        self.path("libSDL-1.2.so.0")
                        self.path("libllkdu.so", "../bin/libllkdu.so") # llkdu goes in bin for some reason
                        self.end_prefix("lib")


class Linux_x86_64Manifest(LinuxManifest):
        def construct(self):
                super(Linux_x86_64Manifest, self).construct()
                self.path("secondlife-x86_64-bin-stripped","bin/secondlife-bin")
                # TODO: I get the sense that this isn't fully fleshed out
                if self.prefix("../../libraries/x86_64-linux/lib_release_client", "lib"):
                        self.path("libkdu_v42R.so")
                        self.path("libxmlrpc.so.0")
                        # self.path("libllkdu.so", "../bin/libllkdu.so") # llkdu goes in bin for some reason
                        self.end_prefix("lib")


if __name__ == "__main__":
        main(srctree=viewer_dir, dsttree=os.path.join(viewer_dir, "packaged"))
