#!/usr/bin/env python
"""\
@file install.py
@author Phoenix
@date 2008-01-27
@brief Install files into an indra checkout.

Install files as specified by:
https://wiki.lindenlab.com/wiki/User:Phoenix/Library_Installation


$LicenseInfo:firstyear=2007&license=mit$

Copyright (c) 2007-2009, Linden Research, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
$/LicenseInfo$
"""

import sys
import os.path

# Look for indra/lib/python in all possible parent directories ...
# This is an improvement over the setup-path.py method used previously:
#  * the script may blocated anywhere inside the source tree
#  * it doesn't depend on the current directory
#  * it doesn't depend on another file being present.

def add_indra_lib_path():
    root = os.path.realpath(__file__)
    # always insert the directory of the script in the search path
    dir = os.path.dirname(root)
    if dir not in sys.path:
        sys.path.insert(0, dir)

    # Now go look for indra/lib/python in the parent dies
    while root != os.path.sep:
        root = os.path.dirname(root)
        dir = os.path.join(root, 'indra', 'lib', 'python')
        if os.path.isdir(dir):
            if dir not in sys.path:
                sys.path.insert(0, dir)
            return root
    else:
        print >>sys.stderr, "This script is not inside a valid installation."
        sys.exit(1)

base_dir = add_indra_lib_path()

import copy
import optparse
import os
import platform
import pprint
import shutil
import tarfile
import tempfile
import urllib2
import urlparse

try:
    # Python 2.6
    from hashlib import md5
except ImportError:
    # Python 2.5 and earlier
    from md5 import new as md5

from indra.base import llsd
from indra.util import helpformatter

# *HACK: Necessary for python 2.3. Consider removing this code wart
# after etch has deployed everywhere. 2008-12-23 Phoenix
try:
    sorted = sorted
except NameError:
    def sorted(in_list):
        "Return a list which is a sorted copy of in_list."
        # Copy the source to be more functional and side-effect free.
        out_list = copy.copy(in_list)
        out_list.sort()
        return out_list

class InstallFile(object):
    "This is just a handy way to throw around details on a file in memory."
    def __init__(self, pkgname, url, md5sum, cache_dir, platform_path):
        self.pkgname = pkgname
        self.url = url
        self.md5sum = md5sum
        filename = urlparse.urlparse(url)[2].split('/')[-1]
        self.filename = os.path.join(cache_dir, filename)
        self.platform_path = platform_path

    def __str__(self):
        return "ifile{%s:%s}" % (self.pkgname, self.url)

    def _is_md5sum_match(self):
        hasher = md5(file(self.filename, 'rb').read())
        if hasher.hexdigest() == self.md5sum:
            return  True
        return False

    def is_match(self, platform):
        """@brief Test to see if this ifile is part of platform
        @param platform The target platform. Eg, windows or linux/i686/gcc/3.3
        @return Returns True if the ifile is in the platform.
        """
        if self.platform_path[0] == 'common':
            return True
        req_platform_path = platform.split('/')
        #print "platform:",req_platform_path
        #print "path:",self.platform_path
        # to match, every path part much match
        match_count = min(len(req_platform_path), len(self.platform_path))
        for ii in range(0, match_count):
            if req_platform_path[ii] != self.platform_path[ii]:
                return False
        #print "match!"
        return True

    def fetch_local(self):
        #print "Looking for:",self.filename
        if not os.path.exists(self.filename):
            pass
        elif self.md5sum and not self._is_md5sum_match():
            print "md5 mismatch:", self.filename
            os.remove(self.filename)
        else:
            print "Found matching package:", self.filename
            return
        print "Downloading",self.url,"to local file",self.filename
        file(self.filename, 'wb').write(urllib2.urlopen(self.url).read())
        if self.md5sum and not self._is_md5sum_match():
            raise RuntimeError("Error matching md5 for %s" % self.url)

class LicenseDefinition(object):
    def __init__(self, definition):
        #probably looks like:
        # { text : ...,
        #   url : ...
        #   blessed : ...
        # }
        self._definition = definition


class InstallableDefinition(object):
    def __init__(self, definition):
        #probably looks like:
        # { packages : {platform...},
        #   copyright : ...
        #   license : ...
        #   description: ...
        # }
        self._definition = definition

    def _ifiles_from(self, tree, pkgname, cache_dir):
        return self._ifiles_from_path(tree, pkgname, cache_dir, [])

    def _ifiles_from_path(self, tree, pkgname, cache_dir, path):
        ifiles = []
        if 'url' in tree:
            ifiles.append(InstallFile(
                pkgname,
                tree['url'],
                tree.get('md5sum', None),
                cache_dir,
                path))
        else:
            for key in tree:
                platform_path = copy.copy(path)
                platform_path.append(key)
                ifiles.extend(
                    self._ifiles_from_path(
                        tree[key],
                        pkgname,
                        cache_dir,
                        platform_path))
        return ifiles

    def ifiles(self, pkgname, platform, cache_dir):
        """@brief return a list of appropriate InstallFile instances to install
        @param pkgname The name of the package to be installed, eg 'tut'
        @param platform The target platform. Eg, windows or linux/i686/gcc/3.3
        @param cache_dir The directory to cache downloads.
        @return Returns a list of InstallFiles which are part of this install
        """
        if 'packages' not in self._definition:
            return []
        all_ifiles = self._ifiles_from(
            self._definition['packages'],
            pkgname,
            cache_dir)
        if platform == 'all':
            return all_ifiles
        #print "Considering", len(all_ifiles), "packages for", pkgname
        # split into 2 lines because pychecker thinks it might return none.
        files = [ifile for ifile in all_ifiles if ifile.is_match(platform)]
        return files

class InstalledPackage(object):
    def __init__(self, definition):
        # looks like:
        # { url1 : { files: [file1,file2,...], md5sum:... },
        #   url2 : { files: [file1,file2,...], md5sum:... },...
        # }
        self._installed = {}
        for url in definition:
            self._installed[url] = definition[url]

    def urls(self):
        return self._installed.keys()

    def files_in(self, url):
        return self._installed[url].get('files', [])

    def get_md5sum(self, url):
        return self._installed[url].get('md5sum', None)

    def remove(self, url):
        self._installed.pop(url)

    def add_files(self, url, files):
        if url not in self._installed:
            self._installed[url] = {}
        self._installed[url]['files'] = files

    def set_md5sum(self, url, md5sum):
        if url not in self._installed:
            self._installed[url] = {}
        self._installed[url]['md5sum'] = md5sum

class Installer(object):
    def __init__(self, install_filename, installed_filename, dryrun):
        self._install_filename = install_filename
        self._install_changed = False
        self._installed_filename = installed_filename
        self._installed_changed = False
        self._dryrun = dryrun
        self._installables = {}
        self._licenses = {}
        self._installed = {}
        self.load()

    def load(self):
        if os.path.exists(self._install_filename):
            install = llsd.parse(file(self._install_filename, 'rb').read())
            try:
                for name in install['installables']:
                    self._installables[name] = InstallableDefinition(
                        install['installables'][name])
            except KeyError:
                pass
            try:
                for name in install['licenses']:
                    self._licenses[name] = LicenseDefinition(install['licenses'][name])
            except KeyError:
                pass
        if os.path.exists(self._installed_filename):
            installed = llsd.parse(file(self._installed_filename, 'rb').read())
            try:
                bins = installed['installables']
                for name in bins:
                    self._installed[name] = InstalledPackage(bins[name])
            except KeyError:
                pass

    def _write(self, filename, state):
        print "Writing state to",filename
        if not self._dryrun:
            file(filename, 'wb').write(llsd.format_pretty_xml(state))

    def save(self):
        if self._install_changed:
            state = {}
            state['licenses'] = {}
            for name in self._licenses:
                state['licenses'][name] = self._licenses[name]._definition
            #print "self._installables:",self._installables
            state['installables'] = {}
            for name in self._installables:
                state['installables'][name] = \
                                        self._installables[name]._definition
            self._write(self._install_filename, state)
        if self._installed_changed:
            state = {}
            state['installables'] = {}
            bin = state['installables']
            for name in self._installed:
                #print "installed:",name,self._installed[name]._installed
                bin[name] = self._installed[name]._installed
            self._write(self._installed_filename, state)

    def is_valid_license(self, bin):
        "@brief retrun true if we have valid license info for installable."
        installable = self._installables[bin]._definition
        if 'license' not in installable:
            print >>sys.stderr, "No license info found for", bin
            print >>sys.stderr, 'Please add the license with the',
            print >>sys.stderr, '--add-installable option. See', \
                                 sys.argv[0], '--help'
            return False
        if installable['license'] not in self._licenses:
            lic = installable['license']
            print >>sys.stderr, "Missing license info for '" + lic + "'.",
            print >>sys.stderr, 'Please add the license with the',
            print >>sys.stderr, '--add-license option. See', sys.argv[0],
            print >>sys.stderr, '--help'
            return False
        return True

    def list_installables(self):
        "Return a list of all known installables."
        return sorted(self._installables.keys())

    def detail_installable(self, name):
        "Return a installable definition detail"
        return self._installables[name]._definition

    def list_licenses(self):
        "Return a list of all known licenses."
        return sorted(self._licenses.keys())

    def detail_license(self, name):
        "Return a license definition detail"
        return self._licenses[name]._definition

    def list_installed(self):
        "Return a list of installed packages."
        return sorted(self._installed.keys())

    def detail_installed(self, name):
        "Return file list for specific installed package."
        filelist = []
        for url in self._installed[name]._installed.keys():
            filelist.extend(self._installed[name].files_in(url))
        return filelist

    def _update_field(self, description, field, value, multiline=False):
        """Given a block and a field name, add or update it.
        @param description a dict containing all the details of a description.
        @param field the name of the field to update.
        @param value the value of the field to update; if omitted, interview
                     will ask for value.
        @param multiline boolean specifying whether field is multiline or not.
        """
        if value:
            description[field] = value
        else:
            if field in description:
                print "Update value for '" + field + "'"
                print "(Leave blank to keep current value)"
                print "Current Value:  '" + description[field] + "'"
            else:
                print "Specify value for '" + field + "'"
            if not multiline:
                new_value = raw_input("Enter New Value: ")
            else:
                print "Please enter " + field + ". End input with EOF (^D)."
                new_value = sys.stdin.read()

            if field in description and not new_value:
                pass
            elif new_value:
                description[field] = new_value

        self._install_changed = True
        return True

    def _update_installable(self, name, platform, url, md5sum):
        """Update installable entry with specific package information.
        @param installable[in,out] a dict containing installable details. 
        @param platform Platform info, i.e. linux/i686, windows/i686 etc.
        @param url URL of tar file
        @param md5sum md5sum of tar file
        """
        installable  = self._installables[name]._definition
        path = platform.split('/')
        if 'packages' not in  installable:
            installable['packages'] = {}
        update = installable['packages']
        for child in path:
            if child not in update:
                update[child] = {}
            parent = update
            update = update[child]
        parent[child]['url'] = llsd.uri(url)
        parent[child]['md5sum'] = md5sum

        self._install_changed = True
        return True


    def add_installable_package(self, name, **kwargs):
        """Add an url for a platform path to the installable.
        @param installable[in,out] a dict containing installable details.
        """
        platform_help_str = """\
Please enter a new package location and url. Some examples:
common -- specify a package for all platforms
linux -- specify a package for all arch and compilers on linux
darwin/universal -- specify a mac os x universal
windows/i686/vs/2003 -- specify a windows visual studio 2003 package"""
        if name not in self._installables:
            print "Error: must add library with --add-installable or " \
                  +"--add-installable-metadata before using " \
                  +"--add-installable-package option"
            return False
        else:
            print "Updating installable '" + name + "'."
        for arg in ('platform', 'url', 'md5sum'):
            if not kwargs[arg]:
                if arg == 'platform': 
                    print platform_help_str
                kwargs[arg] = raw_input("Package "+arg+":")
        #path = kwargs['platform'].split('/')

        return self._update_installable(name, kwargs['platform'], 
                        kwargs['url'], kwargs['md5sum'])

    def add_installable_metadata(self, name, **kwargs):
        """Interactively add (only) library metadata into install, 
        w/o adding installable"""
        if name not in self._installables:
            print "Adding installable '" + name + "'."
            self._installables[name] = InstallableDefinition({})
        else:
            print "Updating installable '" + name + "'."
        installable  = self._installables[name]._definition
        for field in ('copyright', 'license', 'description'):
            self._update_field(installable, field, kwargs[field])
        print "Added installable '" + name + "':"
        pprint.pprint(self._installables[name])

        return True

    def add_installable(self, name, **kwargs):
        "Interactively pull a new installable into the install"
        ret_a = self.add_installable_metadata(name, **kwargs)
        ret_b = self.add_installable_package(name, **kwargs)
        return (ret_a and ret_b)

    def remove_installable(self, name):
        self._installables.pop(name)
        self._install_changed = True

    def add_license(self, name, **kwargs):
        if name not in self._licenses:
            print "Adding license '" + name + "'."
            self._licenses[name] = LicenseDefinition({})
        else:
            print "Updating license '" + name + "'."
        the_license  = self._licenses[name]._definition
        for field in ('url', 'text'):
            multiline = False
            if field == 'text':
                multiline = True
            self._update_field(the_license, field, kwargs[field], multiline)
        self._install_changed = True
        return True

    def remove_license(self, name):
        self._licenses.pop(name)
        self._install_changed = True

    def _uninstall(self, installables):
        """@brief Do the actual removal of files work.
        *NOTE: This method is not transactionally safe -- ie, if it
        raises an exception, internal state may be inconsistent. How
        should we address this?
        @param installables The package names to remove
        """
        remove_file_list = []
        for pkgname in installables:
            for url in self._installed[pkgname].urls():
                remove_file_list.extend(
                    self._installed[pkgname].files_in(url))
                self._installed[pkgname].remove(url)
                if not self._dryrun:
                    self._installed_changed = True
            if not self._dryrun:
                self._installed.pop(pkgname)
        remove_dir_set = set()
        for filename in remove_file_list:
            print "rm",filename
            if not self._dryrun:
                if os.path.exists(filename):
                    remove_dir_set.add(os.path.dirname(filename))
                    try:
                        os.remove(filename)
                    except OSError:
                        # This is just for cleanup, so we don't care
                        # about normal failures.
                        pass
        for dirname in remove_dir_set:
            try:
                os.removedirs(dirname)
            except OSError:
                # This is just for cleanup, so we don't care about
                # normal failures.
                pass

    def uninstall(self, installables, install_dir):
        """@brief Remove the packages specified.
        @param installables The package names to remove
        @param install_dir The directory to work from
        """
        print "uninstall",installables,"from",install_dir
        cwd = os.getcwdu()
        os.chdir(install_dir)
        try:
            self._uninstall(installables)
        finally:
            os.chdir(cwd)

    def _build_ifiles(self, platform, cache_dir):
        """@brief determine what files to install
        @param platform The target platform. Eg, windows or linux/i686/gcc/3.3
        @param cache_dir The directory to cache downloads.
        @return Returns the ifiles to install
        """
        ifiles = []
        for bin in self._installables:
            ifiles.extend(self._installables[bin].ifiles(bin, 
                                                         platform, 
                                                         cache_dir))
        to_install = []
        #print "self._installed",self._installed
        for ifile in ifiles:
            if ifile.pkgname not in self._installed:
                to_install.append(ifile)
            elif ifile.url not in self._installed[ifile.pkgname].urls():
                to_install.append(ifile)
            elif ifile.md5sum != \
                 self._installed[ifile.pkgname].get_md5sum(ifile.url):
                # *TODO: We may want to uninstall the old version too
                # when we detect it is installed, but the md5 sum is
                # different.
                to_install.append(ifile)
            else:
                #print "Installation up to date:",
                #        ifile.pkgname,ifile.platform_path
                pass
        #print "to_install",to_install
        return to_install

    def _install(self, to_install, install_dir):
        for ifile in to_install:
            tar = tarfile.open(ifile.filename, 'r')
            print "Extracting",ifile.filename,"to",install_dir
            if not self._dryrun:
                # *NOTE: try to call extractall, which first appears
                # in python 2.5. Phoenix 2008-01-28
                try:
                    tar.extractall(path=install_dir)
                except AttributeError:
                    _extractall(tar, path=install_dir)
            if ifile.pkgname in self._installed:
                self._installed[ifile.pkgname].add_files(
                    ifile.url,
                    tar.getnames())
                self._installed[ifile.pkgname].set_md5sum(
                    ifile.url,
                    ifile.md5sum)
            else:
                # *HACK: this understands the installed package syntax.
                definition = { ifile.url :
                               {'files': tar.getnames(),
                                'md5sum' : ifile.md5sum } }
                self._installed[ifile.pkgname] = InstalledPackage(definition)
            self._installed_changed = True

    def install(self, installables, platform, install_dir, cache_dir):
        """@brief Do the installation for for the platform.
        @param installables The requested installables to install.
        @param platform The target platform. Eg, windows or linux/i686/gcc/3.3
        @param install_dir The root directory to install into. Created
        if missing.
        @param cache_dir The directory to cache downloads. Created if
        missing.
        """
        # The ordering of steps in the method is to help reduce the
        # likelihood that we break something.
        install_dir = os.path.realpath(install_dir)
        cache_dir = os.path.realpath(cache_dir)
        _mkdir(install_dir)
        _mkdir(cache_dir)
        to_install = self._build_ifiles(platform, cache_dir)

        # Filter for files which we actually requested to install.
        to_install = [ifl for ifl in to_install if ifl.pkgname in installables]
        for ifile in to_install:
            ifile.fetch_local()
        self._install(to_install, install_dir)

    def do_install(self, installables, platform, install_dir, cache_dir=None, 
                   check_license=True, scp=None):
        """Determine what installables should be installed. If they were
        passed in on the command line, use them, otherwise install
        all known installables.
        """
        if not cache_dir: 
            cache_dir = _default_installable_cache()
        all_installables = self.list_installables()
        if not len(installables):
            install_installables = all_installables
        else:
            # passed in on the command line. We'll need to verify we
            # know about them here.
            install_installables = installables
            for installable in install_installables:
                if installable not in all_installables:
                    raise RuntimeError('Unknown installable: %s' % 
                                       (installable,))
        if check_license:
            # *TODO: check against a list of 'known good' licenses.
            # *TODO: check for urls which conflict -- will lead to
            # problems.
            for installable in install_installables:
                if not self.is_valid_license(installable):
                    return 1
    
        # Set up the 'scp' handler
        opener = urllib2.build_opener()
        scp_or_http = SCPOrHTTPHandler(scp)
        opener.add_handler(scp_or_http)
        urllib2.install_opener(opener)
    
        # Do the work of installing the requested installables.
        self.install(
            install_installables,
            platform,
            install_dir,
            cache_dir)
        scp_or_http.cleanup()
    
    def do_uninstall(self, installables, install_dir):
        # Do not bother to check license if we're uninstalling.
        all_installed = self.list_installed()
        if not len(installables):
            uninstall_installables = all_installed
        else:
            # passed in on the command line. We'll need to verify we
            # know about them here.
            uninstall_installables = installables
            for installable in uninstall_installables:
                if installable not in all_installed:
                    raise RuntimeError('Installable not installed: %s' % 
                                       (installable,))
        self.uninstall(uninstall_installables, install_dir)

class SCPOrHTTPHandler(urllib2.BaseHandler):
    """Evil hack to allow both the build system and developers consume
    proprietary binaries.
    To use http, export the environment variable:
    INSTALL_USE_HTTP_FOR_SCP=true
    """
    def __init__(self, scp_binary):
        self._scp = scp_binary
        self._dir = None

    def scp_open(self, request):
        #scp:codex.lindenlab.com:/local/share/install_pkgs/package.tar.bz2
        remote = request.get_full_url()[4:]
        if os.getenv('INSTALL_USE_HTTP_FOR_SCP', None) == 'true':
            return self.do_http(remote)
        try:
            return self.do_scp(remote)
        except:
            self.cleanup()
            raise

    def do_http(self, remote):
        url = remote.split(':',1)
        if not url[1].startswith('/'):
            # in case it's in a homedir or something
            url.insert(1, '/')
        url.insert(0, "http://")
        url = ''.join(url)
        print "Using HTTP:",url
        return urllib2.urlopen(url)

    def do_scp(self, remote):
        if not self._dir:
            self._dir = tempfile.mkdtemp()
        local = os.path.join(self._dir, remote.split('/')[-1:][0])
        command = []
        for part in (self._scp, remote, local):
            if ' ' in part:
                # I hate shell escaping.
                part.replace('\\', '\\\\')
                part.replace('"', '\\"')
                command.append('"%s"' % part)
            else:
                command.append(part)
        #print "forking:", command
        rv = os.system(' '.join(command))
        if rv != 0:
            raise RuntimeError("Cannot fetch: %s" % remote)
        return file(local, 'rb')

    def cleanup(self):
        if self._dir:
            shutil.rmtree(self._dir)


#
# *NOTE: PULLED FROM PYTHON 2.5 tarfile.py Phoenix 2008-01-28
#
def _extractall(tar, path=".", members=None):
    """Extract all members from the archive to the current working
       directory and set owner, modification time and permissions on
       directories afterwards. `path' specifies a different directory
       to extract to. `members' is optional and must be a subset of the
       list returned by getmembers().
    """
    directories = []

    if members is None:
        members = tar

    for tarinfo in members:
        if tarinfo.isdir():
            # Extract directory with a safe mode, so that
            # all files below can be extracted as well.
            try:
                os.makedirs(os.path.join(path, tarinfo.name), 0777)
            except EnvironmentError:
                pass
            directories.append(tarinfo)
        else:
            tar.extract(tarinfo, path)

    # Reverse sort directories.
    directories.sort(lambda a, b: cmp(a.name, b.name))
    directories.reverse()

    # Set correct owner, mtime and filemode on directories.
    for tarinfo in directories:
        path = os.path.join(path, tarinfo.name)
        try:
            tar.chown(tarinfo, path)
            tar.utime(tarinfo, path)
            tar.chmod(tarinfo, path)
        except tarfile.ExtractError, e:
            if tar.errorlevel > 1:
                raise
            else:
                tar._dbg(1, "tarfile: %s" % e)


def _mkdir(directory):
    "Safe, repeatable way to make a directory."
    if not os.path.exists(directory):
        os.makedirs(directory)

def _get_platform():
    "Return appropriate platform packages for the environment."
    platform_map = {
        'darwin': 'darwin',
        'linux2': 'linux',
        'win32' : 'windows',
        'cygwin' : 'windows',
        'solaris' : 'solaris'
        }
    this_platform = platform_map[sys.platform]
    if this_platform == 'linux':
        if platform.architecture()[0] == '64bit':
            # TODO -- someday when install.py accepts a platform of the form 
            # os/arch/compiler/compiler_version then we can replace the 
            # 'linux64' platform with 'linux/x86_64/gcc/4.1'
            this_platform = 'linux'
    return this_platform

def _getuser():
    "Get the user"
    try:
        # Unix-only.
        import getpass
        return getpass.getuser()
    except ImportError:
        import ctypes
        MAX_PATH = 260                  # according to a recent WinDef.h
        name = ctypes.create_unicode_buffer(MAX_PATH)
        namelen = ctypes.c_int(len(name)) # len in chars, NOT bytes
        if not ctypes.windll.advapi32.GetUserNameW(name, ctypes.byref(namelen)):
            raise ctypes.WinError()
        return name.value

def _default_installable_cache():
    """In general, the installable files do not change much, so find a 
    host/user specific location to cache files."""
    user = _getuser()
    cache_dir = "/var/tmp/%s/install.cache" % user
    if _get_platform() == 'windows':
        cache_dir = os.path.join(tempfile.gettempdir(), \
                                 'install.cache.%s' % user)
    return cache_dir

def parse_args():
    parser = optparse.OptionParser(
        usage="usage: %prog [options] [installable1 [installable2...]]",
        formatter = helpformatter.Formatter(),
        description="""This script fetches and installs installable packages.
It also handles uninstalling those packages and manages the mapping between
packages and their license.

The process is to open and read an install manifest file which specifies
what files should be installed. For each installable to be installed.
 * make sure it has a license
 * check the installed version
 ** if not installed and needs to be, download and install
 ** if installed version differs, download & install

If no installables are specified on the command line, then the defaut
behavior is to install all known installables appropriate for the platform
specified or uninstall all installables if --uninstall is set. You can specify
more than one installable on the command line.

When specifying a platform, you can specify 'all' to install all
packages, or any platform of the form:

OS[/arch[/compiler[/compiler_version]]]

Where the supported values for each are:
OS: darwin, linux, windows, solaris
arch: i686, x86_64, ppc, universal
compiler: vs, gcc
compiler_version: 2003, 2005, 2008, 3.3, 3.4, 4.0, etc.

No checks are made to ensure a valid combination of platform
parts. Some exmples of valid platforms:

windows
windows/i686/vs/2005
linux/x86_64/gcc/3.3
linux/x86_64/gcc/4.0
darwin/universal/gcc/4.0
""")
    parser.add_option(
        '--dry-run', 
        action='store_true',
        default=False,
        dest='dryrun',
        help='Do not actually install files. Downloads will still happen.')
    parser.add_option(
        '--install-manifest', 
        type='string',
        default=os.path.join(base_dir, 'install.xml'),
        dest='install_filename',
        help='The file used to describe what should be installed.')
    parser.add_option(
        '--installed-manifest', 
        type='string',
        default=os.path.join(base_dir, 'installed.xml'),
        dest='installed_filename',
        help='The file used to record what is installed.')
    parser.add_option(
        '--export-manifest', 
        action='store_true',
        default=False,
        dest='export_manifest',
        help="Print the install manifest to stdout and exit.")
    parser.add_option(
        '-p', '--platform', 
        type='string',
        default=_get_platform(),
        dest='platform',
        help="""Override the automatically determined platform. \
You can specify 'all' to do a installation of installables for all platforms.""")
    parser.add_option(
        '--cache-dir', 
        type='string',
        default=_default_installable_cache(),
        dest='cache_dir',
        help='Where to download files. Default: %s'% \
             (_default_installable_cache()))
    parser.add_option(
        '--install-dir', 
        type='string',
        default=base_dir,
        dest='install_dir',
        help='Where to unpack the installed files.')
    parser.add_option(
        '--list-installed', 
        action='store_true',
        default=False,
        dest='list_installed',
        help="List the installed package names and exit.")
    parser.add_option(
        '--skip-license-check', 
        action='store_false',
        default=True,
        dest='check_license',
        help="Do not perform the license check.")
    parser.add_option(
        '--list-licenses', 
        action='store_true',
        default=False,
        dest='list_licenses',
        help="List known licenses and exit.")
    parser.add_option(
        '--detail-license', 
        type='string',
        default=None,
        dest='detail_license',
        help="Get detailed information on specified license and exit.")
    parser.add_option(
        '--add-license', 
        type='string',
        default=None,
        dest='new_license',
        help="""Add a license to the install file. Argument is the name of \
license. Specify --license-url if the license is remote or specify \
--license-text, otherwse the license text will be read from standard \
input.""")
    parser.add_option(
        '--license-url', 
        type='string',
        default=None,
        dest='license_url',
        help="""Put the specified url into an added license. \
Ignored if --add-license is not specified.""")
    parser.add_option(
        '--license-text', 
        type='string',
        default=None,
        dest='license_text',
        help="""Put the text into an added license. \
Ignored if --add-license is not specified.""")
    parser.add_option(
        '--remove-license', 
        type='string',
        default=None,
        dest='remove_license',
        help="Remove a named license.")
    parser.add_option(
        '--remove-installable', 
        type='string',
        default=None,
        dest='remove_installable',
        help="Remove a installable from the install file.")
    parser.add_option(
        '--add-installable', 
        type='string',
        default=None,
        dest='add_installable',
        help="""Add a installable into the install file. Argument is \ 
the name of the installable to add.""")
    parser.add_option(
        '--add-installable-metadata', 
        type='string',
        default=None,
        dest='add_installable_metadata',
        help="""Add package for library into the install file. Argument is \
the name of the library to add.""")
    parser.add_option(
        '--installable-copyright', 
        type='string',
        default=None,
        dest='installable_copyright',
        help="""Copyright for specified new package. Ignored if \
--add-installable is not specified.""")
    parser.add_option(
        '--installable-license', 
        type='string',
        default=None,
        dest='installable_license',
        help="""Name of license for specified new package. Ignored if \
--add-installable is not specified.""")
    parser.add_option(
        '--installable-description', 
        type='string',
        default=None,
        dest='installable_description',
        help="""Description for specified new package. Ignored if \
--add-installable is not specified.""")
    parser.add_option(
        '--add-installable-package', 
        type='string',
        default=None,
        dest='add_installable_package',
        help="""Add package for library into the install file. Argument is \
the name of the library to add.""")
    parser.add_option(
        '--package-platform', 
        type='string',
        default=None,
        dest='package_platform',
        help="""Platform for specified new package. \
Ignored if --add-installable or --add-installable-package is not specified.""")
    parser.add_option(
        '--package-url', 
        type='string',
        default=None,
        dest='package_url',
        help="""URL for specified package. \
Ignored if --add-installable or --add-installable-package is not specified.""")
    parser.add_option(
        '--package-md5', 
        type='string',
        default=None,
        dest='package_md5',
        help="""md5sum for new package. \
Ignored if --add-installable or --add-installable-package is not specified.""")
    parser.add_option(
        '--list', 
        action='store_true',
        default=False,
        dest='list_installables',
        help="List the installables in the install manifest and exit.")
    parser.add_option(
        '--detail', 
        type='string',
        default=None,
        dest='detail_installable',
        help="Get detailed information on specified installable and exit.")
    parser.add_option(
        '--detail-installed', 
        type='string',
        default=None,
        dest='detail_installed',
        help="Get list of files for specified installed installable and exit.")
    parser.add_option(
        '--uninstall', 
        action='store_true',
        default=False,
        dest='uninstall',
        help="""Remove the installables specified in the arguments. Just like \
during installation, if no installables are listed then all installed \
installables are removed.""")
    parser.add_option(
        '--scp', 
        type='string',
        default='scp',
        dest='scp',
        help="Specify the path to your scp program.")

    return parser.parse_args()

def main():
    options, args = parse_args()
    installer = Installer(
        options.install_filename,
        options.installed_filename,
        options.dryrun)

    #
    # Handle the queries for information
    #
    if options.list_installed:
        print "installed list:", installer.list_installed()
        return 0
    if options.list_installables:
        print "installable list:", installer.list_installables()
        return 0
    if options.detail_installable:
        try:
            detail = installer.detail_installable(options.detail_installable)
            print "Detail on installable",options.detail_installable+":"
            pprint.pprint(detail)
        except KeyError:
            print "Installable '"+options.detail_installable+"' not found in",
            print "install file."
        return 0
    if options.detail_installed:
        try:
            detail = installer.detail_installed(options.detail_installed)
            #print "Detail on installed",options.detail_installed+":"
            for line in detail:
                print line
        except:
            raise
            print "Installable '"+options.detail_installed+"' not found in ",
            print "install file."
        return 0
    if options.list_licenses:
        print "license list:", installer.list_licenses()
        return 0
    if options.detail_license:
        try:
            detail = installer.detail_license(options.detail_license)
            print "Detail on license",options.detail_license+":"
            pprint.pprint(detail)
        except KeyError:
            print "License '"+options.detail_license+"' not defined in",
            print "install file."
        return 0
    if options.export_manifest:
        # *HACK: just re-parse the install manifest and pretty print
        # it. easier than looking at the datastructure designed for
        # actually determining what to install
        install = llsd.parse(file(options.install_filename, 'rb').read())
        pprint.pprint(install)
        return 0

    #
    # Handle updates -- can only do one of these
    # *TODO: should this change the command line syntax?
    #
    if options.new_license:
        if not installer.add_license(
            options.new_license,
            text=options.license_text,
            url=options.license_url):
            return 1
    elif options.remove_license:
        installer.remove_license(options.remove_license)
    elif options.remove_installable:
        installer.remove_installable(options.remove_installable)
    elif options.add_installable:
        if not installer.add_installable(
            options.add_installable,
            copyright=options.installable_copyright,
            license=options.installable_license,
            description=options.installable_description,
            platform=options.package_platform,
            url=options.package_url,
            md5sum=options.package_md5):
            return 1
    elif options.add_installable_metadata:
        if not installer.add_installable_metadata(
            options.add_installable_metadata,
            copyright=options.installable_copyright,
            license=options.installable_license,
            description=options.installable_description):
            return 1
    elif options.add_installable_package:
        if not installer.add_installable_package(
            options.add_installable_package,
            platform=options.package_platform,
            url=options.package_url,
            md5sum=options.package_md5):
            return 1
    elif options.uninstall:
        installer.do_uninstall(args, options.install_dir)
    else:
        installer.do_install(args, options.platform, options.install_dir, 
                             options.cache_dir, options.check_license, 
                             options.scp) 

    # save out any changes
    installer.save()
    return 0

if __name__ == '__main__':
    #print sys.argv
    sys.exit(main())
