#!/usr/bin/env python
"""\
@file viewer_manifest.py
@author Ryan Williams
@brief Description of all installer viewer files, and methods for packaging
       them into installers for all supported platforms.

$LicenseInfo:firstyear=2006&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2006-2014, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""
import sys
import os
import os.path
import shutil
import errno
import json
import re
import tarfile
import time
import random
import subprocess

viewer_dir = os.path.dirname(__file__)
# Add indra/lib/python to our path so we don't have to muck with PYTHONPATH.
# Put it FIRST because some of our build hosts have an ancient install of
# indra.util.llmanifest under their system Python!
sys.path.insert(0, os.path.join(viewer_dir, os.pardir, "lib", "python"))
from indra.util.llmanifest import LLManifest, main, path_ancestors, CHANNEL_VENDOR_BASE, RELEASE_CHANNEL, ManifestError
from llbase import llsd

class ViewerManifest(LLManifest):
    def is_packaging_viewer(self):
        # Some commands, files will only be included
        # if we are packaging the viewer on windows.
        # This manifest is also used to copy
        # files during the build (see copy_w_viewer_manifest
        # and copy_l_viewer_manifest targets)
        return 'package' in self.args['actions']
    
    def construct(self):
        super(ViewerManifest, self).construct()
        self.path(src="../../scripts/messages/message_template.msg", dst="app_settings/message_template.msg")
        self.path(src="../../etc/message.xml", dst="app_settings/message.xml")

        if self.is_packaging_viewer():
            with self.prefix(src="app_settings"):
                self.exclude("logcontrol.xml")
                self.exclude("logcontrol-dev.xml")
                self.path("*.pem")
                self.path("*.ini")
                self.path("*.xml")
                self.path("*.db2")

                # include the entire shaders directory recursively
                self.path("shaders")
                # include the extracted list of contributors
                contributions_path = "../../doc/contributions.txt"
                contributor_names = self.extract_names(contributions_path)
                self.put_in_file(contributor_names, "contributors.txt", src=contributions_path)

                # ... and the entire windlight directory
                self.path("windlight")

                # ... and the entire image filters directory
                self.path("filters")
            
                # ... and the included spell checking dictionaries
                pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
                with self.prefix(src=pkgdir,dst=""):
                    self.path("dictionaries")

                # include the extracted packages information (see BuildPackagesInfo.cmake)
                self.path(src=os.path.join(self.args['build'],"packages-info.txt"), dst="packages-info.txt")

                # CHOP-955: If we have "sourceid" or "viewer_channel" in the
                # build process environment, generate it into
                # settings_install.xml.
                settings_template = dict(
                    sourceid=dict(Comment='Identify referring agency to Linden web servers',
                                  Persist=1,
                                  Type='String',
                                  Value=''),
                    CmdLineGridChoice=dict(Comment='Default grid',
                                  Persist=0,
                                  Type='String',
                                  Value=''),
                    CmdLineChannel=dict(Comment='Command line specified channel name',
                                        Persist=0,
                                        Type='String',
                                        Value=''))
                settings_install = {}
                if 'sourceid' in self.args and self.args['sourceid']:
                    settings_install['sourceid'] = settings_template['sourceid'].copy()
                    settings_install['sourceid']['Value'] = self.args['sourceid']
                    print "Set sourceid in settings_install.xml to '%s'" % self.args['sourceid']

                if 'channel_suffix' in self.args and self.args['channel_suffix']:
                    settings_install['CmdLineChannel'] = settings_template['CmdLineChannel'].copy()
                    settings_install['CmdLineChannel']['Value'] = self.channel_with_pkg_suffix()
                    print "Set CmdLineChannel in settings_install.xml to '%s'" % self.channel_with_pkg_suffix()

                if 'grid' in self.args and self.args['grid']:
                    settings_install['CmdLineGridChoice'] = settings_template['CmdLineGridChoice'].copy()
                    settings_install['CmdLineGridChoice']['Value'] = self.grid()
                    print "Set CmdLineGridChoice in settings_install.xml to '%s'" % self.grid()

                # put_in_file(src=) need not be an actual pathname; it
                # only needs to be non-empty
                self.put_in_file(llsd.format_pretty_xml(settings_install),
                                 "settings_install.xml",
                                 src="environment")


            with self.prefix(src="character"):
                self.path("*.llm")
                self.path("*.xml")
                self.path("*.tga")

            # Include our fonts
            with self.prefix(src="fonts"):
                self.path("*.ttf")
                self.path("*.txt")

            # skins
            with self.prefix(src="skins"):
                    # include the entire textures directory recursively
                    with self.prefix(src="*/textures"):
                            self.path("*/*.tga")
                            self.path("*/*.j2c")
                            self.path("*/*.jpg")
                            self.path("*/*.png")
                            self.path("*.tga")
                            self.path("*.j2c")
                            self.path("*.jpg")
                            self.path("*.png")
                            self.path("textures.xml")
                    self.path("*/xui/*/*.xml")
                    self.path("*/xui/*/widgets/*.xml")
                    self.path("*/*.xml")

                    # Local HTML files (e.g. loading screen)
                    # The claim is that we never use local html files any
                    # longer. But rather than commenting out this block, let's
                    # rename every html subdirectory as html.old. That way, if
                    # we're wrong, a user actually does have the relevant
                    # files; s/he just needs to rename every html.old
                    # directory back to html to recover them.
                    with self.prefix(src="*/html", dst="*/html.old"):
                            self.path("*.png")
                            self.path("*/*/*.html")
                            self.path("*/*/*.gif")


            # local_assets dir (for pre-cached textures)
            with self.prefix(src="local_assets"):
                self.path("*.j2c")
                self.path("*.tga")

            # File in the newview/ directory
            self.path("gpu_table.txt")

            #build_data.json.  Standard with exception handling is fine.  If we can't open a new file for writing, we have worse problems
            #platform is computed above with other arg parsing
            build_data_dict = {"Type":"viewer","Version":'.'.join(self.args['version']),
                            "Channel Base": CHANNEL_VENDOR_BASE,
                            "Channel":self.channel_with_pkg_suffix(),
                            "Platform":self.build_data_json_platform,
                            "Address Size":self.address_size,
                            "Update Service":"https://update.secondlife.com/update",
                            }
            build_data_dict = self.finish_build_data_dict(build_data_dict)
            with open(os.path.join(os.pardir,'build_data.json'), 'w') as build_data_handle:
                json.dump(build_data_dict,build_data_handle)

            #we likely no longer need the test, since we will throw an exception above, but belt and suspenders and we get the
            #return code for free.
            if not self.path2basename(os.pardir, "build_data.json"):
                print "No build_data.json file"

    def finish_build_data_dict(self, build_data_dict):
        return build_data_dict

    def grid(self):
        return self.args['grid']

    def channel(self):
        return self.args['channel']

    def channel_with_pkg_suffix(self):
        fullchannel=self.channel()
        if 'channel_suffix' in self.args and self.args['channel_suffix']:
            fullchannel+=' '+self.args['channel_suffix']
        return fullchannel

    def channel_variant(self):
        global CHANNEL_VENDOR_BASE
        return self.channel().replace(CHANNEL_VENDOR_BASE, "").strip()

    def channel_type(self): # returns 'release', 'beta', 'project', or 'test'
        global CHANNEL_VENDOR_BASE
        channel_qualifier=self.channel().replace(CHANNEL_VENDOR_BASE, "").lower().strip()
        if channel_qualifier.startswith('release'):
            channel_type='release'
        elif channel_qualifier.startswith('beta'):
            channel_type='beta'
        elif channel_qualifier.startswith('project'):
            channel_type='project'
        else:
            channel_type='test'
        return channel_type

    def channel_variant_app_suffix(self):
        # get any part of the channel name after the CHANNEL_VENDOR_BASE
        suffix=self.channel_variant()
        # by ancient convention, we don't use Release in the app name
        if self.channel_type() == 'release':
            suffix=suffix.replace('Release', '').strip()
        # for the base release viewer, suffix will now be null - for any other, append what remains
        if len(suffix) > 0:
            suffix = "_"+ ("_".join(suffix.split()))
        # the additional_packages mechanism adds more to the installer name (but not to the app name itself)
        if 'channel_suffix' in self.args and self.args['channel_suffix']:
            suffix+='_'+("_".join(self.args['channel_suffix'].split()))
        return suffix

    def installer_base_name(self):
        global CHANNEL_VENDOR_BASE
        # a standard map of strings for replacing in the templates
        substitution_strings = {
            'channel_vendor_base' : '_'.join(CHANNEL_VENDOR_BASE.split()),
            'channel_variant_underscores':self.channel_variant_app_suffix(),
            'version_underscores' : '_'.join(self.args['version']),
            'arch':self.args['arch']
            }
        return "%(channel_vendor_base)s%(channel_variant_underscores)s_%(version_underscores)s_%(arch)s" % substitution_strings

    def app_name(self):
        global CHANNEL_VENDOR_BASE
        channel_type=self.channel_type()
        if channel_type == 'release':
            app_suffix='Viewer'
        else:
            app_suffix=self.channel_variant()
        return CHANNEL_VENDOR_BASE + ' ' + app_suffix

    def app_name_oneword(self):
        return ''.join(self.app_name().split())
    
    def icon_path(self):
        return "icons/" + self.channel_type()

    def extract_names(self,src):
        try:
            contrib_file = open(src,'r')
        except IOError:
            print "Failed to open '%s'" % src
            raise
        lines = contrib_file.readlines()
        contrib_file.close()

        # All lines up to and including the first blank line are the file header; skip them
        lines.reverse() # so that pop will pull from first to last line
        while not re.match("\s*$", lines.pop()) :
            pass # do nothing

        # A line that starts with a non-whitespace character is a name; all others describe contributions, so collect the names
        names = []
        for line in lines :
            if re.match("\S", line) :
                names.append(line.rstrip())
        # It's not fair to always put the same people at the head of the list
        random.shuffle(names)
        return ', '.join(names)


class WindowsManifest(ViewerManifest):
    def final_exe(self):
        return self.app_name_oneword()+".exe"

    def finish_build_data_dict(self, build_data_dict):
        #MAINT-7294: Windows exe names depend on channel name, so write that in also
        build_data_dict.update({'Executable':self.final_exe()})
        return build_data_dict

    def test_msvcrt_and_copy_action(self, src, dst):
        # This is used to test a dll manifest.
        # It is used as a temporary override during the construct method
        from test_win32_manifest import test_assembly_binding
        if src and (os.path.exists(src) or os.path.islink(src)):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            if not os.path.isdir(src):
                if(self.args['configuration'].lower() == 'debug'):
                    test_assembly_binding(src, "Microsoft.VC80.DebugCRT", "8.0.50727.4053")
                else:
                    test_assembly_binding(src, "Microsoft.VC80.CRT", "8.0.50727.4053")
                self.ccopy(src,dst)
            else:
                raise Exception("Directories are not supported by test_CRT_and_copy_action()")
        else:
            print "Doesn't exist:", src

    def test_for_no_msvcrt_manifest_and_copy_action(self, src, dst):
        # This is used to test that no manifest for the msvcrt exists.
        # It is used as a temporary override during the construct method
        from test_win32_manifest import test_assembly_binding
        from test_win32_manifest import NoManifestException, NoMatchingAssemblyException
        if src and (os.path.exists(src) or os.path.islink(src)):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            if not os.path.isdir(src):
                try:
                    if(self.args['configuration'].lower() == 'debug'):
                        test_assembly_binding(src, "Microsoft.VC80.DebugCRT", "")
                    else:
                        test_assembly_binding(src, "Microsoft.VC80.CRT", "")
                    raise Exception("Unknown condition")
                except NoManifestException as err:
                    pass
                except NoMatchingAssemblyException as err:
                    pass
                    
                self.ccopy(src,dst)
            else:
                raise Exception("Directories are not supported by test_CRT_and_copy_action()")
        else:
            print "Doesn't exist:", src
        
    def construct(self):
        super(WindowsManifest, self).construct()

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")
        vmpdir = os.path.join(pkgdir, "VMP")
        llbasedir = os.path.join(pkgdir, "lib", "python", "llbase")

        if self.is_packaging_viewer():
            # Find secondlife-bin.exe in the 'configuration' dir, then rename it to the result of final_exe.
            self.path(src='%s/secondlife-bin.exe' % self.args['configuration'], dst=self.final_exe())

            # include the compiled launcher scripts so that it gets included in the file_list
            self.path(src='%s/download_update.exe' % vmpdir, dst="download_update.exe")
            self.path(src='%s/SL_Launcher.exe' % vmpdir, dst="SL_Launcher.exe")

            #IUM is not normally executed directly, just imported.  No exe needed.
            self.path2basename(vmpdir,"InstallerUserMessage.py")

            with self.prefix(src=self.icon_path(), dst="vmp_icons"):
                self.path("secondlife.ico")

            #VMP  Tkinter icons
            with self.prefix("vmp_icons"):
                self.path("*.png")
                self.path("*.gif")

            #before, we only needed llbase at build time.  With VMP, we need it at run time.
            llbase_path = os.path.join(self.get_dst_prefix(),'llbase')
            if not os.path.exists(llbase_path):
                os.makedirs(llbase_path)
            with self.prefix(dst="llbase"):
                self.path2basename(llbasedir,"*.py")
                self.path2basename(llbasedir,"_cllsd.so")

        # Plugin host application
        self.path2basename(os.path.join(os.pardir,
                                        'llplugin', 'slplugin', self.args['configuration']),
                           "slplugin.exe")
        
        # Get shared libs from the shared libs staging directory
        with self.prefix(src=os.path.join(os.pardir, 'sharedlibs', self.args['configuration']),
                       dst=""):

            # Get llcommon and deps. If missing assume static linkage and continue.
            try:
                self.path('llcommon.dll')
                self.path('libapr-1.dll')
                self.path('libaprutil-1.dll')
                self.path('libapriconv-1.dll')
                
            except RuntimeError as err:
                print err.message
                print "Skipping llcommon.dll (assuming llcommon was linked statically)"

            # Mesh 3rd party libs needed for auto LOD and collada reading
            try:
                self.path("glod.dll")
            except RuntimeError as err:
                print err.message
                print "Skipping GLOD library (assumming linked statically)"

            # Get fmodex dll, continue if missing
            try:
                if(self.address_size == 64):
                    self.path("fmodex64.dll")
                else:
                    self.path("fmodex.dll")
            except:
                print "Skipping fmodex audio library(assuming other audio engine)"

            # For textures
            self.path("openjpeg.dll")

            # These need to be installed as a SxS assembly, currently a 'private' assembly.
            # See http://msdn.microsoft.com/en-us/library/ms235291(VS.80).aspx
            if self.args['configuration'].lower() == 'debug':
                self.path("msvcr120d.dll")
                self.path("msvcp120d.dll")
                self.path("msvcr100d.dll")
                self.path("msvcp100d.dll")
            else:
                self.path("msvcr120.dll")
                self.path("msvcp120.dll")
                self.path("msvcr100.dll")
                self.path("msvcp100.dll")

            # Vivox runtimes
            self.path("SLVoice.exe")
            self.path("vivoxsdk.dll")
            self.path("ortp.dll")
            self.path("libsndfile-1.dll")
            self.path("vivoxoal.dll")
            
            # Security
            self.path("ssleay32.dll")
            self.path("libeay32.dll")

            # Hunspell
            self.path("libhunspell.dll")

            # For google-perftools tcmalloc allocator.
            try:
                if self.args['configuration'].lower() == 'debug':
                    self.path('libtcmalloc_minimal-debug.dll')
                else:
                    self.path('libtcmalloc_minimal.dll')
            except:
                print "Skipping libtcmalloc_minimal.dll"


        self.path(src="licenses-win32.txt", dst="licenses.txt")
        self.path("featuretable.txt")
        self.path("ca-bundle.crt")

        # Media plugins - CEF
        with self.prefix(src='../media_plugins/cef/%s' % self.args['configuration'], dst="llplugin"):
            self.path("media_plugin_cef.dll")

        # Media plugins - LibVLC
        with self.prefix(src='../media_plugins/libvlc/%s' % self.args['configuration'], dst="llplugin"):
            self.path("media_plugin_libvlc.dll")

        # Media plugins - Example (useful for debugging - not shipped with release viewer)
        if self.channel_type() != 'release':
            with self.prefix(src='../media_plugins/example/%s' % self.args['configuration'], dst="llplugin"):
                self.path("media_plugin_example.dll")

        # CEF runtime files - debug
        if self.args['configuration'].lower() == 'debug':
            with self.prefix(src=os.path.join(os.pardir, 'packages', 'bin', 'debug'), dst="llplugin"):
                self.path("chrome_elf.dll")
                self.path("d3dcompiler_43.dll")
                self.path("d3dcompiler_47.dll")
                self.path("libcef.dll")
                self.path("libEGL.dll")
                self.path("libGLESv2.dll")
                self.path("dullahan_host.exe")
                self.path("natives_blob.bin")
                self.path("snapshot_blob.bin")
                self.path("widevinecdmadapter.dll")
        else:
        # CEF runtime files - not debug (release, relwithdebinfo etc.)
            with self.prefix(src=os.path.join(os.pardir, 'packages', 'bin', 'release'), dst="llplugin"):
                self.path("chrome_elf.dll")
                self.path("d3dcompiler_43.dll")
                self.path("d3dcompiler_47.dll")
                self.path("libcef.dll")
                self.path("libEGL.dll")
                self.path("libGLESv2.dll")
                self.path("dullahan_host.exe")
                self.path("natives_blob.bin")
                self.path("snapshot_blob.bin")
                self.path("widevinecdmadapter.dll")

        # MSVC DLLs needed for CEF and have to be in same directory as plugin
        with self.prefix(src=os.path.join(os.pardir, 'sharedlibs', 'Release'), dst="llplugin"):
            self.path("msvcp120.dll")
            self.path("msvcr120.dll")

        # CEF files common to all configurations
        with self.prefix(src=os.path.join(os.pardir, 'packages', 'resources'), dst="llplugin"):
            self.path("cef.pak")
            self.path("cef_100_percent.pak")
            self.path("cef_200_percent.pak")
            self.path("cef_extensions.pak")
            self.path("devtools_resources.pak")
            self.path("icudtl.dat")

        with self.prefix(src=os.path.join(os.pardir, 'packages', 'resources', 'locales'), dst=os.path.join('llplugin', 'locales')):
            self.path("am.pak")
            self.path("ar.pak")
            self.path("bg.pak")
            self.path("bn.pak")
            self.path("ca.pak")
            self.path("cs.pak")
            self.path("da.pak")
            self.path("de.pak")
            self.path("el.pak")
            self.path("en-GB.pak")
            self.path("en-US.pak")
            self.path("es-419.pak")
            self.path("es.pak")
            self.path("et.pak")
            self.path("fa.pak")
            self.path("fi.pak")
            self.path("fil.pak")
            self.path("fr.pak")
            self.path("gu.pak")
            self.path("he.pak")
            self.path("hi.pak")
            self.path("hr.pak")
            self.path("hu.pak")
            self.path("id.pak")
            self.path("it.pak")
            self.path("ja.pak")
            self.path("kn.pak")
            self.path("ko.pak")
            self.path("lt.pak")
            self.path("lv.pak")
            self.path("ml.pak")
            self.path("mr.pak")
            self.path("ms.pak")
            self.path("nb.pak")
            self.path("nl.pak")
            self.path("pl.pak")
            self.path("pt-BR.pak")
            self.path("pt-PT.pak")
            self.path("ro.pak")
            self.path("ru.pak")
            self.path("sk.pak")
            self.path("sl.pak")
            self.path("sr.pak")
            self.path("sv.pak")
            self.path("sw.pak")
            self.path("ta.pak")
            self.path("te.pak")
            self.path("th.pak")
            self.path("tr.pak")
            self.path("uk.pak")
            self.path("vi.pak")
            self.path("zh-CN.pak")
            self.path("zh-TW.pak")

            with self.prefix(src=os.path.join(os.pardir, 'packages', 'bin', 'release'), dst="llplugin"):
                self.path("libvlc.dll")
                self.path("libvlccore.dll")
                self.path("plugins/")

        # pull in the crash logger and updater from other projects
        # tag:"crash-logger" here as a cue to the exporter
        self.path(src='../win_crash_logger/%s/windows-crash-logger.exe' % self.args['configuration'],
                  dst="win_crash_logger.exe")

        if not self.is_packaging_viewer():
            self.package_file = "copied_deps"    

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
            'version_registry' : '%s(%s)' %
            ('.'.join(self.args['version']), self.address_size),
            'final_exe' : self.final_exe(),
            'flags':'',
            'app_name':self.app_name(),
            'app_name_oneword':self.app_name_oneword()
            }

        installer_file = self.installer_base_name() + '_Setup.exe'
        substitution_strings['installer_file'] = installer_file
        
        version_vars = """
        !define INSTEXE "SL_Launcher.exe"
        !define VERSION "%(version_short)s"
        !define VERSION_LONG "%(version)s"
        !define VERSION_DASHES "%(version_dashes)s"
        !define VERSION_REGISTRY "%(version_registry)s"
        !define VIEWER_EXE "%(final_exe)s"
        """ % substitution_strings
        
        if self.channel_type() == 'release':
            substitution_strings['caption'] = CHANNEL_VENDOR_BASE
        else:
            substitution_strings['caption'] = self.app_name() + ' ${VERSION}'

        inst_vars_template = """
            OutFile "%(installer_file)s"
            !define INSTNAME   "%(app_name_oneword)s"
            !define SHORTCUT   "%(app_name)s"
            !define URLNAME   "secondlife"
            Caption "%(caption)s"
            """

        if(self.address_size == 64):
            engage_registry="SetRegView 64"
            program_files="$PROGRAMFILES64"
        else:
            engage_registry="SetRegView 32"
            program_files="$PROGRAMFILES32"

        tempfile = "secondlife_setup_tmp.nsi"
        # the following replaces strings in the nsi template
        # it also does python-style % substitution
        self.replace_in("installers/windows/installer_template.nsi", tempfile, {
                "%%VERSION%%":version_vars,
                "%%SOURCE%%":self.get_src_prefix(),
                "%%INST_VARS%%":inst_vars_template % substitution_strings,
                "%%INSTALL_FILES%%":self.nsi_file_commands(True),
                "%%PROGRAMFILES%%":program_files,
                "%%ENGAGEREGISTRY%%":engage_registry,
                "%%DELETE_FILES%%":self.nsi_file_commands(False)})

        # If we're on a build machine, sign the code using our Authenticode certificate. JC
        # note that the enclosing setup exe is signed later, after the makensis makes it.
        # Unlike the viewer binary, the VMP filenames are invariant with respect to version, os, etc.
        for exe in (
            "download_update.exe",
            "SL_Launcher.exe",
            ):
            self.sign(exe)
            
        # We use the Unicode version of NSIS, available from
        # http://www.scratchpaper.com/
        # Check two paths, one for Program Files, and one for Program Files (x86).
        # Yay 64bit windows.
        for ProgramFiles in 'ProgramFiles', 'ProgramFiles(x86)':
            NSIS_path = os.path.expandvars(r'${%s}\NSIS\Unicode\makensis.exe' % ProgramFiles)
            if os.path.exists(NSIS_path):
                break
        installer_created=False
        nsis_attempts=3
        nsis_retry_wait=15
        for attempt in xrange(nsis_attempts):
            try:
                self.run_command('"' + NSIS_path + '" /V2 ' + self.dst_path_of(tempfile))
            except ManifestError as err:
                if attempt+1 < nsis_attempts:
                    print >> sys.stderr, "nsis failed, waiting %d seconds before retrying" % nsis_retry_wait
                    time.sleep(nsis_retry_wait)
                    nsis_retry_wait*=2
            else:
                # NSIS worked! Done!
                break
        else:
            print >> sys.stderr, "Maximum nsis attempts exceeded; giving up"
            raise

        self.sign(installer_file)
        self.created_path(self.dst_path_of(installer_file))
        self.package_file = installer_file

    def sign(self, exe):
        sign_py = os.environ.get('SIGN', r'C:\buildscripts\code-signing\sign.py')
        python  = os.environ.get('PYTHON', 'python')
        if os.path.exists(sign_py):
            dst_path = self.dst_path_of(exe)
            print "about to run signing of: ", dst_path
            self.run_command(' '.join((python, self.escape_slashes(sign_py),
                                       self.escape_slashes(dst_path))))
        else:
            print "Skipping code signing of %s: %s not found" % (exe, sign_py)

    def escape_slashes(self, path):
        return path.replace('\\', '\\\\\\\\')

class Windows_i686_Manifest(WindowsManifest):
    # Although we aren't literally passed ADDRESS_SIZE, we can infer it from
    # the passed 'arch', which is used to select the specific subclass.
    address_size = 32
    build_data_json_platform = 'win32'

class Windows_x86_64_Manifest(WindowsManifest):
    address_size = 64
    build_data_json_platform = 'win'


class DarwinManifest(ViewerManifest):
    build_data_json_platform = 'mac'

    def finish_build_data_dict(self, build_data_dict):
        build_data_dict.update({'Bundle Id':self.args['bundleid']})
        return build_data_dict

    def is_packaging_viewer(self):
        # darwin requires full app bundle packaging even for debugging.
        return True

    def construct(self):
        # copy over the build result (this is a no-op if run within the xcode script)
        self.path(self.args['configuration'] + "/Second Life.app", dst="")

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")
        vmpdir = os.path.join(pkgdir, "VMP")
        llbasedir = os.path.join(pkgdir, "lib", "python", "llbase")
        requestsdir = os.path.join(pkgdir, "lib", "python", "requests")
        urllib3dir = os.path.join(pkgdir, "lib", "python", "urllib3")
        chardetdir = os.path.join(pkgdir, "lib", "python", "chardet")
        idnadir = os.path.join(pkgdir, "lib", "python", "idna")

        with self.prefix(src="", dst="Contents"):  # everything goes in Contents
            self.path("Info.plist", dst="Info.plist")

            # copy additional libs in <bundle>/Contents/MacOS/
            self.path(os.path.join(relpkgdir, "libndofdev.dylib"), dst="Resources/libndofdev.dylib")
            self.path(os.path.join(relpkgdir, "libhunspell-1.3.0.dylib"), dst="Resources/libhunspell-1.3.0.dylib")   

            with self.prefix(dst="MacOS"):              
                #this copies over the python wrapper script, associated utilities and required libraries, see SL-321, SL-322, SL-323
                self.path2basename(vmpdir,"SL_Launcher")
                self.path2basename(vmpdir,"*.py")
                # certifi will be imported by requests; this is our custom version to get our ca-bundle.crt 
                certifi_path = os.path.join(self.get_dst_prefix(),'certifi')
                if not os.path.exists(certifi_path):
                    os.makedirs(certifi_path)
                with self.prefix(dst="certifi"):
                    self.path2basename(os.path.join(vmpdir,"certifi"),"*")
                # llbase provides our llrest service layer and llsd decoding
                llbase_path = os.path.join(self.get_dst_prefix(),'llbase')
                if not os.path.exists(llbase_path):
                    os.makedirs(llbase_path)
                with self.prefix(dst="llbase"):
                    self.path2basename(llbasedir,"*.py")
                    self.path2basename(llbasedir,"_cllsd.so")
                #requests module needed by llbase/llrest.py
                #this is only needed on POSIX, because in Windows we compile it into the EXE
                requests_path = os.path.join(self.get_dst_prefix(),'requests')
                if not os.path.exists(requests_path):
                    os.makedirs(requests_path)
                with self.prefix(dst="requests"):
                    self.path2basename(requestsdir,"*")
                urllib3_path = os.path.join(self.get_dst_prefix(),'urllib3')
                if not os.path.exists(urllib3_path):
                    os.makedirs(urllib3_path)
                with self.prefix(dst="urllib3"):
                    self.path2basename(urllib3dir,"*")
                chardet_path = os.path.join(self.get_dst_prefix(),'chardet')
                if not os.path.exists(chardet_path):
                    os.makedirs(chardet_path)
                with self.prefix(dst="chardet"):
                    self.path2basename(chardetdir,"*")
                idna_path = os.path.join(self.get_dst_prefix(),'idna')
                if not os.path.exists(idna_path):
                    os.makedirs(idna_path)
                with self.prefix(dst="idna"):
                    self.path2basename(idnadir,"*")

            # most everything goes in the Resources directory
            with self.prefix(src="", dst="Resources"):
                super(DarwinManifest, self).construct()

                with self.prefix("cursors_mac"):
                    self.path("*.tif")

                self.path("licenses-mac.txt", dst="licenses.txt")
                self.path("featuretable_mac.txt")
                self.path("SecondLife.nib")
                self.path("ca-bundle.crt")

                icon_path = self.icon_path()
                with self.prefix(src=icon_path, dst="") :
                    self.path("secondlife.icns")

                with self.prefix(src=icon_path, dst="vmp_icons"):
                    self.path("secondlife.ico")

                #VMP Tkinter icons
                with self.prefix("vmp_icons"):
                    self.path("*.png")
                    self.path("*.gif")

                self.path("SecondLife.nib")
                
                # Translations
                self.path("English.lproj/language.txt")
                self.replace_in(src="English.lproj/InfoPlist.strings",
                                dst="English.lproj/InfoPlist.strings",
                                searchdict={'%%VERSION%%':'.'.join(self.args['version'])}
                                )
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

                def path_optional(src, dst):
                    """
                    For a number of our self.path() calls, not only do we want
                    to deal with the absence of src, we also want to remember
                    which were present. Return either an empty list (absent)
                    or a list containing dst (present). Concatenate these
                    return values to get a list of all libs that are present.
                    """
                    if self.path(src, dst):
                        return [dst]
                    print "Skipping %s" % dst
                    return []

                # dylibs is a list of all the .dylib files we expect to need
                # in our bundled sub-apps. For each of these we'll create a
                # symlink from sub-app/Contents/Resources to the real .dylib.
                # Need to get the llcommon dll from any of the build directories as well.
                libfile = "libllcommon.dylib"
                dylibs = path_optional(self.find_existing_file(os.path.join(os.pardir,
                                                               "llcommon",
                                                               self.args['configuration'],
                                                               libfile),
                                                               os.path.join(relpkgdir, libfile)),
                                       dst=libfile)

                for libfile in (
                                "libapr-1.0.dylib",
                                "libaprutil-1.0.dylib",
                                "libcollada14dom.dylib",
                                "libexpat.1.dylib",
                                "libexception_handler.dylib",
                                "libGLOD.dylib",
                                ):
                    dylibs += path_optional(os.path.join(relpkgdir, libfile), libfile)

                # SLVoice and vivox lols, no symlinks needed
                for libfile in (
                                'libortp.dylib',
                                'libsndfile.dylib',
                                'libvivoxoal.dylib',
                                'libvivoxsdk.dylib',
                                'libvivoxplatform.dylib',
                                'SLVoice',
                                ):
                    self.path2basename(relpkgdir, libfile)

                # dylibs that vary based on configuration
                if self.args['configuration'].lower() == 'debug':
                    for libfile in (
                                "libfmodexL.dylib",
                                ):
                        dylibs += path_optional(os.path.join(debpkgdir, libfile), libfile)
                else:
                    for libfile in (
                                "libfmodex.dylib",
                                ):
                        dylibs += path_optional(os.path.join(relpkgdir, libfile), libfile)
                
                # our apps
                for app_bld_dir, app in (("mac_crash_logger", "mac-crash-logger.app"),
                                         # plugin launcher
                                         (os.path.join("llplugin", "slplugin"), "SLPlugin.app"),
                                         ):
                    self.path2basename(os.path.join(os.pardir,
                                                    app_bld_dir, self.args['configuration']),
                                       app)

                    # our apps dependencies on shared libs
                    # for each app, for each dylib we collected in dylibs,
                    # create a symlink to the real copy of the dylib.
                    resource_path = self.dst_path_of(os.path.join(app, "Contents", "Resources"))
                    for libfile in dylibs:
                        src = os.path.join(os.pardir, os.pardir, os.pardir, libfile)
                        dst = os.path.join(resource_path, libfile)
                        try:
                            symlinkf(src, dst)
                        except OSError as err:
                            print "Can't symlink %s -> %s: %s" % (src, dst, err)

                # Dullahan helper apps go inside SLPlugin.app
                with self.prefix(src="", dst="SLPlugin.app/Contents/Frameworks"):
                    helperappfile = 'DullahanHelper.app'
                    self.path2basename(relpkgdir, helperappfile)

                    pluginframeworkpath = self.dst_path_of('Chromium Embedded Framework.framework');
                    # Putting a Frameworks directory under Contents/MacOS
                    # isn't canonical, but the path baked into Dullahan
                    # Helper.app/Contents/MacOS/DullahanHelper is:
                    # @executable_path/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework
                    # (notice, not @executable_path/../Frameworks/etc.)
                    # So we'll create a symlink (below) from there back to the
                    # Frameworks directory nested under SLPlugin.app.
                    helperframeworkpath = \
                        self.dst_path_of('DullahanHelper.app/Contents/MacOS/'
                                         'Frameworks/Chromium Embedded Framework.framework')


                helperexecutablepath = self.dst_path_of('SLPlugin.app/Contents/Frameworks/DullahanHelper.app/Contents/MacOS/DullahanHelper')
                self.run_command('install_name_tool -change '
                                 '"@rpath/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" '
                                 '"@executable_path/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" "%s"' % helperexecutablepath)

                # SLPlugin plugins
                with self.prefix(src="", dst="llplugin"):
                    self.path2basename("../media_plugins/cef/" + self.args['configuration'],
                                       "media_plugin_cef.dylib")

                    # copy LibVLC plugin itself
                    self.path2basename("../media_plugins/libvlc/" + self.args['configuration'],
                                       "media_plugin_libvlc.dylib")

                    # copy LibVLC dynamic libraries
                    with self.prefix(src=os.path.join(os.pardir, 'packages', 'lib', 'release' ), dst="lib"):
                        self.path( "libvlc*.dylib*" )

                    # copy LibVLC plugins folder
                    with self.prefix(src=os.path.join(os.pardir, 'packages', 'lib', 'release', 'plugins' ), dst="lib"):
                        self.path( "*.dylib" )
                        self.path( "plugins.dat" )


                # do this install_name_tool *after* media plugin is copied over
                dylibexecutablepath = self.dst_path_of('llplugin/media_plugin_cef.dylib')
                self.run_command('install_name_tool -change '
                                 '"@rpath/Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" '
                                 '"@executable_path/../Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework" "%s"' % dylibexecutablepath)


            # CEF framework goes inside Second Life.app/Contents/Frameworks
            with self.prefix(src="", dst="Frameworks"):
                frameworkfile="Chromium Embedded Framework.framework"
                self.path2basename(relpkgdir, frameworkfile)

            # This code constructs a relative path from the
            # target framework folder back to the location of the symlink.
            # It needs to be relative so that the symlink still works when
            # (as is normal) the user moves the app bundle out of the DMG
            # and into the /Applications folder. Note we also call 'raise'
            # to terminate the process if we get an error since without
            # this symlink, Second Life web media can't possibly work.
            # Real Framework folder:
            #   Second Life.app/Contents/Frameworks/Chromium Embedded Framework.framework/
            # Location of symlink and why it's relative 
            #   Second Life.app/Contents/Resources/SLPlugin.app/Contents/Frameworks/Chromium Embedded Framework.framework/
            # Real Frameworks folder, with the symlink inside the bundled SLPlugin.app (and why it's relative)
            #   <top level>.app/Contents/Frameworks/Chromium Embedded Framework.framework/
            #   <top level>.app/Contents/Resources/SLPlugin.app/Contents/Frameworks/Chromium Embedded Framework.framework ->
            # It might seem simpler just to create a symlink Frameworks to
            # the parent of Chromimum Embedded Framework.framework. But
            # that would create a symlink cycle, which breaks our
            # packaging step. So make a symlink from Chromium Embedded
            # Framework.framework to the directory of the same name, which
            # is NOT an ancestor of the symlink.
            frameworkpath = os.path.join(os.pardir, os.pardir, os.pardir,
                                         os.pardir, "Frameworks",
                                         "Chromium Embedded Framework.framework")
            try:
                # from SLPlugin.app/Contents/Frameworks/Chromium Embedded
                # Framework.framework back to Second
                # Life.app/Contents/Frameworks/Chromium Embedded Framework.framework
                origin, target = pluginframeworkpath, frameworkpath
                symlinkf(target, origin)
                # from SLPlugin.app/Contents/Frameworks/Dullahan
                # Helper.app/Contents/MacOS/Frameworks/Chromium Embedded
                # Framework.framework back to
                # SLPlugin.app/Contents/Frameworks/Chromium Embedded Framework.framework
                self.cmakedirs(os.path.dirname(helperframeworkpath))
                origin = helperframeworkpath
                target = os.path.join(os.pardir, frameworkpath)
                symlinkf(target, origin)
            except OSError as err:
                print "Can't symlink %s -> %s: %s" % (origin, target, err)
                raise


        # NOTE: the -S argument to strip causes it to keep enough info for
        # annotated backtraces (i.e. function names in the crash log).  'strip' with no
        # arguments yields a slightly smaller binary but makes crash logs mostly useless.
        # This may be desirable for the final release.  Or not.
        if ("package" in self.args['actions'] or 
            "unpacked" in self.args['actions']):
            self.run_command('strip -S %(viewer_binary)r' %
                             { 'viewer_binary' : self.dst_path_of('Contents/MacOS/Second Life')})

    def package_finish(self):
        global CHANNEL_VENDOR_BASE
        # MBW -- If the mounted volume name changes, it breaks the .DS_Store's background image and icon positioning.
        #  If we really need differently named volumes, we'll need to create multiple DS_Store file images, or use some other trick.

        volname=CHANNEL_VENDOR_BASE+" Installer"  # DO NOT CHANGE without understanding comment above

        imagename = self.installer_base_name()

        sparsename = imagename + ".sparseimage"
        finalname = imagename + ".dmg"
        # make sure we don't have stale files laying about
        self.remove(sparsename, finalname)

        self.run_command('hdiutil create %(sparse)r -volname %(vol)r -fs HFS+ -type SPARSE -megabytes 1300 -layout SPUD' % {
                'sparse':sparsename,
                'vol':volname})

        # mount the image and get the name of the mount point and device node
        try:
            hdi_output = subprocess.check_output(['hdiutil', 'attach', '-private', sparsename])
        except subprocess.CalledProcessError as err:
            sys.exit("failed to mount image at '%s'" % sparsename)
            
        try:
            devfile = re.search("/dev/disk([0-9]+)[^s]", hdi_output).group(0).strip()
            volpath = re.search('HFS\s+(.+)', hdi_output).group(1).strip()

            if devfile != '/dev/disk1':
                # adding more debugging info based upon nat's hunches to the
                # logs to help track down 'SetFile -a V' failures -brad
                print "WARNING: 'SetFile -a V' command below is probably gonna fail"

            # Copy everything in to the mounted .dmg

            app_name = self.app_name()

            # Hack:
            # Because there is no easy way to coerce the Finder into positioning
            # the app bundle in the same place with different app names, we are
            # adding multiple .DS_Store files to svn. There is one for release,
            # one for release candidate and one for first look. Any other channels
            # will use the release .DS_Store, and will look broken.
            # - Ambroff 2008-08-20
            dmg_template = os.path.join(
                'installers', 'darwin', '%s-dmg' % self.channel_type())

            if not os.path.exists (self.src_path_of(dmg_template)):
                dmg_template = os.path.join ('installers', 'darwin', 'release-dmg')

            for s,d in {self.get_dst_prefix():app_name + ".app",
                        os.path.join(dmg_template, "_VolumeIcon.icns"): ".VolumeIcon.icns",
                        os.path.join(dmg_template, "background.jpg"): "background.jpg",
                        os.path.join(dmg_template, "_DS_Store"): ".DS_Store"}.items():
                print "Copying to dmg", s, d
                self.copy_action(self.src_path_of(s), os.path.join(volpath, d))

            # Hide the background image, DS_Store file, and volume icon file (set their "visible" bit)
            for f in ".VolumeIcon.icns", "background.jpg", ".DS_Store":
                pathname = os.path.join(volpath, f)
                # We've observed mysterious "no such file" failures of the SetFile
                # command, especially on the first file listed above -- yet
                # subsequent inspection of the target directory confirms it's
                # there. Timing problem with copy command? Try to handle.
                for x in xrange(3):
                    if os.path.exists(pathname):
                        print "Confirmed existence: %r" % pathname
                        break
                    print "Waiting for %s copy command to complete (%s)..." % (f, x+1)
                    sys.stdout.flush()
                    time.sleep(1)
                # If we fall out of the loop above without a successful break, oh
                # well, possibly we've mistaken the nature of the problem. In any
                # case, don't hang up the whole build looping indefinitely, let
                # the original problem manifest by executing the desired command.
                self.run_command('SetFile -a V %r' % pathname)

            # Create the alias file (which is a resource file) from the .r
            self.run_command('Rez %r -o %r' %
                             (self.src_path_of("installers/darwin/release-dmg/Applications-alias.r"),
                              os.path.join(volpath, "Applications")))

            # Set the alias file's alias and custom icon bits
            self.run_command('SetFile -a AC %r' % os.path.join(volpath, "Applications"))

            # Set the disk image root's custom icon bit
            self.run_command('SetFile -a C %r' % volpath)

            # Sign the app if requested; 
            # do this in the copy that's in the .dmg so that the extended attributes used by 
            # the signature are preserved; moving the files using python will leave them behind
            # and invalidate the signatures.
            if 'signature' in self.args:
                app_in_dmg=os.path.join(volpath,self.app_name()+".app")
                print "Attempting to sign '%s'" % app_in_dmg
                identity = self.args['signature']
                if identity == '':
                    identity = 'Developer ID Application'

                # Look for an environment variable set via build.sh when running in Team City.
                try:
                    build_secrets_checkout = os.environ['build_secrets_checkout']
                except KeyError:
                    pass
                else:
                    # variable found so use it to unlock keychain followed by codesign
                    home_path = os.environ['HOME']
                    keychain_pwd_path = os.path.join(build_secrets_checkout,'code-signing-osx','password.txt')
                    keychain_pwd = open(keychain_pwd_path).read().rstrip()

                    # Note: As of macOS Sierra, keychains are created with names postfixed with '-db' so for example, the
                    #       SL Viewer keychain would by default be found in ~/Library/Keychains/viewer.keychain-db instead of
                    #       just ~/Library/Keychains/viewer.keychain in earlier versions.
                    #
                    #       Because we have old OS files from previous versions of macOS on the build hosts, the configurations
                    #       are different on each host. Some have viewer.keychain, some have viewer.keychain-db and some have both.
                    #       As you can see in the line below, this script expects the Linden Developer cert/keys to be in viewer.keychain.
                    #
                    #       To correctly sign builds you need to make sure ~/Library/Keychains/viewer.keychain exists on the host
                    #       and that it contains the correct cert/key. If a build host is set up with a clean version of macOS Sierra (or later)
                    #       then you will need to change this line (and the one for 'codesign' command below) to point to right place or else
                    #       pull in the cert/key into the default viewer keychain 'viewer.keychain-db' and export it to 'viewer.keychain'
                    self.run_command('security unlock-keychain -p "%s" "%s/Library/Keychains/viewer.keychain"' % ( keychain_pwd, home_path ) )
                    signed=False
                    sign_attempts=3
                    sign_retry_wait=15
                    while (not signed) and (sign_attempts > 0):
                        try:
                            sign_attempts-=1;
                            self.run_command(
                                # Note: See blurb above about names of keychains
                               'codesign --verbose --deep --force --keychain "%(home_path)s/Library/Keychains/viewer.keychain" --sign %(identity)r %(bundle)r' % {
                                   'home_path' : home_path,
                                   'identity': identity,
                                   'bundle': app_in_dmg
                                   })
                            signed=True # if no exception was raised, the codesign worked
                        except ManifestError as err:
                            if sign_attempts:
                                print >> sys.stderr, "codesign failed, waiting %d seconds before retrying" % sign_retry_wait
                                time.sleep(sign_retry_wait)
                                sign_retry_wait*=2
                            else:
                                print >> sys.stderr, "Maximum codesign attempts exceeded; giving up"
                                raise
                    self.run_command('spctl -a -texec -vv %(bundle)r' % { 'bundle': app_in_dmg })

            imagename="SecondLife_" + '_'.join(self.args['version'])


        finally:
            # Unmount the image even if exceptions from any of the above 
            self.run_command('hdiutil detach -force %r' % devfile)

        print "Converting temp disk image to final disk image"
        self.run_command('hdiutil convert %(sparse)r -format UDZO -imagekey zlib-level=9 -o %(final)r' % {'sparse':sparsename, 'final':finalname})
        self.run_command('hdiutil internet-enable -yes %(final)r' % {'final':finalname})
        # get rid of the temp file
        self.package_file = finalname
        self.remove(sparsename)


class Darwin_i386_Manifest(DarwinManifest):
    address_size = 32


class Darwin_i686_Manifest(DarwinManifest):
    """alias in case arch is passed as i686 instead of i386"""
    pass


class Darwin_x86_64_Manifest(DarwinManifest):
    address_size = 64


class LinuxManifest(ViewerManifest):
    build_data_json_platform = 'lnx'

    def construct(self):
        super(LinuxManifest, self).construct()

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")

        self.path("licenses-linux.txt","licenses.txt")
        with self.prefix("linux_tools", dst=""):
            self.path("client-readme.txt","README-linux.txt")
            self.path("client-readme-voice.txt","README-linux-voice.txt")
            self.path("client-readme-joystick.txt","README-linux-joystick.txt")
            self.path("wrapper.sh","secondlife")
            with self.prefix(src="", dst="etc"):
                self.path("handle_secondlifeprotocol.sh")
                self.path("register_secondlifeprotocol.sh")
                self.path("refresh_desktop_app_entry.sh")
                self.path("launch_url.sh")
            self.path("install.sh")

        with self.prefix(src="", dst="bin"):
            self.path("secondlife-bin","do-not-directly-run-secondlife-bin")
            self.path("../linux_crash_logger/linux-crash-logger","linux-crash-logger.bin")
            self.path2basename("../llplugin/slplugin", "SLPlugin") 
            #this copies over the python wrapper script, associated utilities and required libraries, see SL-321, SL-322 and SL-323
            self.path2basename("../viewer_components/manager","SL_Launcher")
            self.path2basename("../viewer_components/manager","*.py")
            llbase_path = os.path.join(self.get_dst_prefix(),'llbase')
            if not os.path.exists(llbase_path):
                os.makedirs(llbase_path)
            with self.prefix(dst="llbase"):
                self.path2basename("../packages/lib/python/llbase","*.py")
                self.path2basename("../packages/lib/python/llbase","_cllsd.so")         

        with self.prefix("res-sdl"):
            self.path("*")
            # recurse

        # Get the icons based on the channel type
        icon_path = self.icon_path()
        print "DEBUG: icon_path '%s'" % icon_path
        with self.prefix(src=icon_path, dst="") :
            self.path("secondlife_256.png","secondlife_icon.png")
            with self.prefix(src="",dst="res-sdl") :
                self.path("secondlife_256.BMP","ll_icon.BMP")

        # plugins
        with self.prefix(src="", dst="bin/llplugin"):
            self.path("../media_plugins/gstreamer010/libmedia_plugin_gstreamer010.so", "libmedia_plugin_gstreamer.so")
            self.path("../media_plugins/libvlc/libmedia_plugin_libvlc.so", "libmedia_plugin_libvlc.so")

        with self.prefix(src=os.path.join(os.pardir, 'packages', 'lib', 'vlc', 'plugins'), dst="bin/llplugin/vlc/plugins"):
            self.path( "plugins.dat" )
            self.path( "*/*.so" )

        with self.prefix(src=os.path.join(os.pardir, 'packages', 'lib' ), dst="lib"):
            self.path( "libvlc*.so*" )

        # llcommon
        if not self.path("../llcommon/libllcommon.so", "lib/libllcommon.so"):
            print "Skipping llcommon.so (assuming llcommon was linked statically)"

        self.path("featuretable_linux.txt")
        self.path("ca-bundle.crt")

    def package_finish(self):
        installer_name = self.installer_base_name()

        self.strip_binaries()

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
            # only create tarball if it's a release build.
            if self.args['buildtype'].lower() == 'release':
                # --numeric-owner hides the username of the builder for
                # security etc.
                self.run_command('tar -C %(dir)s --numeric-owner -cjf '
                                 '%(inst_path)s.tar.bz2 %(inst_name)s' % {
                        'dir': self.get_build_prefix(),
                        'inst_name': installer_name,
                        'inst_path':self.build_path_of(installer_name)})
            else:
                print "Skipping %s.tar.bz2 for non-Release build (%s)" % \
                      (installer_name, self.args['buildtype'])
        finally:
            self.run_command("mv %(inst)s %(dst)s" % {
                'dst': self.get_dst_prefix(),
                'inst': self.build_path_of(installer_name)})

    def strip_binaries(self):
        if self.args['buildtype'].lower() == 'release' and self.is_packaging_viewer():
            print "* Going strip-crazy on the packaged binaries, since this is a RELEASE build"
            # makes some small assumptions about our packaged dir structure
            self.run_command(r"find %(d)r/bin %(d)r/lib -type f \! -name \*.py \! -name SL_Launcher \! -name update_install | xargs --no-run-if-empty strip -S" % {'d': self.get_dst_prefix()} ) 

class Linux_i686_Manifest(LinuxManifest):
    address_size = 32

    def construct(self):
        super(Linux_i686_Manifest, self).construct()

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")

        with self.prefix(relpkgdir, dst="lib"):
            self.path("libapr-1.so")
            self.path("libapr-1.so.0")
            self.path("libapr-1.so.0.4.5")
            self.path("libaprutil-1.so")
            self.path("libaprutil-1.so.0")
            self.path("libaprutil-1.so.0.4.1")
            self.path("libdb*.so")
            self.path("libexpat.so.*")
            self.path("libGLOD.so")
            self.path("libuuid.so*")
            self.path("libSDL-1.2.so.*")
            self.path("libdirectfb-1.*.so.*")
            self.path("libfusion-1.*.so.*")
            self.path("libdirect-1.*.so.*")
            self.path("libopenjpeg.so*")
            self.path("libdirectfb-1.4.so.5")
            self.path("libfusion-1.4.so.5")
            self.path("libdirect-1.4.so.5*")
            self.path("libhunspell-1.3.so*")
            self.path("libalut.so*")
            self.path("libopenal.so*")
            self.path("libopenal.so", "libvivoxoal.so.1") # vivox's sdk expects this soname
            # KLUDGE: As of 2012-04-11, the 'fontconfig' package installs
            # libfontconfig.so.1.4.4, along with symlinks libfontconfig.so.1
            # and libfontconfig.so. Before we added support for library-file
            # wildcards, though, this self.path() call specifically named
            # libfontconfig.so.1.4.4 WITHOUT also copying the symlinks. When I
            # (nat) changed the call to self.path("libfontconfig.so.*"), we
            # ended up with the libfontconfig.so.1 symlink in the target
            # directory as well. But guess what! At least on Ubuntu 10.04,
            # certain viewer fonts look terrible with libfontconfig.so.1
            # present in the target directory. Removing that symlink suffices
            # to improve them. I suspect that means we actually do better when
            # the viewer fails to find our packaged libfontconfig.so*, falling
            # back on the system one instead -- but diagnosing and fixing that
            # is a bit out of scope for the present project. Meanwhile, this
            # particular wildcard specification gets us exactly what the
            # previous call did, without having to explicitly state the
            # version number.
            self.path("libfontconfig.so.*.*")

            # Include libfreetype.so. but have it work as libfontconfig does.
            self.path("libfreetype.so.*.*")

            try:
                self.path("libtcmalloc.so*") #formerly called google perf tools
                pass
            except:
                print "tcmalloc files not found, skipping"
                pass

            try:
                self.path("libfmodex-*.so")
                self.path("libfmodex.so")
                pass
            except:
                print "Skipping libfmodex.so - not found"
                pass


        # Vivox runtimes
        with self.prefix(src=relpkgdir, dst="bin"):
            self.path("SLVoice")
        with self.prefix(src=relpkgdir, dst="lib"):
            self.path("libortp.so")
            self.path("libsndfile.so.1")
            #self.path("libvivoxoal.so.1") # no - we'll re-use the viewer's own OpenAL lib
            self.path("libvivoxsdk.so")
            self.path("libvivoxplatform.so")

        self.strip_binaries()


class Linux_x86_64_Manifest(LinuxManifest):
    address_size = 64

    def construct(self):
        super(Linux_x86_64_Manifest, self).construct()

        # support file for valgrind debug tool
        self.path("secondlife-i686.supp")

################################################################

def symlinkf(src, dst):
    """
    Like ln -sf, but uses os.symlink() instead of running ln.
    """
    try:
        os.symlink(src, dst)
    except OSError as err:
        if err.errno != errno.EEXIST:
            raise
        # We could just blithely attempt to remove and recreate the target
        # file, but that strategy doesn't work so well if we don't have
        # permissions to remove it. Check to see if it's already the
        # symlink we want, which is the usual reason for EEXIST.
        elif os.path.islink(dst):
            if os.readlink(dst) == src:
                # the requested link already exists
                pass
            else:
                # dst is the wrong symlink; attempt to remove and recreate it
                os.remove(dst)
                os.symlink(src, dst)
        elif os.path.isdir(dst):
            print "Requested symlink (%s) exists but is a directory; replacing" % dst
            shutil.rmtree(dst)
            os.symlink(src, dst)
        elif os.path.exists(dst):
            print "Requested symlink (%s) exists but is a file; replacing" % dst
            os.remove(dst)
            os.symlink(src, dst)
        else:
            # see if the problem is that the parent directory does not exist
            # and try to explain what is missing
            (parent, tail) = os.path.split(dst)
            while not os.path.exists(parent):
                (parent, tail) = os.path.split(parent)
            if tail:
                raise Exception("Requested symlink (%s) cannot be created because %s does not exist"
                                % os.path.join(parent, tail))
            else:
                raise

if __name__ == "__main__":
    main()
